/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Snippet to be included in rv_trace.h
 */

#ifdef CONFIG_RV_MON_PAGEFAULT
DEFINE_LTL_EVENT_ID(event_pagefault);
DEFINE_LTL_ERROR_ID(error_pagefault);
#endif /* CONFIG_RV_MON_PAGEFAULT */
