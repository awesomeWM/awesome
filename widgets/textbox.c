/*
 * textbox.c - text box widget
 *
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

extern AwesomeConf globalconf;

typedef struct
{
    char *text;
    int width;
} Data;

static int
textbox_draw(widget_t *widget, draw_context_t *ctx, int offset, int used)
{
    Data *d = widget->data;

    if(d->width)
        widget->area.width = d->width;
    else if(widget->alignment == AlignFlex)
        widget->area.width = widget->statusbar->width - used;
    else
        widget->area.width = MIN(draw_text_extents(ctx->connection, ctx->phys_screen,
                                                   globalconf.screens[widget->statusbar->screen].styles.normal.font, d->text).width,
                                 widget->statusbar->width - used);

    widget->area.height = widget->statusbar->height;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_text(ctx, widget->area, d->text,
              &globalconf.screens[widget->statusbar->screen].styles.normal);

    return widget->area.width;
}

static widget_tell_status_t
textbox_tell(widget_t *widget, char *property, char *new_value)
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
textbox_new(statusbar_t *statusbar, cfg_t *config)
{
    widget_t *w;
    Data *d;

    w = p_new(widget_t, 1);
    widget_common_new(w, statusbar, config);
    w->draw = textbox_draw;
    w->tell = textbox_tell;
    w->alignment = cfg_getalignment(config, "align");

    w->data = d = p_new(Data, 1);

    d->width = cfg_getint(config, "width");

    d->text = a_strdup(cfg_getstr(config, "text"));

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
