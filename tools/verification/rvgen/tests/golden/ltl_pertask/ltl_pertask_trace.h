/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_LTL_PERTASK
DEFINE_LTL_EVENT_ID(event_ltl_pertask);
DEFINE_LTL_ERROR_ID(error_ltl_pertask);
#endif /* CONFIG_RV_MON_LTL_PERTASK */
