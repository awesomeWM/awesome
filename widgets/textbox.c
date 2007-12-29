/*
 * textbox.c - text box widget
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
#include "util.h"
#include "widget.h"
#include "xutil.h"

extern AwesomeConf globalconf;

typedef struct
{
    char *text;
    XColor fg;
    XColor bg;
} Data;

static void
update(Widget *widget, char *text)
{
    Data *d = widget->data;
    if (d->text)
        p_delete(&d->text);
    d->text = a_strdup(text);
}

static int
textbox_draw(Widget *widget, DrawCtx *ctx, int offset,
             int used __attribute__ ((unused)))
{
    Data *d = widget->data;

    widget->width = textwidth(widget->font, d->text);
    widget->location = widget_calculate_offset(widget->statusbar->width,
                                               widget->width,
                                               offset,
                                               widget->alignment);

    draw_text(ctx, widget->location, 0, widget->width, widget->statusbar->height,
              widget->font, d->text, d->fg, d->bg);

    return widget->width;
}

static void
textbox_tell(Widget *widget, char *command)
{
    update(widget, command);
}

Widget *
textbox_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;
    char *buf;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = textbox_draw;
    w->tell = textbox_tell;

    w->data = d = p_new(Data, 1);

    if((buf = cfg_getstr(config, "fg")))
        d->fg = initxcolor(statusbar->screen, buf);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        d->bg = initxcolor(statusbar->screen, buf);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    update(w, cfg_getstr(config, "text"));
    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
