/**
 * Raspberry Pi's firmware interface device driver
 *
 * Creates a chardev /dev/rpi-fi which will provide user access to Raspberry Pi's
 * firmware settings via ioctl()
 *
 * Written by MkfsSion <mkfssion@mkfssion.com>
 * Copyright (c) 2019, MkfsSion
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <soc/bcm2835/raspberrypi-firmware.h>

#define DEVICE_NAME "rpi-fi"
#define DRIVER_NAME "rpi-fi"
#define DEVICE_MINOR 0

struct rpi_fi_private {
	struct device *dev;
	struct cdev rpi_fi_cdev;
	dev_t rpi_fi_devid;
	struct class *rpi_fi_class;
	struct device *rpi_fi_dev;
};

static struct rpi_fi_private *rfp;

static int rpi_fi_open(struct inode *inode, struct file *file)
{
	int devid = iminor(inode);
	int ret = 0;

	if (devid != DEVICE_MINOR)
	{
		dev_err(rfp->dev, "Unknown minor device %d", devid);
		ret = -ENXIO;
	}
	return ret;
}

static int rpi_fi_release(struct inode *inode, struct file *file)
{
	int devid = iminor(inode);
	int ret = 0;

	if(devid != DEVICE_MINOR)
	{
		dev_err(rfp->dev, "Unknown minor device %d", devid);
		ret = -ENXIO;
	}
	return ret;
}

static long rpi_fi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	return ret;
}

static const struct file_operations rpi_fi_fops = {
	.owner = THIS_MODULE,
	.open = rpi_fi_open,
	.release = rpi_fi_release,
	.unlocked_ioctl = rpi_fi_ioctl
};

static int rpi_fi_dev_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static int rpi_fi_probe(struct platform_device *pdev)
{
	int ret;

	/* Allocate memory for private data */
	rfp = kzalloc(sizeof(struct rpi_fi_private), GFP_KERNEL);

	if(!rfp)
	{
		ret = -ENOMEM;
		goto failed_private_alloc;
	}

	rfp->dev = &pdev->dev;

	ret = alloc_chrdev_region(&rfp->rpi_fi_devid, DEVICE_MINOR, 1, DEVICE_NAME);

	if (ret)
	{
		dev_err(rfp->dev, "Failed to allocate device minor number");
		goto failed_alloc_chrdev;
	}

	cdev_init(&rfp->rpi_fi_cdev, &rpi_fi_fops);
	rfp->rpi_fi_cdev.owner = THIS_MODULE;
	ret = cdev_add(&rfp->rpi_fi_cdev, rfp->rpi_fi_devid, 1);

	if (ret)
	{
		dev_err(rfp->dev, "Failed to register device");
		goto failed_cdev_add;
	}

	rfp->rpi_fi_class = class_create(THIS_MODULE, DEVICE_NAME);

	if (IS_ERR(rfp->rpi_fi_class))
	{
		ret = PTR_ERR(rfp->rpi_fi_class);
		goto failed_class_create;
	}
	rfp->rpi_fi_class->dev_uevent = rpi_fi_dev_uevent;

	rfp->rpi_fi_dev = device_create(rfp->rpi_fi_class, NULL, rfp->rpi_fi_devid, NULL, DEVICE_NAME);

	if(IS_ERR(rfp->rpi_fi_dev))
	{
		ret = PTR_ERR(rfp->rpi_fi_dev);
		goto failed_device_create;
	}

	dev_info(rfp->dev, "Register %s driver successfully", DRIVER_NAME);

	return ret;

failed_device_create:
	class_destroy(rfp->rpi_fi_class);

failed_class_create:
	cdev_del(&rfp->rpi_fi_cdev);

failed_cdev_add:
	unregister_chrdev_region(rfp->rpi_fi_devid, 1);

failed_alloc_chrdev:
	kfree(rfp);

failed_private_alloc:
	dev_err(&pdev->dev, "Failed to load %s driver", DRIVER_NAME);
	return ret;
}

static int rpi_fi_remove(struct platform_device *pdev)
{
	device_destroy(rfp->rpi_fi_class, rfp->rpi_fi_devid);
	class_destroy(rfp->rpi_fi_class);
	cdev_del(&rfp->rpi_fi_cdev);
	unregister_chrdev_region(rfp->rpi_fi_devid, 1);
	kfree(rfp);

	dev_info(&pdev->dev, "Raspberry Pi's firmware interface driver removed");
	return 0;
}


static const struct of_device_id rpi_fi_of_match[] = {
	{ .compatible = "raspberrypi,bcm2835-firmware-interface" },
	{}
};


MODULE_DEVICE_TABLE(of, rpi_fi_of_match);

static struct platform_driver rpi_fi_driver = {
	.probe = rpi_fi_probe,
	.remove = rpi_fi_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = rpi_fi_of_match
	}
};

module_platform_driver(rpi_fi_driver);

MODULE_ALIAS("platform:rpi-fi");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Raspberry Pi's firmware interface driver");
MODULE_AUTHOR("MkfsSion <mkfssion@mkfssion.com>");
