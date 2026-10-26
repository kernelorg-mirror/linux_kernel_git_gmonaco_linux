// SPDX-License-Identifier: GPL-2.0
/*
 * BPF monitor support: allows rv to control BPF monitors.
 *
 * Copyright (C) 2026 Red Hat Inc, Gabriele Monaco <gmonaco@redhat.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <inttypes.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <bpf/btf.h>

#include <bpf_monitor.h>
#include <trace.h>
#include <utils.h>
#include <rv.h>

static char bpf_base_paths[][MAX_PATH] = {
	".",
	"/etc/rv",
	"/usr/local/share/rv",
	"/usr/share/rv",
	"", /* Marker */
};

/* Path used for development, searched first */
#define DEVEL_PATH 0

#define MAX_ENUMS 64
#define MAX_LINKS 16
#define PROG_ENABLE_MON "enable_monitor"
#define PROG_INIT_TASK "init_task"
#define RV_TRACE_STRUCT "rv_trace_entry"
#define RV_TRACE_TYPE_ENUM "rv_trace_type"

enum trace_type_id {
	TRACE_TYPE_ERROR,
	TRACE_TYPE_EVENT,
	TRACE_TYPE_MAX,
};

static const char *const trace_type_names[] = {
	[TRACE_TYPE_ERROR] = "RV_TRACE_ERROR",
	[TRACE_TYPE_EVENT] = "RV_TRACE_EVENT",
};

struct bpf_monitor_ctx {
	char monitor_name[MAX_DA_NAME_LEN];
	int trace_types[TRACE_TYPE_MAX];
};

/*
 * bpf_read_enable - reads monitor's enable status
 *
 * Iterate through all BPF maps in the system, if the rv_mon_NAME map is
 * loaded, the monitor is enabled.
 * Since map names have limited size and may get truncated, check that also the
 * corresponding BTF matches.
 */
static int bpf_read_enable(const char *monitor_name)
{
	char ringbuf_name[2 * MAX_DA_NAME_LEN];
	uint32_t id = 0;

	snprintf(ringbuf_name, sizeof(ringbuf_name),
		 "rv_mon_%s", monitor_name);

	while (bpf_map_get_next_id(id, &id) == 0) {
		struct bpf_map_info info = { 0 };
		uint32_t info_len = sizeof(info);
		struct btf *btf;
		int type_id;
		int fd = bpf_map_get_fd_by_id(id);

		if (fd < 0)
			continue;

		if (bpf_map_get_info_by_fd(fd, &info, &info_len) != 0) {
			close(fd);
			continue;
		}
		close(fd);

		if (strncmp(info.name, ringbuf_name, BPF_OBJ_NAME_LEN - 1) != 0)
			continue;

		if (!info.btf_id)
			continue;
		btf = btf__load_from_kernel_by_id(info.btf_id);
		if (!btf)
			continue;

		type_id = btf__find_by_name_kind(btf, ringbuf_name, BTF_KIND_VAR);
		btf__free(btf);
		if (type_id > 0)
			return 1;
	}

	return 0;
}

/*
 * bpf_read_desc - read monitors' description
 *
 * Return the provided string containing the monitor's description, NULL
 * otherwise.
 */
