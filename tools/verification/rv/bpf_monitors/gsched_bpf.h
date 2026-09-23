/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of gsched_bpf automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME gsched_bpf

enum states_gsched_bpf {
	out_gsched_bpf,
	sched_gsched_bpf,
	state_max_gsched_bpf,
};

#define INVALID_STATE state_max_gsched_bpf

enum events_gsched_bpf {
	sched_entry_gsched_bpf,
	sched_exit_gsched_bpf,
	event_max_gsched_bpf,
};

struct automaton_gsched_bpf {
	char state_names[state_max_gsched_bpf][32];
	char event_names[event_max_gsched_bpf][32];
	unsigned char function[state_max_gsched_bpf][event_max_gsched_bpf];
	unsigned char initial_state;
	bool final_states[state_max_gsched_bpf];
};

static const struct automaton_gsched_bpf automaton_gsched_bpf = {
	.state_names = {
		"out",
		"sched",
	},
	.event_names = {
		"sched_entry",
		"sched_exit",
	},
	.function = {
		{         sched_gsched_bpf,            INVALID_STATE },
		{            INVALID_STATE,           out_gsched_bpf },
	},
	.initial_state = out_gsched_bpf,
	.final_states = { 1, 0 },
};
