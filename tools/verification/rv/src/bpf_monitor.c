// SPDX-License-Identifier: GPL-2.0
/*
 * BPF monitor support: allows rv to control BPF monitors.
 *
 * Copyright (C) 2026 Red Hat Inc, Gabriele Monaco <gmonaco@redhat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <bpf/btf.h>

#include <bpf_monitor.h>
#include <utils.h>
#include <rv.h>

static char bpf_monitor_paths[][MAX_PATH] = {
	"./bpf_monitors/",
	"/usr/share/rv/bpf_monitors/",
	"", /* Marker */
};

/* Path used for development monitors, searched first */
#define DEVEL_PATH 0

#define MAX_ENUMS 64
#define MAX_LINKS 16
#define RV_TRACE_EVENT	0
#define RV_TRACE_ERROR	1
#define PROG_ENABLE_MON "enable_monitor"
#define BPF_PIN_BASE_PATH "/sys/fs/bpf/rv"

struct da_field_offset {
	size_t event_type;
	size_t id;
	size_t cpu;
	size_t pid;
	size_t comm;
	size_t is_final;
	size_t curr_state;
	size_t event;
	size_t next_state;
};

struct bpf_monitor_ctx {
	char monitor_name[MAX_DA_NAME_LEN];
	char state_names[MAX_ENUMS][MAX_DA_NAME_LEN];
	char event_names[MAX_ENUMS][MAX_DA_NAME_LEN];
	int num_states;
	int num_events;
	struct da_field_offset offset;
};

/*
 * bpf_read_enable - reads monitor's enable status
 *
 * Iterate through all BPF maps in the system, if the da_ringbuf_NAME map is
 * loaded, the monitor is enabled.
 */
static int bpf_read_enable(char *monitor_name)
{
	char ringbuf_name[64];
	struct bpf_map_info info;
	uint32_t info_len = sizeof(info);
	uint32_t id = 0;
	int fd;

	snprintf(ringbuf_name, sizeof(ringbuf_name),
		 "da_ringbuf_%s", monitor_name);

	while (bpf_map_get_next_id(id, &id) == 0) {
		fd = bpf_map_get_fd_by_id(id);
		if (fd < 0)
			continue;

		memset(&info, 0, sizeof(info));
		if (bpf_map_get_info_by_fd(fd, &info, &info_len) == 0) {
			if (strcmp(info.name, ringbuf_name) == 0) {
				close(fd);
				return 1;
			}
		}
		close(fd);
	}

	return 0;
}

/*
 * bpf_read_desc - read monitors' description
 *
 * Return the provided string containing the monitor's description, NULL
 * otherwise.
 */
static char *bpf_read_desc(char *desc, struct bpf_object *obj, char *monitor_name)
{
	const char *desc_data;
	struct bpf_map *map;
	size_t desc_size;

	map = bpf_object__find_map_by_name(obj, ".rodata.description");
	if (!map) {
		debug_msg("bpf: cannot not find desc map for %s\n",
			  monitor_name);
		return NULL;
	}
	desc_data = bpf_map__initial_value(map, &desc_size);
	if (!desc_data || desc_size == 0) {
		debug_msg("bpf: empty description for %s\n", monitor_name);
		*desc = 0;
		return desc;
	}

	if (desc_size >= MAX_DESCRIPTION)
		desc_size = MAX_DESCRIPTION - 1;
	strncpy(desc, desc_data, desc_size);
	desc[desc_size] = '\0';

	return desc;
}

/*
 * bpf_fill_monitor_paths - fill the path for development builds
 *
 * RV searches for BPF monitors on absolute paths on the system as well
 * as in the same directory of the rv binary. This is useful when running
 * rv from the kernel tree. This function resolves right location.
 */
static void bpf_fill_monitor_paths(void)
{
	char *curr_path;
	ssize_t len;

	curr_path = bpf_monitor_paths[DEVEL_PATH];
	len = readlink("/proc/self/exe", curr_path, MAX_PATH);
	if (len != -1) {
		curr_path[len] = '\0';
		dirname(curr_path);
		strncat(curr_path, "/bpf_monitors", MAX_PATH);
	}
}

/*
 * list_monitors_from_path - list monitors from a specific path
 */
