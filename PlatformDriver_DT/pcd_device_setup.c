#include<linux/module.h>
#include<linux/platform_device.h>

#include "platform.h"

/* To print the respective function name in every print statement in dmesg*/
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__ 

void pcdev_release(struct device *dev)
{
    pr_info("Device released\n");
}

/* 1. Create 2 platform data - structure form platform.h */
struct pcdev_platform_data pcdev_pdata[2] = {
    [0] = {.size = 512, .perm = RDWR, .serial_number = "PCDEV001"},
    [1] = {.size = 1024, .perm = RDWR, .serial_number = "PCDEV002"}
};

/* 2. Create 2 platflorm devices */
struct platform_device platform_pcdev1 = {
    .name = "pcdev-A1x",
    .id = 0, 
    .dev = {
        .platform_data = &pcdev_pdata[0], /* Making device specific data available */
        .release = pcdev_release /*Platform driver release function*/
    }
};

struct platform_device platform_pcdev2 = {
    .name = "pcdev-B1x",
    .id = 1,
    .dev = {
        .platform_data = &pcdev_pdata[1],
        .release = pcdev_release
    }
};

static int __init pcdev_platform_init(void)
{   
    /* Register 2 platform devices */
    platform_device_register(&platform_pcdev1);
    platform_device_register(&platform_pcdev2);

    pr_info("Device setup module loaded\n");

    return 0;
}

static void __exit pcdev_platform_exit(void)
{
    /* Unregister the devices*/
    platform_device_unregister(&platform_pcdev1);
    platform_device_unregister(&platform_pcdev2);

    pr_info("Device setup module unloaded\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Module registers platform devices");
MODULE_AUTHOR("SWAROOP");