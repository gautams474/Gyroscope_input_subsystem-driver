#include <linux/module.h>  // Module Defines and Macros (THIS_MODULE)
#include <linux/kernel.h>  // 
#include <linux/fs.h>	   // Inode and File types
#include <linux/cdev.h>    // Character Device Types and functions.
#include <linux/types.h>
#include <linux/slab.h>	   // Kmalloc/Kfree
#include <asm/uaccess.h>   // Copy to/from user space
#include <linux/string.h>
#include <linux/device.h>  // Device Creation / Destruction functions
#include <linux/i2c.h>     // i2c Kernel Interfaces
#include <linux/i2c-dev.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/gpio.h>
#include <linux/ioctl.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include<linux/init.h>
#include "game.h"
#include <linux/unistd.h>
#include <asm/msr.h>

#define DEVICE_NAME "GYRO"  // device name to be created and registered
#define DEVICE_ADDR 0x68 // Since AD0 of MPU6050 is low
#define cali_samp 400
#define I2CMUX 29 // Clear for I2C function
#define alpha (50)
#define MY_IOCTL 'G'
#define SETLEVEL _IO(MY_IOCTL, 0)
DEFINE_MUTEX(my_mutex);
unsigned long time_rise, time_fall, time_diff;
/* per device structure */
struct gyro_dev {
	struct cdev cdev;               /* The cdev structure */
	// Local variables
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	int X;
	int Y;
	int Z;
	int level;
	bool caliberate_flag;
} *gyro_devp;

static dev_t gyro_dev_number;      /* Allotted device number */
struct class *gyro_dev_class;          /* Tie with the device model */
static struct device *gyro_dev_device;
static struct input_dev *gyro;
static struct task_struct * MyThread = NULL;

static int thread_fn(void *mydata)
{
	int gyro_y_sum =0,gyro_z_sum =0,gyro_y_offset=0,gyro_z_offset=0;
	struct gyro_dev *mydevp = mydata; 
	//unsigned long time_rise, time_fall, time_diff;
	int ret,i=1;
	short X,Y,Z;
	//char result;
	char data[2];
	char data_gyro[6];
	data[0] = 0x43;
	
	while(!kthread_should_stop())
	{
		//rdtscl(time_rise);
		ret = i2c_master_send(mydevp->client, data, 1); //send data
		if(ret < 0){
			printk("i2c couldnt set address\n");
			return -1;
		}
		msleep(7);
		ret = i2c_master_recv(mydevp->client, data_gyro, 6);
		if(ret < 0 ){
			printk("i2c couldnt read data\n");
			return -1;
		}
		
		
		X = ((data_gyro[0]<<8 | (data_gyro[1] & 0xff) ));
		X= -((~X)+1);
		Y = ((data_gyro[2]<<8 | (data_gyro[3] & 0xff) ));
		Y= -((~Y)+1);
		Z = ((data_gyro[4]<<8 | (data_gyro[5] & 0xff) ));
		Z= -((~Z)+1);

		if(mydevp->caliberate_flag ==0){
			if( i <cali_samp){
				gyro_y_sum += Y;
				gyro_z_sum += Z;
				i++;
			}
			else{
				i =1;
				mydevp->caliberate_flag = 1;
				printk("caliberating done\n");
			
				gyro_y_offset = gyro_y_sum/cali_samp;
				gyro_z_offset = gyro_z_sum/cali_samp;
				printk("gyro_y_offset =%d\tgyro_z_offset =%d\n",gyro_y_offset,gyro_z_offset );
			}
		}
	    else{
			mydevp->Y = Y - gyro_y_offset;
			mydevp->Z = Z - gyro_z_offset;
			
		   /*
			//mydevp->X = ((alpha) * X + (100-alpha) *  mydevp->X ) / 100;
			mydevp->Y = ((alpha) * Y + (100-alpha) *  mydevp->Y ) / 100;
			mydevp->Z = ((alpha) * Z + (100-alpha) *  mydevp->Z ) / 100;
			//printk("X: %d\n"  , X );
			//printk("Y: %d\n"  , Y );
			//printk("Z: %d\n\n", Z ); 
			*/
			//input_report_abs( gyro, ABS_X, (int)mydevp->X);
			input_report_abs( gyro, ABS_Y, (int)mydevp->Y);
			input_report_abs( gyro, ABS_Z, (int)mydevp->Z);
			//printk("level =%d\n",mydevp->level);                                         
			input_sync(gyro);
			//msleep(100);
			msleep(100*(mydevp->level));
			//rdtscl(time_fall);
			//time_diff = time_fall - time_rise;
			//printk("dt =%lu\n",time_diff/400);
		}
	}
	
	return 0;
}