static char *bpf_read_desc(char *desc, struct bpf_object *obj, const char *monitor_name)
{
	struct bpf_map *map = bpf_object__find_map_by_name(obj, ".rodata.description");
	const char *desc_data;
	size_t desc_size;

	if (!map) {
		debug_msg("bpf: cannot find description for %s\n",
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
 * bpf_fill_base_paths - fill the path for development builds
 *
 * RV searches for BPF monitors on absolute paths on the system as well
 * as in the same directory of the rv binary. This is useful when running
 * rv from the kernel tree. This function resolves the right location.
 */
static void bpf_fill_base_paths(void)
{
	char tmp_path[MAX_PATH], *dir;
	ssize_t len;

	len = readlink("/proc/self/exe", tmp_path, MAX_PATH);
	if (len > 0 && len != MAX_PATH) {
		tmp_path[len] = '\0';
		dir = dirname(tmp_path);
		snprintf(bpf_base_paths[DEVEL_PATH], MAX_PATH, "%s", dir);
	}
}

static void bpf_object_iterate_path(const char *base_path, const char *subdir,
				    void (*action)(const char *name, struct bpf_object *obj))
{
	char path[MAX_PATH];
	struct dirent *entry;
	DIR *dir;
	char *ext;

	snprintf(path, sizeof(path), "%s/%s", base_path, subdir);
	dir = opendir(path);
	if (!dir) {
		debug_msg("bpf: error opening directory: %s\n", path);
		return;
	}

	while ((entry = readdir(dir)) != NULL) {
		size_t size;
		struct bpf_object *obj;
		char name[MAX_DA_NAME_LEN], obj_path[MAX_PATH];

		if (entry->d_name[0] == '.')
			continue;

		ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".o") != 0)
			continue;

		size = (size_t)(ext - entry->d_name);
		if (size >= MAX_DA_NAME_LEN) {
			err_msg("bpf: name too long, skipping %s/%s\n", path,
				entry->d_name);
			continue;
		}
		memcpy(name, entry->d_name, size);
		name[size] = '\0';

		size = snprintf(obj_path, sizeof(obj_path), "%s/%s", path,
				entry->d_name);
		obj = bpf_object__open_file(obj_path, NULL);
		if (!obj || size >= MAX_PATH) {
			err_msg("bpf: error opening object file %s: %s\n",
				obj_path, strerror(errno));
			continue;
		}

		action(name, obj);

		bpf_object__close(obj);
	}

	closedir(dir);
}

static void list_monitor_action(const char *name, struct bpf_object *obj)
{
	char desc[MAX_DESCRIPTION];

	if (!bpf_read_desc(desc, obj, name)) {
		err_msg("bpf: monitor %s does not have desc map, bug?\n", name);
		return;
	}

	printf("%-*s %s %s\n", MAX_DA_NAME_LEN, name,
	       desc, bpf_read_enable(name) ? "[ON]" : "[OFF]");
}

/*
 * list_monitors_from_path - list monitors from a specific base path
 */
static void list_monitors_from_path(const char *base_path)
{
	bpf_object_iterate_path(base_path, "bpf_monitors", list_monitor_action);
}

/*
 * bpf_list_monitors - list available BPF monitors from all sources
 *
 * @container: BPF monitors are not nested, skip listing.
 *
 * Returns 0 on success
 */
int bpf_list_monitors(char *container)
{
	if (container)
		return 0;
	bpf_fill_base_paths();
	for (int i = 0; bpf_base_paths[i][0]; i++)
		list_monitors_from_path(bpf_base_paths[i]);

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
 * extract_trace_type_metadata - extract trace type enum values from BTF
 *
 * Introspect the rv_trace_type enum via BTF and store enum values for
 * event type dispatch and configuration.
 *
 * Returns 0 on success, -1 on error
 */
static int extract_trace_type_metadata(const struct btf *btf, struct bpf_monitor_ctx *ctx)
{
	const struct btf_type *enum_type;
	const struct btf_enum *enums;
	int type_id, vlen, filled = 0;

	for (int i = 0; i < TRACE_TYPE_MAX; i++)
		ctx->trace_types[i] = -1;

	type_id = btf__find_by_name_kind(btf, RV_TRACE_TYPE_ENUM, BTF_KIND_ENUM);
	if (type_id <= 0) {
		debug_msg("bpf: could not find enum '%s' in BTF\n", RV_TRACE_TYPE_ENUM);
		return -1;
	}

	enum_type = btf__type_by_id(btf, type_id);
	if (!enum_type) {
		debug_msg("bpf: could not get type for '%s'\n", RV_TRACE_TYPE_ENUM);
		return -1;
	}

	enums = btf_enum(enum_type);
	vlen = btf_vlen(enum_type);

	for (int i = 0; i < vlen; i++) {
		const char *name = btf__name_by_offset(btf, enums[i].name_off);

		if (!name)
			continue;

		for (int j = 0; j < TRACE_TYPE_MAX; j++) {
			if (strcmp(name, trace_type_names[j]) == 0) {
				ctx->trace_types[j] = enums[i].val;
				++filled;
				break;
			}
		}
	}

	if (filled < TRACE_TYPE_MAX) {
		debug_msg("bpf: could not find all trace types in BTF\n");
		return -1;
	}

	return 0;
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

	if (extract_trace_type_metadata(btf, ctx)) {
		err_msg("bpf: failed to extract trace type metadata\n");
		return -1;
	}

	return 0;
}

/*
 * find_bpf_file - search for a BPF object file in a specific subdirectory
 */
static int find_bpf_file(const char *subdir, const char *name, char *path_out, size_t path_len)
{
	char path[MAX_PATH];

	for (int i = 0; bpf_base_paths[i][0]; i++) {
		size_t size = snprintf(path, sizeof(path), "%s/%s/%s.o",
				       bpf_base_paths[i], subdir, name);

		if (size < MAX_PATH && access(path, R_OK) == 0) {
			strncpy(path_out, path, path_len - 1);
			path_out[path_len - 1] = '\0';
			return 1;
		}
	}

	return 0;
}

/*
 * find_bpf_monitor - search for BPF monitor object file in all directories
 */
static int find_bpf_monitor(const char *monitor_name, char *path_out, size_t path_len)
{
	return find_bpf_file("bpf_monitors", monitor_name, path_out, path_len);
}

/*
 * bpf_usage_print_reactors - print available BPF reactors
 */
void bpf_usage_print_reactors(void)
{
	fprintf(stderr, "  available BPF reactors: nop\n");
}

/*
 * bpf_enable_tracing - set the trace_level variable in .rodata
 *
 * Find trace_level in a special section of .rodata and set its value before
 * loading.
 *
 * Returns 0 on success, -1 on error.
 */
static int bpf_enable_tracing(struct bpf_object *obj, int val)
{
	struct bpf_map *map = bpf_object__find_map_by_name(obj, ".rodata.trace_level");
	size_t data_size;
	int *trace_level;

	if (!map)
		return -1;

	trace_level = bpf_map__initial_value(map, &data_size);
	if (!trace_level || data_size != sizeof(*trace_level))
		return -1;

	*trace_level = val;
	return 0;
}

#ifdef __x86_64__
#define X86_64 'y'
#else
#define X86_64 'n'
#endif

/*
 * open_bpf_monitor - open and load a BPF monitor object from a file path
 *
 * Returns loaded BPF object on success, NULL on error.
 */
static struct bpf_object *open_bpf_monitor(const char *path, struct bpf_monitor_ctx *ctx)
{
	struct bpf_object *obj = NULL;
	int res, nproc = libbpf_num_possible_cpus();
	char kconfig[128];

	if (nproc < 0) {
		err_msg("bpf: error getting possible CPUs: %s\n", strerror(-nproc));
		return NULL;
	}
	/* Define statically what is known, Kconfig may not be available. */
	snprintf(kconfig, sizeof(kconfig), "CONFIG_NR_CPUS=%d\nCONFIG_X86_64=%c\n",
		 nproc, X86_64);

	LIBBPF_OPTS(bpf_object_open_opts, opts,
		.object_name = ctx->monitor_name,
		.kconfig = kconfig,
	);

	obj = bpf_object__open_file(path, &opts);
	if (!obj) {
		err_msg("bpf: error opening object: %s\n", strerror(errno));
		return NULL;
	}

	if (config.trace) {
		res = extract_btf_info(obj, ctx);
		if (res || bpf_enable_tracing(obj, ctx->trace_types[TRACE_TYPE_EVENT])) {
			err_msg("bpf: failed to enable tracing\n");
			bpf_object__close(obj);
			return NULL;
		}
	}

	res = bpf_object__load(obj);
	if (res) {
		err_msg("bpf: error loading object: %s\n", strerror(-res));
		bpf_object__close(obj);
		return NULL;
	}

	return obj;
}

/*
 * attach_bpf_handlers - attach all BPF programs
 *
 * Attach all handler programs and store links in the provided array. Skip
 * known non-handler programs.
 *
 * Returns fd of enable program on success, -1 on error.
 */
static int attach_bpf_handlers(const char *monitor_name, struct bpf_object *obj,
				struct bpf_link **links, int *link_count)
{
	struct bpf_program *prog;
	int enable_mon_fd = -1;

	bpf_object__for_each_program(prog, obj) {
		struct bpf_link *link = NULL;
		const char *prog_name;

		/* Special program to initialise the monitor */
		if (!strcmp(bpf_program__name(prog), PROG_ENABLE_MON)) {
			enable_mon_fd = bpf_program__fd(prog);
			continue;
		}

		/* Task iterator program to initialise existing tasks */
		if (!strcmp(bpf_program__name(prog), PROG_INIT_TASK))
			continue;

		if (*link_count >= MAX_LINKS) {
			err_msg("bpf: too many programs to attach (%d)\n", *link_count);
			return -1;
		}

		prog_name = bpf_program__name(prog);
		link = bpf_program__attach(prog);
		if (!link) {
			err_msg("bpf: error attaching program '%s': %s\n",
				prog_name, strerror(errno));
			return -1;
		}
		links[(*link_count)++] = link;
	}
	if (enable_mon_fd < 0)
		err_msg("bpf: could not find program %s\n", PROG_ENABLE_MON);
	return enable_mon_fd;
}

/*
 * bpf_init_tasks - initialize storage for existing tasks using task iterator
 *
 * Runs the task iterator program if present in the object, initializing
 * task storage for all currently existing tasks before monitor is enabled.
 * Silently succeed if the program is not there (not a per-task monitor).
 *
 * Returns 0 on success, -errno on error.
 */
static int bpf_init_tasks(struct bpf_object *obj)
{
	struct bpf_program *prog;
	struct bpf_link *link;
	char buf[128];
	int iter_fd, retval = 0;
	ssize_t len;

	prog = bpf_object__find_program_by_name(obj, PROG_INIT_TASK);
	if (!prog)
		return 0;

	link = bpf_program__attach_iter(prog, NULL);
	if (!link)
		return -errno;

	iter_fd = bpf_iter_create(bpf_link__fd(link));
	if (iter_fd < 0) {
		bpf_link__destroy(link);
		return -errno;
	}

	do {
		len = read(iter_fd, buf, sizeof(buf));
	} while (len > 0 || (len < 0 && errno == EINTR));

	retval = (len < 0) ? -errno : 0;

	close(iter_fd);
	bpf_link__destroy(link);

	return retval;
}

/*
 * bpf_validate_name - check that a monitor name is valid
 *
 * For simplicity, just avoid path separators.
 *
 * Returns 0 if the name is valid, -1 otherwise.
 */
static int bpf_validate_name(const char *monitor_name)
{
	if (strchr(monitor_name, '/'))
		return -1;

	return 0;
}

/*
 * bpf_run_monitor - load and run a BPF monitor
 *
 * Returns 1 if monitor was found and executed, 0 if not found, -1 on error
 */
int bpf_run_monitor(char *monitor_name, int argc, char **argv)
{
	struct bpf_link *links[MAX_LINKS] = {0};
	struct trace_instance inst = {0};
	struct bpf_monitor_ctx ctx = {0};
	struct bpf_object *obj = NULL;
	int res, link_count = 0, enable_mon_fd, retval = -1;
	char monitor_path[MAX_PATH];

	libbpf_set_print(libbpf_print_fn);
	bpf_fill_base_paths();

	if (bpf_validate_name(monitor_name)) {
		err_msg("bpf: invalid monitor name: %s\n", monitor_name);
		return -1;
	}

	if (!find_bpf_monitor(monitor_name, monitor_path, sizeof(monitor_path)))
		return 0;

	config.is_bpf = true;

	res = bpf_read_enable(monitor_name);
	if (res) {
		err_msg("bpf: monitor %s (BPF) is already enabled\n", monitor_name);
		return -1;
	}

	/* we should be good to go */
	res = parse_arguments(monitor_name, argc, argv);
	if (res)
		mon_usage(1, monitor_name, "bpf: failed parsing arguments");

	strncpy(ctx.monitor_name, monitor_name, sizeof(ctx.monitor_name) - 1);


	obj = open_bpf_monitor(monitor_path, &ctx);
	if (!obj)
		goto cleanup;

	if (config.trace) {
		if (trace_instance_setup(&inst, monitor_name))
			goto cleanup;
	}

	enable_mon_fd = attach_bpf_handlers(monitor_name, obj, links, &link_count);
	if (enable_mon_fd < 0)
		goto cleanup;

	res = bpf_init_tasks(obj);
	if (res) {
		err_msg("bpf: error initializing tasks: %s\n", strerror(-res));
		goto cleanup;
	}

	res = bpf_prog_test_run_opts(enable_mon_fd, NULL);
	if (res) {
		err_msg("bpf: error enabling the monitor: %s\n", strerror(-res));
		goto cleanup;
	}

	if (config.trace)
		trace_print_header(inst.seq);

	while (!should_stop()) {
		if (config.trace) {
			res = tracefs_iterate_raw_events(inst.tep,
							 inst.inst,
							 NULL,
							 0,
							 collect_registered_events,
							 &inst);
			if (res) {
				err_msg("bpf: error reading trace buffer\n");
				break;
			}
		}

		sleep(1);
	}

	retval = 1;

cleanup:
	for (int i = 0; i < link_count; i++)
		bpf_link__destroy(links[i]);

	trace_instance_destroy(&inst);

	bpf_object__close(obj);

	return retval;
}