static void list_monitors_from_path(const char *path)
{
	struct dirent *entry;
	DIR *dir;
	char *ext;

	dir = opendir(path);
	if (!dir) {
		debug_msg("bpf: error opening directory: %s\n", path);
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		size_t size;
		struct bpf_object *obj;
		char name[MAX_DA_NAME_LEN], mon_path[MAX_PATH], desc[MAX_DESCRIPTION];

		if (entry->d_name[0] == '.')
			continue;

		ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".o") != 0)
			continue;

		size = snprintf(mon_path, sizeof(mon_path), "%s/%s", path,
				entry->d_name);
		obj = bpf_object__open_file(mon_path, NULL);
		if (!obj || size > MAX_PATH) {
			err_msg("bpf: error opening object file %s: %s\n",
				mon_path, strerror(errno));
			continue;
		}

		strncpy(name, entry->d_name, sizeof(name));
		ext = strrchr(name, '.');
		if (ext)
			*ext = '\0';

		if (!bpf_read_desc(desc, obj, name)) {
			err_msg("bpf: monitor %s does not have desc map, bug?\n", name);
			continue;
		}

		printf("%-*s %s %s\n", MAX_DA_NAME_LEN, name,
		       desc, bpf_read_enable(name) ? "[ON]" : "[OFF]");

		bpf_object__close(obj);
	}

	closedir(dir);
}

/*
 * bpf_list_monitors - list available BPF monitors from all sources
 *
 * @container: Unused for BPF monitors.
 *
 * Returns 0 on success
 */
int bpf_list_monitors(char *container)
{
	bpf_fill_monitor_paths();
	for (int i = 0; bpf_monitor_paths[i][0]; i++)
		list_monitors_from_path(bpf_monitor_paths[i]);

	return 0;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
			   va_list args)
{
	if (level == LIBBPF_DEBUG && !config.debug)
		return 0;
	return vfprintf(stderr, format, args);
}

/*
 * Helper functions for state/event name lookup
 */
static const char *get_state_name(struct bpf_monitor_ctx *ctx, uint32_t state)
{
	static char buf[16];

	if (state < ctx->num_states && ctx->state_names[state][0] != '\0')
		return ctx->state_names[state];

	snprintf(buf, sizeof(buf), "%u", state);
	return buf;
}

static const char *get_event_name(struct bpf_monitor_ctx *ctx, uint32_t event)
{
	static char buf[16];

	if (event < ctx->num_events && ctx->event_names[event][0] != '\0')
		return ctx->event_names[event];

	snprintf(buf, sizeof(buf), "%u", event);
	return buf;
}

/*
 * extract_field_offsets - extract field offsets from BTF for efficient parsing
 *
 * Introspects the rv_trace_entry structure via BTF and stores field offsets
 * for direct memory access during event processing (zero runtime overhead).
 *
 * Returns 0 on success, -1 on error
 */
static int extract_field_offsets(const struct btf *btf, struct bpf_monitor_ctx *ctx)
{
	const struct btf_type *trace_type;
	const struct btf_member *members;
	char struct_name[64];
	__u32 type_id;
	int i, vlen;

	snprintf(struct_name, sizeof(struct_name), "rv_trace_entry");
	type_id = btf__find_by_name_kind(btf, struct_name, BTF_KIND_STRUCT);
	if ((int)type_id <= 0) {
		debug_msg("bpf: could not find struct '%s' in BTF\n", struct_name);
		return -1;
	}

	trace_type = btf__type_by_id(btf, type_id);
	if (!trace_type) {
		debug_msg("bpf: could not get type for '%s'\n", struct_name);
		return -1;
	}

	members = btf_members(trace_type);
	vlen = btf_vlen(trace_type);

	for (i = 0; i < vlen; i++) {
		const char *name = btf__name_by_offset(btf, members[i].name_off);
		size_t offset = btf_member_bit_offset(trace_type, i) / 8;

		if (!name)
			continue;

		debug_msg("bpf: field '%s' at offset %zu\n", name, offset);

		if (strcmp(name, "event_type") == 0)
			ctx->offset.event_type = offset;
		else if (strcmp(name, "is_final") == 0)
			ctx->offset.is_final = offset;
		else if (strcmp(name, "id") == 0) {
			ctx->offset.id = offset;
			config.has_id = true;
		} else if (strcmp(name, "curr_state") == 0)
			ctx->offset.curr_state = offset;
		else if (strcmp(name, "cpu") == 0)
			ctx->offset.cpu = offset;
		else if (strcmp(name, "pid") == 0)
			ctx->offset.pid = offset;
		else if (strcmp(name, "comm") == 0)
			ctx->offset.comm = offset;
		else if (strcmp(name, "event") == 0)
			ctx->offset.event = offset;
		else if (strcmp(name, "next_state") == 0)
			ctx->offset.next_state = offset;
	}

	return 0;
}

