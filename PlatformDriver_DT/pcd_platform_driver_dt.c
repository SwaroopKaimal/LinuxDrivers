#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>
#include<linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/of.h>
#include<linux/of_device.h>

#include "platform.h"

#define MAX_DEVICES 10

/* To print the respective function name in every print statement in dmesg*/
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__ 

/* Device private data structures - Structure to store data for each device file */
struct pcdev_private_data
{
	struct pcdev_platform_data pdata; /* Will have serial no, size and perm */
	char *buffer; /* Pointer to memory buffer allocated to the devices */
	dev_t dev_num; /* Holds device number */
	struct cdev cdev; /* This structure is needed for each device file */
};

/*Memory will be allocated to device private structure only through probe function, dynamically */

/* Driver's private data structure - Structure to store data of this driver */
struct pcdrv_private_data
{
	int total_devices;
	dev_t device_num_base; /* Store the base of device number */
	struct class *class_pcd;
  	struct device *device_pcd;

};

/*Driver's private data instance - for driver create one variable globally */
struct pcdrv_private_data pcdrv_data;

static loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*) filp->private_data; /* Typecast from void pointer */

	int max_size = pcdev_data->pdata.size;
	
	
	loff_t temp;

	pr_info("lseek requested\n");
	pr_info("Current value of position = %lld\n",filp->f_pos);	

	switch(whence){

		case SEEK_SET:
			if( (offset>max_size) || (offset < 0))
				return -EINVAL;
			filp->f_pos = offset;	
			break;

		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp>max_size) || (temp<0))
				return -EINVAL;
			filp->f_pos = temp;
			break;	

		case SEEK_END:
			temp = max_size + offset;
			if((temp>max_size) || (temp<0))
				return -EINVAL;
			filp->f_pos = temp;
			break;

		default:
			return -EINVAL;
	}
	
	pr_info("Updated value of file  position = %lld\n",filp->f_pos);	
	return filp->f_pos;
}

static ssize_t pcd_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{	
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*) filp->private_data; /* Typecast from void pointer */

	int max_size = pcdev_data->pdata.size;

	pr_info("read requested for %zu bytes\n",count);
	pr_info("Current value of position = %lld\n",*f_pos);	
	
	/* Adjust the count */
	if((*f_pos + count) > max_size)
	       count = max_size - *f_pos;

	/* Copy to user */
	if(copy_to_user(buff, pcdev_data->buffer + (*f_pos), count)){
		return -EFAULT;
	}

	/* Update the current file position */
	*f_pos += count;

	pr_info("Number of bytes sucessfully read = %zu\n", count);
	pr_info("Updated value of current position = %lld\n",*f_pos);	

	/* Return number of bytes successfully read*/
	return count;
}

static ssize_t pcd_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*) filp->private_data; /* Typecast from void pointer */

	int max_size = pcdev_data->pdata.size;

	pr_info("write requested for %zu bytes\n",count);
	pr_info("Current value of position = %lld\n",*f_pos);	
	
	/* Adjust the count */
	if((*f_pos + count) > max_size)
	       count = max_size - *f_pos;

	if(!count)
		return -ENOMEM;

	/* Copy from user */
	if(copy_from_user(pcdev_data->buffer + (*f_pos), buff, count)){
		return -EFAULT;
	}

	/* Update the current file position */
	*f_pos += count;

	pr_info("Number of bytes sucessfully written = %zu\n", count);
	pr_info("Updated value of current position = %lld\n",*f_pos);	

	/* Return number of bytes successfully written*/
	return count;
}
static int check_permission(int dev_perm, int acc_mode){

	if(dev_perm == RDWR)
		return 0;
	
	/* Ensures read only access */
	if((dev_perm == RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE) ))
		return 0;

	/* Ensures write only access */
	if((dev_perm == WRONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ) ))
		return 0;

	return -EPERM;
}

