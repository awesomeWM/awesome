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
} textbox_data_t;

static int
textbox_draw(draw_context_t *ctx, int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset, int used,
             void *p __attribute__ ((unused)))
{
    textbox_data_t *d = w->widget->data;
    draw_parser_data_t pdata, *pdata_arg = NULL;

    if(d->width)
        w->area.width = d->width;
    else if(w->widget->align == AlignFlex)
        w->area.width = ctx->width - used;
    else
    {
        w->area.width = MIN(draw_text_extents(ctx->connection,
                                              ctx->phys_screen,
                                              globalconf.font, d->text, &pdata).width,
                            ctx->width - used);
        pdata_arg = &pdata;
    }

    w->area.height = ctx->height;

    w->area.x = widget_calculate_offset(ctx->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

    draw_text(ctx, globalconf.font, w->area, d->text, pdata_arg);

    return w->area.width;
}

static widget_tell_status_t
textbox_tell(widget_t *widget, const char *property, const char *new_value)
{
    textbox_data_t *d = widget->data;

    if(!a_strcmp(property, "text"))
    {
        p_delete(&d->text);
        a_iso2utf8(new_value, &d->text);
    }
    else if(!a_strcmp(property, "width"))
        d->width = atoi(new_value);
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

static void
textbox_destructor(widget_t *w)
{
    textbox_data_t *d = w->data;
    p_delete(&d->text);
    p_delete(&d);
}

widget_t *
textbox_new(alignment_t align)
{
    widget_t *w;
    textbox_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = textbox_draw;
    w->tell = textbox_tell;
    w->destructor = textbox_destructor;
    w->data = d = p_new(textbox_data_t, 1);

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
