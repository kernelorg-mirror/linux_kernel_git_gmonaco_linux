/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_DA_PERTASK_DESC
DEFINE_DA_EVENT_ID(event_da_pertask_desc);
DEFINE_DA_ERROR_ID(error_da_pertask_desc);
#endif /* CONFIG_RV_MON_DA_PERTASK_DESC */
