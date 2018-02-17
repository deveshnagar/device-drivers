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

int delay = HZ;
//int delay = 200;

module_param(delay, int, 0);
// For command line param :
// insmod sched_timeout_wait.ko delay=300

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	unsigned long j1, j2;
	printk(KERN_INFO "sched_timeout_wait: thread_fn +\n");
	allow_signal(SIGKILL);
	while (!kthread_should_stop()) 
	{
		j1 = jiffies;
		set_current_state(TASK_INTERRUPTIBLE); // This will remove it from run queue
		/*
		 * without set_current_state(TASK_INTERRUPTIBLE); cpu usage will be high
		 */
		schedule_timeout(delay);
		j2 = jiffies;
		printk(KERN_INFO "sched_timeout_wait: Jit Start = %lu  Jit End = %lu\n", j1, j2);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "sched_timeout_wait: Thread Stopping\n");
	thread_st = NULL;
	do_exit(0);
}

static int __init sched_timeout_wait(void)
{
	printk(KERN_INFO "sched_timeout_wait: sched_timeout_wait_module init\n");
	printk(KERN_INFO "sched_timeout_wait: HZ = %d\n", HZ);
	thread_st = kthread_run(thread_fn, NULL, "sched_timeout_thread");
	return 0;
}

static void __exit sched_timeout_wait_module_cleanup(void)
{
	printk(KERN_INFO "sched_timeout_wait: sched_timeout_wait_module_cleanup +\n");
	if (thread_st != NULL)
	{
		kthread_stop(thread_st);
		printk(KERN_INFO "sched_timeout_wait: Thread stopped\n");
	}
}

module_init(sched_timeout_wait);
module_exit(sched_timeout_wait_module_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Sched Timeout Example");


/*
 *	#define time_before(a,b)  ((long)((a) - (b)) < 0))
 *	time_before(jiffies, j2) --> ((jiffies - j2) < 0)
 */

/*
 *	signed long __sched schedule_timeout (signed long timeout);
 *		timeout : timeout value in jiffies
 *	schedule_timeout() :
 *		Make the current task sleep until timeout jiffies have elapsed.
 *		The routine will return immediately unless the current task state has been set (see set_current_state).
 *		You can set the task state as follows -
 *		TASK_UNINTERRUPTIBLE - at least timeout jiffies are guaranteed to pass before the routine returns. The routine will return 0
 *		TASK_INTERRUPTIBLE - the routine may return early if a signal is delivered to the current task.
 *		In this case the remaining time in jiffies will be returned, or 0 if the timer expired in time
 *		The current task state is guaranteed to be TASK_RUNNING when this routine returns.
 *		Specifying a timeout value of MAX_SCHEDULE_TIMEOUT will schedule the CPU away without a bound on the timeout.
 *		In this case the return value will be MAX_SCHEDULE_TIMEOUT.
 *		In all cases the return value is guaranteed to be non-negative.
 */
