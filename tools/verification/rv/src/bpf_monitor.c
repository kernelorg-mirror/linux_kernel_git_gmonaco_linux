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
