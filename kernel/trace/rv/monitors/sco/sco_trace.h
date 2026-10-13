/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_SCO
DEFINE_DA_EVENT(event_sco);
DEFINE_DA_ERROR(error_sco);
#endif /* CONFIG_RV_MON_SCO */
