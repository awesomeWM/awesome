/*
 * textbox.c - text box widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
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
#include "screen.h"
#include "common/util.h"
#include "common/configopts.h"

extern awesome_t globalconf;

typedef struct
{
    char *text;
    int width;
} Data;

static int
textbox_draw(widget_node_t *w, statusbar_t *statusbar, int offset, int used)
{
    Data *d = w->widget->data;

    if(d->width)
        w->area.width = d->width;
    else if(w->widget->align == AlignFlex)
        w->area.width = statusbar->width - used;
    else
        w->area.width = MIN(draw_text_extents(statusbar->ctx->connection,
                                              statusbar->ctx->phys_screen,
                                              globalconf.font, d->text).width,
                            statusbar->width - used);

    w->area.height = statusbar->height;

    w->area.x = widget_calculate_offset(statusbar->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

    draw_text(statusbar->ctx, globalconf.font,
              &statusbar->colors.fg,
              w->area, d->text);

    return w->area.width;
}

static widget_tell_status_t
textbox_tell(widget_t *widget, const char *property, const char *new_value)
{
    Data *d = widget->data;

    if(!a_strcmp(property, "text"))
    {
        p_delete(&d->text);
        d->text = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "width"))
        d->width = atoi(new_value);
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

widget_t *
textbox_new(alignment_t align)
{
    widget_t *w;
    Data *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = textbox_draw;
    w->tell = textbox_tell;
    w->data = d = p_new(Data, 1);

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
