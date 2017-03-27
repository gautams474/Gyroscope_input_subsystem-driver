KDIR:=/home/esp/SDK/sysroots/i586-poky-linux/usr/src/kernel
#PWD:= $(shell pwd)

CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SROOT=/home/esp/SDK/sysroots/i586-poky-linux/

APP1 = main
#APP1 = test_app
#APP3 = tmpTest


obj-m:= gyro.o
 #gpio_kernel.o

all:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(KDIR) M=$(PWD) modules

	i586-poky-linux-gcc -o $(APP1) main.c -lm --sysroot=$(SROOT) 
#	i586-poky-linux-gcc -o $(APP1) trial1.c --sysroot=$(SROOT) 
#	i586-poky-linux-gcc -o $(APP3) tmpTest.c --sysroot=$(SROOT) 


clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f \.*.cmd
	rm -f Module.markers
	rm -f $(APP1)
#	rm -f $(APP2)
#	rm -f $(APP3)
