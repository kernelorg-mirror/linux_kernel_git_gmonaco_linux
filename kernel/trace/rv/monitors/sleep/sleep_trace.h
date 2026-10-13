/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_SLEEP
DEFINE_LTL_EVENT_ID(event_sleep);
DEFINE_LTL_ERROR_ID(error_sleep);
#endif /* CONFIG_RV_MON_SLEEP */
