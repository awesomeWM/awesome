/*
 * progressbar.c - progress bar widget
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

#include <string.h>
#include "widget.h"
#include "screen.h"
#include "common/util.h"

extern AwesomeConf globalconf;

typedef struct
{
    /** Percent 0 to 100 */
    int *percent;
    /** data_title of the data */
    char **data_title;
    /** Width of the data_items */
    int width;
    /** Padding */
    int padding;
    /** Pixel between data_items (bars) */
    int gap;
    /** Number of data_items (bars) */
    int data_items;
    /** Height 0-1, where 1 is height of statusbar */
    float height;
    /** Foreground color */
    XColor *fg;
    /** Foreground color when bar is half-full */
    XColor **pfg_center;
    /** Foreground color when bar is full */
    XColor **pfg_end;
    /** Background color */
    XColor *bg;
    /** Border color */
    XColor *bordercolor;
} Data;

static int
progressbar_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    int i, width, pwidth, margin_top, pb_height, left_offset;
    Area rectangle;

    Data *d = widget->data;

    if (!(d->data_items) || (d->width - d->padding < 3))
        return 0;

    width = d->width - 2 * d->padding;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 d->width,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    margin_top = (int) (widget->statusbar->height * (1 - d->height)) / 2 + 0.5 + widget->area.y;
    pb_height = (int) ((widget->statusbar->height * d->height - (d->gap * (d->data_items - 1))) / d->data_items + 0.5);
    left_offset = widget->area.x + d->padding;

    for(i = 0; i < d->data_items; i++)
    {
        pwidth = (int) d->percent[i] ? ((width - 2) * d->percent[i]) / 100 : 0;

        rectangle.x = left_offset;
        rectangle.y = margin_top;
        rectangle.width = width;
        rectangle.height = pb_height;

        draw_rectangle(ctx, rectangle, False, d->bordercolor[i]);

        if(pwidth > 0)
        {
            rectangle.x = left_offset + 1;
            rectangle.y = margin_top + 1;
            rectangle.width = pwidth;
            rectangle.height = pb_height - 2;
            draw_rectangle_gradient(ctx, rectangle, width - 2, True, d->fg[i],
                                    d->pfg_center[i], d->pfg_end[i]);
        }

        if(width - 2 - pwidth > 0) /* not filled area */
        {
            rectangle.x = left_offset + 1 + pwidth;
            rectangle.y = margin_top + 1;
            rectangle.width = width - 2 - pwidth;
            rectangle.height = pb_height - 2;
            draw_rectangle(ctx, rectangle, True, d->bg[i]);
        }

        margin_top += (pb_height + d->gap);
    }

    widget->area.width = d->width;
    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static void
