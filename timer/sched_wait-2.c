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

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	unsigned long j1, j2;
	printk(KERN_INFO "sched_wait-2: thread_fn +\n");
	allow_signal(SIGKILL);
	while (!kthread_should_stop()) 
	{
		j1 = jiffies;
		j2 = j1 + delay;
		while (time_before(jiffies, j2))
		{
			
			/*
			 * This will not consume CPU %
			 * 
			 * Ask scheduler to allow another process to schedule.
			 * Note: this process is now REMOVED from run queue.
			 * i.e., It will not schedule again untill receives a signal by :
			 * 	kill -9 PID or wake_up_process(sleeping_task);
			 */
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
		j2 = jiffies;
		printk(KERN_INFO "sched_wait-2: Jit Start = %lu  Jit End = %lu\n", j1, j2);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "sched_wait-2: Thread Stopping\n");
	thread_st = NULL;
	do_exit(0);
}

static int __init sched_wait(void)
{
	printk(KERN_INFO "sched_wait-2: sched_wait_module init\n");
	printk(KERN_INFO "sched_wait-2: HZ = %d\n", HZ);
	thread_st = kthread_run(thread_fn, NULL, "sched_wait_thread");
	return 0;
}

static void __exit sched_wait_module_cleanup(void)
{
	printk(KERN_INFO "sched_wait-2: sched_wait_module_cleanup +\n");
	if (thread_st != NULL)
	{
		kthread_stop(thread_st);
		printk(KERN_INFO "sched_wait-2: Thread stopped\n");
	}
}

module_init(sched_wait);
module_exit(sched_wait_module_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Sched Wait Example");


/*
 *	#define time_before(a,b)  ((long)((a) - (b)) < 0))
 *	time_before(jiffies, j2) --> ((jiffies - j2) < 0)
 */


