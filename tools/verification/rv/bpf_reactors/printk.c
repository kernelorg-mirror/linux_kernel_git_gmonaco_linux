// SPDX-License-Identifier: GPL-2.0

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

extern int bpf_vprintk_deferred(const char *fmt, const void *args,
				__u32 len) __weak __ksym;

__noinline __hidden int bpf_rv_react(const char *fmt, u32 fmt_size, u64 *data,
				     u32 data_len)
{
	return bpf_vprintk_deferred(fmt, data, data_len);
}

char LICENSE[] SEC("license") = "GPL";
char RDESCRIPTION[] SEC(".rodata.description") =
	"prints the exception msg to the ring buffer.";