/*
 * bpf_print_header - print trace output header
 */
static void bpf_print_header(void)
{
	printf("%16s-%-8s %5s %5s ", "<TASK>", "PID", "[CPU]", "TYPE");
	if (config.has_id)
		printf(" %8s", "ID");

	printf("%24s x %-24s -> %-24s %s\n",
		"STATE",
		"EVENT",
		"NEXT_STATE",
		"FINAL");

	printf("%16s %-8s %5s %5s ", " | ", " | ", " | ", " | ");

	if (config.has_id)
		printf(" %8s", " | ");
	printf("%24s   %-24s    %-24s %s\n", " | ", " | ", " | ", "|");
}

/*
 * handle_event - ring buffer callback for trace events
 */
static int handle_event(void *ctx, void *data, size_t data_sz)
{
	struct bpf_monitor_ctx *mon_ctx = ctx;
	const uint8_t *raw = data;
	const char *comm;
	uint8_t event_type, is_final;
	uint32_t id, cpu, pid, curr_state, event, next_state;

	if (should_stop())
		return 1;

	if (config.has_id)
		id = *(const uint32_t *)(raw + mon_ctx->offset.id);
	pid = *(const uint64_t *)(raw + mon_ctx->offset.pid);

	if (config.has_id && (config.my_pid == id))
		return 0;
	else if (config.my_pid == pid)
		return 0;

	event_type = *(const uint8_t *)(raw + mon_ctx->offset.event_type);
	cpu = *(const uint64_t *)(raw + mon_ctx->offset.cpu);
	comm = (const char *)(raw + mon_ctx->offset.comm);
	curr_state = *(const uint32_t *)(raw + mon_ctx->offset.curr_state);
	event = *(const uint32_t *)(raw + mon_ctx->offset.event);

	printf("%16s-%-8d [%.3d] ", comm, pid, cpu);
	if (event_type == RV_TRACE_ERROR) {
		printf("error ");
		if (config.has_id)
			printf(" %8u", id);
		printf(" %24s x %-24s\n",
		       get_state_name(mon_ctx, curr_state),
		       get_event_name(mon_ctx, event));
	} else {
		printf("event ");
		is_final = *(const uint8_t *)(raw + mon_ctx->offset.is_final);
		next_state = *(const uint32_t *)(raw + mon_ctx->offset.next_state);

		if (config.has_id)
			printf(" %8u", id);
		printf(" %24s x %-24s -> %-24s %s\n",
		       get_state_name(mon_ctx, curr_state),
		       get_event_name(mon_ctx, event),
		       get_state_name(mon_ctx, next_state),
		       is_final ? "Y" : "N");
	}

	return 0;
}

/*
 * extract_enum_names - extract names from a BTF enum
 *
 * Reads enum member names from BTF and stores them in dest array.
 * Returns the number of enum members extracted (excluding the
 * {state/event}_max_NAME entry and trimming the _NAME padding).
 */
