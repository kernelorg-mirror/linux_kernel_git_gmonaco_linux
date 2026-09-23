/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of csched_kern automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME csched_kern

enum states_csched_kern {
	out_csched_kern,
	sched_csched_kern,
	state_max_csched_kern,
};

#define INVALID_STATE state_max_csched_kern

enum events_csched_kern {
	sched_entry_csched_kern,
	sched_exit_csched_kern,
	event_max_csched_kern,
};

struct automaton_csched_kern {
	char *state_names[state_max_csched_kern];
	char *event_names[event_max_csched_kern];
	unsigned char function[state_max_csched_kern][event_max_csched_kern];
	unsigned char initial_state;
	bool final_states[state_max_csched_kern];
};

static const struct automaton_csched_kern automaton_csched_kern = {
	.state_names = {
		"out",
		"sched",
	},
	.event_names = {
		"sched_entry",
		"sched_exit",
	},
	.function = {
		{         sched_csched_kern,             INVALID_STATE },
		{             INVALID_STATE,           out_csched_kern },
	},
	.initial_state = out_csched_kern,
	.final_states = { 1, 0 },
};