progressbar_tell(Widget *widget, char *property, char *command)
{
    Data *d = widget->data;
    int i = 0, percent;
    Bool flag;
    char *title, *setting;

    if(!property || !command || !d->data_items)
        return;

    if(!a_strcmp(property, "data"))
    {
        title = strtok(command, " ");
        setting = strtok(NULL, " ");
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                percent = atoi(setting);
                d->percent[i] = (percent < 0 ? 0 : (percent > 100 ? 100 : percent));
                return;
            }
        warn("No such data section title: %s\n", title);
    }
    else if(!a_strcmp(property, "fg"))
    {
        title = strtok(command, " ");
        setting = strtok(NULL, " ");
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                               setting, &d->fg[i]);
                return;
            }
        warn("No such data section title: %s\n", title);
    }
    else if(!a_strcmp(property, "bg"))
    {
        title = strtok(command, " ");
        setting = strtok(NULL, " ");
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                               setting, &d->bg[i]);
                return;
            }
        warn("No such data section title: %s\n", title);
    }
    else if(!a_strcmp(property, "fg_center"))
    {
        title = strtok(command, " ");
        setting = strtok(NULL, " ");
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                flag = False;
                if(!d->pfg_center[i])
                {
                    flag = True; /* restore to NULL & p_delete */
                    d->pfg_center[i] = p_new(XColor, 1);
                }
                if(!(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                               setting, d->pfg_center[i])))
                    if(flag) /* restore */
                    {
                        p_delete(&d->pfg_center[i]);
                        d->pfg_center[i] = NULL;
                    }
                return;
            }
        warn("No such data section title: %s\n", title);
    }
    else if(!a_strcmp(property, "fg_end"))
    {
        title = strtok(command, " ");
        setting = strtok(NULL, " ");
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                flag = False;
                if(!d->pfg_end[i])
                {
                    flag = True; /* restore to NULL & p_delete */
                    d->pfg_end[i] = p_new(XColor, 1);
                }
                if(!(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                               setting, d->pfg_end[i])))
                    if(flag) /* restore */
                    {
                        p_delete(&d->pfg_end[i]);
                        d->pfg_end[i] = NULL;
                    }
                return;
            }
        warn("No such data section title: %s\n", title);
    }
    else if(!a_strcmp(property, "bordercolor"))
    {
        title = strtok(command, " ");
        setting = strtok(NULL, " ");
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                               setting, &d->bordercolor[i]);
                return;
            }
        warn("No such data section title: %s\n", title);
    }

    else if(!a_strcmp(property, "gap"))
        d->gap = atoi(command);

    else if(!a_strcmp(property, "width"))
    {
        d->width = atoi(command);
        if(d->width - d->padding < 3)
        warn("Progressbar widget needs: (width - padding) >= 3\n");
    }

    else if(!a_strcmp(property, "height"))
        d->height = atof(command);

    else if(!a_strcmp(property, "padding"))
        d->padding = atoi(command);

    else if(!a_strcmp(property, "align") || !a_strcmp(property, "mouse") ||
            !a_strcmp(property, "x") || !a_strcmp(property, "y"))
        warn("Property \"%s\" can't get changed.\n", property);

    else
        warn("No such property: %s\n", property);
    return;
}

Widget *
progressbar_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *color;
    int i, phys_screen = get_phys_screen(statusbar->screen);
    cfg_t *cfg;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    d = w->data = p_new(Data, 1);
    d->width = cfg_getint(config, "width");
    d->height = cfg_getfloat(config, "height");
    d->gap = cfg_getint(config, "gap");
    d->padding = cfg_getint(config, "padding");

    w->alignment = draw_get_align(cfg_getstr(config, "align"));


    if(!(d->data_items = cfg_size(config, "data")))
    {
        warn("Progressbar widget needs at least one bar section\n");
        return w;
    }

    d->fg = p_new(XColor, d->data_items);
    d->pfg_end = p_new(XColor *, d->data_items);
    d->pfg_center = p_new(XColor *, d->data_items);
    d->bg = p_new(XColor, d->data_items);
    d->bordercolor = p_new(XColor, d->data_items);
    d->percent = p_new(int, d->data_items);
    d->data_title = p_new(char *, d->data_items);

    for(i = 0; i < d->data_items; i++)
    {
        cfg = cfg_getnsec(config, "data", i);

        d->data_title[i] = a_strdup(cfg_title(cfg));

        if((color = cfg_getstr(cfg, "fg")))
            draw_color_new(globalconf.display, phys_screen, color, &d->fg[i]);
        else
            d->fg[i] = globalconf.screens[statusbar->screen].colors_normal[ColFG];

        if((color = cfg_getstr(cfg, "fg_center")))
        {
            d->pfg_center[i] = p_new(XColor, 1);
            draw_color_new(globalconf.display, phys_screen, color, d->pfg_center[i]);
        }

        if((color = cfg_getstr(cfg, "fg_end")))
        {
            d->pfg_end[i] = p_new(XColor, 1);
            draw_color_new(globalconf.display, phys_screen, color, d->pfg_end[i]);
        }

        if((color = cfg_getstr(cfg, "bg")))
            draw_color_new(globalconf.display, phys_screen, color, &d->bg[i]);
        else
            d->bg[i] = globalconf.screens[statusbar->screen].colors_normal[ColBG];

        if((color = cfg_getstr(cfg, "bordercolor")))
            draw_color_new(globalconf.display, phys_screen, color, &d->bordercolor[i]);
        else
            d->bordercolor[i] = d->fg[i];
    }

    /* complete setup, since someone might 'enable' it with increasing the width with widget_tell */
    if(d->width - d->padding < 3)
    {
        warn("Progressbar widget needs: (width - padding) >= 3\n");
        return w;
    }

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
