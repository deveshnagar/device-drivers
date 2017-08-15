#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> // required for various structures related to files liked fops.
#include <asm/uaccess.h> // required for copy_from and copy_to user functions
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>

#define FIRST_MINOR 0
#define TOTAL_MINOR 1

/*
 * current :
 *		The current pointer refers to the user process currently executing.
 *		During the execution of a system call, such as open or read, the current process is the one that invoked the call. 
 */

static dev_t dev;
static struct cdev c_dev;
static struct class *cl;
struct task_struct *task;
static struct semaphore sem;

int open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "sem:open: Inside open\n");
	task = current;                 // Saving the task_struct for future use.
	//We will check status of task in remove() function

	/*
	 * down(&sem);
	 * This is not interruptible. That means :
	 * When semaphore is taken, and if user does cat '/dev/semchar0' it will be blocked,
	 * And he cant do ctrl+c to come out from 'cat /dev/semchar0'
	 */

	if(down_interruptible(&sem)) 
	{
		printk(KERN_INFO "sem:open: could not hold semaphore");
		return -1;
	}
	printk(KERN_INFO "sem:open: State after = %ld\n", task->state); // printing the state of user process
	//state = 0 means runnable
	return 0;
}

int release(struct inode *inode, struct file *filp) 
{
	printk (KERN_INFO "sem:release: Inside close \n");
	printk(KERN_INFO "sem:release: Releasing semaphore\n");
	up(&sem);
	return 0;
}

ssize_t read(struct file *filp, char *buff, size_t count, loff_t *offp) 
{
	printk(KERN_INFO "sem:read: Inside read \n");
	return 0;
}

ssize_t write(struct file *filp, const char *buff, size_t count, loff_t *offp) 
{   

	printk(KERN_INFO "sem:write: Inside write \n");
	return count;
}

int hold(struct file *filp,char *buf,size_t count,loff_t *offp)
{
	int len=0;
	if (down_interruptible(&sem))   // holding the semaphore
		return -ERESTARTSYS;
	printk(KERN_INFO "sem:hold: Inside hold\n");
	return len;
}

int remove(struct file *file,const char *buffer, size_t count, loff_t *off)
{
	/* according to linux/sched.h the value of state and their meanings are
	 * -1 unrunnable, 0 runnable, >0 stopped
	 */
	printk(KERN_INFO "sem:remove: Before up() task.state= %ld\n", task->state);  // = 1, stopped,  printing the state of user process
	up(&sem);
	printk(KERN_INFO "sem:remove: After up() task.state=%ld\n", task->state); // = 0, became runnable

	return count;
}
struct file_operations p_fops = {
	.read = hold,
	.write = remove,
};


void create_new_proc_entry(void)
{
	proc_create("hold",0,NULL,&p_fops);
}

struct file_operations fops = {
	read:	read,
	write:	write,
	open:	open,
	release:	release
};


int semdemo_init (void) 
{
	if (alloc_chrdev_region(&dev, FIRST_MINOR, TOTAL_MINOR, "sem") < 0)
	{
		return -1;
	}
	printk(KERN_INFO "sem: Major Nr: %d\n", MAJOR(dev));

	cdev_init(&c_dev, &fops);

	if (cdev_add(&c_dev, dev, TOTAL_MINOR) == -1)
	{
		unregister_chrdev_region(dev, TOTAL_MINOR);
		return -1;
	}

	if ((cl = class_create(THIS_MODULE, "sem_chardrv")) == NULL)
	{
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, TOTAL_MINOR);
		return -1;
	}
	if (IS_ERR(device_create(cl, NULL, dev, NULL, "semchar%d", 0)))
	{
		class_destroy(cl);
		cdev_del(&c_dev);
		unregister_chrdev_region(dev, TOTAL_MINOR);
		return -1;
	}

	sema_init(&sem, 1);

	create_new_proc_entry();
	return 0;
}

void semdemo_cleanup(void) 
{
	printk(KERN_INFO "sem:  Inside cleanup_module\n");
	device_destroy(cl, dev);
	class_destroy(cl);
	cdev_del(&c_dev);
	unregister_chrdev_region(dev, TOTAL_MINOR);
	remove_proc_entry("hold",NULL);
}

module_init(semdemo_init);
module_exit(semdemo_cleanup);
MODULE_LICENSE("GPL");   
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Semaphore Example");

/*
 * struct semaphore {
 * 		spinlock_t              lock;
 * 		unsigned int            count;
 * 		struct list_head        wait_list;
 * };
 *
 *	Does semaphore uses spin lock internally to ?
 */

/*
 * 	void sema_init(struct semaphore *sem, int val);
 *	sem : semaphore structure
 *	val : value of semaphore to initialize
 *	sema_init():
 *		initializes a semaphore sem with value val.
 *		value is ussage count, i.e. number of threads can acquire semaphore at at time.
 *		That means if val=1, is equivalent to use mutex.
 */

/*	void up(struct semaphore *sem);
 *	sem : semaphore structure
 *	up() :
 *		In up() routine, process increments the counter value and wakes up next process from the queue.
 *		It is not necessary that up() is called by the same process which called down.
 *		Any process can call this function and wake up any process sleeping on this semaphore.
 */

/*
 *	void down(struct semaphore *sem);
 *	sem : semaphore structure
 *	down() :
 *		Decrements semaphore counter. Acquire semaphore.
 *		If semaphore counter or value is greater than 0, it returns and semaphore acquired.
 *		If counter or value is smaller than 1, it will put calling thread to semaphore queue.
 *		And it keep waiting forever, untill other task releases semaphore by calling up(sem);.
 */

/*
 *	int down_interruptible(struct semaphore *sem);
 *	sem : semaphore structure
 *	down_interruptible() :
 *		This is same as down(), but can be come out if an signal occurs.
 *		Returns -EINTR if signal occurs. Returns 0 if semaphore acquired.
 *
 *	Also check :
 *		int down_killable(struct semaphore *sem);
 *		int down_timeout(struct semaphore *sem, long jiffies);
 *		int down_trylock(struct semaphore *sem);
 */

