/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_WAKEUP
DEFINE_LTL_EVENT_ID(event_wakeup);
DEFINE_LTL_ERROR_ID(error_wakeup);
#endif /* CONFIG_RV_MON_WAKEUP */
