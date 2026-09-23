/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of cempty_kern automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME cempty_kern

enum states_cempty_kern {
	out_cempty_kern,
	sched_cempty_kern,
	state_max_cempty_kern,
};

#define INVALID_STATE state_max_cempty_kern

enum events_cempty_kern {
	sched_entry_cempty_kern,
	sched_exit_cempty_kern,
	event_max_cempty_kern,
};

struct automaton_cempty_kern {
	char *state_names[state_max_cempty_kern];
	char *event_names[event_max_cempty_kern];
	unsigned char function[state_max_cempty_kern][event_max_cempty_kern];
	unsigned char initial_state;
	bool final_states[state_max_cempty_kern];
};

static const struct automaton_cempty_kern automaton_cempty_kern = {
	.state_names = {
		"out",
		"sched",
	},
	.event_names = {
		"sched_entry",
		"sched_exit",
	},
	.function = {
		{         sched_cempty_kern,             INVALID_STATE },
		{             INVALID_STATE,           out_cempty_kern },
	},
	.initial_state = out_cempty_kern,
	.final_states = { 1, 0 },
};