static int extract_enum_names(const struct btf *btf, const char *enum_kind,
			       char dest[][MAX_DA_NAME_LEN], struct bpf_monitor_ctx *ctx)
{
	const struct btf_type *enum_type;
	const struct btf_enum *enums;
	char enum_name[64];
	__u32 type_id;
	int i, vlen;
	int count = 0;

	snprintf(enum_name, sizeof(enum_name), "%ss_%s", enum_kind, ctx->monitor_name);
	type_id = btf__find_by_name_kind(btf, enum_name, BTF_KIND_ENUM);
	if ((int)type_id <= 0) {
		err_msg("bpf: could not find enum '%s' in BTF\n", enum_name);
		return -1;
	}
	enum_type = btf__type_by_id(btf, type_id);
	if (!enum_type) {
		err_msg("bpf: could not get enum type for '%s'\n", enum_name);
		return -1;
	}

	enums = btf_enum(enum_type);
	vlen = btf_vlen(enum_type);

	snprintf(enum_name, sizeof(enum_name), "%s_max_%s", enum_kind, ctx->monitor_name);
	for (i = 0; i < vlen && count < MAX_ENUMS; i++) {
		const char *name = btf__name_by_offset(btf, enums[i].name_off);
		size_t name_len;
		char *padding;

		if (!name || !strcmp(name, enum_name))
			continue;

		padding = strrchr(name, '_');
		name_len = strlen(name);
		if (padding && !strcmp(ctx->monitor_name, padding + 1))
			name_len = (size_t)(padding - name);

		if (name_len >= MAX_DA_NAME_LEN)
			name_len = MAX_DA_NAME_LEN - 1;
		strncpy(dest[count], name, name_len);
		dest[count][name_len] = '\0';
		count++;
	}

	return count;
}

/*
 * extract_btf_info - extract BTF types information from the monitor
 *
 * Extract state and event names from enums using BTF and extract field
 * offsets for flexible event parsing.
 */
static int extract_btf_info(struct bpf_object *obj, struct bpf_monitor_ctx *ctx)
{
	const struct btf *btf;

	btf = bpf_object__btf(obj);
	if (!btf) {
		err_msg("bpf: no BTF found in BPF object\n");
		return -1;
	}

	if (extract_field_offsets(btf, ctx)) {
		err_msg("bpf: failed to extract field offsets\n");
		return -1;
	}

	ctx->num_states = extract_enum_names(btf, "state", ctx->state_names, ctx);
	if (ctx->num_states < 0)
		err_msg("bpf: failed to extract state names\n");
	ctx->num_events = extract_enum_names(btf, "event", ctx->event_names, ctx);
	if (ctx->num_events < 0)
		err_msg("bpf: failed to extract event names\n");

	if (ctx->num_states > 0 || ctx->num_events > 0)
		return 0;

	return -1;
}

/*
 * find_bpf_object - search for BPF monitor object file in all directories
 */
static int find_bpf_object(const char *monitor_name, char *path_out, size_t path_len)
{
	char path[MAX_PATH];

	bpf_fill_monitor_paths();
	for (int i = 0; bpf_monitor_paths[i][0]; i++) {
		size_t size = snprintf(path, sizeof(path), "%s/%s.o",
				       bpf_monitor_paths[i], monitor_name);

		if (size < MAX_PATH && access(path, R_OK) == 0) {
			strncpy(path_out, path, path_len - 1);
			path_out[path_len - 1] = '\0';
			return 1;
		}
	}

	return 0;
}

/*
 * bpf_setup_ring_buffer - set up the ring buffer to trace events
 *
 * Find the ring buffer map and set up the events handler.
 */
static struct ring_buffer *
bpf_setup_ring_buffer(char *monitor_name, struct bpf_object *obj, void *ctx)
{
	struct ring_buffer *rb;
	struct bpf_map *map;
	char ringbuf_name[64];

	snprintf(ringbuf_name, sizeof(ringbuf_name), "da_ringbuf_%s", monitor_name);
	map = bpf_object__find_map_by_name(obj, ringbuf_name);
	if (!map) {
		err_msg("bpf: error finding ring buffer %s\n", ringbuf_name);
		return NULL;
	}

	rb = ring_buffer__new(bpf_map__fd(map), handle_event, ctx, NULL);
	if (!rb) {
		err_msg("bpf: error opening ring buffer: %s\n", strerror(errno));
		return NULL;
	}

	return rb;
}

/*
 * bpf_usage_print_reactors - print available BPF reactors (not supported)
 */
void bpf_usage_print_reactors(void)
{
	fprintf(stderr, "  reactors are not supported on BPF monitors\n");
}

/*
 * bpf_run_monitor - load and run a BPF monitor
 *
 * Returns 1 if monitor was found and executed, 0 if not found, -1 on error
 */
