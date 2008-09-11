/*
 * appicon.c - application icon widget
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include "widget.h"
#include "ewmh.h"
#include "titlebar.h"

extern awesome_t globalconf;

/** Draw a application icon widget.
 * \param ctx The draw context.
 * \param screen The screen.
 * \param w The widget node we are linked from.
 * \param offset Offset to draw at.
 * \param used The size used on the element.
 * \param p A pointer to the object we're draw onto.
 * \param type The type of the object.
 * \return The width used.
 */
static int
appicon_draw(draw_context_t *ctx, int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset, int used,
             void *p, awesome_type_t type)
{
    client_t *c = NULL;
    draw_image_t *image;

    switch(type)
    {
      case AWESOME_TYPE_STATUSBAR:
        c = globalconf.screen_focus->client_focus;
        break;
      case AWESOME_TYPE_TITLEBAR:
        c = client_getbytitlebar(p);
        break;
    }

    if(c)
    {
        if(!(image = draw_image_new(c->icon_path)))
            image = c->icon;

        if(image)
        {
            w->area.width = ((double) ctx->height / (double) image->height) * image->width;
            w->area.x = widget_calculate_offset(ctx->width,
                                                w->area.width,
                                                offset,
                                                w->widget->align);
            draw_image(ctx, w->area.x,
                       w->area.y, ctx->height, image);
            if(image != c->icon)
                draw_image_delete(&image);
        }
    }
    else
        w->area.width = 0;

    return w->area.width;
}

/** Create a new appicon widget.
 * \param align Widget alignment.
 * \return A brand new widget.
 */
widget_t *
appicon_new(alignment_t align)
{
    widget_t *w;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = appicon_draw;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
