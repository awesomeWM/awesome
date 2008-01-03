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

#include <stdio.h>
#include "util.h"
#include "widget.h"
#include "layout.h"
#include "client.h"
#include "tag.h"
#include "focus.h"
#include "xutil.h"
#include "screen.h"
#include "event.h"


#define ISVISIBLE_ON_TB(c, screen) (client_isvisible(c, screen) && !c->skiptb)

extern AwesomeConf globalconf;

typedef struct
{
    XColor fg_sel;
    XColor bg_sel;
    XColor fg;
    XColor bg;
} Data;

static int
tasklist_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    Client *c;
    Data *d = widget->data;
    Client *sel = focus_get_current_client(widget->statusbar->screen);
    int n = 0, i = 0, box_width = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
            n++;
    
    if(!n)
    {
        widget->width = 0;
        return widget->width;
    }

    box_width = (vscreen.statusbar->width - used) / n;

    widget->location = widget_calculate_offset(vscreen.statusbar->width,
                                               0,
                                               offset,
                                               widget->alignment);

    for(c = globalconf.clients; c; c = c->next)
        if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
        {
            if(sel == c)
            {
                draw_text(ctx, widget->location + box_width * i, 0,
                          box_width,
                          vscreen.statusbar->height,
                          widget->font->height / 2, widget->font, c->name,
                          d->fg_sel, d->bg_sel);
            }
            else
                draw_text(ctx, widget->location + box_width * i, 0,
                          box_width,
                          vscreen.statusbar->height,
                          widget->font->height / 2, widget->font, c->name,
                          d->fg, d->bg);
            if(sel->isfloating)
                draw_circle(ctx, widget->location + box_width * i, 0,
                            (widget->font->height + 2) / 4,
                            sel->ismax, d->fg);
            i++;
        }

    widget->width = vscreen.statusbar->width - used;

    return widget->width;
}

static void
tasklist_button_press(Widget *widget, XButtonPressedEvent *ev)
{
    Button *b;
    Client *c;
    int n = 0, box_width = 0, i = 0, ci = 0;

    /* button1 give focus */
    if(ev->button == Button1 && CLEANMASK(ev->state) == NoSymbol)
    {
        for(c = globalconf.clients; c; c = c->next)
            if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
                n++;
    
        if(!n)
            return;

        box_width = widget->width / n;

        if(ev->button == Button1 && CLEANMASK(ev->state) == NoSymbol)
            if(widget->statusbar->position == BarTop
               || widget->statusbar->position == BarBot)
            {
                ci = (ev->x - widget->location) / box_width;

                /* found first visible client */
                for(c = globalconf.clients;
                    c && !ISVISIBLE_ON_TB(c, widget->statusbar->screen);
                    c = c->next);
                /* found ci-th visible client */
                for(; c && i < ci; c = c->next)
                    if(ISVISIBLE_ON_TB(c, widget->statusbar->screen))
                        i++;

                focus(c, True, widget->statusbar->screen);
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
    int phys_screen = get_phys_screen(statusbar->screen);

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = tasklist_draw;
    w->button_press = tasklist_button_press;
    w->alignment = AlignFlex;
    w->data = d = p_new(Data, 1);

    if((buf = cfg_getstr(config, "fg")))
        d->fg = initxcolor(phys_screen, buf);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        d->bg = initxcolor(phys_screen, buf);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    if((buf = cfg_getstr(config, "focus_bg")))
        d->bg_sel = initxcolor(phys_screen, buf);
    else
        d->bg_sel = globalconf.screens[statusbar->screen].colors_selected[ColBG];

    if((buf = cfg_getstr(config, "focus_fg")))
        d->fg_sel = initxcolor(phys_screen, buf);
    else
        d->fg_sel = globalconf.screens[statusbar->screen].colors_selected[ColFG];

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
