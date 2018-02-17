#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>

static struct timer_list my_timer;

void my_timer_callback(unsigned long data)
{
	printk(KERN_INFO "timers: my_timer_callback jiffies = %ld\n", jiffies);
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(200));
}

int timer_module(void)
{
	int ret;

	printk(KERN_INFO "timers: timer_module init +\n");
	setup_timer(&my_timer, my_timer_callback, 0);
	printk(KERN_INFO "timers: Starting timer to fire in 200ms (%ld)\n", jiffies);
	ret = mod_timer(&my_timer, jiffies + msecs_to_jiffies(200));
	if (ret)
	{
		printk(KERN_ERR "timers: Error in mod_timer\n");
	}
	return 0;
}

void timer_module_cleanup(void)
{
	int ret;
	
	printk(KERN_INFO "timers: cleanup_module +\n");
	ret = del_timer(&my_timer);
	if (ret)
		printk(KERN_INFO"timers: The timer is still in use...\n");

	return;
}

module_init(timer_module);
module_exit(timer_module_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devesh Nagar <devesh66@gmail.com>");
MODULE_DESCRIPTION("Kernel Timers Example");


/*
 *	setup_timer(timer, fn, data);
 *		#define setup_timer(timer, fn, data)\
 *		setup_timer_key((timer), NULL, NULL, (fn), (data))
 */

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

