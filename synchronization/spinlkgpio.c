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
#include <linux/interrupt.h>
#include <linux/gpio.h>


static struct task_struct *thread1,*thread2;
static spinlock_t my_lock;
static volatile int i = 0;

#define GPIO_KEY 5

static int irq_line;

static irqreturn_t button_irq_handler(int irq, void *data)
{
	printk(KERN_INFO "spinlkgpio:button_irq_handler: GPIO value is %d\n", gpio_get_value(GPIO_KEY));
	i = 1;
	return IRQ_HANDLED;
}

int thread_fn1(void *dummy) 
{
	int j;
	allow_signal(SIGKILL);
	for (j = 0; j < 2; j++) 
	{
		printk(KERN_INFO "spinlkgpio:thread_fn1: Entering in a Lock\n");
		spin_lock(&my_lock);
		while (i == 0)
			;
		spin_unlock(&my_lock);
		printk(KERN_INFO "spinlkgpio:thread_fn1: Out of spin lock\n");
	}
	thread1 = NULL;
	do_exit(0);
}

int thread_fn2(void *dummy) 
{
	allow_signal(SIGKILL);
	printk(KERN_INFO "spinlkgpio:thread_fn2: Thread 2 going to sleep\n");
	ssleep(20);
	i = 1;
	printk(KERN_INFO "spinlkgpio:thread_fn2: Thread 2 out of sleep\n");
	thread2 = NULL;
	return 0;
}

int spinlock_init (void) 
{
	int irq_req_res;
	spin_lock_init(&my_lock);
	gpio_request_one(GPIO_KEY, GPIOF_IN, "button");
	if ( (irq_line = gpio_to_irq(GPIO_KEY)) < 0)
	{
		printk(KERN_ALERT "spinlkgpio:spinlock_init: Gpio %d cannot be used as interrupt",GPIO_KEY);
		return -EINVAL;
	}

	if ( (irq_req_res = request_irq(irq_line, button_irq_handler, 
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "gpio_change_state", NULL)) < 0) 
	{
		printk(KERN_ERR "spinlkgpio:spinlock_init: Keypad: registering irq failed\n");
		return -EINVAL;
	}
	thread1 = kthread_create(thread_fn1, NULL, "thread1");
	if(thread1)
	{
		wake_up_process(thread1);
	}
	thread2 = kthread_create(thread_fn2, NULL, "thread2");

	if(thread2)
	{
		wake_up_process(thread2);
	}
	
	return 0;
}


void spinlock_cleanup(void) 
{
	free_irq(irq_line, NULL);
	if (thread1 != NULL) 
	{
		kthread_stop(thread2);
		printk(KERN_INFO "spinlkgpio:spinlock_cleanup: Thread 1 stopped");
	}
	if (thread2 != NULL) 
	{
		kthread_stop(thread2);
		printk(KERN_INFO "spinlkgpio:spinlock_cleanup: Thread 2 stopped");
	}

}
module_init(spinlock_init);
module_exit(spinlock_cleanup);

MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("Spin Lock Example");


/*	int gpio_request_one(unsigned gpio, unsigned long flags, const char *label);
 *	gpio : gpio pin number
 *	flags : initial configuration :
 * 		GPIOF_DIR_IN		- to configure direction as input
 * 		GPIOF_DIR_OUT		- to configure direction as output
 * 		GPIOF_INIT_LOW	- as output, set initial level to LOW
 * 		GPIOF_INIT_HIGH	- as output, set initial level to HIGH
 * 		GPIOF_OPEN_DRAIN	- gpio pin is open drain type.
 * 		GPIOF_OPEN_SOURCE	- gpio pin is open source type.
 * 		GPIOF_EXPORT_DIR_FIXED	- export gpio to sysfs, keep direction
 * 		GPIOF_EXPORT_DIR_CHANGEABLE	- also export, allow changing direction
 **		GPIOF_IN		- configure as input
 **		GPIOF_OUT_INIT_LOW	- configured as output, initial level LOW
 **		GPIOF_OUT_INIT_HIGH	- configured as output, initial level HIGH
 *
 *	gpio_request_one() :
 *	request a single GPIO, with initial configuration specified by
 *	'flags', identical to gpio_request() wrt other arguments and return value
 *
 */

/*
 *	int gpio_to_irq(unsigned gpio);
 *	gpio : gpio pin number
 *	gpio_to_irq() :
 *		The given gpio must have been obtained with gpio_request() and put into the input mode first.
 *		If there is an associated interrupt number, it will be passed back as the return value from gpio_to_irq();
 *		otherwise a negative error number will be returned.
 *		Once obtained in this manner, the interrupt number can be passed to request_irq() to set up the handling of the interrupt. 
 *
 */

/*
 *	int request_irq(unsigned int irq, irq_handler_t handler, unsigned long irqflags, const char* devname, void* dev_id);
 *	irq      : Interrupt line to allocate
 *	handler  : Function to be called when the IRQ occurs
 *	irqflags : Interrupt type flags
 *	devname  : An ascii name for the claiming device
 *	dev_id   : A cookie passed back to the handler function
 *	request_irq() :
 *		This call allocates interrupt resources and enables the interrupt line and IRQ handling.
 *		From the point this call is made your handler function may be invoked.
 *		Since your handler function must clear any interrupt the board raises,
 *		you must take care both to initialise your hardware and to set up the interrupt handler in the right order.
 *
 *		Dev_id must be globally unique. Normally the address of the device data structure is used as the cookie.
 *		Since the handler receives this value it makes sense to use it.
 */
