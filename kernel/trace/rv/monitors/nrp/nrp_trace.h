/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_NRP
DEFINE_DA_EVENT_ID(event_nrp);
DEFINE_DA_ERROR_ID(error_nrp);
#endif /* CONFIG_RV_MON_NRP */
