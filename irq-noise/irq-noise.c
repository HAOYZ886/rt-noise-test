#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/smp.h>

#define MODULE_NAME "irq_noise"

static struct hrtimer latency_timer;
static unsigned long latency_us = 0;
static int target_cpu = 0;

static enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
	udelay(latency_us);
	printk(KERN_INFO "irq_noise: Burned %lu us in Hard IRQ context on CPU%d\n",
	       latency_us, smp_processor_id());
	return HRTIMER_NORESTART;
}

/* Run on target_cpu via smp_call_function_single to pin the hrtimer */
static void trigger_timer(void *data)
{
	hrtimer_start(&latency_timer, ktime_set(0, 0), HRTIMER_MODE_REL_HARD);
}

static int latency_us_set(const char *val, const struct kernel_param *kp)
{
	unsigned long new_val;
	int ret;

	ret = kstrtoul(val, 10, &new_val);
	if (ret)
		return ret;

	if (new_val > 1000000) {
		printk(KERN_WARNING "irq_noise: Latency too high, capping at 1s\n");
		new_val = 1000000;
	}

	latency_us = new_val;

	/* Pin the timer to target_cpu so the callback fires on that CPU
	 * in hard IRQ context (HRTIMER_MODE_REL_HARD).
	 */
	smp_call_function_single(target_cpu, trigger_timer, NULL, 1);

	return 0;
}

static const struct kernel_param_ops latency_param_ops = {
	.set = latency_us_set,
	.get = param_get_ulong,
};

module_param_cb(latency_us, &latency_param_ops, &latency_us, 0644);
MODULE_PARM_DESC(latency_us, "Latency in microseconds to inject (max 1000000)");

module_param_named(cpu, target_cpu, int, 0644);
MODULE_PARM_DESC(cpu, "CPU on which the hrtimer callback runs in hard IRQ");

static int __init irq_init(void)
{
	/* HRTIMER_MODE_REL_HARD forces the callback into hard IRQ context
	 * (not softirq). smp_call_function_single in latency_us_set
	 * ensures the timer is enqueued on target_cpu's clock base.
	 */
	hrtimer_init(&latency_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_HARD);
	latency_timer.function = timer_callback;

	printk(KERN_INFO "irq_noise: Module loaded. CPU=%d. "
	       "Write to /sys/module/irq_noise/parameters/latency_us to trigger.\n",
	       target_cpu);
	return 0;
}

static void __exit irq_exit(void)
{
	hrtimer_cancel(&latency_timer);
	printk(KERN_INFO "irq_noise: Module unloaded.\n");
}

module_init(irq_init);
module_exit(irq_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simulate Hard IRQ Latency using Hrtimers pinned to a CPU");