/*
 Open driver
*/
int gyro_driver_open(struct inode *inode, struct file *file)
{
	struct gyro_dev* gyro_devp;
	//char test = 0x75; // test register address
	char result;
	int ret = 0;
	char data[2];
	
	
	/* Get the per-device structure that contains this cdev */
	gyro_devp = container_of(inode->i_cdev, struct gyro_dev, cdev);

	/* Easy access to gyro_devp from rest of the entry points */
	file->private_data = gyro_devp;
	
	data[0] = MPU6050_RA_WHO_AM_I;
	ret = i2c_master_send(gyro_devp->client, data, 1); //send data
	msleep(1);
	if(ret < 0){
		printk("i2c couldnt set address\n");
		return -1;
	}	
	msleep(5);
	ret = i2c_master_recv(gyro_devp->client, &result, 1);
	if(ret < 0 || result !=0x68 ){
		if (result !=0x68){
			printk("Result unmatched.\nCheck connection\n");
			return -1;
		}
		else{
			printk("i2c couldnt read\n");
			return -1;
		}
	}

	

//	MPU6050_RA_SMPLRT_DIV, 0x07 //MPU6050_RA_SMPLRT_DIV 0x19
/*	data[0] = MPU6050_RA_SMPLRT_DIV;
	data[1] = 0x07;
	ret = i2c_master_send(gyro_devp->client, data, 2); //send data
	if(ret < 0){
		printk("i2c couldnt set address\n");
		return -1;
	}
*/
//	MPU6050_RA_CONFIG, 0x00 //#define MPU6050_RA_CONFIG 0x1A
	data[0] = MPU6050_RA_CONFIG; //LPF
	data[1] = 0x06; //0x00
	ret = i2c_master_send(gyro_devp->client, data, 2); //send data
	if(ret < 0){
		printk("i2c couldnt set address\n");
		return -1;
	}

//	MPU6050_RA_ACCEL_CONFIG, 0b00000000
	data[0] = MPU6050_RA_ACCEL_CONFIG;   //0x1C
	data[1] = 0x00;
	ret = i2c_master_send(gyro_devp->client, data, 2); //send data
	if(ret < 0){
		printk("i2c couldnt set address\n");
		return -1;
	}

//	MPU6050_RA_GYRO_CONFIG, 0b00000000
	data[0] = MPU6050_RA_GYRO_CONFIG;   //0x1B
	data[1] = 0x00;
	ret = i2c_master_send(gyro_devp->client, data, 2); //send data
	if(ret < 0){
		printk("i2c couldnt set address\n");
		return -1;
	}	

	
//	MPU6050_RA_PWR_MGMT_1 , 0b00000010 //#define MPU6050_RA_PWR_MGMT_1 0x6B
	data[0] = MPU6050_RA_PWR_MGMT_1;
	data[1] = 0x02;
	ret = i2c_master_send(gyro_devp->client, data, 2); //send data
	if(ret < 0){
		printk("i2c couldnt set address\n");
		return -1;
	}

	MyThread = kthread_run(thread_fn,gyro_devp,"mythread");
	return 0;

}
static long gyro_ioctl(struct file *file, unsigned cmd, unsigned long arg)
{
	struct gyro_dev *gyro_devp = file->private_data;
	mutex_lock(&my_mutex);   //lock_kernel();
	if(cmd == SETLEVEL){
		gyro_devp->level+=1;
		if (gyro_devp->level >3)
			gyro_devp->level =1;
	}
	mutex_unlock(&my_mutex);
	printk("\n\nioctl level =%d\n",gyro_devp->level);
	return gyro_devp->level;
}

/*
 * Release driver
 */
int gyro_driver_release(struct inode *inode, struct file *file)
{
	//struct gyro_dev *gyro_devp = file->private_data;


	
	return 0;
}

/*
 * Write to driver
 */
ssize_t gyro_driver_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	//int ret;
	//struct gyro_dev *gyro_devp = file->private_data;
	
	return 0;
	
}


