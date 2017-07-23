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
#define GPIO_NUMBER 4		//gpio number 4 is input key on radxa (big button)

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
	printk(KERN_INFO "gpio_key: gpio_read +\n");
	unsigned int ret;
	char temp[2];
	ret = gpio_get_value(GPIO_NUMBER);
	temp[0] = ret + '0';
	temp[1] = 0;
	printk(KERN_INFO "gpio_key: read value = %c\n", temp[0]);

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

#if 0
/*
 * This example is only for demonstrating reading from gpio pin.
 * Not for writing.
 */
static ssize_t gpio_write(struct file* file_p, const char *buf, size_t count, loff_t *f_pos)
{
	char temp;
	printk(KERN_INFO "gpio_key: gpio_write +");

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
			printk(KERN_INFO "gpio_key: wrong option entered");
			break;
	}

	return count;
}
#endif
static int gpio_open(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "gpio_key: gpio_open +");
	return 0;
}

static int gpio_close(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "gpio_key: gpio_close +");
	return 0;
}

static struct file_operations file_ops =
{
	.owner		= THIS_MODULE,
	.open		= gpio_open,
	.read		= gpio_read,
	.release	= gpio_close,
};

static int gpio_init(void)
{
	int ret;
	struct device *dev_ret;
	printk(KERN_INFO "gpio_key: gpio_init +");
	if ((ret = alloc_chrdev_region(&dev_no, 0, 1, "gpio_key")) < 0)
	{
		printk(KERN_ALERT "Device registration failed\n");
		return ret;
	}
	printk("Major Nr: %d\n", MAJOR(dev_no));

	//Creatig a class with name gpiodrv. Will create a dir /sys/class/gpiodrv 
	if (IS_ERR(class_dev = class_create(THIS_MODULE, "gpiokey")))
	{
		printk(KERN_ALERT "Class creation failed\n");
		unregister_chrdev_region(dev_no, 1);
		return PTR_ERR(class_dev);
	}

	if (IS_ERR(dev_ret = device_create(class_dev, NULL, dev_no, NULL, "gpio_key%d", 0)))
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
#if 0
/*
 * Since GPIO0_A4 (power key) is already defined in dts, I faced error while doing insmod 
 * code. That is why commenting it out. But you can read from /dev/gpio_key0.
 * Let us check in next versin of gpio example with some other gpio key.
 */

	ret = gpio_is_valid(GPIO_NUMBER);
	if(ret < 0)
	{
		printk("gpio_key: gpio %d is not-valid error \n", GPIO_NUMBER);
		device_destroy(class_dev, dev_no);
		class_destroy(class_dev);
		unregister_chrdev_region(dev_no, 1);
		return -1;
	}
	ret = gpio_request(GPIO_NUMBER, "GPIO_4");
	if(ret < 0)
	{
		printk("gpio_key: gpio %d request error \n", GPIO_NUMBER);
		device_destroy(class_dev, dev_no);
		class_destroy(class_dev);
		unregister_chrdev_region(dev_no, 1);
		return -1;
	}
	gpio_direction_input(GPIO_NUMBER);
#endif
	return 0;
}

void gpio_exit(void)
{
	gpio_free(GPIO_NUMBER);
	cdev_del(&char_dev);
	device_destroy(class_dev, dev_no);
	class_destroy(class_dev);
	unregister_chrdev_region(dev_no, 1);

	printk(KERN_INFO "gpio_key: gpio_exit -\n");
}

module_init(gpio_init);
module_exit(gpio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("GPIO LED example");



/*
 * If before doing insmod you creat a normal file in /dev/ with same name as that module will create,
 * You will not able to use that node file /dev/.. Please delete it before loading module.
 * Or you have to delete, and create mannually using mknod
 *
 */


/*
 * int gpio_is_valid(int number);
 *	number : gpio pin number GPIO_NUMBER.
 *	gpio_is_valid()
 *		checks gpio pin number is valid or not
 */

/*
 * int gpio_request(unsigned gpio, const char *label);
 *	gpio : gpio pin number
 *	label : ?, this is used for debug purpose
 *			Check cat /sys/kernel/debug/gpio 
 *
 */
/*
 * void gpio_direction_output (int id, bool value)
 * 	id : the pin to switch direction
 *	value : the initial value to drive on the pin (0/1 on/off)
 *	gpio_direction_output () :
 *		GPIO pins can be used for input or output. This function make the specified pin an output pin.
 *		The pin state can the be changed using gpio_set_pin.
 *		But I am doing pin state change with gpio_set_vapue() !
 */
