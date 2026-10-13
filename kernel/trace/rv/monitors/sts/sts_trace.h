/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_STS
DEFINE_DA_EVENT(event_sts);
DEFINE_DA_ERROR(error_sts);
#endif /* CONFIG_RV_MON_STS */
