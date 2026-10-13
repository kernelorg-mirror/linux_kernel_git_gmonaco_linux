/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_WWNR
/* id is the pid of the task */
DEFINE_DA_EVENT_ID(event_wwnr);
DEFINE_DA_ERROR_ID(error_wwnr);
#endif /* CONFIG_RV_MON_WWNR */
