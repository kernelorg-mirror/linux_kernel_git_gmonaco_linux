// SPDX-License-Identifier: GPL-2.0

#include "vmlinux.h"

#define RV_MON_TYPE RV_MON_PER_CPU
#include "csched_bpf.h"
#include <rv/da_monitor.h>

/*
 * This is the instrumentation part of the monitor.
 *
 * This is the section where manual work is required. Here the kernel events
 * are translated into model's event.
 */
SEC("tp_btf/sched_entry_tp")
int BPF_PROG(handle_sched_entry, bool preempt)
{
	RV_BENCH();
	da_handle_event(sched_entry_csched_bpf);
	return 0;
}

SEC("tp_btf/sched_exit_tp")
int BPF_PROG(handle_sched_exit, bool is_switch)
{
	RV_BENCH();
	da_handle_start_event(sched_exit_csched_bpf);
	return 0;
}

static struct rv_monitor rv_this = {
	.enabled = 0,
};

char LICENSE[] SEC("license") = "GPL";
char DESCRIPTION[] SEC(".rodata.description") = "auto-generated";
