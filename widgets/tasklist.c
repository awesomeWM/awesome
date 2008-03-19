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
#include "common/util.h"

extern AwesomeConf globalconf;

typedef enum
{
    ShowFocus,
    ShowTags,
    ShowAll,
} ShowClient;

typedef struct
{
    ShowClient show;
    Bool show_icons;
    Alignment align;
    struct
    {
        style_t normal;
        style_t focus;
        style_t urgent;
    } styles;
} Data;

static inline Bool
tasklist_isvisible(Client *c, int screen, ShowClient show)
{
    if(c->skip || c->skiptb)
        return False;

    switch(show)
    {
      case ShowAll:
        if(c->screen == screen)
            return True;
        break;
      case ShowTags:
        if(client_isvisible(c, screen))
            return True;
        break;
      case ShowFocus:
        if(c == globalconf.focus->client)
            return True;
        break;
    }

    return False;
}

static int
tasklist_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    Client *c;
    Data *d = widget->data;
    Client *sel = focus_get_current_client(widget->statusbar->screen);
    Rule *r;
    area_t area;
    style_t style;
    int n = 0, i = 0, box_width = 0, icon_width = 0, box_width_rest = 0;
    NetWMIcon *icon;

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

            if(c->isurgent)
                style = d->styles.urgent;
            else if(sel == c)
                style = d->styles.focus;
            else
                style = d->styles.normal;

            if(d->show_icons)
            {
                /* draw a background for icons */
                area.x = widget->area.x + box_width * i;
                area.y = widget->area.y;
                area.height = widget->statusbar->height;
                area.width = box_width;

                draw_rectangle(ctx, area, True, style.bg);

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

            draw_text(ctx, area, d->align,
                      style.font->height / 2, c->name,
                      style);

            if(c == globalconf.scratch.client)
            {
                area.x = widget->area.x + icon_width + box_width * i;
                area.y = widget->area.y;
                area.width = (style.font->height + 2) / 3;
                area.height = (style.font->height + 2) / 3;
                draw_rectangle(ctx, area, c->isfloating, style.fg);
            }
            else if(c->isfloating || c->ismax)
                draw_circle(ctx, widget->area.x + icon_width + box_width * i,
                            widget->area.y,
                            (style.font->height + 2) / 4,
                            c->ismax, style.fg);
            i++;
        }

    widget->area.width = widget->statusbar->width - used;
    widget->area.height = widget->statusbar->height;

    return widget->area.width;
}

static void
tasklist_button_press(Widget *widget, XButtonPressedEvent *ev)
{
    Button *b;
    Client *c;
    Data *d = widget->data;
    Tag *tag;
    int n = 0, box_width = 0, i, ci = 0;

    /* button1 give focus */
    if(ev->button == Button1 && CLEANMASK(ev->state) == NoSymbol)
    {
        for(c = globalconf.clients; c; c = c->next)
            if(tasklist_isvisible(c, widget->statusbar->screen, d->show))
                n++;

        if(!n)
            return;

        box_width = widget->area.width / n;

        if(ev->button == Button1 && CLEANMASK(ev->state) == NoSymbol)
        {
            switch(widget->statusbar->position)
            {
              case Top:
              case Bottom:
                ci = (ev->x - widget->area.x) / box_width;
                break;
              case Right:
                ci = (ev->y - widget->area.x) / box_width;
                break;
              default:
                ci = ((widget->statusbar->width - ev->y) - widget->area.x) / box_width;
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
                client_focus(c, widget->statusbar->screen, True);
            }

            return;
        }
    }

    for(b = widget->buttons; b; b = b->next)
        if(ev->button == b->button && CLEANMASK(ev->state) == b->mod && b->func)
        {
            b->func(widget->statusbar->screen, b->arg);
            return;
        }
}

Widget *
tasklist_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *buf;
    cfg_t *cfg_styles;
    int phys_screen = get_phys_screen(statusbar->screen);

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = tasklist_draw;
    w->button_press = tasklist_button_press;
    w->alignment = AlignFlex;
    w->data = d = p_new(Data, 1);

    cfg_styles = cfg_getsec(config, "styles");

    draw_style_init(globalconf.display, phys_screen,
                    cfg_getsec(cfg_styles, "normal"),
                    &d->styles.normal,
                    &globalconf.screens[statusbar->screen].styles.normal);

    draw_style_init(globalconf.display, phys_screen,
                    cfg_getsec(cfg_styles, "focus"),
                    &d->styles.focus,
                    &globalconf.screens[statusbar->screen].styles.focus);

    draw_style_init(globalconf.display, phys_screen,
                    cfg_getsec(cfg_styles, "urgent"),
                    &d->styles.urgent,
                    &globalconf.screens[statusbar->screen].styles.urgent);

    d->align = * (Alignment *) cfg_getptr(config, "text_align");
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
