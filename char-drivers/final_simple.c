#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/uaccess.h>	/* copy_to_user   */
#include <linux/device.h>	/* device_destroy */
#include <linux/cdev.h>		/* cdev_del       */
#include <linux/slab.h>		/* kfree          */


#define FIRST_MINOR 1
#define TOTAL_MINOR 1 


/*
 *This variable handles major and minor number of device.
 */
static dev_t dev_no;
static struct cdev char_dev;
static struct class *final_simple_class;



static int myOpen(struct inode *i, struct file *f)
{
	printk(KERN_INFO "final_simple: myOpen() +");
	return 0;
}
static int myClose(struct inode *i, struct file *f)
{
	printk(KERN_INFO "final_simple: myClose() +");
	return 0;
}

char *sendData;
static ssize_t myRead(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	unsigned int ret = 12;
	printk(KERN_INFO "final_simple: myReade() +");
	sendData = "Hello World\n"; //12 bytes size
	if(*off == 0)
	{
		ret = copy_to_user(buf, sendData, 12);
		*off += 12;
	}	
	if(ret == 0)
	{
			printk(KERN_INFO "final_simple: Copied all to user");
	}
	else
	{
			printk(KERN_ERR "final_simple: Pending bytes to copy %u", ret);
	}

//	return 0;
	return (12-ret);
/*
 * On doing cat /dev/final_simple0 :  This function wil be called two times 
 */
}

static char* receiveData;
static ssize_t myWrite(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	ssize_t ret;
	printk(KERN_INFO "final_simple: myWrite() +");
	if(len <= 0)
	{
		printk(KERN_INFO "final_simple: No data receive, returning");
		return 0;
	}

	//receiveData = kmalloc(len, GFP_KERNEL); //non initialize data. while printing, it may print garbage also.
	receiveData = kzalloc(len, GFP_KERNEL); //initiaize with 0, it will not print garbage while printing.

	ret = copy_from_user(receiveData, buf, len);
	if(ret > 0)
	{
		printk(KERN_ERR "final_simple: Not receive bytes : %u", ret);
	}
	
	printk(KERN_INFO "final_simple: receiveData = %s", receiveData);
	printk(KERN_INFO "final_simple: after printing receivedData.");
	kfree(receiveData);

	return (len-ret);
}

static struct file_operations final_simple_fops =
{
	.owner = THIS_MODULE,
	.open = myOpen,
	.release = myClose,
	.read = myRead,
	.write = myWrite
};

/*
 *Init function
 */
static int __init myInit(void)
{
	int ret;
	struct device *device_ret;

	ret = alloc_chrdev_region(&dev_no, FIRST_MINOR, TOTAL_MINOR, "final_simple");
	if (ret < 0)
	{
		printk(KERN_ERR "final_simple: final_simple register failed");
		return ret;
	}
	else
	{
		printk(KERN_INFO "final_simple: Namaskar, final_simple registered");
		printk(KERN_INFO "final_simple: final_smiple <Major, Minor>: <%d, %d>\n", MAJOR(dev_no), MINOR(dev_no));
	}

	cdev_init(&char_dev, &final_simple_fops);
	ret = cdev_add(&char_dev, dev_no, TOTAL_MINOR);
	if(ret < 0)
	{
		printk(KERN_ERR "final_simple: final_simple cdev_add failed");
		unregister_chrdev_region(dev_no, TOTAL_MINOR);
		return ret;
	}

	final_simple_class = class_create(THIS_MODULE, "char");
	if(IS_ERR(final_simple_class))
	{
		printk(KERN_ERR "final_simple: final_simple class_create failed");
		cdev_del(&char_dev);
		unregister_chrdev_region(dev_no, TOTAL_MINOR);
		return PTR_ERR(final_simple_class);
	}


	//Create sysfs entry in /sys/class/char and device node in /dev/
	device_ret = device_create(final_simple_class, NULL, dev_no, NULL, "final_simple%d", FIRST_MINOR);
	if(IS_ERR(device_ret))
	{
		class_destroy(final_simple_class);
		cdev_del(&char_dev);
		unregister_chrdev_region(dev_no, TOTAL_MINOR);
		return PTR_ERR(device_ret);
	}

	return 0;
}


/*
 *Exit function
 */
static void __exit myExit(void)
{
	device_destroy(final_simple_class, dev_no);
	class_destroy(final_simple_class);
	cdev_del(&char_dev);
	unregister_chrdev_region(dev_no, TOTAL_MINOR);
	printk(KERN_INFO "final_simple: final_simple unregistered");
	printk(KERN_INFO "final_simple: after printing exit message");
}

module_init(myInit);
module_exit(myExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Final Simple Character Driver Template");


/*
 * If you declare vars inbetween code, warning: ISO C90 forbids mixed declarations and code
 *
 */

/*
 * FIRST_MINOR  : First minor number, we can chose any number. Range will wtart from this number.
 * TOTAL_MINOR  : Total minor numbers required.
 * dev_no : This variable is used to store major number and minor number.
 * myInit() : Init function
 * myExit() : Exit function
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
 *void unregister_chrdev_region (dev_t from, unsigned count);
 *	from : the first in the range of numbers to unregister 
 *	count: the number of device numbers to unregister
 *	unregister_chrdev_region() :
 *		This function will unregister a range of count device numbers, starting with from.
 *		The caller should normally be the one who allocated those numbers in the first place.
 */

/*
 *void cdev_init (struct cdev* cdev, const struct file_operations* fops);
 *	cdev : the structure to initialize
 *	fops : the file_operations for this device
 *	cdev_init() :
 *		Initializes cdev, remembering fops, making it ready to add to the system with cdev_add.
 */

/*
 *int cdev_add (struct cdev* p, dev_t dev, unsigned count);
 *	p   : the cdev structure for the device
 *	dev : the first device number for which this device is responsible
 *	count : the number of consecutive minor numbers corresponding to this device
 *	cdev_add() :
 *		cdev_add adds the device represented by p to the system, making it live immediately.
 *		A negative error code is returned on failure.
 */

/*
 *struct class * class_create (struct module* owner, const char* name);
 *	owner : pointer to the module that is to “own” this struct class
 *	name  : pointer to a string for the name of this class
 *	class_create() :
 *		This is used to create a struct class pointer that can then be used in calls to class_device_create.
 *		Note, the pointer created here is to be destroyed when finished by making a call to class_destroy.
 */

/*
 *struct device* device_create(struct class* class, struct device* parent, dev_t devt, const char* fmt, ...);
 *	class  : pointer to the struct class that this device should be registered to
 *	parent : pointer to the parent struct device of this new device, if any
 *	devt : the dev_t for the char device to be added
 *	fmt  : string for the device's name
 *	device_create() :
 *		This function can be used by char device classes. A struct device will be created in sysfs, registered to the specified class.
 *		A “dev” file will be created, showing the dev_t for the device, if the dev_t is not 0,0.
 *		If a pointer to a parent struct device is passed in, the newly created struct device will be a child of that device in sysfs.
 *		The pointer to the struct device will be returned from the call.
 *		Any further sysfs files that might be required can be created using this pointer.
 *		The struct class passed to this function must have previously been created with a call to class_create.
 *
 */
