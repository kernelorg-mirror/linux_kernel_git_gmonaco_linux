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
#include <linux/ftrace.h>
#include <linux/trace_clock.h>

struct rv_bench_ctx {
	const char *mod;
	const char *func;
	u64 start;
};

/* trace_printk embeds the caller's name, which is the handler since this
 * function gets inlined, store the monitor name because that's usually
 * defined after including this. */
static inline void __rv_bench_end(struct rv_bench_ctx *ctx)
{
	u64 diff = trace_clock_local() - ctx->start;

	trace_printk("%s: %llu ns\n", ctx->mod, diff);
}

/* instrument all monitors with:
 *  find kernel/trace/rv/monitors -name "*.c" -exec perl -0777 -pi -e \
 *      'for $h (/rv_attach_trace_probe\([^;]* (\w+)\)/g) {
 *          s/($h\([^{]*?\)\s*\{\n\t)([^R])/$1RV_BENCH();\n\t$2/gs
 *      }' {} +
 */

#define RV_BENCH()                                                \
	struct rv_bench_ctx __rv_start __attribute__((            \
		cleanup(__rv_bench_end))) = { .mod = MODULE_NAME, \
					      .start = trace_clock_local() }

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
