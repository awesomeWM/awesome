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

extern AwesomeConf globalconf;

typedef struct
{
    char *image;
    Bool resize;
} Data;

static int
iconbox_draw(Widget *widget, DrawCtx *ctx, int offset,
             int used __attribute__ ((unused)))
{
    Data *d = widget->data;
    Area area = draw_get_image_size(d->image);

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

static void
iconbox_tell(Widget *widget, char *property, char *command)
{
    Data *d = widget->data;

    if(!property || !command)
        return;

    if(!a_strcmp(property, "image"))
    {
        if(d->image)
            p_delete(&d->image);
        d->image = a_strdup(command);
    }
    else if(!a_strcmp(property, "resize")) /* XXX how to ignorecase compare? */
    {
        if(!a_strcmp(command, "true") || !a_strcmp(command, "1"))
            d->resize = True;
        else if (!a_strcmp(command, "false") || !a_strcmp(command, "0"))
            d->resize = False;
        else
            warn("Resize value must be (case-sensitive) \"true\", \"false\",\
                  \"1\" or \"0\". But is: %s.\n", command);
    }
    else if(!a_strcmp(property, "align") || !a_strcmp(property, "mouse") ||
            !a_strcmp(property, "x") || !a_strcmp(property, "y"))
        warn("Property \"%s\" can't get changed.\n", property);

    else
    {
        warn("No such property: %s\n", property);
        return;
    }
}

Widget *
iconbox_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->alignment = draw_get_align(cfg_getstr(config, "align"));
    w->draw = iconbox_draw;
    w->tell = iconbox_tell;
    w->data = d = p_new(Data, 1);
    d->image = a_strdup(cfg_getstr(config, "image"));
    d->resize = cfg_getbool(config, "resize");

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
