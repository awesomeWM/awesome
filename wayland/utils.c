/*
 * Copyright © 2019 Preston Carpenter <APragmaticPlace@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "wayland/utils.h"

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include <wayland-client.h>

#include "draw.h"
#include "globalconf.h"

const char *anon_prefix = "/wayland-";

static int anon_file(void)
{
    char name[32] = {0};
    strncpy(name, anon_prefix, sizeof(name));
    for (char *cursor = name + strlen(anon_prefix);
         cursor < name + sizeof(name) - 1;
         cursor++)
    {
        long int num = random();
        char c = (num % 26) + 65;
        *cursor = c;
    }
    name[31] = '\0';
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    shm_unlink(name);
    return fd;
}

void wayland_setup_buffer(area_t geo, struct wl_buffer **out_buffer,
		int *out_stride, void **out_shm_data, size_t *shm_size)
{
    assert(out_buffer);
	assert(out_stride);
    assert(out_shm_data);
    assert(shm_size);

    if (*out_shm_data != NULL) {
        munmap(*out_shm_data, *shm_size);
        *out_shm_data = NULL;
        *shm_size = 0;
    }

    *out_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, geo.width);
    *shm_size = *out_stride * geo.height;
    if (*shm_size == 0)
        return;

    int fd = anon_file();
    if (fd < 0)
        fatal("Could not allocate any wl_shm_pools buffers");
    if (ftruncate(fd, *shm_size) < 0)
        fatal("Could not resize shared memory file");

    *out_shm_data = mmap(NULL, *shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (*out_shm_data == MAP_FAILED)
        fatal("mmap failed");

	struct wl_shm_pool *pool = wl_shm_create_pool(globalconf.wl_shm, fd, *shm_size);
    assert(pool);

    *out_buffer = wl_shm_pool_create_buffer(pool, 0,
            geo.width, geo.height, *out_stride, WL_SHM_FORMAT_ARGB8888);
    assert(*out_buffer);

    wl_shm_pool_destroy(pool);
    close(fd);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
