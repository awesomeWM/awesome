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

#include "drawable.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1.h"
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

static void setup_buffer(area_t geo, int stride, struct wl_buffer **out_buffer,
        void **out_shm_data, size_t *size)
{
    assert(out_buffer);
    assert(out_shm_data);
    assert(size);

    if (*out_shm_data != NULL) {
        munmap(*out_shm_data, *size);
        *out_shm_data = NULL;
        *size = 0;
    }

    *size = stride * geo.height;
    if (*size == 0)
        return;

    int fd = anon_file();
    if (fd < 0)
        fatal("Could not allocate any wl_shm_pools buffers");
    if (ftruncate(fd, *size) < 0)
        fatal("Could not resize shared memory file");

    *out_shm_data = mmap(NULL, *size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (*out_shm_data == MAP_FAILED)
        fatal("mmap failed");

	struct wl_shm_pool *pool = wl_shm_create_pool(globalconf.wl_shm, fd, *size);
    assert(pool);

    *out_buffer = wl_shm_pool_create_buffer(pool, 0,
            geo.width, geo.height, stride, WL_SHM_FORMAT_ARGB8888);
    assert(*out_buffer);

    wl_shm_pool_destroy(pool);
    close(fd);
}

xcb_pixmap_t wayland_get_pixmap(struct drawable_t *drawable)
{
    fatal("Attempted to get an X11 pixmap while using Wayland");
}

void wayland_drawable_allocate(struct drawable_t *drawable)
{
    drawable->impl_data = calloc(1, sizeof(struct wayland_drawable));

    struct wayland_drawable *wayland_drawable = drawable->impl_data;
    wayland_drawable->wl_surface =
        wl_compositor_create_surface(globalconf.wl_compositor);
}

void wayland_drawable_cleanup(struct drawable_t *drawable)
{
    wayland_drawable_unset_surface(drawable);

    struct wayland_drawable *wayland_drawable = drawable->impl_data;
    if (wayland_drawable->wl_surface != NULL)
        wl_surface_destroy(wayland_drawable->wl_surface);

    free(drawable->impl_data);
    drawable->impl_data = NULL;
}

void wayland_drawable_create_buffer(struct drawable_t *drawable)
{
    struct wayland_drawable *wayland_drawable = drawable->impl_data;

    if (drawable->geometry.width == 0 || drawable->geometry.height == 0)
        return;
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
            drawable->geometry.width);

    wayland_drawable_unset_surface(drawable);
    setup_buffer(drawable->geometry, stride,
            &wayland_drawable->buffer, &wayland_drawable->shm_data,
            &wayland_drawable->shm_size);

    drawable->surface =
        cairo_image_surface_create_for_data(wayland_drawable->shm_data,
                CAIRO_FORMAT_ARGB32, drawable->geometry.width,
                drawable->geometry.height,
                stride);
}

void wayland_drawable_unset_surface(struct drawable_t *drawable)
{
    struct wayland_drawable *wayland_drawable = drawable->impl_data;

    if (wayland_drawable->wl_surface != NULL && wayland_drawable->buffer != NULL)
    {
        wl_surface_attach(wayland_drawable->wl_surface, NULL, 0, 0);
        wl_buffer_destroy(wayland_drawable->buffer);
        wayland_drawable->buffer = NULL;
    }

}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
