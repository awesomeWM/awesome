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

#include <confuse.h>
#include "config.h"
#include "util.h"
#include "widget.h"
#include "tag.h"

extern AwesomeConf globalconf;

/** Check if at least one client is tagged with tag number t and is on screen
 * screen
 * \param screen screen number
 * \param t tag
 * \return True or False
 */
static Bool
isoccupied(int screen, Tag *t)
{
    Client *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t, screen))
            return True;
    return False;
}

static Bool
isurgent(int screen, Tag *t)
{
    Client *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t, screen) && c->isurgent)
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
    int w = 0, width = 0, location;
    int flagsize;
    XColor *colors;

    flagsize = (vscreen.font->height + 2) / 4;

    for(tag = vscreen.tags; tag; tag = tag->next)
        width += textwidth(ctx, vscreen.font, tag->name);

    location = widget_calculate_offset(vscreen.statusbar->width,
                                       width,
                                       offset,
                                       widget->alignment);

    width = 0;
    for(tag = vscreen.tags; tag; tag = tag->next)
    {
        w = textwidth(ctx, vscreen.font, tag->name);
        if(tag->selected)
            colors = vscreen.colors_selected;
        else if(isurgent(widget->statusbar->screen, tag))
            colors = vscreen.colors_urgent;
        else
            colors = vscreen.colors_normal;
        draw_text(ctx, location + width, 0, w,
                  vscreen.statusbar->height,
                  vscreen.font,
                  tag->name,
                  colors[ColFG],
                  colors[ColBG]);
        if(isoccupied(widget->statusbar->screen, tag))
            draw_rectangle(ctx, location + width, 0, flagsize, flagsize,
                           sel && is_client_tagged(sel,
                                                   tag,
                                                   widget->statusbar->screen),
                           colors[ColFG]);
        width += w;
    }
    return width;
}

Widget *
taglist_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = taglist_draw;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
