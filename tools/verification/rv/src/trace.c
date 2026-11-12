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
#include <errno.h>

#include <rv.h>
#include <trace.h>
#include <utils.h>

#define FILTER_MAX 128

/**
 * collect_registered_events - print an event collected from the trace buffer
 *
 * Print the event with a standard prefix for common fields and the
 * event-specific pretty-print for the event content.
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

	tep_print_event(event->tep, s, record, "%16s-%-8d [%.3d] %s: %s\n",
			TEP_PRINT_COMM, TEP_PRINT_PID, TEP_PRINT_CPU,
			TEP_PRINT_NAME, TEP_PRINT_INFO);

	trace_seq_do_printf(s);
	trace_seq_reset(s);

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

	trace->inst = tracefs_instance_create(name);
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
 * trace_print_header - print the header of the monitor events output
 */
void trace_print_header(struct trace_seq *s)
{
	trace_seq_printf(s, "%16s-%-8s %5s %5s\n", "<TASK>", "PID", "[CPU]", "TYPE");
	trace_seq_printf(s, "%16s %-8s %5s %5s\n", " | ", " | ", " | ", " | ");
	trace_seq_do_printf(s);
	trace_seq_reset(s);
}

/*
 * apply_filter_enable - if a filter is provided, apply it, then enable the event
 */
static int apply_filter_enable(struct trace_instance *inst,
			       struct tep_event *tevent, const char *filter)
{
	if (*filter) {
		if (tracefs_event_filter_apply(inst->inst, tevent, filter) < 0) {
			char *err = tracefs_error_last(inst->inst);

			err_msg("trace: fail applying filter %s to event %s:\n%s\n",
				filter, tevent->name, err ?: "(no error)");
			free(err);
			return -1;
		}
	}

	if (tracefs_event_enable(inst->inst, tevent->system, tevent->name) < 0) {
		err_msg("trace: fail enabling the rv:%s event: %s\n",
			tevent->name, strerror(errno));
		return -1;
	}

	return 0;

}

/*
 * enable_specific_event - enable a monitor-specific event with filters
 */
static int enable_specific_event(struct trace_instance *inst, const char *event_name)
{
	struct tep_event *tevent = tep_find_event_by_name(inst->tep, "rv", event_name);
	char filter[FILTER_MAX] = "";

	if (!tevent) {
		err_msg("trace: fail finding the rv:%s event\n", event_name);
		return -1;
	}

	if (config.my_pid > 0) {
		if (tep_find_field(tevent, "id"))
			snprintf(filter, sizeof(filter), "common_pid != %d && id != %d",
				 config.my_pid, config.my_pid);
		else
			snprintf(filter, sizeof(filter), "common_pid != %d",
				 config.my_pid);
	}

	return apply_filter_enable(inst, tevent, filter);
}

/*
 * enable_generic_event - enable a generic event with filters
 */
static int enable_generic_event(struct trace_instance *inst, const char *event_name, char *name)
{
	struct tep_event *tevent = tep_find_event_by_name(inst->tep, "rv", event_name);
	char filter[FILTER_MAX] = "";
	size_t len;

	if (!tevent) {
		err_msg("trace: fail finding the rv:%s event\n", event_name);
		return -1;
	}

	len = snprintf(filter, sizeof(filter), "mon == \"%s\"", name);
	if (len >= sizeof(filter)) {
		err_msg("trace: monitor name %s too long\n", name);
		return -1;
	}

	if (config.my_pid > 0)
		snprintf(filter + len, sizeof(filter) - len,
			 " && common_pid != %d && !(has_id == 1 && id == %d)",
			 config.my_pid, config.my_pid);

	return apply_filter_enable(inst, tevent, filter);
}

/*
 * enable_bpf_events - enable rv:event_monitor and rv:error_monitor for a monitor
 *
 * BPF monitors report via generic rv:event_monitor and rv:error_monitor events.
 * Enable the events and filter them to only collect the messages of the given monitor.
 *
 * Returns 0 on success, -1 otherwise.
 */
static int enable_bpf_events(struct trace_instance *inst, char *name)
{
	if (!config.trace_errors_only && enable_generic_event(inst, "event_monitor", name))
		return -1;

	if (enable_generic_event(inst, "error_monitor", name))
		return -1;

	return 0;
}

/**
 * trace_enable_monitor_events - enable a monitor's events in a trace instance
 *
 * Enable the rv:event_"monitor" and rv:error_"monitor" events in the
 * given trace instance, and register the filters for these events.
 * If enabled, the filter for self pid (-s) is applied here.
 * Skip the "event" events if errors only (-E) is specified.
 *
 * Returns 0 on success, -1 otherwise.
 */
static int trace_enable_monitor_events(struct trace_instance *inst, char *name)
{
	char event[MAX_DA_NAME_LEN + 7]; /* max(error_,event_) + '0' = 7 */

	if (config.is_bpf)
		return enable_bpf_events(inst, name);

	if (!config.trace_errors_only) {
		snprintf(event, sizeof(event), "event_%s", name);
		if (enable_specific_event(inst, event))
			return -1;
	}

	snprintf(event, sizeof(event), "error_%s", name);
	if (enable_specific_event(inst, event))
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
	free(abs_path);
	if (!dp)
		return -1;

	while (!retval && (ep = readdir(dp))) {
		if (ep->d_type != DT_DIR || ep->d_name[0] == '.')
			continue;
		retval = trace_enable_monitor_events(inst, ep->d_name);
	}

	closedir(dp);
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
