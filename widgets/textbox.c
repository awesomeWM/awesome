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
#include "common/configopts.h"

extern AwesomeConf globalconf;

typedef struct
{
    char *text;
    int width;
    Alignment align;
    style_t style;
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
        widget->area.width = MIN(draw_textwidth(ctx->connection, ctx->default_screen,
                                                d->style.font, d->text),
                                 widget->statusbar->width - used);

    widget->area.height = widget->statusbar->height;

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);
    if(!widget->user_supplied_y)
        widget->area.y = 0;

    draw_text(ctx, widget->area, d->align, 0, d->text, d->style);

    return widget->area.width;
}

static widget_tell_status_t
textbox_tell(Widget *widget, char *property, char *command)
{
    Data *d = widget->data;
    font_t *newfont;

    if(!a_strcmp(property, "text"))
    {
        if (d->text)
            p_delete(&d->text);
        d->text = a_strdup(command);
    }
    else if(!a_strcmp(property, "fg"))
        if(draw_color_new(globalconf.connection, widget->statusbar->screen, command, &d->style.fg))
            return WIDGET_NOERROR;
        else
            return WIDGET_ERROR_FORMAT_COLOR;
    else if(!a_strcmp(property, "bg"))
        if(draw_color_new(globalconf.connection, widget->statusbar->screen, command, &d->style.bg))
            return WIDGET_NOERROR;
        else
            return WIDGET_ERROR_FORMAT_COLOR;
    else if(!a_strcmp(property, "font"))
    {
        if((newfont = draw_font_new(globalconf.connection, globalconf.default_screen, command)))
        {
            if(d->style.font != globalconf.screens[widget->statusbar->screen].styles.normal.font)
                draw_font_delete(&d->style.font);
            d->style.font = newfont;
        }
        else
            return WIDGET_ERROR_FORMAT_FONT;
    }
    else if(!a_strcmp(property, "width"))
        d->width = atoi(command);
    else if(!a_strcmp(property, "text_align"))
        d->align = draw_align_get_from_str(command);
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

Widget *
textbox_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;

    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = textbox_draw;
    w->tell = textbox_tell;
    w->alignment = cfg_getalignment(config, "align");

    w->data = d = p_new(Data, 1);

    draw_style_init(globalconf.connection, statusbar->phys_screen,
                    cfg_getsec(config, "style"),
                    &d->style,
                    &globalconf.screens[statusbar->screen].styles.normal);

    d->width = cfg_getint(config, "width");
    d->align = cfg_getalignment(config, "text_align");

    d->text = a_strdup(cfg_getstr(config, "text"));

    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
