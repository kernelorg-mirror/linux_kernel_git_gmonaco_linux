// SPDX-License-Identifier: GPL-2.0
#include <linux/ftrace.h>
#include <linux/tracepoint.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rv.h>
#include <rv/instrumentation.h>

#define MODULE_NAME "cempty_kern"

#include <trace/events/sched.h>
#include <rv_trace.h>

/*
 * This is the self-generated part of the monitor. Generally, there is no need
 * to touch this section.
 */
#define RV_MON_TYPE RV_MON_PER_CPU
#include "cempty_kern.h"
#include <rv/da_monitor.h>

/*
 * This is the instrumentation part of the monitor.
 *
 * This is the section where manual work is required. Here the kernel events
 * are translated into model's event.
 *
 */
static void handle_sched_entry(void *data, bool preempt)
{
	RV_BENCH();
}

static void handle_sched_exit(void *data, bool is_switch)
{
	RV_BENCH();
}

static int enable_cempty_kern(void)
{
	int retval;

	retval = da_monitor_init();
	if (retval)
		return retval;

	rv_attach_trace_probe("cempty_kern", sched_entry_tp, handle_sched_entry);
	rv_attach_trace_probe("cempty_kern", sched_exit_tp, handle_sched_exit);

	return 0;
}

static void disable_cempty_kern(void)
{
	rv_this.enabled = 0;

	rv_detach_trace_probe("cempty_kern", sched_entry_tp, handle_sched_entry);
	rv_detach_trace_probe("cempty_kern", sched_exit_tp, handle_sched_exit);

	da_monitor_destroy();
}

/*
 * This is the monitor register section.
 */
static struct rv_monitor rv_this = {
	.name = "cempty_kern",
	.description = "auto-generated",
	.enable = enable_cempty_kern,
	.disable = disable_cempty_kern,
	.reset = da_monitor_reset_all,
	.enabled = 0,
};

static int __init register_cempty_kern(void)
{
	return rv_register_monitor(&rv_this, NULL);
}

static void __exit unregister_cempty_kern(void)
{
	rv_unregister_monitor(&rv_this);
}

module_init(register_cempty_kern);
module_exit(unregister_cempty_kern);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rvgen: auto-generated");
MODULE_DESCRIPTION("cempty_kern: auto-generated");
