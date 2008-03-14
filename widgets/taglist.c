/*
 * taglist.c - tag list widget
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

#include "config.h"
#include "client.h"
#include "widget.h"
#include "tag.h"
#include "event.h"
#include "common/util.h"

extern AwesomeConf globalconf;

/** Check if at least one client is tagged with tag number t and is on screen
 * screen
 * \param screen screen number
 * \param t tag
 * \return True or False
 */
static Bool
isoccupied(Tag *t)
{
    Client *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t) && !c->skip && c != globalconf.scratch.client)
            return True;

    return False;
}

static Bool
isurgent(Tag *t)
{
    Client *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t) && c->isurgent)
            return True;

    return False;
}

static int
taglist_draw(Widget *widget,
             DrawCtx *ctx,
             int offset,
             int used __attribute__ ((unused)))
{
    Tag *tag;
    Client *sel = globalconf.focus->client;
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int w = 0, flagsize;
    style_t style;
    Area area;

    flagsize = (vscreen.styles.normal.font->height + 2) / 3;

    widget->area.width = 0;

    for(tag = vscreen.tags; tag; tag = tag->next)
    {
        style = tag->selected ? vscreen.styles.focus : vscreen.styles.normal;
        widget->area.width += draw_textwidth(ctx->display, style.font, tag->name)
            + style.font->height;
    }

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    widget->area.width = 0;
    for(tag = vscreen.tags; tag; tag = tag->next)
    {
        if(tag->selected)
            style = vscreen.styles.focus;
        else if(isurgent(tag))
            style = vscreen.styles.urgent;
        else
            style = vscreen.styles.normal;
        w = draw_textwidth(ctx->display, style.font, tag->name) + style.font->height;
        area.x = widget->area.x + widget->area.width;
        area.y = widget->area.y;
        area.width = w;
        area.height = widget->statusbar->height;
        draw_text(ctx, area,
                  AlignCenter,
                  style.font->height / 2,
                  tag->name,
                  style);

        if(isoccupied(tag))
        {
            Area rectangle = { widget->area.x + widget->area.width,
                               widget->area.y,
                               flagsize,
                               flagsize,
                               NULL, NULL };
            draw_rectangle(ctx, rectangle, sel && is_client_tagged(sel, tag), style.fg);
        }
        widget->area.width += w;
    }

    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static void
taglist_button_press(Widget *widget, XButtonPressedEvent *ev)
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    Button *b;
    Tag *tag;
    char buf[4];
    int prev_width = 0, width = 0, i = 1;
    style_t style;

    for(b = widget->buttons; b; b = b->next)
        if(ev->button == b->button && CLEANMASK(ev->state) == b->mod && b->func)
        {
            /* XXX this should be in a function */
            if(tag->selected)
                style = vscreen.styles.focus;
            else if(isurgent(tag))
                style = vscreen.styles.urgent;
            else
                style = vscreen.styles.normal;

            switch(widget->statusbar->position)
            {
              case Top:
              case Bottom:
                for(tag = vscreen.tags; tag; tag = tag->next, i++)
                {
                    width = draw_textwidth(globalconf.display, style.font, tag->name)
                        + style.font->height;
                    if(ev->x >= widget->area.x + prev_width
                       && ev->x < widget->area.x + prev_width + width)
                    {
                        snprintf(buf, sizeof(buf), "%d", i);
                        b->func(widget->statusbar->screen, buf);
                        return;
                    }
                   prev_width += width;
                }
                break;
              case Right:
                for(tag = vscreen.tags; tag; tag = tag->next, i++)
                {
                    width = draw_textwidth(globalconf.display, style.font, tag->name) + style.font->height;
                    if(ev->y >= widget->area.x + prev_width
                       && ev->y < widget->area.x + prev_width + width)
                    {
                        snprintf(buf, sizeof(buf), "%d", i);
                        b->func(widget->statusbar->screen, buf);
                        return;
                    }
                    prev_width += width;
                }
                break;
              default:
                for(tag = vscreen.tags; tag; tag = tag->next, i++)
                {
                    width = draw_textwidth(globalconf.display, style.font, tag->name) + style.font->height;
                    if(widget->statusbar->width - ev->y >= widget->area.x + prev_width
                       && widget->statusbar->width - ev->y < widget->area.x + prev_width + width)
                    {
                        snprintf(buf, sizeof(buf), "%d", i);
                        b->func(widget->statusbar->screen, buf);
                        return;
                    }
                    prev_width += width;
                }
                break;
            }
        }
}

Widget *
taglist_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = taglist_draw;
    w->button_press = taglist_button_press;
    w->alignment = draw_get_align(cfg_getstr(config, "align"));

    /* Set cache property */
    w->cache.flags = WIDGET_CACHE_TAGS | WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
