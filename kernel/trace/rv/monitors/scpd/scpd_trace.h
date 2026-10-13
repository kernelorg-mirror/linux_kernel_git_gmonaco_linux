/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_SCPD
DEFINE_DA_EVENT(event_scpd);
DEFINE_DA_ERROR(error_scpd);
#endif /* CONFIG_RV_MON_SCPD */