static int pcd_open(struct inode *inode, struct file *filp)
{
	int ret;
	
	int minor_n;

	struct pcdev_private_data *pcdev_data;
	
	/* Find out on which device file the open was attempted by user space */
	minor_n = MINOR(inode->i_rdev); /* Driver uses minor number to distinguish between different device files */
	pr_info("minor number: %d\n", minor_n);

	/* Get device's private data structure - device specifc info is fetched */
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev); 
	/* We get the address of pcdev_private_data structure which is the "container" of .cdev member, which is also present in inode structure */
	
	/* This extracted private device data structure is required for other functions like read and write, but these functions do not have inode 
	argument in their fuctions. So we have to save this structure for other methods to use. We save this inside the struct file *filp argument */

	filp->private_data = pcdev_data;

	/* Check permission*/
	ret = check_permission(pcdev_data->pdata.perm, filp->f_mode);

	(!ret) ? pr_info("open was successful\n") : pr_info("open unsuccesseful");
	
	return 0;
}

static int pcd_release(struct inode *inode, struct file *filp)
{
	pr_info("release was succesful\n");
	
	return 0;
}

struct file_operations pcd_fops = /* Setting up members of the structure C99 standard - mapping user layer system calls to driver functions */ 
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

struct pcdev_platform_data* pcdev_get_platform_data_from_dt(struct device *dev)
{
	/*Check of_node member of the struct device_node. If NULL, then device not on device tree */
	struct device_node *dev_node = dev->of_node;

	struct pcdev_platform_data *pdata;

	if(!dev_node)
		/*If probe did not happen because of device tree matching, manual matching might have happened, 
		with manual platform device registration function */
		return NULL;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if(!pdata){
		dev_info(dev, "Cannot allocate memory \n");
		return ERR_PTR(-ENOMEM);
	}
	/* Below code is for fetcing the properties of devices from the tree node */

	if(of_property_read_string(dev_node, "org,device-serial-num" /*In DTS file*/, &pdata->serial_number))
	{
		dev_info(dev, "Missing serial number property \n");
		return ERR_PTR(-EINVAL);
	}

	if(of_property_read_u32(dev_node,"org,size",&pdata->size) ){
		dev_info(dev,"Missing size property\n");
		return ERR_PTR(-EINVAL);
	}

	if(of_property_read_u32(dev_node,"org,perm",&pdata->perm) ){
		dev_info(dev,"Missing permission property\n");
		return ERR_PTR(-EINVAL);
	}


	return pdata;
}	

struct of_device_id org_pcdev_dt_match[];

/* Gets called when matched platform device is found */
static int pcd_platform_driver_probe(struct platform_device *pdev)
{
	int ret;
	
	struct pcdev_private_data *dev_data;

	struct pcdev_platform_data *pdata;

	struct device *dev = &pdev->dev;

	int driver_data;

	/* used to store matched entry of 'of_device_id' list of this driver */
	const struct of_device_id *match;

	pr_info(" Device is detected\n");

	
	/*1. Get the platform data - if probe is called, a platform device must exist
	
	Code to check and extract data from the device tree node*/

	/* Check if the device is on a device tree node - if yes, ofnode member will not be NULL*/
	
	/*match will always be NULL if LINUX doesnt support device tree i.e CONFIG_OF is off */
	match = of_match_device(of_match_ptr(org_pcdev_dt_match),dev);

	if(match){
		pdata = pcdev_get_platform_data_from_dt(dev);
		if(IS_ERR(pdata))
			return PTR_ERR(pdata);
		driver_data = (long)match->data;
	}else{
		pdata = (struct pcdev_platform_data*)dev_get_platdata(dev);
		driver_data =  pdev->id_entry->driver_data;
	}
	
	if(!pdata){
		pr_info("No platform data available\n");
		return -EINVAL;
	}

	/*2. Dynamically allocate memory for the device private data  */
	dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data),GFP_KERNEL);
	if(!dev_data){
		pr_info("Cannot allocate memory \n");
		return -ENOMEM;
	}
	
	/*3. Save the device private data pointer in platform device structure. This
	will be later used in the remove() function to remove a device */
	dev_set_drvdata(&pdev->dev,dev_data);

	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("Device serial number = %s\n",dev_data->pdata.serial_number);
	pr_info("Device size = %d\n", dev_data->pdata.size);
	pr_info("Device permission = %d\n",dev_data->pdata.perm);

	/*4. Dynamically allocate memory for the device buffer using size information from the platform data */
	dev_data->buffer = devm_kzalloc(&pdev->dev, sizeof(*dev_data),GFP_KERNEL);
	if(!dev_data){
		pr_info("Cannot allocate memory \n");
		return -ENOMEM;
	}

	/*5. Get the device number - add ID of device to base number of driver */
	dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

	/*6. Do cdev init and cdev add */
	cdev_init(&dev_data->cdev,&pcd_fops);
	
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev,dev_data->dev_num,1);
	if(ret < 0){
		pr_err("Cdev add failed\n");
		return ret;
	}

	/*7. Create device file for the detected platform device */
	pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,dev_data->dev_num,NULL,"pcdev-%d",pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.device_pcd)){
		pr_err("Device create failed\n");
		ret = PTR_ERR(pcdrv_data.device_pcd);
		cdev_del(&dev_data->cdev);
		return ret;
	}

	pcdrv_data.total_devices++;

	pr_info(" Probe was successful\n");


	return 0;
}

