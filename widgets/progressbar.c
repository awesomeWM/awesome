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

#include "widget.h"
#include "screen.h"

extern awesome_t globalconf;

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
    bool *reverse;
    /** 90 Degree's turned */
    bool vertical;
    /** Number of data_items (bars) */
    int data_items;
    /** Height 0-1, where 1.0 is height of statusbar */
    float height;
    /** Foreground color */
    xcolor_t *fg;
    /** Foreground color of turned-off ticks */
    xcolor_t *fg_off;
    /** Foreground color when bar is half-full */
    xcolor_t **pfg_center;
    /** Foreground color when bar is full */
    xcolor_t **pfg_end;
    /** Background color */
    xcolor_t *bg;
    /** Border color */
    xcolor_t *bordercolor;
} Data;

void add_data(Data *, const char *);
static void set_new_p_color(xcolor_t **, char *);
/*static bool check_settings(Data *, int);*/

static void
set_new_p_color(xcolor_t **ppcolor, char *new_color)
{
    bool flag = false;
    if(!*ppcolor)
    {
        flag = true; /* p_delete && restore to NULL, if draw_color_new unsuccessful */
        *ppcolor = p_new(xcolor_t, 1);
    }
    if(!(draw_color_new(globalconf.connection,
                    globalconf.default_screen,
                    new_color, *ppcolor)))
    {
        if(flag) /* restore */
        {
            p_delete(ppcolor);
            *ppcolor = NULL;
        }
    }
}

/*static bool*/
/*check_settings(Data *d, int status_height)*/
/*{*/
    /*int tmp, h_total;*/

    /*h_total = (int)(status_height * d->height + 0.5);*/

    /*if(!d->vertical) [> horizontal <]*/
    /*{*/
        /*tmp = d->width - 2 * (d->border_width + d->border_padding) - 1;*/
        /*if((d->ticks_count && tmp - (d->ticks_count - 1) * d->ticks_gap - d->ticks_count + 1 < 0)*/
           /*|| (!d->ticks_count && tmp < 0))*/
        /*{*/
            /*warn("progressbar's 'width' is too small for the given configuration options");*/
            /*return false;*/
        /*}*/
        /*tmp = h_total - d->data_items * (2 * (d->border_width + d->border_padding) + 1)*/
              /*- (d->data_items - 1) * d->gap;*/
        /*if(tmp < 0)*/
        /*{*/
            /*warn("progressbar's 'height' is too small for the given configuration options");*/
            /*return false;*/
        /*}*/
    /*}*/
    /*else [> vertical / standing up <]*/
    /*{*/
        /*tmp = h_total - 2 * (d->border_width + d->border_padding) - 1;*/
        /*if((d->ticks_count && tmp - (d->ticks_count - 1) * d->ticks_gap - d->ticks_count + 1 < 0)*/
           /*|| (!d->ticks_count && tmp < 0))*/
        /*{*/
            /*warn("progressbar's 'height' is too small for the given configuration options");*/
            /*return false;*/
        /*}*/
        /*tmp = d->width - d->data_items * (2 * (d->border_width + d->border_padding) + 1)*/
              /*- (d->data_items - 1) * d->gap;*/
        /*if(tmp < 0)*/
        /*{*/
            /*warn("progressbar's 'width' is too small for the given configuration options");*/
            /*return false;*/
        /*}*/
    /*}*/
    /*return true;*/
/*}*/

void add_data(Data *d, const char *new_data_title)
{
    d->data_items++;

    /* memory (re-)allocating */
    p_realloc(&(d->data_title), d->data_items);
    p_realloc(&(d->percent), d->data_items);
    p_realloc(&(d->reverse), d->data_items);
    p_realloc(&(d->fg), d->data_items);
    p_realloc(&(d->fg_off), d->data_items);
    p_realloc(&(d->bg), d->data_items);
    p_realloc(&(d->bordercolor), d->data_items);
    p_realloc(&(d->pfg_center), d->data_items);
    p_realloc(&(d->pfg_end), d->data_items);

    /* initialize values for new data section */
    d->reverse[d->data_items - 1] = false;
    d->data_title[d->data_items - 1] = a_strdup(new_data_title);
    d->percent[d->data_items - 1] = 0;

    d->fg[d->data_items - 1] = globalconf.colors.fg;
    d->fg_off[d->data_items - 1] = globalconf.colors.bg;
    d->bg[d->data_items - 1] = globalconf.colors.bg;
    d->bordercolor[d->data_items - 1] = globalconf.colors.fg;
    d->pfg_center[d->data_items - 1] = NULL;
    d->pfg_end[d->data_items - 1] = NULL;

}

