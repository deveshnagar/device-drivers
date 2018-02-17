#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> // required for various structures related to files liked fops.
#include <asm/uaccess.h> // required for copy_from and copy_to user functions
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define FIRST_MINOR 0
#define TOTAL_MINOR 1

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
struct task_struct *task;
static struct rw_semaphore rwsem;


int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "rwsem:open: Inside open \n");
	task = current;
	return 0;
}

int release(struct inode *inode, struct file *filp) 
{
	printk (KERN_INFO "rwsem:release: Inside close \n");
	return 0;
}

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp) 
{

	printk("Inside read \n");
	down_read(&rwsem);
	printk(KERN_INFO "rwsem:read: Got the Semaphore in Read\n");
	printk(KERN_INFO"rwsem:read: Going to Sleep\n");
	ssleep(30);
	up_read(&rwsem);
	return 0;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) 
{   
	printk(KERN_INFO "rwsem:write: Inside write \n");
	down_write(&rwsem);
	printk(KERN_INFO "rwsem:write: Got the Semaphore in Write\n");
	ssleep(30);
	up_write(&rwsem);
	return count;
}

struct file_operations fops = {
	read:        read,
	write:        write,
	open:         open,
	release:    release
};


int rw_sem_init (void) 
{
	if (alloc_chrdev_region(&dev, FIRST_MINOR, TOTAL_MINOR, "SCD") < 0)
	{
		return -1;
	}
	printk(KERN_INFO"rwsem:rw_sem_init: Major Nr: %d\n", MAJOR(dev));

	cdev_init(&c_dev, &fops);

	if (cdev_add(&c_dev, dev, TOTAL_MINOR) == -1)
	{
		unregister_chrdev_region(dev, TOTAL_MINOR);
		return -1;
	}

	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, TOTAL_MINOR);
		return -1;
	}
	if (IS_ERR(device_create(cl, NULL, dev, NULL, "rwsem%d", 0)))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, TOTAL_MINOR);
		return -1;
	}
	init_rwsem(&rwsem);

	return 0;
}

void rw_sem_cleanup(void) 
{
	printk(KERN_INFO "rwsem:rw_sem_cleanup: Inside cleanup_module\n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, TOTAL_MINOR);
}

module_init(rw_sem_init);
module_exit(rw_sem_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Reader Writer Semaphore Example");


/*
 *

struct rw_semaphore {
	atomic_long_t count;
	struct list_head wait_list;
	raw_spinlock_t wait_lock;
#ifdef CONFIG_RWSEM_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; // spinner MCS lock 
	//
	// Write owner. Used as a speculative check to see
	// if the owner is running on the cpu.
	//
	struct task_struct *owner;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
};

 *
 */

/*
 *	#define init_rwsem(sem)						\
 *	do {										\
 *	static struct lock_class_key __key;			\
 *												\
 *	__init_rwsem((sem), #sem, &__key);			\
 *	} while (0)
 *
 */

/*
 *	void down_read(struct rw_semaphore *sem);
 *	sem : rw_semaphore structure
 *	down_read():
 *
 */

/*
 *	void up_read(struct rw_semaphore *sem);
 *	sem : rw_semaphore structure
 *	up_read() :
 *
 */

/*
 *	void down_write(struct rw_semaphore *sem);
 *	sem : rw_semaphore structure
 *	down_write() :
 *
 */

/*
 *	void up_write(struct rw_semaphore *sem);
 *	sem : rw_semaphore structure
 *	up_write() :
 *
 */
