/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_WIP
DEFINE_DA_EVENT(event_wip);
DEFINE_DA_ERROR(error_wip);
#endif /* CONFIG_RV_MON_WIP */
