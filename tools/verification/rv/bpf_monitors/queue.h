/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Automatically generated C representation of queue automaton
 * For further information about this format, see kernel documentation:
 *   Documentation/trace/rv/deterministic_automata.rst
 */

#define MONITOR_NAME queue

enum states_queue {
	dequeued_queue,
	enqueued_queue,
	state_max_queue,
};

#define INVALID_STATE state_max_queue

enum events_queue {
	sched_dequeue_queue,
	sched_enqueue_queue,
	event_max_queue,
};

struct automaton_queue {
	char *state_names[state_max_queue];
	char *event_names[event_max_queue];
	unsigned char function[state_max_queue][event_max_queue];
	unsigned char initial_state;
	bool final_states[state_max_queue];
};

static const struct automaton_queue automaton_queue = {
	.state_names = {
		"dequeued",
		"enqueued",
	},
	.event_names = {
		"sched_dequeue",
		"sched_enqueue",
	},
	.function = {
		{       INVALID_STATE,      enqueued_queue },
		{      dequeued_queue,       INVALID_STATE },
	},
	.initial_state = dequeued_queue,
	.final_states = { 1, 0 },
};
