/*
 * tasklist.c - task list widget
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
#include "client.h"
#include "focus.h"
#include "screen.h"
#include "event.h"
#include "ewmh.h"
#include "tag.h"
#include "common/configopts.h"

extern AwesomeConf globalconf;

typedef enum
{
    ShowFocus,
    ShowTags,
    ShowAll,
} showclient_t;

typedef struct
{
    showclient_t show;
    bool show_icons;
    char *text_normal, *text_urgent, *text_focus;
} Data;

static inline bool
tasklist_isvisible(client_t *c, int screen, showclient_t show)
{
    if(c->skip || c->skiptb)
        return false;

    switch(show)
    {
      case ShowAll:
        return (c->screen == screen);
      case ShowTags:
        return client_isvisible(c, screen);
      case ShowFocus:
        return (c == focus_get_current_client(screen));
    }
    return false;
}

static int
tasklist_draw(widget_node_t *w, statusbar_t *statusbar, int offset, int used)
{
    client_t *c;
    Data *d = w->widget->data;
    area_t area;
    char *text;
    int n = 0, i = 0, box_width = 0, icon_width = 0, box_width_rest = 0;
    NetWMIcon *icon;

    if(used >= statusbar->width)
        return (w->area.width = 0);

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, statusbar->screen, d->show))
            n++;

    if(!n)
        return (w->area.width = 0);

    box_width = (statusbar->width - used) / n;
    /* compute how many pixel we left empty */
    box_width_rest = (statusbar->width - used) % n;

    w->area.x = widget_calculate_offset(statusbar->width,
                                        0, offset, w->widget->align);

    w->area.y = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, statusbar->screen, d->show))
        {
            icon_width = 0;

            if(globalconf.focus->client == c)
                text = d->text_focus;
            else if(c->isurgent)
                text = d->text_urgent;
            else
                text = d->text_normal;

            text = client_markup_parse(c, text, a_strlen(text));

            if(d->show_icons)
            {
                /* draw a background for icons */
                area.x = w->area.x + box_width * i;
                area.y = w->area.y;
                area.height = statusbar->height;
                area.width = box_width;

                draw_rectangle(statusbar->ctx, area, 1.0, true,
                               statusbar->colors.bg);

                if(c->icon_path)
                {
                    area = draw_get_image_size(c->icon_path);
                    if(area.width > 0 && area.height > 0)
                    {
                        icon_width = ((double) statusbar->height / (double) area.height) * area.width;
                        draw_image(statusbar->ctx,
                                   w->area.x + box_width * i,
                                   w->area.y,
                                   statusbar->height,
                                   c->icon_path);
                    }
                }

                if(!icon_width && (icon = ewmh_get_window_icon(c->win)))
                {
                    icon_width = ((double) statusbar->height / (double) icon->height)
                        * icon->width;
                    draw_image_from_argb_data(statusbar->ctx,
                                              w->area.x + box_width * i,
                                              w->area.y,
                                              icon->width, icon->height,
                                              statusbar->height, icon->image);
                    p_delete(&icon->image);
                    p_delete(&icon);
                }
            }

            area.x = w->area.x + icon_width + box_width * i;
            area.y = w->area.y;
            area.width = box_width - icon_width;
            area.height = statusbar->height;

            /* if we're on last elem, it has the last pixels left */
            if(i == n - 1)
                area.width += box_width_rest;

            draw_text(statusbar->ctx, globalconf.font,
                      &statusbar->colors.fg,
                      area, text);

            p_delete(&text);

            if(c->isfloating)
                draw_circle(statusbar->ctx, w->area.x + icon_width + box_width * i,
                            w->area.y,
                            (globalconf.font->height + 2) / 4,
                            false, statusbar->colors.fg);
            i++;
        }

    w->area.width = statusbar->width - used;
    w->area.height = statusbar->height;

    return w->area.width;
}

static void
tasklist_button_press(widget_node_t *w, statusbar_t *statusbar,
                      xcb_button_press_event_t *ev)
{
    button_t *b;
    client_t *c, **lc;
    Data *d = w->widget->data;
    int n = 0, box_width = 0, i, ci = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, statusbar->screen, d->show))
            n++;

    if(!n)
        return;

    box_width = w->area.width / n;

    switch(statusbar->position)
    {
      case Top:
      case Bottom:
        ci = (ev->event_x - w->area.x) / box_width;
        break;
      case Right:
        ci = (ev->event_y - w->area.x) / box_width;
        break;
      default:
         ci = ((statusbar->width - ev->event_y) - w->area.x) / box_width;
         break;
    }
    /* found first visible client */
    for(c = globalconf.clients;
        c && !tasklist_isvisible(c, statusbar->screen, d->show);
        c = c->next);
    /* found ci-th visible client */
    for(i = 0; c ; c = c->next)
        if(tasklist_isvisible(c, statusbar->screen, d->show))
            if(i++ >= ci)
                break;

    if(c)
        for(b = w->widget->buttons; b; b = b->next)
            if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
            {
                luaA_client_userdata_new(c);
                luaA_dofunction(globalconf.L, b->fct, 1);
            }
}

static widget_tell_status_t
tasklist_tell(widget_t *widget, const char *property, const char *new_value)
{
    Data *d = widget->data;

    if(!a_strcmp(property, "text_normal"))
    {
        p_delete(&d->text_normal);
        d->text_normal = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "text_focus"))
    {
        p_delete(&d->text_focus);
        d->text_focus = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "text_urgent"))
    {
        p_delete(&d->text_urgent);
        d->text_urgent = a_strdup(new_value);
    }
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

widget_t *
tasklist_new(alignment_t align __attribute__ ((unused)))
{
    widget_t *w;
    Data *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->draw = tasklist_draw;
    w->button_press = tasklist_button_press;
    w->align = AlignFlex;
    w->data = d = p_new(Data, 1);
    w->tell = tasklist_tell;

    d->text_normal = a_strdup(" <title/> ");
    d->text_focus = a_strdup(" <title/> ");
    d->text_urgent = a_strdup(" <title/> ");
    d->show_icons = true;
    d->show = ShowTags;

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
