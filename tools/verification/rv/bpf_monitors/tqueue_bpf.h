/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of tqueue_bpf automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME tqueue_bpf

enum states_tqueue_bpf {
	dequeued_tqueue_bpf,
	enqueued_tqueue_bpf,
	state_max_tqueue_bpf,
};

#define INVALID_STATE state_max_tqueue_bpf

enum events_tqueue_bpf {
	sched_dequeue_tqueue_bpf,
	sched_enqueue_tqueue_bpf,
	event_max_tqueue_bpf,
};

struct automaton_tqueue_bpf {
	char state_names[state_max_tqueue_bpf][32];
	char event_names[event_max_tqueue_bpf][32];
	unsigned char function[state_max_tqueue_bpf][event_max_tqueue_bpf];
	unsigned char initial_state;
	bool final_states[state_max_tqueue_bpf];
};

static const struct automaton_tqueue_bpf automaton_tqueue_bpf = {
	.state_names = {
		"dequeued",
		"enqueued",
	},
	.event_names = {
		"sched_dequeue",
		"sched_enqueue",
	},
	.function = {
		{            INVALID_STATE,      enqueued_tqueue_bpf },
		{      dequeued_tqueue_bpf,            INVALID_STATE },
	},
	.initial_state = dequeued_tqueue_bpf,
	.final_states = { 1, 0 },
};
