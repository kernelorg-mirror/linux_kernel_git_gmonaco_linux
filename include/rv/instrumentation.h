/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019-2022 Red Hat, Inc. Daniel Bristot de Oliveira <bristot@kernel.org>
 *
 * Helper functions to facilitate the instrumentation of auto-generated
 * RV monitors create by dot2k.
 *
 * The dot2k tool is available at tools/verification/dot2/
 */

#pragma once

#ifndef __BPF__
#include <linux/ftrace.h>
#include <linux/trace_clock.h>

struct rv_bench_ctx {
	const char *mod;
	u64 start;
};
#else

#define trace_printk            bpf_printk
#define ktime_get_mono_fast_ns  bpf_ktime_get_ns
#endif

/* trace_printk embeds the caller's name, which is the handler since this
 * function gets inlined, store the monitor name because that's usually
 * defined after including this. */
static inline void __rv_bench_end(struct rv_bench_ctx *ctx)
{
	u64 diff = ktime_get_mono_fast_ns() - ctx->start;

	trace_printk("%s: %llu ns\n", ctx->mod, diff);
}

#define RV_BENCH()                                                \
	struct rv_bench_ctx __rv_start __attribute__((            \
		cleanup(__rv_bench_end))) = { .mod = __stringify(MONITOR_NAME), \
					      .start = ktime_get_mono_fast_ns() }

/*
 * rv_attach_trace_probe - check and attach a handler function to a tracepoint
 */
#define rv_attach_trace_probe(monitor, tp, rv_handler)					\
	do {										\
		check_trace_callback_type_##tp(rv_handler);				\
		WARN_ONCE(register_trace_##tp(rv_handler, NULL),			\
				"fail attaching " #monitor " " #tp "handler");		\
	} while (0)

/*
 * rv_detach_trace_probe - detach a handler function to a tracepoint
 */
#define rv_detach_trace_probe(monitor, tp, rv_handler)					\
	do {										\
		unregister_trace_##tp(rv_handler, NULL);				\
	} while (0)
