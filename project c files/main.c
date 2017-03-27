#include <linux/fs.h>	   // Inode and File types
#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <time.h>
//#include <math.h>
#define MY_IOCTL 'G'
#define SETLEVEL _IO(MY_IOCTL, 0)     // busy status is read 

	
#define KEYFILE "/dev/input/event2"
#define print_line printf("LINE NO. %d \n", __LINE__)
extern int errno; 
int counter =0;
static char r=0,c=0;
float z=0,y=0;
int flags =0;
struct timespec start_spec;
struct timespec end_spec;
struct timespec spec;
time_t start, end;
int gyro_fd = 0;
void rand_init();
void disp_fireworks(int fd , struct spi_ioc_transfer xfer , char* writeBuf);
void disp_center(int fd , struct spi_ioc_transfer xfer , char* writeBuf);
void clear_display(int fd , struct spi_ioc_transfer xfer , char* writeBuf);
void spi_send(char row ,char col,int fd, struct spi_ioc_transfer xfer , char* writeBuf);
void disp_level(int fd , struct spi_ioc_transfer xfer , char* writeBuf, int level);
int main(int argc, char** argv)
{
	int fd =0, fd_SPI = 0 , fd_gpio= 0;
	int ret=0;
	struct input_event ie;
	int gyro_z, gyro_y;
	
	unsigned char mode = 0;
	unsigned char lsb = 0; 
	unsigned char bpw = 8;
	unsigned long ms = 10000000;
	char readBuf[2];
	char writeBuf[2];
	struct spi_ioc_transfer xfer;
	
	
	gyro_fd = open("/dev/GYRO",O_RDWR);   //KEYFILE
	if(gyro_fd < 0)
	{
		printf("Error: could not open GYRO.\n");
		return -1;
	}
	
	
	/*****SPI*****************/
	fd_gpio = open("/sys/class/gpio/export", O_WRONLY);
	write(fd_gpio, "42", 2);
	close(fd_gpio); 
	
	// Set GPIO Direction
	fd_gpio = open("/sys/class/gpio/gpio42/direction", O_WRONLY);
	write(fd_gpio, "out", 3);
	close(fd_gpio);
	
	// Set Value
	fd_gpio = open("/sys/class/gpio/gpio42/value", O_WRONLY);
	write(fd_gpio, "0", 1);

	fd_gpio = open("/sys/class/gpio/export", O_WRONLY);
	write(fd_gpio, "43", 2);
	close(fd_gpio); 
	
	// Set GPIO Direction
	fd_gpio = open("/sys/class/gpio/gpio43/direction", O_WRONLY);
	write(fd_gpio, "out", 3);
	close(fd_gpio);
	
	// Set Value
	fd_gpio = open("/sys/class/gpio/gpio43/value", O_WRONLY);
	write(fd_gpio, "0", 1);
    close(fd_gpio);
	
	fd_gpio = open("/sys/class/gpio/export", O_WRONLY);
	write(fd_gpio, "55", 2);
	close(fd_gpio);

	// Set GPIO Direction
	fd_gpio = open("/sys/class/gpio/gpio55/direction", O_WRONLY);
	write(fd_gpio, "out", 3);
	close(fd_gpio);
	
	// Set Value
	fd_gpio = open("/sys/class/gpio/gpio55/value", O_WRONLY);
	write(fd_gpio, "0", 1);
	close(fd_gpio);

	fd_SPI = open("/dev/spidev1.0", O_RDWR);

	mode |= SPI_MODE_0;
	
	// Mode Settings could include:
	// 		SPI_LOOP		0
	//		SPI_MODE_0		1
	//		SPI_MODE_1		2
	//		SPI_MODE_2		3
	//		SPI_LSB_FIRST	4
	//		SPI_CS_HIGH		5
	//		SPI_3WIRE		6
	//		SPI_NO_CS		7		
	
	if(ioctl(fd_SPI, SPI_IOC_WR_MODE, &mode) < 0)
	{
		perror("SPI Set Mode Failed");
		close(fd_SPI);
		return -1;
	}
	
	if(ioctl(fd_SPI, SPI_IOC_WR_LSB_FIRST, &lsb) < 0)
	{
		perror("SPI Set LSB Failed");
		close(fd_SPI);
		return -1;
	}

	if(ioctl(fd_SPI, SPI_IOC_WR_BITS_PER_WORD, &bpw) < 0)
	{
		perror("SPI Set LSB Failed");
		close(fd_SPI);
		return -1;
	}

	if(ioctl(fd_SPI, SPI_IOC_WR_MAX_SPEED_HZ, &ms) < 0)
	{
		perror("SPI Set LSB Failed");
		close(fd_SPI);
		return -1;
	}
	
	// Setup Commands could include:
	//		SPI_IOC_RD_MODE
	//		SPI_IOC_WR_MODE
	//		SPI_IOC_RD_LSB_FIRST
	//		SPI_IOC_WR_LSB_FIRST
	//		SPI_IOC_RD_BITS_PER_WORD
	//		SPI_IOC_WR_BITS_PER_WORD
	//		SPI_IOC_RD_MAX_SPEED_HZ
	//		SPI_IOC_WR_MAX_SPEED_HZ

//1. 0x00 to reg 0x09; 2. 0x03 to 0x0A ; 3. 0x07 to reg 0x0B ; 4. 0x01 to reg 0x0C; 5.(test) 0x01 to reg 0x0F OR (no test) 0x00 to reg 0x0F
	// Setup a Write Transfer
	
	// Setup Transaction 
	xfer.tx_buf = (unsigned long)writeBuf;
	xfer.rx_buf = (unsigned long)NULL;
	xfer.len = 2; // Bytes to send
	xfer.cs_change = 0;
	xfer.delay_usecs = 0;
	xfer.speed_hz = 10000000;
	xfer.bits_per_word = 8;

	writeBuf[1] = 0x00; // Step 1
	writeBuf[0] = 0x09; 
	
	// Send Message
	if(ioctl(fd_SPI, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");

	writeBuf[1] = 0x03; // Step 2
	writeBuf[0] = 0x0A; 
	
	// Send Message
	if(ioctl(fd_SPI, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");

	writeBuf[1] = 0x07; // Step 3
	writeBuf[0] = 0x0B; 
	
	// Send Message
	if(ioctl(fd_SPI, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");

	writeBuf[1] = 0x01; // Step 4
	writeBuf[0] = 0x0C; 
	
	// Send Message
	if(ioctl(fd_SPI, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");
	clear_display(fd_SPI ,  xfer , writeBuf);
	disp_level(fd_SPI,xfer,writeBuf,1);
	usleep(2000*1000);
	clear_display(fd_SPI ,  xfer , writeBuf);
	disp_center(fd_SPI,xfer,writeBuf);
	clear_display(fd_SPI ,  xfer , writeBuf);
	
	writeBuf[1] = 0x40;    //print ball
	writeBuf[0] = 0x04;
	
	// Send Message
	if(ioctl(fd_SPI, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");

	fd = open(KEYFILE,O_RDONLY);   //
	if(fd < 0)
	{
		printf("Error: could not open event2.\n");
		return -1;
	}
	flags = fcntl(fd, F_GETFL,0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	while(1){		
	while(read(fd, &ie, sizeof(struct input_event))) {
			//printf("in read\n");
			if(ie.type == EV_ABS && ie.code == ABS_Z)
			{	
				gyro_z = (float)ie.value/131;
				gyro_z = gyro_z*0.037;
				z += /*z*0.6 +*/ gyro_z;				
				//printf("Z value =%f\tZ*dt =%f\n", z, (gyro_z*0.037));
				if(z <= -3){
					//printf("shift right by 2   %f\n",z);
					spi_send(2 ,0, fd_SPI,  xfer ,  writeBuf);
				    
					}
				else if((z<-1) && (z>-3)){
					//printf("shift right by 1   %f\n",z);
					spi_send(1 ,0 , fd_SPI,  xfer ,  writeBuf);
					}
				else if((z >= 1) && (z<3)){
					//printf("shift left by 1   %f\n",z);
					spi_send(-1 ,0 , fd_SPI,  xfer ,  writeBuf);
					}
				else if(z>=3){
					//printf("shift left by 2   %f\n",z);
					spi_send(-2 ,0 , fd_SPI,  xfer ,  writeBuf);	 
					}
				else
					spi_send(0 ,0 , fd_SPI,  xfer ,  writeBuf);	 
				
			}
			
			if(ie.type == EV_ABS && ie.code == ABS_Y)
			{	
				gyro_y= ie.value/131 ;
				//printf("Y value =%d\tY/131 =%d\n", ie.value, gyro_y);
				
				gyro_y = gyro_y*0.037;
				y += /*z*0.6 +*/ gyro_y;				
				//printf("Y value =%f\tY*dt =%f\n", y, (gyro_y));
				if(y <= -3){
					//printf("shift down by 2   %f\n",z);
					spi_send(0 ,-2, fd_SPI,  xfer ,  writeBuf);
				    
					}
				else if((y<-1) && (y>-3)){
					//printf("shift down by 1   %f\n",z);
					spi_send(0 ,-1 , fd_SPI,  xfer ,  writeBuf);
					}
				else if((y >= 1) && (y<3)){
					//printf("shift up by 1   %f\n",z);
					spi_send(0 ,1 , fd_SPI,  xfer ,  writeBuf);
					}
				else if(y>=3){
					//printf("shift up by 2   %f\n",z);
					spi_send(0 ,2 , fd_SPI,  xfer ,  writeBuf);	 
					}
				else 
					spi_send(0 ,0 , fd_SPI,  xfer ,  writeBuf);	 
			}
			

			
				
		}
		usleep(50000);//500000
	}

	
	close(gyro_fd);
	//close(fd);
	return 0;
}
char led_arr[8] ={0x80 , 0x40 , 0x20 , 0x10, 0x08, 0x04, 0x02, 0x01  };
char x_arr[8] = {0x81, 0x42, 0x24, 0x18,0x18, 0x24, 0x42, 0x81};
char level_arr[3][8] = {0x0,0x84,0x82,0xFF,0x80,0x80,0x80,0x0,
						0x0,0x0,0x2,0xF1,0x91,0x9E,0x80,0x0,
						0x0,0x0,0x44,0x92,0x92,0x6E,0x0,0x0,
						};
void spi_send(char row ,char col,int fd, struct spi_ioc_transfer xfer , char* writeBuf){
	
	int i = 0,j=0,ret=0;
 	//printf("enter here");
	writeBuf[1] = 0x00; // Step 1
	writeBuf[0] = r;
	
	if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");

	//usleep(5000);
	r = r + row;
	c = c + col;
	//printf("r =%d\tc =%d\n",r,c);
	if((r>=4 && r<=5)&&(c>=4 && c<=5)){
		clock_gettime(CLOCK_REALTIME,&start_spec);
		end = start_spec.tv_sec;
		
		//counter++;
		if(end-start > 3){
		printf("success\n");
		disp_fireworks(fd,xfer,writeBuf);
		sleep(1);
		clear_display(fd ,  xfer , writeBuf);
		ret = ioctl(gyro_fd,SETLEVEL);
		if(ret < 0)
		{
			printf("Error: ioctl failed.\n");
   		 }
		else
			disp_level(fd,xfer,writeBuf,(int)ret);
		clear_display(fd ,  xfer , writeBuf);
		rand_init();
		writeBuf[1] = led_arr[c-1];
		writeBuf[0] = r; //r-1 //address

		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
			perror("SPI Message Send Failed");
		}	
	}
	else{
		clock_gettime(CLOCK_REALTIME,&start_spec);
		start = start_spec.tv_sec;
		}
		//counter =0;
	if((r>=1 && r<=8)&&(c>=1 && c<=8)){
		//print_line;
		writeBuf[1] = led_arr[c-1];
		writeBuf[0] = r; //r-1 //address

		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
			perror("SPI Message Send Failed");
	}
	else  //ball drops flashing X
	{
		//printf("flashing\n");
		for(j=0;j<3;j++){
			for(i=0;i<8;i++){

				writeBuf[1] = x_arr[i];
				writeBuf[0] = (char)i+1;  //address

				if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
				perror("SPI Message Send Failed");
			}
			usleep(500*1000);

		 	for(i = 1 ; i<= 8 ; i++){
				writeBuf[1] = 0x00;//array[i-1]; // Step 1
				writeBuf[0] = (char)i;  //address

				if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
						perror("SPI Message Send Failed");
			}
			usleep(50000);
		}
		
		rand_init();
		//printf("setting constants to zero\n");
		z = 0;
		y= 0;
		writeBuf[1] = led_arr[c-1];
		writeBuf[0] = r;
		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
			perror("SPI Message Send Failed");
	}

}
char fireworks[4][8] = {0x0,0x0,0x0,0x18,0x18,0x00,0x00,0x00,
0x0,0x0,0x3C,0x24,0x24,0x3C,0x0,0x0,
0x0,0x7E,0x42,0x42,0x42,0x42,0x7E,0x0,
0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF,
};
char center[8]={0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00};
void clear_display(int fd , struct spi_ioc_transfer xfer , char* writeBuf)
{
	int i=0;
	for(i = 1 ; i<= 8 ; i++){
		writeBuf[1] = 0x00;//array[i-1]; // Step 1
		writeBuf[0] = (char)i;  //address

		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
				perror("SPI Message Send Failed");
	}
}
void disp_fireworks(int fd , struct spi_ioc_transfer xfer , char* writeBuf){
	int i = 0,j=0;	
for(j=0;j<4;j++)
	for(i = 1 ; i<= 8 ; i++){
	
	writeBuf[1] = fireworks[j][i-1]; // Step 1
	writeBuf[0] = (char)i;  //address
	
	if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
		perror("SPI Message Send Failed");
	}
	usleep(50000);
}

void disp_center(int fd , struct spi_ioc_transfer xfer , char* writeBuf){
	int i = 0,j=0;	
	for(j=0;j<7;j++){
		for(i = 1 ; i<= 8 ; i++){
	
		writeBuf[1] = center[i-1]; // Step 1
		writeBuf[0] = (char)i;  //address
	
		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
			perror("SPI Message Send Failed");
		}
		usleep(500000);
		for(i = 1 ; i<= 8 ; i++){
	
		writeBuf[1] = 0x00; // Step 1
		writeBuf[0] = (char)i;  //address
	
		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
			perror("SPI Message Send Failed");
		}
		usleep(500000);
	}
}

void disp_level(int fd , struct spi_ioc_transfer xfer , char* writeBuf, int level){
		int i = 0;	
		for(i = 1 ; i<= 8 ; i++){
	
		writeBuf[1] = level_arr[level-1][i-1]; // Step 1
		writeBuf[0] = (char)i;  //address
	
		if(ioctl(fd, SPI_IOC_MESSAGE(1), &xfer) < 0)
			perror("SPI Message Send Failed");
		}
		usleep(500000);
}
time_t t;

void rand_init(){
	
	//printf("time =%lu\n",spec.tv_nsec%6);
	do{
		clock_gettime(CLOCK_REALTIME,&spec);
		r =  1 + spec.tv_nsec%6;		
	}while(r==4 || r==5);

	do{
		clock_gettime(CLOCK_REALTIME,&spec);
		c= 1 + spec.tv_nsec%6;
	}while(c==4 || c==5);
}
