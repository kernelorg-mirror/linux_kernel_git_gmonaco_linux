/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of gsched_kern automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME gsched_kern

enum states_gsched_kern {
	out_gsched_kern,
	sched_gsched_kern,
	state_max_gsched_kern,
};

#define INVALID_STATE state_max_gsched_kern

enum events_gsched_kern {
	sched_entry_gsched_kern,
	sched_exit_gsched_kern,
	event_max_gsched_kern,
};

struct automaton_gsched_kern {
	char *state_names[state_max_gsched_kern];
	char *event_names[event_max_gsched_kern];
	unsigned char function[state_max_gsched_kern][event_max_gsched_kern];
	unsigned char initial_state;
	bool final_states[state_max_gsched_kern];
};

static const struct automaton_gsched_kern automaton_gsched_kern = {
	.state_names = {
		"out",
		"sched",
	},
	.event_names = {
		"sched_entry",
		"sched_exit",
	},
	.function = {
		{         sched_gsched_kern,             INVALID_STATE },
		{             INVALID_STATE,           out_gsched_kern },
	},
	.initial_state = out_gsched_kern,
	.final_states = { 1, 0 },
};