/*
 * Read from driver
 */
ssize_t gyro_driver_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	//int ret;
	//struct gyro_dev *gyro_devp = file->private_data;
	printk("In read");
	return 0;
}


/* File operations structure. Defined in linux/fs.h */
static struct file_operations gyro_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= gyro_driver_open,        /* Open method */
    .release	= gyro_driver_release,     /* Release method */
    .write		= gyro_driver_write,       /* Write method */
    .read		= gyro_driver_read,        /* Read method */
	.unlocked_ioctl = gyro_ioctl,
	
};

static int __init gyro_driver_init(void)
{
	int ret =0;
	int min, max , fuzz, flat;
	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&gyro_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	gyro_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	gyro_devp = kmalloc(sizeof(struct gyro_dev), GFP_KERNEL);
		
	if (!gyro_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	//sprintf(gyro_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&gyro_devp->cdev, &gyro_fops);
	gyro_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&gyro_devp->cdev, (gyro_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	gyro_dev_device = device_create(gyro_dev_class, NULL, MKDEV(MAJOR(gyro_dev_number), 0), NULL, DEVICE_NAME);	
	
	ret = gpio_request(I2CMUX, "I2CMUX");
	if(ret)
	{
		printk("\n\t i2cflash GPIO %d is not requested.\n", I2CMUX);
	}

	ret = gpio_direction_output(I2CMUX, 0);
	if(ret)
	{
		printk("\n\t i2cflash GPIO %d is not set as output.\n", I2CMUX);
	}
	gpio_set_value_cansleep(I2CMUX, 0); // Direction output didn't seem to init correctly.

	// Create Adapter using:
	gyro_devp->adapter = i2c_get_adapter(0); // /dev/i2c-0
	if(gyro_devp->adapter == NULL){
		printk("\n\t gyro Could not acquire i2c adapter.\n");
		return -1;
	}
	
	printk("\n\t gyro: init complete\n");
	// Create Client Structure
	gyro_devp->client = (struct i2c_client*) kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	gyro_devp->client->addr = DEVICE_ADDR; // Device Address (set by hardware)
	snprintf(gyro_devp->client->name, I2C_NAME_SIZE, "gyro");
	gyro_devp->client->adapter = gyro_devp->adapter;
	
	gyro_devp->X = 0;
	gyro_devp->Y = 0;
	gyro_devp->Z = 0;
	gyro_devp->level =1;
	gyro_devp->caliberate_flag=0;

	gyro = input_allocate_device();
	gyro->name = "Example 1 device";
    /* phys is unique on a running system */
    gyro->phys = "A/Fake/Path";
    gyro->id.bustype = BUS_HOST;
    gyro->id.vendor = 0x0001;
    gyro->id.product = 0x0001;
    gyro->id.version = 0x0100;
	
	min = -32767; max = 32768; fuzz = 40; flat =40;
	
	
	set_bit(EV_ABS, gyro->evbit);
	set_bit(ABS_X, gyro->absbit);
	set_bit(ABS_Y, gyro->absbit);
	set_bit(ABS_Z, gyro->absbit);
	
	input_set_abs_params(gyro, ABS_X, min, max, fuzz, flat);
	input_set_abs_params(gyro, ABS_Y, min, max, fuzz, flat);
	input_set_abs_params(gyro, ABS_Z, min, max, fuzz, flat);

	ret = input_register_device(gyro);
	
	return 0;
}

/* Driver Exit */
void __exit gyro_driver_exit(void)
{
		
	
	if(MyThread)
	{
		printk("stop MyThread\n");
		kthread_stop(MyThread);
	}
	// Close and cleanup
	i2c_put_adapter(gyro_devp->adapter);
	kfree(gyro_devp->client);
	
	/* Release the major number */
	unregister_chrdev_region((gyro_dev_number), 1);
	
	/* Destroy device */
	device_destroy (gyro_dev_class, MKDEV(MAJOR(gyro_dev_number), 0));
	cdev_del(&gyro_devp->cdev);
	kfree(gyro_devp);
	input_unregister_device(gyro);
	/* Destroy driver_class */
	class_destroy(gyro_dev_class);
	
	gpio_free(I2CMUX);
	printk("\n\t gyro: exit complete\n");
}

module_init(gyro_driver_init);
module_exit(gyro_driver_exit);
MODULE_LICENSE("GPL v2");
