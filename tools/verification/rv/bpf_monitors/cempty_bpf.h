/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of cempty_bpf automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME cempty_bpf

enum states_cempty_bpf {
	out_cempty_bpf,
	sched_cempty_bpf,
	state_max_cempty_bpf,
};

#define INVALID_STATE state_max_cempty_bpf

enum events_cempty_bpf {
	sched_entry_cempty_bpf,
	sched_exit_cempty_bpf,
	event_max_cempty_bpf,
};

struct automaton_cempty_bpf {
	char state_names[state_max_cempty_bpf][32];
	char event_names[event_max_cempty_bpf][32];
	unsigned char function[state_max_cempty_bpf][event_max_cempty_bpf];
	unsigned char initial_state;
	bool final_states[state_max_cempty_bpf];
};

static const struct automaton_cempty_bpf automaton_cempty_bpf = {
	.state_names = {
		"out",
		"sched",
	},
	.event_names = {
		"sched_entry",
		"sched_exit",
	},
	.function = {
		{         sched_cempty_bpf,            INVALID_STATE },
		{            INVALID_STATE,           out_cempty_bpf },
	},
	.initial_state = out_cempty_bpf,
	.final_states = { 1, 0 },
};
