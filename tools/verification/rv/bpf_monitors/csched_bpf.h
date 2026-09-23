/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of csched_bpf automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME csched_bpf

enum states_csched_bpf {
	out_csched_bpf,
	sched_csched_bpf,
	state_max_csched_bpf,
};

#define INVALID_STATE state_max_csched_bpf

enum events_csched_bpf {
	sched_entry_csched_bpf,
	sched_exit_csched_bpf,
	event_max_csched_bpf,
};

struct automaton_csched_bpf {
	char state_names[state_max_csched_bpf][32];
	char event_names[event_max_csched_bpf][32];
	unsigned char function[state_max_csched_bpf][event_max_csched_bpf];
	unsigned char initial_state;
	bool final_states[state_max_csched_bpf];
};

static const struct automaton_csched_bpf automaton_csched_bpf = {
	.state_names = {
		"out",
		"sched",
	},
	.event_names = {
		"sched_entry",
		"sched_exit",
	},
	.function = {
		{         sched_csched_bpf,            INVALID_STATE },
		{            INVALID_STATE,           out_csched_bpf },
	},
	.initial_state = out_csched_bpf,
	.final_states = { 1, 0 },
};
