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
#define TOTAL_MINOR 4		//Total minors
#define GPIO_LED_1 1		//gpio number
#define GPIO_LED_2 6		//gpio numner
#define GPIO_LED_3 7		//gpio numner
#define GPIO_LED_4 9		//gpio numner
unsigned int gpio_led[] = {GPIO_LED_1, GPIO_LED_2, GPIO_LED_3, GPIO_LED_4};
int i;

/*
 * It is not working with GPIO_NUMBER=7 !!... might be because I defined same pin in DTS file also for led class.!
 * To make it work, I added :
 * For init :
 * 	gpio_is_valid(GPIO_NUMBER);
 * 	gpio_request(GPIO_NUMBER, "GPIO_7");
 * 	gpio_direction_output(GPIO_NUMBER, 0);
 * For cleanup :
 * 	gpio_free(GPIO_NUMBER);
 * 
 * After on doing insmod I can see :
 * 	/sys/class/gpiodrv
 *
 */

/*
 * Radxa rock gpio pins
 * GPIO0_A5 = 165 = 5	//Can  use
 * GPIO0_A6 = 166 = 6	//Used by devesh in dts led class, check dts if not used
 * GPIO0_A7 = 167 = 7	//Can use
 * GPIO0_B0 = 168 = 8	//Can use
 * GPIO0_A1 = 161 = 1	//Can use
 * GPIO0_B1 = 169 = 9	//Can use
 * GPIO3_D4 = 284 = 124 //Can ?
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
	printk(KERN_INFO "gpio_led: gpio_read from = %d:%d\n", imajor(file_p->f_inode), iminor(file_p->f_inode) );
	unsigned int ret;
	char temp[2];
	int gpio_number;

	switch(iminor(file_p->f_inode))
	{
		case FIRST_MINOR:
			gpio_number = gpio_led[0];
			break;
		case 2:
			gpio_number = gpio_led[1];
			break;
		case 3:
			gpio_number = gpio_led[2];
			break;
		case TOTAL_MINOR:
			gpio_number = gpio_led[3];
			break;
		default:
			return -1;
	}
	ret = gpio_get_value(gpio_number);

	temp[0] = ret + '0';
	temp[1] = '\n';
	printk(KERN_INFO "gpio_led: read value = %c\n", temp[0]);

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
	unsigned int gpio_number = GPIO_LED_1;
	printk(KERN_INFO "gpio_led: gpio_write from = %d:%d\n", imajor(file_p->f_inode), iminor(file_p->f_inode) );

	if (copy_from_user(&temp, buf, count))
	{
		return -EFAULT;
	}

	switch(iminor(file_p->f_inode))
	{
		case FIRST_MINOR:
			gpio_number = gpio_led[0];
			break;
		case 2:
			gpio_number = gpio_led[1];
			break;
		case 3:
			gpio_number = gpio_led[2];
			break;
		case TOTAL_MINOR:
			gpio_number = gpio_led[3];
			break;
		default:
			return -1;
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
			printk(KERN_INFO "gpio_led: wrong option entered");
			break;
	}

	return count;
}
static int gpio_open(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "gpio_led: gpio_open from = %d:%d\n", imajor(inode), iminor(inode));
	return 0;
}

static int gpio_close(struct inode *inode, struct file *file_p)
{
	printk(KERN_INFO "gpio_led: gpio_close from = %d:%d\n", imajor(inode), iminor(inode));
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
	ret = alloc_chrdev_region(&dev_no, FIRST_MINOR, TOTAL_MINOR, "gpio-led");
	if (ret < 0)
	{
		printk(KERN_ALERT "gpio_led: Device registration failed\n");
		return ret;
	}
	printk("Major Nr: %d\n", MAJOR(dev_no));
	
	cdev_init(&char_dev, &file_ops);
	ret = cdev_add(&char_dev, dev_no, TOTAL_MINOR);
	if (ret < 0)
	{
		printk(KERN_ALERT "gpio_led: Device addition failed\n");
		unregister_chrdev_region(dev_no, 1);
		return ret;
	}
	
	//Creatig a class with name gpiodrv. Will create a dir /sys/class/gpio-led
	class_dev = class_create(THIS_MODULE, "gpio-led");
	if (IS_ERR(class_dev))
	{
		printk(KERN_ALERT "gpio_led: Class creation failed\n");
		unregister_chrdev_region(dev_no, 1);
		return PTR_ERR(class_dev);
	}
	
	dev_ret = device_create(class_dev, NULL, MKDEV(MAJOR(dev_no), FIRST_MINOR), NULL, "gpio_led%d", FIRST_MINOR);
	for(i = FIRST_MINOR + 1; i <= TOTAL_MINOR; i++)
	{
		dev_ret = device_create(class_dev, dev_ret, MKDEV(MAJOR(dev_no), i), NULL, "gpio_led%d", i);

		if (IS_ERR(dev_ret))
		{
			printk(KERN_ALERT "gpio_led: Device creation failed\n");
			device_destroy(class_dev, dev_no);
			class_destroy(class_dev);
			unregister_chrdev_region(dev_no, TOTAL_MINOR);
			return PTR_ERR(dev_ret);
		}
	}
	

	char gpio_name[8] = {0,};
	for(i = FIRST_MINOR-1; i < TOTAL_MINOR; i++)
	{
		printk(KERN_ALERT "gpio_led: i = %d\n", i);
		ret = gpio_is_valid(gpio_led[i]);
		if(ret < 0)
		{
			printk(KERN_ALERT "gpio_led: gpio %u is not-valid error \n", gpio_led[i]);
			return -1;
		}

		sprintf(gpio_name, "LED_%d", i+1);
		printk(KERN_ALERT "gpio_led: name = %s\n", gpio_name);
		ret = gpio_request(gpio_led[i], gpio_name);
		if(ret < 0)
		{
			printk(KERN_ALERT "gpio_led: gpio %u request error \n", gpio_led[i]);
			return -1;
		}
		gpio_direction_output(gpio_led[i], 1);  //by defatult output is high from second param
	}
	
	return 0;
}

void gpio_exit(void)
{
	for(i = TOTAL_MINOR; i >= FIRST_MINOR; i--) //Is it should be in that order !
	{
		gpio_free(gpio_led[i-1]);
		device_destroy(class_dev, MKDEV(MAJOR(dev_no), i));
	}
	class_destroy(class_dev);
	cdev_del(&char_dev);
	unregister_chrdev_region(dev_no, TOTAL_MINOR);

	printk(KERN_INFO "gpio_led: gpio_exit -\n");
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
