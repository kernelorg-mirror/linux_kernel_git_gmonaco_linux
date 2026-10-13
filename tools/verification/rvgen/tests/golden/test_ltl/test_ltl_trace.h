/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_TEST_LTL
DEFINE_LTL_EVENT_ID(event_test_ltl);
DEFINE_LTL_ERROR_ID(error_test_ltl);
#endif /* CONFIG_RV_MON_TEST_LTL */
