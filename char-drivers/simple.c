#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

#define FIRST_MINOR 0
#define TOTAL_MINOR 1 


/*
 *This variable handles major and minor number of device.
 */
static dev_t dev_no;


/*
 *Init function
 */
static int __init myInit(void)
{
	int ret;
	ret = alloc_chrdev_region(&dev_no, FIRST_MINOR, TOTAL_MINOR, "simple");
	if (ret < 0)
	{
		printk(KERN_ERR "simple: simple register failed");
		return ret;
	}
	else
	{
		printk(KERN_INFO "simple: Namaskar, simple registered");
		printk(KERN_INFO "simple: smiple <Major, Minor>: <%d, %d>\n", MAJOR(dev_no), MINOR(dev_no));
	}
	return 0;
}


/*
 *Exit function
 */
static void __init myExit(void)
{
	printk(KERN_INFO "simple: simple unregistered");
	unregister_chrdev_region(dev_no, TOTAL_MINOR);
}

module_init(myInit);
module_exit(myExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Simple Character Driver Template");

/*
 * FIRST_MINOR  : First minor number, we can chose any number. Range will wtart from this number.
 * TOTAL_NUMBER : Total minor numbers required.
 * These both sould not be same, I faced crash at rmmod.
 * dev_no : This variable is used to store major number and minor number.
 * myInit() : Init function
 * myExit() : Exit function
 * In exit function, printk should not be on last, I faced crash if it was in last.
 *
 */


/*
 *
 *int alloc_chrdev_region (dev_t* dev, unsigned baseminor, unsigned count, const char* name);
 *	dev : output parameter for first assigned number 
 *	baseminor : first of the requested range of minor numbers
 *	count : the number of minor numbers required
 *	name : the name of the associated device or driver
 *	alloc_chrdev_region() :
 *		Allocates a range of char device numbers.
 *		The major number will be chosen dynamically, and returned (along with the first minor number) in dev.
 *		Returns zero or a negative error code. 
 */
 
 
/*
 *void unregister_chrdev_region (dev_t from, unsigned count)
 *	from : the first in the range of numbers to unregister 
 *	count: the number of device numbers to unregister
 *	unregister_chrdev_region() :
 *		This function will unregister a range of count device numbers, starting with from.
 *		The caller should normally be the one who allocated those numbers in the first place.
 */
