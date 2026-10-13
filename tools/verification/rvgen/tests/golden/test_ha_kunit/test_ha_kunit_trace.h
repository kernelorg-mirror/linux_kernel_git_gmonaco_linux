/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_TEST_HA_KUNIT
DEFINE_DA_EVENT_ID(event_test_ha_kunit);
DEFINE_DA_ERROR_ID(error_test_ha_kunit);
DEFINE_HA_ERROR_ENV_ID(error_env_test_ha_kunit);
#endif /* CONFIG_RV_MON_TEST_HA_KUNIT */
