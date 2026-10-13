/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_%%MODEL_NAME_UP%%
DEFINE_LTL_EVENT_ID(event_%%MODEL_NAME%%);
DEFINE_LTL_ERROR_ID(error_%%MODEL_NAME%%);
#endif /* CONFIG_RV_MON_%%MODEL_NAME_UP%% */
