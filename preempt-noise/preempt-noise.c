#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <uapi/linux/sched/types.h>
#include <linux/smp.h>

#define MODULE_NAME "preempt_noise"

static int cpu = 0, rt_priority = 50;
static unsigned long latency_us = 0;

static int preempt_thread_fn(void *data)
{
	struct sched_param sp;

	if (rt_priority >= 0) {
		pr_info("preempt_noise: Setting RT priority to %d\n", rt_priority);
		sp.sched_priority = rt_priority;
		sched_setscheduler_nocheck(current, SCHED_FIFO, &sp);
	}

	if (latency_us) {
		preempt_disable();
		udelay(latency_us);
		preempt_enable();
		pr_info("preempt_noise: Burned %lu us in Preempt-Off context on CPU%d\n",
			latency_us, smp_processor_id());
	} else {
		udelay(500);
		pr_info("preempt_noise: Burned 500 us in Preempt-On context on CPU%d\n",
			 smp_processor_id());
	}

	return 0;
}

static int latency_us_set(const char *val, const struct kernel_param *kp)
{
	struct task_struct *thread;
	unsigned long new_val;
	int ret;

	ret = kstrtoul(val, 10, &new_val);
	if (ret)
		return ret;

	if (new_val > 1000000) {
		printk(KERN_WARNING "preempt_noise: Latency too high, capping at 1s\n");
		new_val = 1000000;
	}

	latency_us = new_val;

	thread = kthread_run_on_cpu(preempt_thread_fn, NULL, cpu, "k_preempt_noise");
	if (IS_ERR(thread)) {
		printk(KERN_ERR "preempt_noise: Failed to create thread\n");
		return PTR_ERR(thread);
	}

	return 0;
}

static int rt_prio_set(const char *val, const struct kernel_param *kp)
{
	long new_val;
	int ret;

	ret = kstrtol(val, 10, &new_val);
	if (ret)
		return ret;

	if (new_val > 99) {
		pr_warn("preempt_noise: RT priority too high, capping at 99\n");
		new_val = 99;
	} else if (new_val < -1) {
		pr_warn("preempt_noise: set policy to non-RT\n");
		new_val = -1;
	}

	rt_priority = new_val;
	return 0;
}

static const struct kernel_param_ops latency_param_ops = {
	.set = latency_us_set,
	.get = param_get_ulong,
};

module_param_cb(latency_us, &latency_param_ops, &latency_us, 0644);
MODULE_PARM_DESC(latency_us, "Latency in microseconds to inject (max 1000000)");

module_param_named(cpu, cpu, int, 0644);
MODULE_PARM_DESC(cpu, "CPU to bind the noise thread to");

static const struct kernel_param_ops rt_prio_ops = {
	.set = rt_prio_set,
	.get = param_get_long,
};

module_param_cb(rt_priority, &rt_prio_ops, &rt_priority, 0644);
MODULE_PARM_DESC(rt_priority, "Real-time priority of the noise thread (-1~99, higher is more priority, default 50, -1 means non-RT)");

static int __init preempt_init(void)
{
	pr_info("preempt_noise: Module loaded. CPU=%d RT priority=%d. "
	       "Write to /sys/module/preempt_noise/parameters/latency_us to trigger.\n",
	       cpu, rt_priority);
	return 0;
}

static void __exit preempt_exit(void)
{
	pr_info("preempt_noise: Module unloaded.\n");
}

module_init(preempt_init);
module_exit(preempt_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simulate Preempt-Off Latency using kernel thread pinned to a CPU");
