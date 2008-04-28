/*
 * iconbox.c - icon widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
#include "common/util.h"
#include "common/configopts.h"

extern AwesomeConf globalconf;

typedef struct
{
    char *image;
    bool resize;
} Data;

static int
iconbox_draw(widget_t *widget, draw_context_t *ctx, int offset,
             int used __attribute__ ((unused)))
{
    Data *d = widget->data;
    area_t area = draw_get_image_size(d->image);

    /* image not valid */
    if(area.width < 0 || area.height < 0)
        return (widget->area.width = 0);

    if(d->resize)
        widget->area.width = ((double) widget->statusbar->height / area.height) * area.width;
    else
        widget->area.width = area.width;

    widget->area.height = widget->statusbar->height;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_image(ctx, widget->area.x, widget->area.y,
               d->resize ? widget->statusbar->height : 0, d->image);

    return widget->area.width;
}

static widget_tell_status_t
iconbox_tell(widget_t *widget, char *property, char *new_value)
{
    bool b;
    Data *d = widget->data;

    if(new_value == NULL)
        return WIDGET_ERROR_NOVALUE;

    if(!a_strcmp(property, "image"))
    {
        if(d->image)
            p_delete(&d->image);
        d->image = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "resize"))
    {
        if((b = cfg_parse_boolean(new_value)) != -1)
            d->resize = b;
        else
            return WIDGET_ERROR_FORMAT_BOOL;
    }
    else
       return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

widget_t *
iconbox_new(statusbar_t *statusbar, cfg_t *config)
{
    widget_t *w;
    Data *d;

    w = p_new(widget_t, 1);
    widget_common_new(w, statusbar, config);
    w->alignment = cfg_getalignment(config, "align");
    w->draw = iconbox_draw;
    w->tell = iconbox_tell;
    w->data = d = p_new(Data, 1);
    d->image = a_strdup(cfg_getstr(config, "image"));
    d->resize = cfg_getbool(config, "resize");

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
