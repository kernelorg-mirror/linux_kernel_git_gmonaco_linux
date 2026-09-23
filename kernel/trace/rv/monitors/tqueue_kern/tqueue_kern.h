/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of tqueue_kern automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME tqueue_kern

enum states_tqueue_kern {
	dequeued_tqueue_kern,
	enqueued_tqueue_kern,
	state_max_tqueue_kern,
};

#define INVALID_STATE state_max_tqueue_kern

enum events_tqueue_kern {
	sched_dequeue_tqueue_kern,
	sched_enqueue_tqueue_kern,
	event_max_tqueue_kern,
};

struct automaton_tqueue_kern {
	char *state_names[state_max_tqueue_kern];
	char *event_names[event_max_tqueue_kern];
	unsigned char function[state_max_tqueue_kern][event_max_tqueue_kern];
	unsigned char initial_state;
	bool final_states[state_max_tqueue_kern];
};

static const struct automaton_tqueue_kern automaton_tqueue_kern = {
	.state_names = {
		"dequeued",
		"enqueued",
	},
	.event_names = {
		"sched_dequeue",
		"sched_enqueue",
	},
	.function = {
		{             INVALID_STATE,      enqueued_tqueue_kern },
		{      dequeued_tqueue_kern,             INVALID_STATE },
	},
	.initial_state = dequeued_tqueue_kern,
	.final_states = { 1, 0 },
};
