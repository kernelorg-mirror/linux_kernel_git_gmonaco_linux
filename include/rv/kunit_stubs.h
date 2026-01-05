/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026-2029 Red Hat, Inc. Gabriele Monaco <gmonaco@redhat.com>
 *
 * Declaration of wrappers to allow stubbing core functionality like current
 * and smp_processor_id().
 * Necessary only when mocking may be needed. If the RV KUnit test is
 * enabled, these wrapper incur an additional function call overhead.
 */

#ifdef CONFIG_RV_MONITORS_KUNIT_TEST
struct task_struct *rv_get_current(void);
int rv_current_cpu(void);
#else
#define rv_get_current() current
#define rv_current_cpu() smp_processor_id()
#endif
