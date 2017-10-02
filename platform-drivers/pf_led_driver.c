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

#define DRIVER_NAME "pf_led_driver"
static unsigned int gpio_number = 0;
static dev_t dev_no;			//For Device number major and minor
static struct cdev char_dev;	//For Char device struture
static struct class *class_dev;	//Pointer to the class structure of this device

static ssize_t gpio_read(struct file* file_p, char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "pf_led_driver: gpio_read +\n");
	unsigned int ret;
	char temp[2];
	ret = gpio_get_value(gpio_number);
	temp[0] = ret + '0';
	temp[1] = 0;
	printk(KERN_INFO "pf_led_driver: read value = %c\n", temp[0]);

	ret = 2; //size of data to be written
	if(*f_pos == 0)
	{
		printk(KERN_INFO "f_pos = 0\n");
		ret = copy_to_user(buf, &temp, 2);
		*f_pos = *f_pos + 2; //So that it will come out from cat command. Here 1 is number of bytes written.
		return 2;
	}
	else
	{
		return 0;
	}
}

static ssize_t gpio_write(struct file* file_p, const char *buf, size_t count, loff_t *f_pos)
{
	char temp;
	printk(KERN_INFO "pf_led_driver: gpio_write +\n");

	if (copy_from_user(&temp, buf, count))
	{
		return -EFAULT;
	}

	switch(temp)
	{
		case '0':
			gpio_set_value(gpio_number, 0);
			break;
		case '1':
			gpio_set_value(gpio_number, 1);
			break;
		default :
			printk(KERN_INFO "pf_led_driver: wrong option entered\n");
			break;
	}

	return count;
}
static int gpio_open(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "pf_led_driver: gpio_open +\n");
	return 0;
}

static int gpio_close(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "pf_led_driver: gpio_close +\n");
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

static int pf_led_driver_probe(struct platform_device *pdev)
{
	printk(KERN_INFO "pf_led_driver: pf_led_driver_probe +\n");
	struct device_node *np = pdev->dev.of_node;
	of_property_read_u32(np, "gpio-number", &gpio_number);
	int ret;
	struct device *dev_ret;
	if ((ret = alloc_chrdev_region(&dev_no, 0, 1, "pf_led_drv")) < 0)
	{
		printk(KERN_ALERT "pf_led_driver: Device registration failed\n");
		return ret;
	}
	printk("pf_led_driver: Major Nr: %d\n", MAJOR(dev_no));

	//Creatig a class with name gpiodrv. Will create a dir /sys/class/gpiodrv 
	if (IS_ERR(class_dev = class_create(THIS_MODULE, "pfleddrv")))
	{
		printk(KERN_ALERT "pf_led_driver: Class creation failed\n");
		unregister_chrdev_region(dev_no, 1);
		return PTR_ERR(class_dev);
	}

	if (IS_ERR(dev_ret = device_create(class_dev, NULL, dev_no, NULL, "pf_led_drv%d", 0)))
	{
		printk(KERN_ALERT "pf_led_driver: Device creation failed\n");
		class_destroy(class_dev);
		unregister_chrdev_region(dev_no, 1);
		return PTR_ERR(dev_ret);
	}

	cdev_init(&char_dev, &file_ops);

	if ((ret = cdev_add(&char_dev, dev_no, 1)) < 0)
	{
		printk(KERN_ALERT "pf_led_driver: Device addition failed\n");
		device_destroy(class_dev, dev_no);
		class_destroy(class_dev);
		unregister_chrdev_region(dev_no, 1);
		return ret;
	}

	ret = gpio_is_valid(gpio_number);
	if(ret < 0)
	{
			printk("pf_led_driver: gpio %d is not-valid error \n", gpio_number);
			return -1;
	}
	ret = gpio_request(gpio_number, "gpio_num1234");
	if(ret < 0)
	{
		printk("pf_led_driver: gpio %d request error \n", gpio_number);
		return -1;
	}
	gpio_direction_output(gpio_number, 1);

	return 0;
}

static int pf_led_driver_remove(struct platform_device *pdev)
{
	gpio_free(gpio_number);
	cdev_del(&char_dev);
	device_destroy(class_dev, dev_no);
	class_destroy(class_dev);
	unregister_chrdev_region(dev_no, 1);

	printk(KERN_INFO "pf_led_driver: pf_led_driver_remove -\n");
	return 0;
}

static const struct of_device_id led_driver_dt[] = {
            { .compatible = "my-led", },
                    { }
};  

MODULE_DEVICE_TABLE(of, led_driver_dt);
static struct platform_driver sample_pldriver = {
        .probe          = pf_led_driver_probe,
        .remove         = pf_led_driver_remove,
        .driver = {
                .name  = DRIVER_NAME,
                .of_match_table = of_match_ptr(led_driver_dt),
        },
};


static int gpio_init(void)
{
	printk(KERN_INFO "pf_led_driver: gpio_init +\n");
	// Registering with Kernel
        platform_driver_register(&sample_pldriver);

        return 0;
}

void gpio_exit(void)
{
	// Unregistering from Kernel
        platform_driver_unregister(&sample_pldriver);
	printk(KERN_INFO "pf_led_driver: gpio_exit -\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Platform LED driver example");

/*
 * Add this in your dts file :
 *	my-led {
 * 		compatible = "my-led";
 * 		gpio-number = <0x06>;
 * 	};
 * 
 * This will create a platform device :
 * 	/sys/devices/my-led
 *
 * To on-off led :
 * echo 1 > /dev/pf_led_drv0
 * echo 0 > /dev/pf_led_drv0
 *
 */

/*
 * static inline int of_property_read_u32(const struct device_node *np, const char *propname, u32 *out_value)
 *
 */
