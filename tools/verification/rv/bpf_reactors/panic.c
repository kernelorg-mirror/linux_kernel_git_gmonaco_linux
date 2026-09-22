// SPDX-License-Identifier: GPL-2.0

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

extern void bpf_panic(const char *msg, __u32 msg__sz) __weak __ksym;

__noinline __hidden int bpf_rv_react(const char *fmt, u32 fmt_size, u64 *data,
				     u32 data_len)
{
	char buf[256] = "";

	if (bpf_snprintf(buf, sizeof(buf), fmt, data, data_len) <= 0)
		BPF_SNPRINTF(buf, sizeof(buf), "rv: invalid reactor format");

	bpf_panic(buf, sizeof(buf));
	return 0;
}

char LICENSE[] SEC("license") = "GPL";
char RDESCRIPTION[] SEC(".rodata.description") =
	"panic the system if an exception is found.";
