/*
 * widget.c - widget managing
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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

extern AwesomeConf globalconf;

#include "widgetgen.h"

/** Compute widget alignment.
 * This will process all widget starting at `widget' and will check their
 * alignment and guess it if set to AlignAuto.
 * \param widget a linked list of all widgets
 */
void
widget_calculate_alignments(Widget *widget)
{
    for(; widget && widget->alignment != AlignFlex; widget = widget->next)
    {
        switch(widget->alignment)
        {
          case AlignCenter:
            warn("widgets cannot be center aligned\n");
          case AlignAuto:
            widget->alignment = AlignLeft;
            break;
          default:
            break;
        }
    }

    if(widget)
        for(widget = widget->next; widget; widget = widget->next)
            switch(widget->alignment)
            {
              case AlignFlex:
                warn("multiple flex widgets (%s) in panel -"
                     " ignoring flex for all but the first.\n", widget->name);
                widget->alignment = AlignRight;
                break;
              case AlignCenter:
                warn("widget %s cannot be center aligned\n", widget->name);
              case AlignAuto:
                widget->alignment = AlignRight;
                break;
              default:
                break;
            }
}

/** Compute offset for drawing the first pixel of a widget.
 * \param barwidth the statusbar width
 * \param widgetwidth the widget width
 * \param alignment the widget alignment on statusbar
 * \return the x coordinate to draw at
 */
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

/** Find a widget on a screen by its name
 * \param name the widget name
 * \param screen the screen to look into
 * \return a widget
 */
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

/** Common function for button press event on widget.
 * It will look into configuration to find the callback function to call.
 * \param widget the widget
 * \param ev the XButtonPressedEvent the widget received
 */
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

/** Common tell function for widget, which only warn user that widget
 * cannot be told anything
 * \param widget the widget
 * \param command unused argument
 * \return widget_tell_status_t enum (strucs.h)
 */
static widget_tell_status_t
widget_common_tell(Widget *widget, char *property __attribute__ ((unused)),
                   char *command __attribute__ ((unused)))
{
    warn("%s widget does not accept commands.\n", widget->name);
    return WIDGET_ERROR_CUSTOM;
}

/** Common function for creating a widget
 * \param widget The allocated widget
 * \param statusbar the statusbar the widget is on
 * \param config the cfg_t structure we will parse to set common info
 */
void
widget_common_new(Widget *widget, Statusbar *statusbar, cfg_t *config)
{
    widget->statusbar = statusbar;
    widget->name = a_strdup(cfg_title(config));
    widget->tell = widget_common_tell;
    widget->button_press = widget_common_button_press;
    widget->area.x = cfg_getint(config, "x");
    widget->area.y = cfg_getint(config, "y");
    widget->user_supplied_x = (widget->area.x != (int) 0xffffffff);
    widget->user_supplied_y = (widget->area.y != (int) 0xffffffff);
}

/** Invalidate widgets which should be refresh upon
 * external modifications. Widget who watch flags will
 * be set to be refreshed.
 * \param screen screen id
 * \param flags cache flags
 */
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
    char *p, *property = NULL, *command;
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

    p = strtok(NULL, " ");
    if(!p)
    {
        warn("Ignoring malformed widget command.\n");
        return;
    }

    property = p;

    if(p + a_strlen(p) < arg + len)
    {
        p = p + a_strlen(p) + 1;
        len = a_strlen(p);
        command = p_new(char, len + 1);
        a_strncpy(command, len + 1, p, len);
        switch(widget->tell(widget, property, command))
        {
          case WIDGET_ERROR:
            warn("error changing property %s of widget %s\n",
                 property, widget->name);
            break;
          case WIDGET_ERROR_FORMAT_BOOL:
            warn("error changing property %s of widget %s, must is boolean (0 or 1)\n",
                 property, widget->name);
            break;
          case WIDGET_ERROR_FORMAT_FONT:
            warn("error changing property %s of widget %s, must be a valid font\n",
                 property, widget->name);
            break;
          case WIDGET_ERROR_FORMAT_SECTION:
            warn("error changing property %s of widget %s, section not found\n",
                 property, widget->name);
            break;
          case WIDGET_NOERROR:
          case WIDGET_ERROR_CUSTOM:
            break;
        }
        p_delete(&command);
    }
    else
        widget->tell(widget, property, NULL);

    widget->cache.needs_update = True;

    return;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
