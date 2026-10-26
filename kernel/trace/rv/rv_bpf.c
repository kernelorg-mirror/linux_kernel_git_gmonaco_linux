// SPDX-License-Identifier: GPL-2.0
/*
 * BPF kfuncs for Runtime Verification (RV)
 */

#include <linux/init.h>
#include <linux/bpf.h>
#include <linux/btf.h>
#include <linux/btf_ids.h>
#include <rv_trace.h>

__bpf_kfunc_start_defs();

/**
 * bpf_rv_event_monitor - emit an RV monitor event tracepoint
 * @mon__str: name of the monitor
 * @str__str: prefix / monitor extra string
 * @from__str: initial state
 * @event__str: event name
 * @to__str: target state
 */
__bpf_kfunc void bpf_rv_event_monitor(const char *mon__str, const char *str__str,
				      const char *from__str, const char *event__str,
				      const char *to__str)
{
	trace_event_monitor(mon__str, str__str, from__str, event__str, to__str);
}

/**
 * bpf_rv_error_monitor - emit an RV monitor error tracepoint
 * @mon__str: name of the monitor
 * @error_str__str: error message string
 */
__bpf_kfunc void bpf_rv_error_monitor(const char *mon__str, const char *error_str__str)
{
	trace_error_monitor(mon__str, (char *)error_str__str);
}

__bpf_kfunc_end_defs();

BTF_KFUNCS_START(bpf_rv_kfunc_ids)
BTF_ID_FLAGS(func, bpf_rv_event_monitor)
BTF_ID_FLAGS(func, bpf_rv_error_monitor)
BTF_KFUNCS_END(bpf_rv_kfunc_ids)

static const struct btf_kfunc_id_set bpf_rv_kfunc_set = {
	.owner = THIS_MODULE,
	.set   = &bpf_rv_kfunc_ids,
};

static int __init bpf_rv_kfunc_init(void)
{
	return register_btf_kfunc_id_set(BPF_PROG_TYPE_TRACING, &bpf_rv_kfunc_set);
}
late_initcall(bpf_rv_kfunc_init);
