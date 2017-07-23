#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define FIRST_MINOR 1		//First minor
#define TOTAL_MINOR 1		//Total minors
#define GPIO_NUMBER 6		//gpio number

/*
 * It is not working with GPIO_NUMBER=7 !!... might be because I defined same pin in DTS file also for led class.!
 *
 */

/*
 * Radxa rock gpio pins
 * GPIO0_A6 = 166 = 6
 * GPIO0_A7 = 167 = 7
 * GPIO0_A1 = 161 = 1
 * GPIO0_B1 = 169 = 9
 * GPIO3_D4 = 284 = 124
 * GPIO3_D5 = 285 = 125
 * GPIO1_A0 = 192 = 31
 * GPIO1_A1 = 193 = 32
 * GPIO1_A2 = 194 = 33
 * GPIO1_A3 = 195 = 34
 * For mapping others reffer:
 * http://wiki.radxa.com/Rock/GPIO and http://wiki.radxa.com/File:Extension_header_funca.png
 **BDFORE DOING ANY CHANGE IN GPIO pins, PLEASE REFFER ABOVE LINKS PROPERLY
 */ 

static dev_t dev_no;			//For Device number major and minor
static struct cdev char_dev;	//For Char device struture
static struct class *class_dev;	//Pointer to the class structure of this device

static ssize_t gpio_read(struct file* file_p, char *buf, size_t count, loff_t *f_pos)
{
	unsigned char temp = gpio_get_value(GPIO_NUMBER);

	if (copy_to_user(buf, &temp, 1))
	{
		return -EFAULT;
	}
	return count;
}

static ssize_t gpio_write(struct file* file_p, const char *buf, size_t count, loff_t *f_pos)
{
	char temp;
	printk(KERN_INFO "gpio_led: gpio_write +");

	if (copy_from_user(&temp, buf, count))
	{
		return -EFAULT;
	}

	switch(temp)
	{
		case '0':
			gpio_set_value(GPIO_NUMBER, 0);
			break;
		case '1':
			gpio_set_value(GPIO_NUMBER, 1);
			break;
		default :
			printk(KERN_INFO "gpio_led: wrong option entered");
			break;
	}

	return count;
}
static int gpio_open(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "gpio_led: gpio_open +");
	return 0;
}

static int gpio_close(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "gpio_led: gpio_close +");
	return 0;
}

static struct file_operations file_ops =
{
	.owner		= THIS_MODULE,
	.open		= gpio_open,
	.read		= gpio_read,
	.write		= gpio_write,
	.release	= gpio_close,
};

static int gpio_init(void)
{
	int ret;
	struct device *dev_ret;
	printk(KERN_INFO "gpio_led: gpio_init +");
	if ((ret = alloc_chrdev_region(&dev_no, 0, 1, "gpio_drv")) < 0)
	{
		printk(KERN_ALERT "Device registration failed\n");
		return ret;
	}
	printk("Major Nr: %d\n", MAJOR(dev_no));

	if (IS_ERR(class_dev = class_create(THIS_MODULE, "gpiodrv")))
	{
		printk(KERN_ALERT "Class creation failed\n");
		unregister_chrdev_region(dev_no, 1);
		return PTR_ERR(class_dev);
	}

	if (IS_ERR(dev_ret = device_create(class_dev, NULL, dev_no, NULL, "gpio_drv%d", 0)))
	{
		printk(KERN_ALERT "Device creation failed\n");
		class_destroy(class_dev);
		unregister_chrdev_region(dev_no, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&char_dev, &file_ops);

	if ((ret = cdev_add(&char_dev, dev_no, 1)) < 0)
	{
		printk(KERN_ALERT "Device addition failed\n");
		device_destroy(class_dev, dev_no);
		class_destroy(class_dev);
		unregister_chrdev_region(dev_no, 1);
		return ret;
	}

	return 0;
}

void gpio_exit(void)
{
	cdev_del(&char_dev);
	device_destroy(class_dev, dev_no);
	class_destroy(class_dev);
	unregister_chrdev_region(dev_no, 1);

	printk(KERN_INFO "gpio_led: gpio_exit -\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("GPIO LED example");
