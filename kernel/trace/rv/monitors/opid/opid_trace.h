/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_OPID
DEFINE_DA_EVENT(event_opid);
DEFINE_DA_ERROR(error_opid);
DEFINE_HA_ERROR_ENV(error_env_opid);
#endif /* CONFIG_RV_MON_OPID */
