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
 *
 * @msg must be NULL-terminated, drop the event if it isn't. This happens by
 * construction if using snprintf-like functions and passing the right @msg__sz
 * to both.
 *
 * @mon__str: name of the monitor
 * @msg: message string
 * @msg__sz: message string size
 *
 * Return -EINVAL if @msg is not NULL-terminated, 0 otherwise.
 */
__bpf_kfunc int bpf_rv_event_monitor(const char *mon__str, const int *id__nullable,
				     const char *msg, u32 msg__sz)
{
	if (strnlen(msg, msg__sz) == msg__sz)
		return -EINVAL;

	trace_event_monitor(mon__str, id__nullable, msg);
	return 0;
}

/**
 * bpf_rv_error_monitor - emit an RV monitor error tracepoint
 *
 * @msg must be NULL-terminated, drop the event if it isn't. This happens by
 * construction if using snprintf-like functions and passing the right @msg__sz
 * to both.
 *
 * @mon__str: name of the monitor
 * @msg: message string
 * @msg__sz: message string size
 *
 * Return -EINVAL if @msg is not NULL-terminated, 0 otherwise.
 */
__bpf_kfunc int bpf_rv_error_monitor(const char *mon__str, const int *id__nullable,
				     const char *msg, u32 msg__sz)
{
	if (strnlen(msg, msg__sz) == msg__sz)
		return -EINVAL;

	trace_error_monitor(mon__str, id__nullable, msg);
	return 0;
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
