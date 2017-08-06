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
#include <linux/random.h>

#define TOTAL_THREADS 4
static int counter = 0;
static int thread_id[TOTAL_THREADS];

static struct task_struct *thread_st[TOTAL_THREADS];

static int thread_fn(void *id)
{
	static unsigned int i;
	int thread_id;
	allow_signal(SIGKILL);

	while (!kthread_should_stop()) 
	{
		counter++;
		printk(KERN_INFO "concurrency: Job %d started\n", counter);
		get_random_bytes(&i, sizeof(i));
		ssleep(i % 5);
		printk(KERN_INFO "concurrency: Job %d finished\n", counter);
		if (signal_pending(current))
			break;
	}
	thread_id = *(int *)id;
	thread_st[thread_id] = NULL;
	printk(KERN_INFO "concurrency: Thread Stopping\n");
	do_exit(0);
}


static int __init init_concurrency(void)
{
	int i;
	for (i = 0; i < TOTAL_THREADS; i++)
	{
		printk(KERN_INFO "concurrency: Creating Thread %d\n", i);
		thread_id[i] = i;
		thread_st[i] = kthread_run(thread_fn, &thread_id[i], "concrncy");
	}
	return 0;
}

static void __exit cleanup_concurrency(void)
{
	int i;

	printk(KERN_INFO "concurrency: Cleaning Up\n");
	for (i = 0; i < TOTAL_THREADS; i++)
	{
		if (thread_st[i] != NULL)
		{
			kthread_stop(thread_st[i]);
			printk(KERN_INFO "concurrency: Thread %d stopped", i);
		}
	}
}

module_init(init_concurrency);
module_exit(cleanup_concurrency);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Concurreny Issue Example");


/*
 *	void get_random_bytes(void *buf, int nbytes);
 *		buf : Specifies the address of the buffer in which the requested random  bytes  are stored.
 *		nbytes : Specifies the number of random bytes.
 *		The get_random_bytes() : 
 *			This returns the requested  number of random bytes and stores them in a buffer.
 *			This routine is for kernel modules that cannot be in a wait state.
 */
