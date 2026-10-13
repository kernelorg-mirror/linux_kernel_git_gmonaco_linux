/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_TEST_DA
DEFINE_DA_EVENT(event_test_da);
DEFINE_DA_ERROR(error_test_da);
#endif /* CONFIG_RV_MON_TEST_DA */