static int
progressbar_draw(widget_node_t *w, statusbar_t *statusbar, int offset,
                 int used __attribute__ ((unused)))
{
    /* pb_.. values points to the widget inside a potential border */
    int i, percent_ticks, pb_x, pb_y, pb_height, pb_width, pb_progress, pb_offset;
    int unit = 0; /* tick + gap */
    area_t rectangle, pattern_rect;
    draw_context_t *ctx = statusbar->ctx;
    Data *d = w->widget->data;

    if(!d->data_items)
        return 0;

    if(d->vertical)
    {
        pb_width = (int) ((d->width - 2 * (d->border_width + d->border_padding) * d->data_items
                   - d->gap * (d->data_items - 1)) / d->data_items);
        w->area.width = d->data_items
                        * (pb_width + 2 * (d->border_width + d->border_padding)
                        + d->gap) - d->gap;
    }
    else
    {
        pb_width = d->width - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            unit = (pb_width + d->ticks_gap) / d->ticks_count;
            pb_width = unit * d->ticks_count - d->ticks_gap; /* rounded to match ticks... */
        }
        w->area.width = pb_width + 2 * (d->border_width + d->border_padding);
    }

    w->area.x = widget_calculate_offset(statusbar->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

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

    pb_x = w->area.x + d->border_width + d->border_padding;
    pb_offset = 0;

    if(d->vertical)
    {
        /* TODO: maybe prevent to calculate that stuff below over and over again
         * (->use static-values) */
        pb_height = (int) (statusbar->height * d->height + 0.5)
                    - 2 * (d->border_width + d->border_padding);
        if(d->ticks_count && d->ticks_gap)
        {
            /* '+ d->ticks_gap' because a unit includes a ticks + ticks_gap */
            unit = (pb_height + d->ticks_gap) / d->ticks_count;
            pb_height = unit * d->ticks_count - d->ticks_gap;
        }

        pb_y = w->area.y + ((int) (statusbar->height * (1 - d->height)) / 2)
               + d->border_width + d->border_padding;

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
                rectangle.x = pb_x + pb_offset - d->border_width - d->border_padding;
                rectangle.y = pb_y - d->border_width - d->border_padding;
                rectangle.width = pb_width + 2 * (d->border_padding + d->border_width);
                rectangle.height = pb_height + 2 * (d->border_padding + d->border_width);

                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1.0, true, d->bg[i]);
                draw_rectangle(ctx, rectangle, d->border_width, false, d->bordercolor[i]);
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
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
                else /*REV: bg */
                    draw_rectangle(ctx, rectangle, 1.0, true, d->fg_off[i]);
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
                    draw_rectangle(ctx, rectangle, 1.0, true, d->fg_off[i]);
                else /* REV: fg */
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
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
                    draw_rectangle(ctx, rectangle, 1.0, true, d->bg[i]);
                }
            }
            pb_offset += pb_width + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }
    else /* a horizontal progressbar */
    {
        pb_height = (int) ((statusbar->height * d->height
                    - d->data_items * 2 * (d->border_width + d->border_padding)
                    - (d->gap * (d->data_items - 1))) / d->data_items + 0.5);
        pb_y = w->area.y + ((int) (statusbar->height * (1 - d->height)) / 2)
               + d->border_width + d->border_padding;

        for(i = 0; i < d->data_items; i++)
        {
            if(d->ticks_count && d->ticks_gap)
            {
                /* +0.5 rounds up ticks -> turn on a tick when half of it is reached */
                percent_ticks = (int) (d->ticks_count * (float) d->percent[i] / 100 + 0.5);
                if(percent_ticks)
                    pb_progress = percent_ticks * unit - d->ticks_gap;
                else
                    pb_progress = 0;
            }
            else
                pb_progress = (int) (pb_width * d->percent[i] / 100.0 + 0.5);

            if(d->border_width)
            {
                /* border rectangle */
                rectangle.x = pb_x - d->border_width - d->border_padding;
                rectangle.y = pb_y + pb_offset - d->border_width - d->border_padding;
                rectangle.width = pb_width + 2 * (d->border_padding + d->border_width);
                rectangle.height = pb_height + 2 * (d->border_padding + d->border_width);

                if(d->border_padding)
                    draw_rectangle(ctx, rectangle, 1.0, true, d->bg[i]);
                draw_rectangle(ctx, rectangle, d->border_width, false, d->bordercolor[i]);
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
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
                                            &(d->fg[i]), d->pfg_center[i], d->pfg_end[i]);
                else /* reverse: bg */
                    draw_rectangle(ctx, rectangle, 1.0, true, d->fg_off[i]);
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
                    draw_rectangle(ctx, rectangle, 1.0, true, d->fg_off[i]);
                else /* reverse: fg */
                    draw_rectangle_gradient(ctx, rectangle, 1.0, true, pattern_rect,
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
                    draw_rectangle(ctx, rectangle, 1.0, true, d->bg[i]);
            }

            pb_offset += pb_height + d->gap + 2 * (d->border_width + d->border_padding);
        }
    }

    w->area.height = statusbar->height;
    return w->area.width;
}