/* Gets called when device is removed from system */
/*For newer kernels return type is void for remove fucntion, use prototype 
  for 5.4 use int type return */
static void pcd_platform_driver_remove(struct platform_device *pdev)
{
	pr_info(" Device is removed\n");

	/* We need a method to access the dev_data private variable in this function
	Use dev_get_drvdata() to pass this data from probe() to remove()*/
	struct pcdev_private_data  *dev_data = dev_get_drvdata(&pdev->dev);

	/*1. Remove a device that was created with device_create()*/
	device_destroy(pcdrv_data.class_pcd,dev_data->dev_num);

	/*2. Remove a cdev entry from the system */
	cdev_del(&dev_data->cdev);

	/*3. NOT NEEDED if using devm_kzalloc(). Free the memory held by the device */
	//kfree(dev_data->buffer);
	//kfree(dev_data);

	pcdrv_data.total_devices--;

	pr_info(" Device is removed\n");
}

/* Configuration data of the devices which will be passed in the .data member of the structure*/
enum pcdev_names
{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X
};

/* This structure is used when registering the devices manually with 
platform_device_register() API. This cannot be used with nodes in device tree. We need to use the of_device_id structure*/
struct platform_device_id pcdevs_ids[] =
{
	[0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x", .driver_data = PCDEVB1X},

	{} //Must be a null terminated array
	/* .driver_data can be loaded with the configuration data of devices  */
};

struct of_device_id org_pcdev_dt_match[] = {
	{.compatible = "pcdev-A1x", .data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x", .data = (void*)PCDEVB1X},
	{}
};


struct platform_driver pcd_platform_driver = 
{
    .probe = pcd_platform_driver_probe,
	.remove = pcd_platform_driver_remove,
	.driver = {.name = "pseudo-char-device",
	.of_match_table = org_pcdev_dt_match}, /* This will become redundant when we use ID based matching */
	.id_table = pcdevs_ids,
	
};


static int __init pcd_platform_driver_init(void)
{
	int ret;

	/*1. Dynamically allocate a device number for MAX_DEVICES */
	ret = alloc_chrdev_region(&pcdrv_data.device_num_base,0,MAX_DEVICES,"pcdevs");
	if(ret < 0){
		pr_err("Alloc chrdev failed\n");
		return ret;
	}

	/*2. Create device class under /sys/class. Need to be done only once so keep it here. Probe function 
	can be called multiple times when matching happends in the platform bus*/
	pcdrv_data.class_pcd = class_create("pcd_class");
	if(IS_ERR(pcdrv_data.class_pcd)){
		pr_err("Class creation failed\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
		return ret;
	}

	/*3. Register a platform driver */
	platform_driver_register(&pcd_platform_driver);
	pr_info(" pcd platform driver loaded\n");

	return 0;
}


static void __exit pcd_platform_driver_cleanup(void)
{
	/*1. Unregister the platform driver */
	platform_driver_unregister(&pcd_platform_driver);

	/*2. Class destroy */
	class_destroy(pcdrv_data.class_pcd);

	/*3. Unregister device numbers for MAX_DEVICES */
	unregister_chrdev_region(pcdrv_data.device_num_base,MAX_DEVICES);
	
	pr_info("pcd platform driver unloaded\n");

}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SWAROOP");
MODULE_DESCRIPTION("Pseudo character driver handling 'n' devices");