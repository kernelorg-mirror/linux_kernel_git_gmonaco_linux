// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025-2028 Red Hat, Inc. Gabriele Monaco <gmonaco@redhat.com>
 *
 * RV debug trigger kunit tests:
 *   Tests the RV monitors by triggering fake events to verify monitor
 *   behavior and reactions. Tests start from the first defined event and
 *   trigger events in order to verify error detection.
 */
#include "rv_monitors_test.h"
#include <kunit/static_stub.h>
#include <kunit/test-bug.h>
#include <linux/kernel.h>
#include <linux/rv.h>

__printf(2, 3)
static void stub_rv_react(struct rv_monitor *monitor, const char *msg, ...)
{
	struct rv_kunit_ctx *ctx = kunit_get_current_test()->priv;

	++ctx->reactions;
}

static int stub_rv_get_task_monitor_slot(void)
{
	return 0;
}

static void stub_rv_put_task_monitor_slot(int slot)
{
}

static int rv_trigger_test_init(struct kunit *test)
{
	struct rv_kunit_ctx *ctx;

	ctx = kunit_kzalloc(test, sizeof(*ctx), GFP_KERNEL);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, ctx);

	test->priv = ctx;

	kunit_activate_static_stub(test, rv_react, stub_rv_react);
	kunit_activate_static_stub(test, rv_get_task_monitor_slot,
				   stub_rv_get_task_monitor_slot);
	kunit_activate_static_stub(test, rv_put_task_monitor_slot,
				   stub_rv_put_task_monitor_slot);

	return 0;
}

static struct kunit_case rv_trigger_test_cases[] = {
	KUNIT_CASE(rv_test_sco),
	KUNIT_CASE(rv_test_sssw),
	KUNIT_CASE(rv_test_sts),
	KUNIT_CASE(rv_test_opid),
	KUNIT_CASE(rv_test_throttle),
	{}
};

static struct kunit_suite rv_trigger_test_suite = {
	.name = "rv_trigger",
	.init = rv_trigger_test_init,
	.test_cases = rv_trigger_test_cases,
};

kunit_test_suites(&rv_trigger_test_suite);

MODULE_AUTHOR("Gabriele Monaco <gmonaco@redhat.com>");
MODULE_DESCRIPTION("RV debug trigger kunit tests: test monitors by triggering reactions");
MODULE_LICENSE("GPL");
