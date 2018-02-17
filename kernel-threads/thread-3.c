#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	/*
	 * Please check thread name in `ps -eaf`
	 * Not able to kill using kill command.
	 */
	int i;
	allow_signal(SIGKILL);  //ena

	for (i = 0; i < 15; i++)
	{
		printk(KERN_INFO "thread-3: Thread Running i=%d\n", i);
		ssleep(5);
		if(signal_pending(current))
		{
			/*
			 * Signal received by doing :
			 * kill -9 PID
			 */

			/*
			 * But still while doing `rmmod thread-3`. It is blocking till it come out of the loop.
			 * How to catch signal clean up call kthread_stop(thread_st) !
			 */
			break;
		}
	}
	printk(KERN_INFO "thread-3: Thread Stopping\n");
	thread_st = NULL;	//will checked in cleanup of module
	do_exit(0);
}


static int __init init_thread(void)
{
	printk(KERN_INFO "thread-3: Creating Thread\n");
	thread_st = kthread_create(thread_fn, NULL, "thread-3");
	if (thread_st)
	{
		/*
		 * Thread created and not in running state.
		 * But you can see it in `ps -eaf`.
		 * Call wake_up_process to run it.
		 */
		wake_up_process(thread_st);
		printk(KERN_INFO "thread-3: Thread Created successfully\n");
	}
	else
	{
		printk(KERN_ERR "thread-3: Thread creation failed\n");
	}
	return 0;
}

static void __exit cleanup_thread(void)
{
	if(thread_st)
	{
		kthread_stop(thread_st);
	}
	printk(KERN_INFO "thread-3: Cleaning Up\n");
}

module_init(init_thread);
module_exit(cleanup_thread);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Thread Creation Example");

/*
 * int allow_signal(int);
 * 		Allow to receive signal.
 *		signal_pending(current) will check for signal.
 *		No much info !
 */

/*
 * int allow_signal(int);
 * 		Allow to receive signal.
 *		signal_pending(current) will check for signal.
 *		No much info !
 */

/*
 * kthread_run (threadfn, data, namefmt, ...);
 * kthread_run() :
 *		Convenient wrapper for kthread_create followed by wake_up_process.
 *		Returns the kthread or ERR_PTR(-ENOMEM). 
 */

/*
 * struct task_struct* kthread_create (	int (* 	thread_fn(void *data), void* data, const char namefmt[], ...);
 * 	thread_fn : the function to run until signal_pending(current).
 *				Thread function. Return type should be int and one parameter void*.
 *	data 			: data ptr for thread_fn.
 *	namefmt[], ...	: printf-style name for the thread. 
 *
 *	kthread_create() :
 *		This helper function creates and names a kernel thread.
 *		The thread will be stopped: use wake_up_process to start it. See also kthread_run, kthread_create_on_cpu.
 *		When woken, the thread will run threadfn() with data as its argument.
 *		threadfn can either call do_exit directly if it is a standalone thread for which noone will call kthread_stop, or return when 'kthread_should_stop' is true (which means kthread_stop has been called).
 *		The return value should be zero or a negative error number; it will be passed to kthread_stop.
 *		Returns a task_struct or ERR_PTR(-ENOMEM). 
 */

/*
 * int wake_up_process (struct task_struct * p);
 *	p : The process to be woken up.
 *	wake_up_process() : 
 *		Attempt to wake up the nominated process and move it to the set of runnable processes.
 *		Returns 1 if the process was woken up, 0 if it was already running.
 *		
 */
