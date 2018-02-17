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
//int delay = 200 or 1000; //cant come out from insmod, check from ssh, crash logs comes in dmesg
module_param(delay, int, 0);

static struct task_struct *thread_st;

static int thread_fn(void *unused)
{
	unsigned long j1, j2;
	printk(KERN_INFO "busy_wait: thread_fn +\n");
	allow_signal(SIGKILL);
	while (!kthread_should_stop()) 
	{
		j1 = jiffies;
		j2 = j1 + delay;
		while (time_before(jiffies, j2))
		{
			/*
			 * System hangs here
			 */
			//printk(KERN_INFO "busy_wait: jiffies=%lu end=%lu\n", jiffies, j2);
			//printk(KERN_INFO "busy_wait: diff=%lu\n", jiffies-j2);
			//If do printk here, rsyslogd is take more than 110% cpu
			cpu_relax();
		}
		j2 = jiffies;
		printk(KERN_INFO "busy_wait: Jit Start = %lu  Jit End = %lu\n", j1, j2);
		if (signal_pending(current))
			break;
	}
	printk(KERN_INFO "busy_wait: Thread Stopping\n");
	thread_st = NULL;
	do_exit(0);
}

static int __init busy_wait(void)
{
	printk(KERN_INFO "busy_wait: busy_wait_module init\n");
	printk(KERN_INFO "busy_wait: HZ = %d\n", HZ);
	thread_st = kthread_run(thread_fn, NULL, "busy_wait_thread");
	return 0;
}

static void __exit busy_wait_module_cleanup(void)
{
	printk(KERN_INFO "busy_wait: busy_wait_module_cleanup +\n");
	if (thread_st != NULL)
	{
		kthread_stop(thread_st);
		printk(KERN_INFO "busy_wait: Thread stopped\n");
	}
}

module_init(busy_wait);
module_exit(busy_wait_module_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Busy Wait Example");


/*
 *	#define time_before(a,b)  ((long)((a) - (b)) < 0))
 *	time_before(jiffies, j2) --> ((jiffies - j2) < 0)
 */

