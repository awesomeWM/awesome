/*
 * font.c - font functions
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include <cairo/cairo-xcb.h>
#include <pango/pangocairo.h>

#include "font.h"
#include "screen.h"
#include "globalconf.h"
#include "common/xutil.h"

/** Create a new Pango font.
 * \param fontname Pango fontname (e.g. [FAMILY-LIST] [STYLE-OPTIONS] [SIZE]).
 * \return A new font.
 */
font_t *
draw_font_new(const char *fontname)
{
    cairo_surface_t *surface;
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, globalconf.default_screen);
    cairo_t *cr;
    PangoLayout *layout;
    font_t *font = p_new(font_t, 1);

    /* Create a dummy cairo surface, cairo context and pango layout in
     * order to get font informations */
    surface = cairo_xcb_surface_create(globalconf.connection,
                                       globalconf.default_screen,
                                       globalconf.screens.tab[0].visual,
                                       s->width_in_pixels,
                                       s->height_in_pixels);

    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    /* Get the font description used to set text on a PangoLayout */
    font->desc = pango_font_description_from_string(fontname);
    pango_layout_set_font_description(layout, font->desc);

    /* Get height */
    pango_layout_get_pixel_size(layout, NULL, &font->height);

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return font;
}

/** Delete a font.
 * \param font Font to delete.
 */
void
draw_font_delete(font_t **font)
{
    if(*font)
    {
        pango_font_description_free((*font)->desc);
        p_delete(font);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
