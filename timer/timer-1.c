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
#include <linux/gpio.h>
#include <linux/timer.h>

#define GPIO_NUMBER 9
#define DELAY 1000  //
//#define DELAY HZ 	//What is HZ ?, on radxa rock printk HZ is 100 

/*
 * jiffies :
 * 	Jiffies is a global variable declared in <linux/jiffies.h> as:
 *	extern unsigned long volatile jiffies
 *	store the number of ticks occurred since system start-up.
 *	On kernel boot-up, jiffies is initialized to a special initial value,
 *	and it is incremented by one for each timer interrupt.
 *	there are HZ ticks occurred in one second, and thus there are HZ jiffies in a second.
 *	HZ = no. ticks/sec
 *	jiffies = no. of ticks
 *	
 *	unsigned long now_tick = jiffies ; // now
 *	unsigned long next_tick = jiffies + 1 ; // one tick from now
 *	unsigned long timer_later = jiffies + (10*HZ) ; // 10s from now
 */

static struct task_struct *thread_st;
static wait_queue_head_t wq_head;
static char flag = 0;
void timer_fn(unsigned long data);

static struct timer_list my_timer = TIMER_INITIALIZER(timer_fn, 0, 0);

void timer_fn(unsigned long data)
{
	int ret;
	flag = 1;
	printk(KERN_INFO "timer-1: Timer fired\n");
	wake_up(&wq_head);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(DELAY));
	if(ret)
	{
		printk(KERN_ERR "timer-1: mod_timer error\n");
	}
}

static int thread_fn(void* data)
{
	static char state = 0;
	allow_signal(SIGKILL);  //allow signal to interrupt, but u have to check using signal_pnding()
	while(!kthread_should_stop())
	{
		printk(KERN_INFO "timer-1: Waiting...\n");
		wait_event_interruptible(wq_head, flag == 1);
		printk(KERN_INFO "timer-1: Woken up...\n");
		state ^= 1;
		gpio_set_value(GPIO_NUMBER, state);
		flag = 0;
		if(signal_pending(current))
		{
			// Signal received by doing :
			// kill -9 PID
			printk(KERN_INFO "timer-1: Received allowed signal\n");
			break;
		}
	}
	printk(KERN_INFO "timer-1: Thread stopping\n");
	thread_st = NULL;
	do_exit(0);
}

static int __init timer_module(void)
{
	int ret;
	init_waitqueue_head(&wq_head);
	printk(KERN_INFO "timer-1: init_timer_module +\n");
	thread_st = kthread_run(thread_fn, NULL, "timer_module_thread");
	if (thread_st)
	{
		printk(KERN_INFO "timer-1: Thread Created successfully, WakeUp also\n");
	}
	else
	{
		printk(KERN_ERR "timer-1: Thread creation failed\n");
	}
	
    ret = gpio_is_valid(GPIO_NUMBER);
    if(ret < 0)
    {
            printk("timer-1: gpio %d is not-valid error \n", GPIO_NUMBER);
            return -1;
    }
    ret = gpio_request(GPIO_NUMBER, "GPIO_7");
    if(ret < 0)
    {
        printk("timer-1: gpio %d request error \n", GPIO_NUMBER);
        return -1;
    }
    gpio_direction_output(GPIO_NUMBER, 1);

	printk(KERN_INFO "timer-1: msecs_to_jiffies(%u)=%lu\n", DELAY, msecs_to_jiffies(DELAY));
	mod_timer(&my_timer, jiffies + DELAY);

	return 0;
}

static void __exit timer_module_cleanup(void)
{
	int ret;
	printk(KERN_INFO "timer-1: timer_module_cleanup +\n");
	gpio_free(GPIO_NUMBER);
	if(thread_st)
		ret = kthread_stop(thread_st);
	if(!ret)
	{
		printk(KERN_INFO "timer-1: Thread stopped");
	}
	del_timer(&my_timer);
}

module_init(timer_module);
module_exit(timer_module_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Kernel Timers Example");


/*
 * int mod_timer (struct timer_list* timer, unsigned long expires);
 *	timer : the timer to be modified
 *	expires : new timeout in jiffies
 *	mod_timer() : 
 *		mod_timer is a more efficient way to update the expire field of an active timer (if the timer is inactive it will be activated)
 *		mod_timer(timer, expires) is equivalent to:
 *		del_timer(timer); timer->expires = expires; add_timer(timer);
 *		Note that if there are multiple unserialized concurrent users of the same timer, then mod_timer is the only safe way to modify the timeout, since add_timer cannot modify an already running timer.
 *		The function returns whether it has modified a pending timer or not.
 *		(ie. mod_timer of an inactive timer returns 0, mod_timer of an active timer returns 1.)
 */

/*
 * wait_event_interruptible (wq, condition);
 *	wq : the waitqueue to wait on
 *	condition : a C expression for the event to wait for 
 *	wait_event_interruptible() :
 *		The process is put to sleep (TASK_INTERRUPTIBLE) until the condition evaluates to true or a signal is received.
 *		The condition is checked each time the waitqueue wq is woken up.
 *		wake_up() has to be called after changing any variable that could change the result of the wait condition.
 *
 *		Also see wait_event_interruptible_timeout(wq, condition, timeout)
 */

/*
 *  wait_event_interruptible (wq, condition);
 *	wq 			: the waitqueue to wait on
 *	condition 	: a C expression for the event to wait for
 *	wait_event_interruptible() :
 *		The process is put to sleep (TASK_INTERRUPTIBLE) until the condition evaluates to true or a signal is received.
 *		The condition is checked each time the waitqueue wq is woken up.
 *		wake_up has to be called after changing any variable that could change the result of the wait condition.
 *		The function will return -ERESTARTSYS if it was interrupted by a signal and 0 if condition evaluated to true.
 */

/*
 *
 * wait_queue_head_t my_queue;
 * init_waitqueue_head(&my_queue);
 *		Initialize wait queue my_queue
 */

