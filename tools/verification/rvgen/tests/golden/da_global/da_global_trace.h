/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_DA_GLOBAL
DEFINE_DA_EVENT(event_da_global);
DEFINE_DA_ERROR(error_da_global);
#endif /* CONFIG_RV_MON_DA_GLOBAL */
