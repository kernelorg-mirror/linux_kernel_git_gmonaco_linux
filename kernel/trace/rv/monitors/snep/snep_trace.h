/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_SNEP
DEFINE_DA_EVENT(event_snep);
DEFINE_DA_ERROR(error_snep);
#endif /* CONFIG_RV_MON_SNEP */
