// SPDX-License-Identifier: GPL-2.0
#include <linux/ftrace.h>
#include <linux/tracepoint.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/rv.h>
#include <rv/instrumentation.h>

#define MODULE_NAME "gsched_kern"

#include <trace/events/sched.h>
#include <rv_trace.h>

/*
 * This is the self-generated part of the monitor. Generally, there is no need
 * to touch this section.
 */
#define RV_MON_TYPE RV_MON_GLOBAL
#include "gsched_kern.h"
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
	if (smp_processor_id()) return;
	RV_BENCH();
	da_handle_event(sched_entry_gsched_kern);
}

static void handle_sched_exit(void *data, bool is_switch)
{
	if (smp_processor_id()) return;
	RV_BENCH();
	da_handle_start_event(sched_exit_gsched_kern);
}

static int enable_gsched_kern(void)
{
	int retval;

	retval = da_monitor_init();
	if (retval)
		return retval;

	rv_attach_trace_probe("gsched_kern", sched_entry_tp, handle_sched_entry);
	rv_attach_trace_probe("gsched_kern", sched_exit_tp, handle_sched_exit);

	return 0;
}

static void disable_gsched_kern(void)
{
	rv_this.enabled = 0;

	rv_detach_trace_probe("gsched_kern", sched_entry_tp, handle_sched_entry);
	rv_detach_trace_probe("gsched_kern", sched_exit_tp, handle_sched_exit);

	da_monitor_destroy();
}

/*
 * This is the monitor register section.
 */
static struct rv_monitor rv_this = {
	.name = "gsched_kern",
	.description = "auto-generated",
	.enable = enable_gsched_kern,
	.disable = disable_gsched_kern,
	.reset = da_monitor_reset_all,
	.enabled = 0,
};

static int __init register_gsched_kern(void)
{
	return rv_register_monitor(&rv_this, NULL);
}

static void __exit unregister_gsched_kern(void)
{
	rv_unregister_monitor(&rv_this);
}

module_init(register_gsched_kern);
module_exit(unregister_gsched_kern);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rvgen: auto-generated");
MODULE_DESCRIPTION("gsched_kern: auto-generated");
