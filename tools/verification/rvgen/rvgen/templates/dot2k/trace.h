/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_%%MODEL_NAME_UP%%
DEFINE_DA_EVENT%%MACRO_SUFFIX%%(event_%%MODEL_NAME%%);
DEFINE_DA_ERROR%%MACRO_SUFFIX%%(error_%%MODEL_NAME%%);
#endif /* CONFIG_RV_MON_%%MODEL_NAME_UP%% */
