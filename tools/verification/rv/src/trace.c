// SPDX-License-Identifier: GPL-2.0
/*
 * trace helpers.
 *
 * Copyright (C) 2022 Red Hat Inc, Daniel Bristot de Oliveira <bristot@kernel.org>
 */

#include <tracefs.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <rv.h>
#include <trace.h>
#include <utils.h>

/*
 * create_instance - create a trace instance with *instance_name
 */
static struct tracefs_instance *create_instance(char *instance_name)
{
	return tracefs_instance_create(instance_name);
}

/**
 * collect_registered_events - call the existing callback function for the event
 *
 * If an event has a registered callback function, call it.
 * Otherwise, ignore the event.
 *
 * Returns 0 if the event was collected, 1 if the tool should stop collecting trace.
 */
int
collect_registered_events(struct tep_event *event, struct tep_record *record,
			  int cpu, void *context)
{
	struct trace_instance *trace = context;
	struct trace_seq *s = trace->seq;

	if (should_stop())
		return 1;

	if (!event->handler)
		return 0;

	event->handler(s, record, event, context);

	return 0;
}

/**
 * trace_instance_destroy - destroy and free a rv trace instance
 */
void trace_instance_destroy(struct trace_instance *trace)
{
	if (!trace)
		return;

	if (trace->inst) {
		tracefs_instance_destroy(trace->inst);
		tracefs_instance_free(trace->inst);
		trace->inst = NULL;
	}

	if (trace->seq) {
		free(trace->seq);
		trace->seq = NULL;
	}

	if (trace->tep) {
		tep_free(trace->tep);
		trace->tep = NULL;
	}
}

/**
 * trace_instance_init - create a trace instance
 *
 * It is more than the tracefs instance, as it contains other
 * things required for the tracing, such as the local events and
 * a seq file.
 *
 * Note that the trace instance is returned disabled. This allows
 * the tool to apply some other configs, like setting priority
 * to the kernel threads, before starting generating trace entries.
 *
 * Returns 0 on success, non-zero otherwise.
 */
int trace_instance_init(struct trace_instance *trace, char *name)
{
	trace->seq = calloc(1, sizeof(*trace->seq));
	if (!trace->seq)
		goto out_err;

	trace_seq_init(trace->seq);

	trace->inst = create_instance(name);
	if (!trace->inst)
		goto out_err;

	trace->tep = tracefs_local_events(NULL);
	if (!trace->tep)
		goto out_err;

	/*
	 * Let the main enable the record after setting some other
	 * things such as the priority of the tracer's threads.
	 */
	tracefs_trace_off(trace->inst);

	return 0;

out_err:
	trace_instance_destroy(trace);
	return 1;
}

/**
 * trace_instance_start - start tracing a given rv instance
 *
 * Returns 0 on success, -1 otherwise.
 */
int trace_instance_start(struct trace_instance *trace)
{
	return tracefs_trace_on(trace->inst);
}

static int monitor_has_id(char *name)
{
	char path[MAX_PATH];
	char *format;
	int has_id;

	snprintf(path, MAX_PATH, "events/rv/event_%s/format", name);
	format = tracefs_instance_file_read(NULL, path, NULL);
	if (!format) {
		err_msg("rv: fail reading monitor's %s format event file\n", name);
		return -1;
	}

	/* print fmt: "%d: %s x %s -> %s %s", REC->id, ... */
	has_id = !!strstr(format, "REC->id");

	debug_msg("rv: monitor %s has id: %s\n", name, has_id ? "yes" : "no");

	free(format);

	return has_id;
}

/**
 * trace_print_header - print the header of the monitor events output
 *
 * Print the column header of the rv monitor events output: task, pid,
 * cpu and event type, followed by the state machine transition
 * (state x event -> next state and final). The ID column is printed
 * only if the monitor's events have an id field.
 */
void trace_print_header(struct trace_seq *s)
{
	trace_seq_printf(s, "%16s-%-8s %5s %5s ", "<TASK>", "PID", "[CPU]", "TYPE");
	if (config.has_id)
		trace_seq_printf(s, "%8s ", "ID");

	trace_seq_printf(s, "%24s x %-24s -> %-24s %s\n",
			 "STATE",
			 "EVENT",
			 "NEXT_STATE",
			 "FINAL");

	trace_seq_printf(s, "%16s %-8s %5s %5s ", " | ", " | ", " | ", " | ");

	if (config.has_id)
		trace_seq_printf(s, "%8s ", " | ");

	trace_seq_printf(s, "%24s   %-24s    %-24s %s\n",
			 " | ",
			 " | ",
			 " | ",
			 "|");
}

/*
 * da_event_handler - callback to handle event events
 *
 * Called any time a rv:"monitor"_event events is generated.
 * It parses and prints event.
 */
static int da_event_handler(struct trace_seq *s, struct tep_record *record,
			    struct tep_event *trace_event, void *context)
{
	char *state, *event, *next_state;
	unsigned long long final_state;
	unsigned long long pid;
	unsigned long long id;
	int val;
	bool missing_id;

	if (config.has_id)
		missing_id = tep_get_field_val(s, trace_event, "id", record, &id, 1);

	tep_get_common_field_val(s, trace_event, "common_pid", record, &pid, 1);

	if (config.has_id && (config.my_pid == id))
		return 0;
	else if (config.my_pid == pid)
		return 0;

