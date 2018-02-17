#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>  // for threads
#include <linux/sched.h>  // for task_struct
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>

static struct task_struct *thread1,*thread2;
static spinlock_t my_lock;

int thread_fn1(void *dummy) 
{
	int i;

	allow_signal(SIGKILL);
	printk(KERN_INFO "spinlock1:thread_fn1: Entering in a Lock\n");
	spin_lock(&my_lock);
	for (i = 0; i < 20; i++)
	{
		printk(KERN_INFO "spinlock1:thread_fn1: In Spin Lock\n");
		ssleep(1);
		if (signal_pending(current))
			break;
	}	
	spin_unlock(&my_lock);
	printk(KERN_INFO "spinlock1:thread_fn1: Exiting the Lock\n");
	thread1 = NULL;
	do_exit(0);
}

int thread_fn2(void *dummy) 
{
	int ret=0;
	allow_signal(SIGKILL);
	msleep(100);
	printk(KERN_INFO "spinlock1:thread_fn2: Entering in a Lock\n");
#if 0
	ret=spin_trylock(&my_lock);
#else
	spin_lock(&my_lock);
	ret = 1;
#endif

	if(!ret) 
	{
		printk(KERN_INFO "spinlock1:thread_fn2: Unable to hold lock");
	} 
	else 
	{
		ssleep(18);
		printk(KERN_INFO "spinlock1:thread_fn2: Lock acquired");
		spin_unlock(&my_lock);
	}
	thread2 = NULL;
	do_exit(0);
}


int spinlock_init(void) 
{
	spin_lock_init(&my_lock);
	thread1 = kthread_run(thread_fn1, NULL, "thread1");
	thread2 = kthread_run(thread_fn2, NULL, "thread2");

	return 0;
	//do_exit(0);	//NEVER user this in init !!, Not able to do rmmod.
}


void spinlock_cleanup(void) 
{
	if (thread1 != NULL)
	{
		kthread_stop(thread1);
		printk("Thread 1 Stopped\n");
	}
	if (thread2 != NULL)
	{
		kthread_stop(thread2);
		printk("Thread 2 Stopped\n");
	}
}

module_init(spinlock_init);
module_exit(spinlock_cleanup);

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Spin Lock Example");

/*
 *
typedef struct spinlock {
	union {
		struct raw_spinlock rlock;

#ifdef CONFIG_DEBUG_LOCK_ALLOC
# define LOCK_PADSIZE (offsetof(struct raw_spinlock, dep_map))
		struct {
			u8 __padding[LOCK_PADSIZE];
			struct lockdep_map dep_map;
		};
#endif
	};
} spinlock_t;

 */

/*
 *	void spin_lock_init(spinlock_t *lock);
 *
 *
 */

/*	void spin_lock(spinlock_t *lock);
 *
 *
 *
 */


/*
 *	void spin_unlock(spinlock_t *lock);
 *
 *
 *
 */

/*
 *	int spin_trylock(spinlock_t *lock)
 *	spin_trylock() :
 *		These functions return nonzero on success (the lock was obtained), 0 otherwise.
 *
 */

