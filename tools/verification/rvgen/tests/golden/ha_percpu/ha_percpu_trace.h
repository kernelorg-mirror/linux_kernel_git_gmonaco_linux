/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_HA_PERCPU
DEFINE_DA_EVENT(event_ha_percpu);
DEFINE_DA_ERROR(error_ha_percpu);
DEFINE_HA_ERROR_ENV(error_env_ha_percpu);
#endif /* CONFIG_RV_MON_HA_PERCPU */