int bpf_run_monitor(char *monitor_name, int argc, char **argv)
{
	struct bpf_link *links[MAX_LINKS] = {0};
	struct bpf_monitor_ctx ctx = {0};
	struct bpf_object *obj = NULL;
	struct bpf_program *prog;
	struct ring_buffer *rb = NULL;
	struct bpf_map *map;
	char bpf_obj_path[MAX_PATH];
	int retval, link_count = 0, enable_mon_fd = -1;
	LIBBPF_OPTS(bpf_object_open_opts, opts,
		.pin_root_path = BPF_PIN_BASE_PATH,
	);

	if (!find_bpf_object(monitor_name, bpf_obj_path, sizeof(bpf_obj_path))) {
		err_msg("bpf: error finding monitor %s\n", monitor_name);
		return 0;
	}

	strncpy(ctx.monitor_name, monitor_name, sizeof(ctx.monitor_name) - 1);

	retval = parse_arguments(monitor_name, argc, argv);
	if (retval)
		mon_usage(1, monitor_name, "bpf: failed parsing arguments");

	/* None supported for now */
	if (config.reactor) {
		mon_usage(1, monitor_name, "bpf: failed to set %s reactor",
			  config.reactor);
		return -1;
	}

	libbpf_set_print(libbpf_print_fn);

	obj = bpf_object__open_file(bpf_obj_path, &opts);
	if (!obj) {
		err_msg("bpf: error opening object file: %s\n", strerror(errno));
		return -1;
	}

	if (extract_btf_info(obj, &ctx))
		return -1;

	retval = bpf_object__load(obj);
	if (retval) {
		err_msg("bpf: error loading object file: %s\n", strerror(-retval));
		goto cleanup;
	}

	if (config.trace)
		rb = bpf_setup_ring_buffer(monitor_name, obj, &ctx);

	bpf_object__for_each_program(prog, obj) {
		struct bpf_link *link = NULL;
		char pin_path[MAX_PATH];
		const char *prog_name;
		bool reused = true;

		/* Special program to initialise the monitor */
		if (!strcmp(bpf_program__name(prog), PROG_ENABLE_MON)) {
			enable_mon_fd = bpf_program__fd(prog);
			continue;
		}

		prog_name = bpf_program__name(prog);
		snprintf(pin_path, sizeof(pin_path), "%s/%s_%s",
			 BPF_PIN_BASE_PATH, monitor_name, prog_name);

		if (access(pin_path, F_OK) == 0)
			link = bpf_link__open(pin_path);
		if (!link) {
			reused = false;
			unlink(pin_path);
			link = bpf_program__attach(prog);
			if (!link) {
				err_msg("bpf: error attaching program '%s': %s\n",
					prog_name, strerror(errno));
				goto cleanup;
			}
		}
		links[link_count++] = link;

		if (!reused) {
			retval = bpf_link__pin(link, pin_path);
			if (retval) {
				err_msg("bpf: failed to pin link '%s': %s\n",
					prog_name, strerror(-retval));
				goto cleanup;
			}
		}

		if (link_count >= MAX_LINKS) {
			err_msg("bpf: too many programs to attach\n");
			break;
		}
	}

	if (enable_mon_fd < 0) {
		err_msg("bpf: could not find program %s\n", PROG_ENABLE_MON);
		goto cleanup;
	}
	retval = bpf_prog_test_run_opts(enable_mon_fd, NULL);
	if (retval) {
		err_msg("bpf: error enabling the monitor: %s\n", strerror(-retval));
		goto cleanup;
	}

	if (config.trace)
		bpf_print_header();

	while (!should_stop()) {
		if (!config.trace) {
			sleep(1);
			continue;
		}
		retval = ring_buffer__poll(rb, 100);
		if (retval == -EINTR)
			break;
		if (retval < 0) {
			err_msg("bpf: error polling ring buffer: %s\n", strerror(retval));
			break;
		}
	}

cleanup:
	for (int i = 0; i < link_count; i++) {
		bpf_link__unpin(links[i]);
		bpf_link__destroy(links[i]);
	}

	if (config.trace)
		ring_buffer__free(rb);

	bpf_object__for_each_map(map, obj) {
		if (bpf_map__pin_path(map))
			bpf_map__unpin(map, NULL);
	}

	bpf_object__close(obj);

	return 1;
}
