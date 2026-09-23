// SPDX-License-Identifier: GPL-2.0
#include <linux/ftrace.h>
#include <linux/tracepoint.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rv.h>
#include <rv/instrumentation.h>

#define MODULE_NAME "tqueue_kern"

#include <trace/events/sched.h>
#include <rv_trace.h>

/*
 * This is the self-generated part of the monitor. Generally, there is no need
 * to touch this section.
 */
#define RV_MON_TYPE RV_MON_PER_TASK
#include "tqueue_kern.h"
#include <rv/da_monitor.h>

/*
 * This is the instrumentation part of the monitor.
 *
 * This is the section where manual work is required. Here the kernel events
 * are translated into model's event.
 *
 */
static void handle_sched_dequeue(void *data, struct task_struct *tsk, int cpu)
{
	if (!(tsk->flags & PF_EXITING)) {
		RV_BENCH();
		da_handle_start_event(tsk, sched_dequeue_tqueue_kern);
	}
}

static void handle_sched_enqueue(void *data, struct task_struct *tsk, int cpu)
{
	RV_BENCH();
	da_handle_event(tsk, sched_enqueue_tqueue_kern);
}

static int enable_tqueue_kern(void)
{
	int retval;

	retval = da_monitor_init();
	if (retval)
		return retval;

	rv_attach_trace_probe("tqueue_kern", sched_dequeue_tp, handle_sched_dequeue);
	rv_attach_trace_probe("tqueue_kern", sched_enqueue_tp, handle_sched_enqueue);

	return 0;
}

static void disable_tqueue_kern(void)
{
	rv_this.enabled = 0;

	rv_detach_trace_probe("tqueue_kern", sched_dequeue_tp, handle_sched_dequeue);
	rv_detach_trace_probe("tqueue_kern", sched_enqueue_tp, handle_sched_enqueue);

	da_monitor_destroy();
}

/*
 * This is the monitor register section.
 */
static struct rv_monitor rv_this = {
	.name = "tqueue_kern",
	.description = "auto-generated",
	.enable = enable_tqueue_kern,
	.disable = disable_tqueue_kern,
	.reset = da_monitor_reset_all,
	.enabled = 0,
};

static int __init register_tqueue_kern(void)
{
	return rv_register_monitor(&rv_this, NULL);
}

static void __exit unregister_tqueue_kern(void)
{
	rv_unregister_monitor(&rv_this);
}

module_init(register_tqueue_kern);
module_exit(unregister_tqueue_kern);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rvgen: auto-generated");
MODULE_DESCRIPTION("tqueue_kern: auto-generated");
