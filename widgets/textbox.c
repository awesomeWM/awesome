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

#include "util.h"
#include "widget.h"
#include "xutil.h"
#include "screen.h"

extern AwesomeConf globalconf;

typedef struct
{
    char *text;
    int width;
    Alignment align;
    XColor fg;
    XColor bg;
} Data;

static int
textbox_draw(Widget *widget, DrawCtx *ctx, int offset,
             int used __attribute__ ((unused)))
{
    Data *d = widget->data;

    if(d->width)
        widget->area.width = d->width;
    else
        widget->area.width = draw_textwidth(widget->font, d->text);

    widget->area.height = widget->statusbar->height;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_text(ctx, widget->area, d->align, 0, widget->font, d->text, d->fg, d->bg);

    return widget->area.width;
}

static void
textbox_tell(Widget *widget, char *command)
{
    char *ntok, *tok;
    ssize_t command_len = a_strlen(command);
    int i = 0, phys_screen = get_phys_screen(widget->statusbar->screen);

    Data *d = widget->data;
    if (d->text)
        p_delete(&d->text);

    for (i = 0, tok = command; tok && i < 2 && *tok == '#'; i++)
    {
        ntok = strchr(tok, ' ');
        if((!ntok && command_len - (tok - command) == 7) ||
           ntok - tok == 7)
        {
            if (ntok)
                *ntok = 0;
            if (!i)
                d->fg = initxcolor(globalconf.display, phys_screen, tok);
            else
                d->bg = initxcolor(globalconf.display, phys_screen, tok);
            if (ntok)
                *ntok = ' ';
            tok = ntok + (ntok != NULL);
        }
        else
            break;
    }
    d->text = a_strdup(tok);
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
        d->fg = initxcolor(globalconf.display, statusbar->screen, buf);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        d->bg = initxcolor(globalconf.display, get_phys_screen(statusbar->screen), buf);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    d->width = cfg_getint(config, "width");
    d->align = draw_get_align(cfg_getstr(config, "align"));

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    d->text = a_strdup(cfg_getstr(config, "text"));
    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