static widget_tell_status_t
progressbar_tell(widget_t *widget, const char *property, const char *new_value)
{
    Data *d = widget->data;
    int i = 0, percent, tmp;
    char *title, *setting;
    char *new_val;
    float ftmp;
    bool btmp, found;

    if(new_value == NULL)
        return WIDGET_ERROR_NOVALUE;
    /* seperate for saving some cpu cycles (could be put into next else if...) */
    else if(!a_strcmp(property, "data"))
    {
        new_val = a_strdup(new_value);
        title = strtok(new_val, " ");
        if(!(setting = strtok(NULL, " ")))
        {
            p_delete(&new_val);
            return WIDGET_ERROR_NOVALUE;
        }
        for(i = 0; i < d->data_items; i++)
            if(!a_strcmp(title, d->data_title[i]))
            {
                percent = atoi(setting);
                d->percent[i] = (percent < 0 ? 0 : (percent > 100 ? 100 : percent));
                p_delete(&new_val);
                return WIDGET_NOERROR;
            }
        /* no section found -> create one */
        add_data(d, title);
        percent = atoi(setting);
        d->percent[d->data_items - 1] = (percent < 0 ? 0 : (percent > 100 ? 100 : percent));
        p_delete(&new_val);
        return WIDGET_NOERROR;
    }
    /* following properties need a datasection */
    else if(!a_strcmp(property, "fg")
            || !a_strcmp(property, "fg_off")
            || !a_strcmp(property, "bg")
            || !a_strcmp(property, "bordercolor")
            || !a_strcmp(property, "fg_center")
            || !a_strcmp(property, "fg_end")
            || !a_strcmp(property, "reverse"))
    {
        /* check if this section is defined alrady */
        new_val = a_strdup(new_value);
        title = strtok(new_val, " ");
        if(!(setting = strtok(NULL, " ")))
        {
            p_delete(&new_val);
            return WIDGET_ERROR_NOVALUE;
        }
        for(found = false, i = 0; i < d->data_items; i++)
        {
            if(!a_strcmp(title, d->data_title[i]))
            {
                found = true;
                break;
            }
        }
        /* no section found -> create one */
        if(!found)
            add_data(d, title);

        /* change values accordingly... */
        if(!a_strcmp(property, "fg"))
            draw_color_new(globalconf.connection, globalconf.default_screen, setting, &(d->fg[i]));
        else if(!a_strcmp(property, "bg"))
            draw_color_new(globalconf.connection, globalconf.default_screen, setting, &(d->bg[i]));
        else if(!a_strcmp(property, "fg_off"))
            draw_color_new(globalconf.connection, globalconf.default_screen, setting, &(d->fg_off[i]));
        else if(!a_strcmp(property, "bordercolor"))
            draw_color_new(globalconf.connection, globalconf.default_screen, setting, &(d->bordercolor[i]));
        else if(!a_strcmp(property, "fg_center"))
            set_new_p_color(&(d->pfg_center[i]), setting);
        else if(!a_strcmp(property, "fg_end"))
            set_new_p_color(&(d->pfg_end[i]), setting);
        else if(!a_strcmp(property, "reverse"))
            d->reverse[i] = a_strtobool(setting);

        p_delete(&new_val);
        return WIDGET_NOERROR;
    }
    else if(!a_strcmp(property, "gap"))
    {
        d->gap = atoi(new_value);
    }
    else if(!a_strcmp(property, "ticks_count"))
    {
        tmp = d->ticks_count;
        d->ticks_count = atoi(new_value);
    }
    else if(!a_strcmp(property, "ticks_gap"))
    {
        tmp = d->ticks_gap;
        d->ticks_gap = atoi(new_value);
    }
    else if(!a_strcmp(property, "border_padding"))
    {
        tmp = d->border_padding;
        d->border_padding = atoi(new_value);
    }
    else if(!a_strcmp(property, "border_width"))
    {
        tmp = d->border_width;
        d->border_width = atoi(new_value);
    }
    else if(!a_strcmp(property, "width"))
    {
        tmp = d->width;
        d->width = atoi(new_value);
    }
    else if(!a_strcmp(property, "height"))
    {
        ftmp = d->height;
        d->height = atof(new_value);
    }
    else if(!a_strcmp(property, "vertical"))
    {
        btmp = d->vertical;
        d->vertical = a_strtobool(new_value);
        /*TODO:*/
        /*if(!check_settings(d, widget->statusbar->height))*/
        /*{*/
            /*d->width = tmp; [> restore <]*/
            /*return WIDGET_ERROR_CUSTOM;*/
        /*}*/
    }
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

widget_t *
progressbar_new(alignment_t align)
{
    widget_t *w;
    Data *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = progressbar_draw;
    w->tell = progressbar_tell;
    d = w->data = p_new(Data, 1);

    d->data_items = 0;
    d->data_title = NULL;
    d->fg = d->bg = d->fg_off = d->bordercolor = NULL;
    d->pfg_center =  d->pfg_end = NULL;
    d->percent = NULL;
    d->reverse = NULL;

    d->height = 0.80;
    d->width = 80;

    d->ticks_gap = 1;
    d->border_width = 1;
    d->border_padding = 0;
    d->gap = 2;

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
