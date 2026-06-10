obj-m := pcd_device_setup.o pcd_platform_driver_dt.o
ARCH=arm
CROSS_COMPILE=arm-linux-gnueabihf-
#set KERN_DIR to linux source location 
KERN_DIR = /home/swaroop/Embedded_Systems/Embedded_Linux/Workspace_Drivers/ldd/source/linux_bbb_5.4
HOST_KERN_DIR = /lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) modules
clean:
	make -C $(HOST_KERN_DIR) M=$(PWD) clean
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) clean
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERN_DIR) M=$(PWD) help
host:
	make -C $(HOST_KERN_DIR) M=$(PWD) modules
copy-dtb:
	scp /home/swaroop/Embedded_Systems/Embedded_Linux/Workspace_Drivers/ldd/source/linux_bbb_5.4/arch/arm/boot/dts/am335x-boneblack.dtb debian@192.168.1.2:/home/debian/myfiles

copy-drv:
	scp *.ko debian@192.168.1.2:/home/debian/myfiles
