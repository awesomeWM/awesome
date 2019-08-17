/*
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2010-2012 Uli Schlachter <psychon@znc.in>
 * Copyright ©      2019 Preston Carpenter <APragmaticPlace@gmail.com>
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

#include <cairo-xcb.h>

#include "globalconf.h"

xcb_pixmap_t x11_get_pixmap(struct drawable_t *drawable)
{
    assert(drawable->impl_data);
    struct x11_drawable *x11_data = drawable->impl_data;
    return x11_data->pixmap;
}

void x11_drawable_allocate(struct drawable_t *drawable)
{
    if (!drawable->impl_data)
    {
        drawable->impl_data = calloc(1, sizeof(struct x11_drawable));
    }

    struct x11_drawable *x11_data = drawable->impl_data;
    x11_data->pixmap = XCB_NONE;
}
void x11_drawable_unset_surface(struct drawable_t *drawable)
{

    assert(drawable->impl_data);
    struct x11_drawable *x11_data = drawable->impl_data;

    if (x11_data->pixmap)
    {
        xcb_free_pixmap(globalconf.connection, x11_data->pixmap);
    }
    x11_data->pixmap = XCB_NONE;

}

void x11_drawable_create_pixmap(struct drawable_t *drawable)
{
    assert(drawable->impl_data);
    struct x11_drawable *x11_data = drawable->impl_data;

    x11_data->pixmap = xcb_generate_id(globalconf.connection);
    xcb_create_pixmap(globalconf.connection, globalconf.default_depth, x11_data->pixmap,
            globalconf.screen->root, drawable->geometry.width, drawable->geometry.height);
    drawable->surface = cairo_xcb_surface_create(globalconf.connection,
            x11_data->pixmap, globalconf.visual,
            drawable->geometry.width, drawable->geometry.height);
}

void x11_drawable_cleanup(struct drawable_t *drawable)
{
    if (drawable->impl_data)
    {
        free(drawable->impl_data);
        drawable->impl_data = NULL;
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
