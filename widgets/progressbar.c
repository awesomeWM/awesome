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
    /** reverse drawing */
    Bool *reverse;
    /** 90 Degree's turned */
    Bool vertical;
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
    int i, pb_x, pb_y, pb_height, pb_width, pb_progress, pb_offset = 0;
    Area rectangle, pattern_rect;

    Data *d = widget->data;

    if (!d->data_items)
        return 0;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 d->width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    /* TODO: maybe prevent (use Data-values) to calculate that stuff below over and over again */
    /* data for the first rectangle (data-bar) to draw */
    if(d->vertical)
    {
        pb_width = (int) ((d->width - d->padding - (d->gap * (d->data_items - 1))) / d->data_items + 0.5);
        pb_height = (int) (widget->statusbar->height * d->height + 0.5);
        pb_y = widget->area.y + (int)((widget->statusbar->height - pb_height) / 2 + 0.5);
    }
    else /* horizontal */
    {
        pb_width = d->width - d->padding;
        pb_height = (int) ((widget->statusbar->height * d->height - (d->gap * (d->data_items - 1))) / d->data_items + 0.5);
        pb_y = widget->area.y + (int)((widget->statusbar->height - widget->statusbar->height * d->height) / 2 + 0.5);
    }

    pb_x = widget->area.x + d->padding;

    /* for a 'reversed' progressbar:
     * 1. the full space gets the size of the formerly empty one
     * 2. the pattern must be mirrored
     * 3. the formerly 'empty' side gets drawed with fg colors, the 'full' with bg-color
     *
     * Might could be put into a single function (removing same/similar code) - feel free
     */
    if(d->vertical)
    {
        for(i = 0; i < d->data_items; i++)
        {
            /* border rectangle */
            rectangle.x = pb_x + pb_offset;
            rectangle.y = pb_y;
            rectangle.width = pb_width;
            rectangle.height = pb_height;
            draw_rectangle(ctx, rectangle, False, d->bordercolor[i]);

            /* new value/progress in px + pattern setup */
            if(!d->reverse[i])
            {
                pb_progress = (int)(((pb_height  - 2) * d->percent[i]) / 100 + 0.5);
                /* bottom to top */
                pattern_rect.x = pb_x;
                pattern_rect.y = pb_y + 1 + pb_height ;
                pattern_rect.width =  0;
                pattern_rect.height = -pb_height;
            }
            else
            {
                pb_progress = (int)(((pb_height - 2) * (100 - d->percent[i])) / 100 + 0.5);
                /* top to bottom */
                pattern_rect.x = pb_x + 1;
                pattern_rect.y = pb_y + 1;
                pattern_rect.width =  0;
                pattern_rect.height = pb_height;
            }

            /* bottom part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x + pb_offset + 1;
                rectangle.y = pb_y + 1 + (pb_height - 2) - pb_progress;
                rectangle.width = pb_width - 2;
                rectangle.height = pb_progress;

                /* fg color */
                if(!d->reverse[i])
                    draw_rectangle_gradient(ctx, rectangle, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
                else /*REV: bg */
                    draw_rectangle(ctx, rectangle, True, d->bg[i]);
            }

            /* top part */
            if((pb_height - 2) - pb_progress > 0) /* not filled area */
            {
                rectangle.x = pb_x + 1 + pb_offset;
                rectangle.y = pb_y + 1;
                rectangle.width = pb_width - 2;
                rectangle.height = pb_height - 2 - pb_progress;

                /* bg color */
                if(!d->reverse[i])
                    draw_rectangle(ctx, rectangle, True, d->bg[i]);
                else /* REV: bg */
                    draw_rectangle_gradient(ctx, rectangle, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
            }
            pb_offset += pb_width + d->gap;
        }
    }
    else /* a horizontal progressbar */
    {
        for(i = 0; i < d->data_items; i++)
        {
            /* border rectangle */
            rectangle.x = pb_x;
            rectangle.y = pb_y + pb_offset;
            rectangle.width = pb_width;
            rectangle.height = pb_height;
            draw_rectangle(ctx, rectangle, False, d->bordercolor[i]);

            /* new value/progress in px + pattern setup */
            if(!d->reverse[i])
            {
                pb_progress = (int)(((pb_width - 2) * d->percent[i]) / 100 + 0.5);
                /* left to right */
                pattern_rect.x = pb_x + 1;
                pattern_rect.y = pb_y + 1;
                pattern_rect.width =  pb_width;
                pattern_rect.height = 0;
            }
            else
            {
                pb_progress = (int)(((pb_width - 2) * (100 - d->percent[i])) / 100 + 0.5);
                /* REV: right to left */
                pattern_rect.x = pb_x + 1 + pb_width;
                pattern_rect.y = pb_y + 1;
                pattern_rect.width =  -pb_width;
                pattern_rect.height = 0;
            }

            /* left part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x + 1;
                rectangle.y = pb_y + 1 + pb_offset;
                rectangle.width = pb_progress;
                rectangle.height = pb_height - 2;

                /* fg color */
                if(!d->reverse[i])
                    draw_rectangle_gradient(ctx, rectangle, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
                else /* REV: bg */
                    draw_rectangle(ctx, rectangle, True, d->bg[i]);
            }

            /* right part */
            if(pb_width - 2 - pb_progress > 0)
            {
                rectangle.x = pb_x + 1 + pb_progress;
                rectangle.y = pb_y + 1 +  pb_offset;
                rectangle.width = pb_width - 2 - pb_progress;
                rectangle.height = pb_height - 2;

                /* bg color */
                if(!d->reverse[i])
                    draw_rectangle(ctx, rectangle, True, d->bg[i]);
                else /* REV: fg */
                    draw_rectangle_gradient(ctx, rectangle, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
            }
            pb_offset += pb_height + d->gap;
        }
    }

    widget->area.width = d->width;
    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static widget_tell_status_t
progressbar_tell(Widget *widget, char *property, char *command)
{
    Data *d = widget->data;
    int i = 0, percent, tmp;
    Bool flag;
    char *title, *setting;

    if(!d->data_items)
        return WIDGET_ERROR_CUSTOM; /* error already printed on _new */

    if(!command)
        return WIDGET_ERROR_NOVALUE;

    if(!a_strcmp(property, "data"))
    {
        title = strtok(command, " ");
        if(!(setting = strtok(NULL, " ")))
            return WIDGET_ERROR_NOVALUE;
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                percent = atoi(setting);
                d->percent[i] = (percent < 0 ? 0 : (percent > 100 ? 100 : percent));
                return WIDGET_NOERROR;
            }
        return WIDGET_ERROR_FORMAT_SECTION;
    }
    else if(!a_strcmp(property, "fg"))
    {
        title = strtok(command, " ");
        if(!(setting = strtok(NULL, " ")))
            return WIDGET_ERROR_NOVALUE;
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                if(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                                  setting, &d->fg[i]))
                    return WIDGET_NOERROR;
                else
                    return WIDGET_ERROR_FORMAT_COLOR;
            }
        return WIDGET_ERROR_FORMAT_SECTION;
    }
    else if(!a_strcmp(property, "bg"))
    {
        title = strtok(command, " ");
        if(!(setting = strtok(NULL, " ")))
            return WIDGET_ERROR_NOVALUE;
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                if(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                                  setting, &d->bg[i]))
                    return WIDGET_NOERROR;
                else
                    return WIDGET_ERROR_FORMAT_COLOR;
            }
        return WIDGET_ERROR_FORMAT_SECTION;
    }
    else if(!a_strcmp(property, "fg_center"))
    {
        title = strtok(command, " ");
        if(!(setting = strtok(NULL, " ")))
            return WIDGET_ERROR_NOVALUE;
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                flag = False;
                if(!d->pfg_center[i])
                {
                    flag = True; /* p_delete && restore to NULL, if draw_color_new unsuccessful */
                    d->pfg_center[i] = p_new(XColor, 1);
                }
                if(!(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                                    setting, d->pfg_center[i])))
                {
                    if(flag) /* restore */
                    {
                        p_delete(&d->pfg_center[i]);
                        d->pfg_center[i] = NULL;
                    }
                    return WIDGET_ERROR_FORMAT_COLOR;
                }
                return WIDGET_NOERROR;
            }
        return WIDGET_ERROR_FORMAT_SECTION;
    }
    else if(!a_strcmp(property, "fg_end"))
    {
        title = strtok(command, " ");
        if(!(setting = strtok(NULL, " ")))
            return WIDGET_ERROR_NOVALUE;
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                flag = False;
                if(!d->pfg_end[i])
                {
                    flag = True;
                    d->pfg_end[i] = p_new(XColor, 1);
                }
                if(!(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                                    setting, d->pfg_end[i])))
                {
                    if(flag) /* restore */
                    {
                        p_delete(&d->pfg_end[i]);
                        d->pfg_end[i] = NULL;
                    }
                    return WIDGET_ERROR_FORMAT_COLOR;
                }
                return WIDGET_NOERROR;
            }
    }
    else if(!a_strcmp(property, "bordercolor"))
    {
        title = strtok(command, " ");
        if(!(setting = strtok(NULL, " ")))
            return WIDGET_ERROR_NOVALUE;
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                if(draw_color_new(globalconf.display, get_phys_screen(widget->statusbar->screen),
                                  setting, &d->bordercolor[i]))
                    return WIDGET_NOERROR;
                else
                    return WIDGET_ERROR_FORMAT_COLOR;
            }
        return WIDGET_ERROR_FORMAT_SECTION;
    }
    else if(!a_strcmp(property, "gap"))
        d->gap = atoi(command);
    else if(!a_strcmp(property, "width"))
    {
        tmp = atoi(command);
        if(tmp - d->padding < 3)
        {
            warn("progressbar widget needs: (width - padding) >= 3. Ignoring command.\n");
            return WIDGET_ERROR_CUSTOM;
        }
        else
            d->width = tmp;
    }
    else if(!a_strcmp(property, "height"))
        d->height = atof(command);
    else if(!a_strcmp(property, "padding"))
    {
        tmp = atoi(command);
        if(d->width - tmp < 3)
        {
            warn("progressbar widget needs: (width - padding) >= 3. Ignoring command.\n");
            return WIDGET_ERROR_CUSTOM;
        }
        else
            d->padding = tmp;
    }
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
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
    d->padding = cfg_getint(config, "padding");

    if(d->width - d->padding < 3)
    {
        warn("progressbar widget needs: (width - padding) >= 3\n");
        return w;
    }

    d->height = cfg_getfloat(config, "height");
    d->gap = cfg_getint(config, "gap");
    d->padding = cfg_getint(config, "padding");
    if(!(d->vertical = cfg_getbool(config, "vertical")))
        d->vertical = False;

    w->alignment = draw_get_align(cfg_getstr(config, "align"));

    if(!(d->data_items = cfg_size(config, "data")))
    {
        warn("progressbar widget needs at least one bar section\n");
        return w;
    }

    d->fg = p_new(XColor, d->data_items);
    d->pfg_end = p_new(XColor *, d->data_items);
    d->pfg_center = p_new(XColor *, d->data_items);
    d->bg = p_new(XColor, d->data_items);
    d->bordercolor = p_new(XColor, d->data_items);
    d->percent = p_new(int, d->data_items);
    d->reverse = p_new(Bool, d->data_items);
    d->data_title = p_new(char *, d->data_items);

    for(i = 0; i < d->data_items; i++)
    {
        cfg = cfg_getnsec(config, "data", i);

        d->data_title[i] = a_strdup(cfg_title(cfg));

        if(!(d->reverse[i] = cfg_getbool(cfg, "reverse")))
            d->reverse[i] = False;

        if((color = cfg_getstr(cfg, "fg")))
            draw_color_new(globalconf.display, phys_screen, color, &d->fg[i]);
        else
            d->fg[i] = globalconf.screens[statusbar->screen].colors.normal.fg;

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
            d->bg[i] = globalconf.screens[statusbar->screen].colors.normal.fg;

        if((color = cfg_getstr(cfg, "bordercolor")))
            draw_color_new(globalconf.display, phys_screen, color, &d->bordercolor[i]);
        else
            d->bordercolor[i] = d->fg[i];
    }

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
