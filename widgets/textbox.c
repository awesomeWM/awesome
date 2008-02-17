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

#include "widget.h"
#include "screen.h"
#include "common/util.h"

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
textbox_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    Data *d = widget->data;

    if(d->width)
        widget->area.width = d->width;
    else if(widget->alignment == AlignFlex)
        widget->area.width = widget->statusbar->width - used;
    else
        widget->area.width = MIN(draw_textwidth(ctx->display, widget->font, d->text),
                                 widget->statusbar->width - used);

    widget->area.height = widget->statusbar->height;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_text(ctx, widget->area, d->align, 0, widget->font, d->text,
              globalconf.screens[widget->statusbar->screen].shadow_offset,
              d->fg, d->bg);

    return widget->area.width;
}

static void
textbox_tell(Widget *widget, char *property, char *command)
{
    Data *d = widget->data;

    if(!property || !command)
        return;

    if(!a_strcmp(property, "text"))
    {
        if (d->text)
            p_delete(&d->text);
        d->text = a_strdup(command);
    }
    else if(!a_strcmp(property, "fg"))
        draw_color_new(globalconf.display, widget->statusbar->screen, command, &d->fg);

    else if(!a_strcmp(property, "bg"))
        draw_color_new(globalconf.display, widget->statusbar->screen, command, &d->bg);

    else if(!a_strcmp(property, "font"))
    {
        widget->font = XftFontOpenName(globalconf.display,
                       get_phys_screen(widget->statusbar->screen), command);
        if(!widget->font)
            widget->font = globalconf.screens[widget->statusbar->screen].font;
    }

    else if(!a_strcmp(property, "width"))
        d->width = atoi(command);

    else if(!a_strcmp(property, "text_align"))
    {
        if(!a_strcmp(command, "center") || !a_strcmp(command, "left") ||
           !a_strcmp(command, "right"))
            d->align = draw_get_align(command);
        else
            warn("text_align value must be (case-sensitive) \"center\", \"left\",\
                  or \"right\". But is: %s.\n", command);
    }

    else if(!a_strcmp(property, "align") || !a_strcmp(property, "mouse") ||
            !a_strcmp(property, "x") || !a_strcmp(property, "y"))
        warn("Property \"%s\" can't get changed.\n", property);

    else
        warn("No such property: %s\n", property);
    return;
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
    w->alignment = draw_get_align(cfg_getstr(config, "align"));

    w->data = d = p_new(Data, 1);

    if((buf = cfg_getstr(config, "fg")))
        draw_color_new(globalconf.display, get_phys_screen(statusbar->screen), buf, &d->fg);
    else
        d->fg = globalconf.screens[statusbar->screen].colors_normal[ColFG];

    if((buf = cfg_getstr(config, "bg")))
        draw_color_new(globalconf.display, get_phys_screen(statusbar->screen), buf, &d->bg);
    else
        d->bg = globalconf.screens[statusbar->screen].colors_normal[ColBG];

    d->width = cfg_getint(config, "width");
    d->align = draw_get_align(cfg_getstr(config, "text_align"));

    if((buf = cfg_getstr(config, "font")))
        w->font = XftFontOpenName(globalconf.display, get_phys_screen(statusbar->screen), buf);

    if(!w->font)
        w->font = globalconf.screens[statusbar->screen].font;

    d->text = a_strdup(cfg_getstr(config, "text"));

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
