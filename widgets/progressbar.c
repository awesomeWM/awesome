/*
 * progressbar.c - progressbar widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007-2008 Marco Candrian <mac@calmar.ws>
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

#include "widgets/common.h"
#include "screen.h"
#include "common/util.h"
#include "common/configopts.h"


extern AwesomeConf globalconf;

typedef struct
{
    /** Percent 0 to 100 */
    int *percent;
    /** data_title of the data */
    char **data_title;
    /** Width of the data_items */
    int width;
    /** Pixel between data items (bars) */
    int gap;
    /**  border width in pixels */
    int border_width;
    /**  padding between border and ticks/bar */
    int border_padding;
    /**  gap/distance between the individual ticks */
    int ticks_gap;
    /**  total number of ticks */
    int ticks_count;
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
    /** Foreground color of turned-off ticks */
    XColor *fg_off;
    /** Foreground color when bar is half-full */
    XColor **pfg_center;
    /** Foreground color when bar is full */
    XColor **pfg_end;
    /** Background color */
    XColor *bg;
    /** Border color */
    XColor *bordercolor;
} Data;

static Bool
check_settings(Data *d, int status_height)
{
    int tmp, h_total;

    h_total = (int)(status_height * d->height + 0.5);

    if(!d->vertical) /* horizontal */
    {
        tmp = d->width - 2 * (d->border_width + d->border_padding) - 1;
        if((d->ticks_count && tmp - (d->ticks_count - 1) * d->ticks_gap - d->ticks_count + 1 < 0) ||
           (!d->ticks_count && tmp < 0))
        {
            warn("progressbar's 'width' is too small for the given configuration options\n");
            return False;
        }
        tmp = h_total - d->data_items * (2 * (d->border_width + d->border_padding) + 1) - (d->data_items - 1) * d->gap;
        if(tmp < 0)
        {
            warn("progressbar's 'height' is too small for the given configuration options\n");
            return False;
        }
    }
    else /* vertical / standing up */
    {
        tmp = h_total - 2 * (d->border_width + d->border_padding) - 1;
        if((d->ticks_count && tmp - (d->ticks_count - 1) * d->ticks_gap - d->ticks_count + 1 < 0) ||
           (!d->ticks_count && tmp < 0))
        {
            warn("progressbar's 'height' is too small for the given configuration options\n");
            return False;
        }
        tmp = d->width - d->data_items * (2 * (d->border_width + d->border_padding) + 1) - (d->data_items - 1) * d->gap;
        if(tmp < 0)
        {
            warn("progressbar's 'width' is too small for the given configuration options\n");
            return False;
        }
    }
    return True;
}


