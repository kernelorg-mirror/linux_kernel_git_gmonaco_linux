/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_NOMISS
DEFINE_DA_EVENT_ID(event_nomiss);
DEFINE_DA_ERROR_ID(error_nomiss);
DEFINE_HA_ERROR_ENV_ID(error_env_nomiss);
#endif /* CONFIG_RV_MON_NOMISS */
