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
#include "rules.h"
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
tasklist_draw(widget_t *widget, draw_context_t *ctx, int offset, int used)
{
    client_t *c;
    Data *d = widget->data;
    rule_t *r;
    area_t area;
    char *text;
    int n = 0, i = 0, box_width = 0, icon_width = 0, box_width_rest = 0;
    NetWMIcon *icon;
    style_t *style;

    if(used >= widget->statusbar->width)
        return (widget->area.width = 0);

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, widget->statusbar->screen, d->show))
            n++;

    if(!n)
        return (widget->area.width = 0);

    box_width = (widget->statusbar->width - used) / n;
    /* compute how many pixel we left empty */
    box_width_rest = (widget->statusbar->width - used) % n;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 0,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = widget->area.y = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, widget->statusbar->screen, d->show))
        {
            icon_width = 0;

            style = client_style_get(c);

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
                area.x = widget->area.x + box_width * i;
                area.y = widget->area.y;
                area.height = widget->statusbar->height;
                area.width = box_width;

                draw_rectangle(ctx, area, 1.0, true, style->bg);

                if((r = rule_matching_client(c)) && r->icon)
                {
                    area = draw_get_image_size(r->icon);
                    if(area.width > 0 && area.height > 0)
                    {
                        icon_width = ((double) widget->statusbar->height / (double) area.height) * area.width;
                        draw_image(ctx,
                                   widget->area.x + box_width * i,
                                   widget->area.y,
                                   widget->statusbar->height,
                                   r->icon);
                    }
                }

                if(!icon_width && (icon = ewmh_get_window_icon(c->win)))
                {
                    icon_width = ((double) widget->statusbar->height / (double) icon->height)
                        * icon->width;
                    draw_image_from_argb_data(ctx,
                                              widget->area.x + box_width * i,
                                              widget->area.y,
                                              icon->width, icon->height,
                                              widget->statusbar->height, icon->image);
                    p_delete(&icon->image);
                    p_delete(&icon);
                }
            }

            area.x = widget->area.x + icon_width + box_width * i;
            area.y = widget->area.y;
            area.width = box_width - icon_width;
            area.height = widget->statusbar->height;

            /* if we're on last elem, it has the last pixels left */
            if(i == n - 1)
                area.width += box_width_rest;

            draw_text(ctx, area, text, style);

            p_delete(&text);

            if(c == globalconf.scratch.client)
            {
                area.x = widget->area.x + icon_width + box_width * i;
                area.y = widget->area.y;
                area.width = (style->font->height + 2) / 3;
                area.height = (style->font->height + 2) / 3;
                draw_rectangle(ctx, area, 1.0, c->isfloating, style->fg);
            }
            else if(c->isfloating || c->ismax)
                draw_circle(ctx, widget->area.x + icon_width + box_width * i,
                            widget->area.y,
                            (style->font->height + 2) / 4,
                            c->ismax, style->fg);
            i++;
        }

    widget->area.width = widget->statusbar->width - used;
    widget->area.height = widget->statusbar->height;

    return widget->area.width;
}

static void
tasklist_button_press(widget_t *widget, xcb_button_press_event_t *ev)
{
    Button *b;
    client_t *c;
    Data *d = widget->data;
    tag_t *tag;
    int n = 0, box_width = 0, i, ci = 0;

    /* button1 give focus */
    if(ev->detail == XCB_BUTTON_INDEX_1 && CLEANMASK(ev->state) == XCB_NO_SYMBOL)
    {
        for(c = globalconf.clients; c; c = c->next)
            if(tasklist_isvisible(c, widget->statusbar->screen, d->show))
                n++;

        if(!n)
            return;

        box_width = widget->area.width / n;

        if(ev->detail == XCB_BUTTON_INDEX_1 && CLEANMASK(ev->state) == XCB_NO_SYMBOL)
        {
            switch(widget->statusbar->position)
            {
              case Top:
              case Bottom:
                ci = (ev->event_x - widget->area.x) / box_width;
                break;
              case Right:
                ci = (ev->event_y - widget->area.x) / box_width;
                break;
              default:
                ci = ((widget->statusbar->width - ev->event_y) - widget->area.x) / box_width;
                break;
            }
            /* found first visible client */
            for(c = globalconf.clients;
                c && !tasklist_isvisible(c, widget->statusbar->screen, d->show);
                c = c->next);
            /* found ci-th visible client */
            for(i = 0; c ; c = c->next)
                if(tasklist_isvisible(c, widget->statusbar->screen, d->show))
                    if(i++ >= ci)
                        break;

            if(c)
            {
                /* first switch tag if client not visible */
                if(!client_isvisible(c, widget->statusbar->screen))
                    for(i = 0, tag = globalconf.screens[c->screen].tags; tag; tag = tag->next, i++)
                        if(is_client_tagged(c, tag))
                           tag_view_only_byindex(c->screen, i);
                client_focus(c, widget->statusbar->screen, true);
            }

            return;
        }
    }

    for(b = widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->func)
            b->func(widget->statusbar->screen, b->arg);
}

widget_t *
tasklist_new(statusbar_t *statusbar, cfg_t *config)
{
    widget_t *w;
    Data *d;
    char *buf;

    w = p_new(widget_t, 1);
    widget_common_new(w, statusbar, config);
    w->draw = tasklist_draw;
    w->button_press = tasklist_button_press;
    w->alignment = AlignFlex;
    w->data = d = p_new(Data, 1);

    d->text_normal = a_strdup(cfg_getstr(config, "text_normal"));
    d->text_focus = a_strdup(cfg_getstr(config, "text_focus"));
    d->text_urgent = a_strdup(cfg_getstr(config, "text_urgent"));
    d->show_icons = cfg_getbool(config, "show_icons");

    buf = cfg_getstr(config, "show");
    if(!a_strcmp(buf, "all"))
        d->show = ShowAll;
    else if(!a_strcmp(buf, "tags"))
        d->show = ShowTags;
    else
        d->show = ShowFocus;

    /* Set cache property */
    w->cache.flags = WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
