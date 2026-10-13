/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_DA_PEROBJ_PARENT
DEFINE_DA_EVENT_ID(event_da_perobj_parent);
DEFINE_DA_ERROR_ID(error_da_perobj_parent);
#endif /* CONFIG_RV_MON_DA_PEROBJ_PARENT */
