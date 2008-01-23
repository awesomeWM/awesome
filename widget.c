/*
 * widget.c - widget managing
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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
#include "statusbar.h"
#include "event.h"
#include "common/util.h"

extern AwesomeConf globalconf;

#include "widgetgen.h"

void
widget_calculate_alignments(Widget *widget)
{
    for(; widget; widget = widget->next)
    {
        if(widget->alignment == AlignFlex)
        {
            widget = widget->next;
            break;
        }
        widget->alignment = AlignLeft;
    }

    if(widget)
        for(; widget; widget = widget->next)
        {
            if (widget->alignment == AlignFlex)
                warn("Multiple flex widgets in panel -"
                     " ignoring flex for all but the first.");
            widget->alignment = AlignRight;
        }
}

int
widget_calculate_offset(int barwidth, int widgetwidth, int offset, int alignment)
{
    switch(alignment)
    {
      case AlignLeft:
      case AlignFlex:
        return offset;
    }
    return barwidth - offset - widgetwidth;
}

static Widget *
widget_find(char *name, int screen)
{
    Widget *widget;
    Statusbar *sb;
    
    for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
        for(widget = sb->widgets; widget; widget = widget->next)
            if(a_strcmp(name, widget->name) == 0)
                return widget;

    return NULL;
}

static void
widget_common_button_press(Widget *widget, XButtonPressedEvent *ev)
{
    Button *b;

    for(b = widget->buttons; b; b = b->next)
        if(ev->button == b->button && CLEANMASK(ev->state) == b->mod && b->func)
        {
            b->func(widget->statusbar->screen, b->arg);
            break;
        }
}

static void
widget_common_tell(Widget *widget, char *command __attribute__ ((unused)))
{
    warn("%s widget does not accept commands.\n", widget->name);
}

void
widget_common_new(Widget *widget, Statusbar *statusbar, cfg_t* config)
{
    const char *name;

    widget->statusbar = statusbar;
    name = cfg_title(config);
    widget->name = a_strdup(name);
    widget->tell = widget_common_tell;
    widget->button_press = widget_common_button_press;
    widget->area.x = cfg_getint(config, "x");
    widget->area.y = cfg_getint(config, "y");
    widget->user_supplied_x = (widget->area.x != (int) 0xffffffff);
    widget->user_supplied_y = (widget->area.y != (int) 0xffffffff);
}

void
widget_invalidate_cache(int screen, int flags)
{
    Statusbar *statusbar;
    Widget *widget;

    for(statusbar = globalconf.screens[screen].statusbar;
        statusbar;
        statusbar = statusbar->next)
        for(widget = statusbar->widgets; widget; widget = widget->next)
            if(widget->cache.flags & flags)
                widget->cache.needs_update = True;
}

/** Send command to widget
 * \param screen Screen ID
 * \param arg Widget command. Syntax depends on specific widget.
 * \ingroup ui_callback
 */
void
uicb_widget_tell(int screen, char *arg)
{
    Widget *widget;
    char *p, *command;
    ssize_t len;

    if (!arg)
    {
        warn("Must specify a widget.\n");
        return;
    }

    len = a_strlen(arg);
    p = strtok(arg, " ");
    if(!p)
    {
        warn("Ignoring malformed widget command.\n");
        return;
    }

    widget = widget_find(p, screen);
    if(!widget)
    {
        warn("No such widget: %s\n", p);
        return;
    }

    if(p + a_strlen(p) < arg + len)
    {
        p = p + a_strlen(p) + 1;
        len = a_strlen(p);
        command = p_new(char, len + 1);
        a_strncpy(command, len + 1, p, len);
        widget->tell(widget, command);
        p_delete(&command);
    }
    else
        widget->tell(widget, NULL);

    widget->cache.needs_update = True;

    return;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