	tep_print_event(trace_event->tep, s, record, "%16s-%-8d [%.3d] ",
			TEP_PRINT_COMM, TEP_PRINT_PID, TEP_PRINT_CPU);

	if (config.is_container)
		tep_print_event(trace_event->tep, s, record, "%s ", TEP_PRINT_NAME);
	else
		trace_seq_printf(s, "event ");

	if (config.has_id) {
		if (missing_id)
			/* placeholder if we are dealing with a mixed-type container*/
			trace_seq_printf(s, "        ");
		else
			trace_seq_printf(s, "%8llu ", id);
	}

	state = tep_get_field_raw(s, trace_event, "state", record, &val, 0);
	event = tep_get_field_raw(s, trace_event, "event", record, &val, 0);
	next_state = tep_get_field_raw(s, trace_event, "next_state", record, &val, 0);
	tep_get_field_val(s, trace_event, "final_state", record, &final_state, 1);

	trace_seq_printf(s, "%24s x %-24s -> %-24s %s\n",
			 state,
			 event,
			 next_state,
			 final_state ? "Y" : "N");

	trace_seq_do_printf(s);
	trace_seq_reset(s);

	return 0;
}

/*
 * da_error_handler - callback to handle error events
 *
 * Called any time a rv:"monitor"_errors events is generated.
 * It parses and prints event.
 */
static int da_error_handler(struct trace_seq *s, struct tep_record *record,
			    struct tep_event *trace_event, void *context)
{
	unsigned long long pid, id;
	int cpu = record->cpu;
	char *state, *event;
	int val;
	bool missing_id;

	if (config.has_id)
		missing_id = tep_get_field_val(s, trace_event, "id", record, &id, 1);

	tep_get_common_field_val(s, trace_event, "common_pid", record, &pid, 1);

	if (config.has_id && config.my_pid == id)
		return 0;
	else if (config.my_pid == pid)
		return 0;

	trace_seq_printf(s, "%8lld [%03d] ", pid, cpu);

	if (config.is_container)
		tep_print_event(trace_event->tep, s, record, "%s ", TEP_PRINT_NAME);
	else
		trace_seq_printf(s, "error ");

	if (config.has_id) {
		if (missing_id)
			/* placeholder if we are dealing with a mixed-type container*/
			trace_seq_printf(s, "        ");
		else
			trace_seq_printf(s, "%8llu ", id);
	}

	state = tep_get_field_raw(s, trace_event, "state", record, &val, 0);
	event = tep_get_field_raw(s, trace_event, "event", record, &val, 0);

	trace_seq_printf(s, "%24s x %s\n", state, event);

	trace_seq_do_printf(s);
	trace_seq_reset(s);

	return 0;
}

/**
 * trace_enable_monitor_events - enable a monitor's events in a trace instance
 *
 * Enable the rv:event_"monitor" and rv:error_"monitor" events in the
 * given trace instance, and register the callbacks that parse and
 * print these events.
 *
 * Returns 0 on success, -1 otherwise.
 */
static int trace_enable_monitor_events(struct trace_instance *inst, char *name)
{
	char event[MAX_DA_NAME_LEN + 7]; /* max(error_,event_) + '0' = 7 */
	int retval;

	snprintf(event, sizeof(event), "event_%s", name);
	retval = tracefs_event_enable(inst->inst, "rv",  event);
	if (retval)
		return -1;

	tep_register_event_handler(inst->tep, -1, "rv", event,
				   da_event_handler, NULL);

	snprintf(event, sizeof(event), "error_%s", name);
	retval = tracefs_event_enable(inst->inst, "rv", event);
	if (retval)
		return -1;

	tep_register_event_handler(inst->tep, -1, "rv", event,
				   da_error_handler, NULL);

	/* set if at least 1 monitor has id in case of a container */
	config.has_id = monitor_has_id(name);
	if (config.has_id < 0)
		return -1;

	return 0;
}

static int trace_enable_container_events(struct trace_instance *inst, char *name)
{
	DIR *dp;
	char *abs_path, rv_path[MAX_PATH];
	struct dirent *ep;
	int retval = 0;

	snprintf(rv_path, MAX_PATH, "rv/monitors/%s", name);
	abs_path = tracefs_instance_get_file(NULL, rv_path);
	if (!abs_path)
		return -1;
	dp = opendir(abs_path);
	if (!dp)
		goto out_free;

	while (!retval && (ep = readdir(dp))) {
		if (ep->d_type != DT_DIR || ep->d_name[0] == '.')
			continue;
		retval = trace_enable_monitor_events(inst, ep->d_name);
	}

	closedir(dp);
out_free:
	free(abs_path);
	return retval;
}

/*
 * trace_instance_setup - set up a tracing instance to collect data
 *
 * Create a trace instance, enable rv: events and enable the trace.
 *
 * Returns 0 on success, -1 otherwise.
 */
int trace_instance_setup(struct trace_instance *inst, char *name)
{
	int retval;

	if (!config.trace)
		return -1;

	retval = trace_instance_init(inst, name);
	if (retval)
		return -1;

	if (config.is_container)
		retval = trace_enable_container_events(inst, name);
	else
		retval = trace_enable_monitor_events(inst, name);
	if (retval) {
		trace_instance_destroy(inst);
		return -1;
	}

	/* ready to enable */
	tracefs_trace_on(inst->inst);

	return 0;
}