static int
progressbar_draw(Widget *widget, DrawCtx *ctx, int offset,
                 int used __attribute__ ((unused)))
{
    /* pb_.. values points to the widget inside a potential border */
    int i, percent_ticks, pb_x, pb_y, pb_height, pb_width, pb_progress, pb_offset;
    int unit = 0; /* tick + gap */
    int border_offset;
    int widget_width;
    area_t rectangle, pattern_rect;

    Data *d = widget->data;

    if (!d->data_items)
        return 0;

    /* for a 'reversed' progressbar:
     * basic progressbar:
     * 1. the full space gets the size of the formerly empty one
     * 2. the pattern must be mirrored
     * 3. the formerly 'empty' side gets drawed with fg colors, the 'full' with bg-color
     *
     * ticks:
     * 1. round the values to a full tick accordingly
     * 2. finally draw the gaps
     */

    pb_x = widget->area.x + d->border_width + d->border_padding;
    border_offset = d->border_width / 2;
    pb_offset = 0;

    if(d->vertical)
    {
        /* TODO: maybe prevent to calculate that stuff below over and over again (->use static-values) */
        pb_height = (int) (widget->statusbar->height * d->height + 0.5) -
                    2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            /* '+ d->ticks_gap' because a unit includes a ticks + ticks_gap */
            unit = (pb_height + d->ticks_gap) / d->ticks_count;
            pb_height = unit * d->ticks_count - d->ticks_gap;
        }

        pb_width = (int) ((d->width - 2 * (d->border_width + d->border_padding) * d->data_items -
                   d->gap * (d->data_items - 1)) / d->data_items);

        pb_y = widget->area.y + ((int) (widget->statusbar->height * (1 - d->height)) / 2) +
               d->border_width + d->border_padding;

        widget_width = d->data_items * (pb_width +
                       2 * (d->border_width + d->border_padding) + d->gap) - d->gap;

        for(i = 0; i < d->data_items; i++)
        {
            if(d->ticks_count && d->ticks_gap)
            {
                percent_ticks = (int)(d->ticks_count * (float)d->percent[i] / 100 + 0.5);
                if(percent_ticks)
                    pb_progress = percent_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
            }
            else
                pb_progress = (int)(pb_height * d->percent[i] / 100.0 + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x + pb_offset - border_offset - d->border_padding - 1;

                if(2 * border_offset == d->border_width)
                    rectangle.y = pb_y - border_offset - d->border_padding;
                else
                    rectangle.y = pb_y - border_offset - d->border_padding - 1;

                rectangle.width = pb_width + 2 * d->border_padding + d->border_width + 1;
                rectangle.height = pb_height + d->border_width + 2 * d->border_padding + 1;
                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1, True, d->bg[i]);
                draw_rectangle(ctx, rectangle, d->border_width, False, d->bordercolor[i]);
            }

            /* new value/progress in px + pattern setup */
            if(!d->reverse[i])
            {
                /* bottom to top */
                pattern_rect.x = pb_x;
                pattern_rect.y = pb_y + pb_height;
                pattern_rect.width =  0;
                pattern_rect.height = -pb_height;
            }
            else
            {
                /* invert: top with bottom part */
                pb_progress = pb_height - pb_progress;
                pattern_rect.x = pb_x;
                pattern_rect.y = pb_y;
                pattern_rect.width =  0;
                pattern_rect.height = pb_height;
            }

            /* bottom part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x + pb_offset;
                rectangle.y = pb_y + pb_height - pb_progress;
                rectangle.width = pb_width;
                rectangle.height = pb_progress;

                /* fg color */
                if(!d->reverse[i])
                    draw_rectangle_gradient(ctx, rectangle, 1.0, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
                else /*REV: bg */
                    draw_rectangle(ctx, rectangle, 1.0, True, d->fg_off[i]);
            }

            /* top part */
            if(pb_height - pb_progress > 0) /* not filled area */
            {
                rectangle.x = pb_x + pb_offset;
                rectangle.y = pb_y;
                rectangle.width = pb_width;
                rectangle.height = pb_height - pb_progress;

                /* bg color */
                if(!d->reverse[i])
                    draw_rectangle(ctx, rectangle, 1.0, True, d->fg_off[i]);
                else /* REV: fg */
                    draw_rectangle_gradient(ctx, rectangle, 1.0, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
            }
            /* draw gaps TODO: improve e.g all in one */
            if(d->ticks_count && d->ticks_gap)
            {
                rectangle.width = pb_width;
                rectangle.height = d->ticks_gap;
                rectangle.x = pb_x + pb_offset;
                for(rectangle.y = pb_y + (unit - d->ticks_gap);
                        pb_y + pb_height - d->ticks_gap >= rectangle.y;
                        rectangle.y += unit)
                {
                    draw_rectangle(ctx, rectangle, 1.0, True, d->bg[i]);
                }
            }
            pb_offset += pb_width + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }
    else /* a horizontal progressbar */
    {
        pb_width = d->width - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            unit = (pb_width + d->ticks_gap) / d->ticks_count;
            pb_width = unit * d->ticks_count - d->ticks_gap; /* rounded to match ticks... */
        }

        pb_height = (int) ((widget->statusbar->height * d->height -
                    d->data_items * 2 * (d->border_width + d->border_padding) -
                    (d->gap * (d->data_items - 1))) / d->data_items + 0.5);
        pb_y = widget->area.y + ((int) (widget->statusbar->height * (1 - d->height)) / 2) +
               d->border_width + d->border_padding;

        widget_width = pb_width + 2 * (d->border_width + d->border_padding);

        for(i = 0; i < d->data_items; i++)
        {
            if(d->ticks_count && d->ticks_gap)
            {
                /* +0.5 rounds up ticks -> turn on a tick when half of it is reached */
                percent_ticks = (int)(d->ticks_count * (float)d->percent[i] / 100 + 0.5);
                if(percent_ticks)
                    pb_progress = percent_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
                }
            else
                pb_progress = (int)(pb_width * d->percent[i] / 100.0 + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x - border_offset - d->border_padding - 1;

                if(2 * border_offset == d->border_width)
                    rectangle.y = pb_y + pb_offset - border_offset - d->border_padding;
                else
                    rectangle.y = pb_y + pb_offset - border_offset - d->border_padding - 1;

                rectangle.width = pb_width + 2 * d->border_padding + d->border_width + 1;
                rectangle.height = pb_height + d->border_width + 2 * d->border_padding + 1;
                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1, True, d->bg[i]);
                draw_rectangle(ctx, rectangle, d->border_width, False, d->bordercolor[i]);
            }
            /* new value/progress in px + pattern setup */
            if(!d->reverse[i])
            {
                /* left to right */
                pattern_rect.x = pb_x;
                pattern_rect.y = pb_y;
                pattern_rect.width =  pb_width;
                pattern_rect.height = 0;
            }
            else
            {
                /* reverse: right to left */
                pb_progress = pb_width - pb_progress;
                pattern_rect.x = pb_x + pb_width;
                pattern_rect.y = pb_y;
                pattern_rect.width =  -pb_width;
                pattern_rect.height = 0;
            }

            /* left part */
            if(pb_progress > 0)
            {
                rectangle.x = pb_x;
                rectangle.y = pb_y + pb_offset;
                rectangle.width = pb_progress;
                rectangle.height = pb_height;

                /* fg color */
                if(!d->reverse[i])
                    draw_rectangle_gradient(ctx, rectangle, 1.0, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
                else /* reverse: bg */
                    draw_rectangle(ctx, rectangle, 1.0, True, d->fg_off[i]);
            }

            /* right part */
            if(pb_width - pb_progress > 0)
            {
                rectangle.x = pb_x + pb_progress;
                rectangle.y = pb_y +  pb_offset;
                rectangle.width = pb_width - pb_progress;
                rectangle.height = pb_height;

                /* bg color */
                if(!d->reverse[i])
                    draw_rectangle(ctx, rectangle, 1.0, True, d->fg_off[i]);
                else /* reverse: fg */
                    draw_rectangle_gradient(ctx, rectangle, 1.0, True, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
            }
            /* draw gaps TODO: improve e.g all in one */
            if(d->ticks_count && d->ticks_gap)
            {
                rectangle.width = d->ticks_gap;
                rectangle.height = pb_height;
                rectangle.y = pb_y + pb_offset;
                for(rectangle.x = pb_x + (unit - d->ticks_gap);
                        pb_x + pb_width - d->ticks_gap >= rectangle.x;
                        rectangle.x += unit)
                    draw_rectangle(ctx, rectangle, 1.0, True, d->bg[i]);
            }

            pb_offset += pb_height + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget_width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    widget->area.width = widget_width;
    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static widget_tell_status_t
progressbar_tell(Widget *widget, char *property, char *command)
{
    Data *d = widget->data;
    int i = 0, percent, tmp;
    char *title, *setting;
    float tmpf;

    if(!d->data_items)
        return WIDGET_ERROR_CUSTOM; /* error already printed on _new */

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
        return widget_set_color_for_data(widget, d->fg, command, d->data_items, d->data_title);
    else if(!a_strcmp(property, "fg_off"))
        return widget_set_color_for_data(widget, d->fg_off, command, d->data_items, d->data_title);
    else if(!a_strcmp(property, "bg"))
        return widget_set_color_for_data(widget, d->bg, command, d->data_items, d->data_title);
    else if(!a_strcmp(property, "bordercolor"))
        return widget_set_color_for_data(widget, d->bordercolor, command, d->data_items, d->data_title);
    else if(!a_strcmp(property, "fg_center"))
        return widget_set_color_pointer_for_data(widget, d->pfg_center, command, d->data_items, d->data_title);
    else if(!a_strcmp(property, "fg_end"))
        return widget_set_color_pointer_for_data(widget, d->pfg_end, command, d->data_items, d->data_title);
    else if(!a_strcmp(property, "gap"))
        d->gap = atoi(command);
    else if(!a_strcmp(property, "width"))
    {
        tmp = d->width;
        d->width = atoi(command);
        if(!check_settings(d, widget->statusbar->height))
        {
            d->width = tmp; /* restore */
            return WIDGET_ERROR_CUSTOM;
        }
    }
    else if(!a_strcmp(property, "height"))
    {
        tmpf = d->height;
        d->height = atof(command);
        if(!check_settings(d, widget->statusbar->height))
        {
            d->height = tmpf; /* restore */
            return WIDGET_ERROR_CUSTOM;
        }
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
    int i;
    cfg_t *cfg;


    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    d = w->data = p_new(Data, 1);

    d->height = cfg_getfloat(config, "height");
    d->width = cfg_getint(config, "width");

    d->border_padding = cfg_getint(config, "border_padding");
    d->ticks_gap = cfg_getint(config, "ticks_gap");
    d->ticks_count = cfg_getint(config, "ticks_count");
    d->border_width = cfg_getint(config, "border_width");

    if(!(d->vertical = cfg_getbool(config, "vertical")))
        d->vertical = False;

    d->gap = cfg_getint(config, "gap");

    w->alignment = cfg_getalignment(config, "align");

    if(!(d->data_items = cfg_size(config, "data")))
    {
        warn("progressbar widget needs at least one bar section\n");
        return w;
    }

    if(!check_settings(d, statusbar->height))
    {
        d->data_items = 0;
        return w;
    }

    d->fg = p_new(XColor, d->data_items);
    d->pfg_end = p_new(XColor *, d->data_items);
    d->pfg_center = p_new(XColor *, d->data_items);
    d->fg_off = p_new(XColor, d->data_items);
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
            draw_color_new(globalconf.display, statusbar->phys_screen, color, &d->fg[i]);
        else
            d->fg[i] = globalconf.screens[statusbar->screen].styles.normal.fg;

        if((color = cfg_getstr(cfg, "fg_center")))
        {
            d->pfg_center[i] = p_new(XColor, 1);
            draw_color_new(globalconf.display, statusbar->phys_screen, color, d->pfg_center[i]);
        }

        if((color = cfg_getstr(cfg, "fg_end")))
        {
            d->pfg_end[i] = p_new(XColor, 1);
            draw_color_new(globalconf.display, statusbar->phys_screen, color, d->pfg_end[i]);
        }

        if((color = cfg_getstr(cfg, "fg_off")))
            draw_color_new(globalconf.display, statusbar->phys_screen, color, &d->fg_off[i]);
        else
            d->fg_off[i] = globalconf.screens[statusbar->screen].styles.normal.bg;

        if((color = cfg_getstr(cfg, "bg")))
            draw_color_new(globalconf.display, statusbar->phys_screen, color, &d->bg[i]);
        else
            d->bg[i] = globalconf.screens[statusbar->screen].styles.normal.bg;

        if((color = cfg_getstr(cfg, "bordercolor")))
            draw_color_new(globalconf.display, statusbar->phys_screen, color, &d->bordercolor[i]);
        else
            d->bordercolor[i] = d->fg[i];
    }
    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
