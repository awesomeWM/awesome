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
 * \param widget A linked list of all widgets.
 */
void
widget_calculate_alignments(widget_t *widget)
{
    for(; widget && widget->alignment != AlignFlex; widget = widget->next)
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
 * \param barwidth The statusbar width.
 * \param widgetwidth The widget width.
 * \param alignment The widget alignment on statusbar.
 * \return The x coordinate to draw at.
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

/** Find a widget on a screen by its name.
 * \param statusbar The statusbar to look into.
 * \param name The widget name.
 * \return A widget pointer.
 */
widget_t *
widget_getbyname(statusbar_t *sb, char *name)
{
    widget_t *widget;

    for(widget = sb->widgets; widget; widget = widget->next)
        if(!a_strcmp(name, widget->name))
            return widget;

    return NULL;
}

/** Common function for button press event on widget.
 * It will look into configuration to find the callback function to call.
 * \param widget The widget.
 * \param ev The button press event the widget received.
 */
static void
widget_common_button_press(widget_t *widget, xcb_button_press_event_t *ev)
{
    Button *b;

    for(b = widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod
           && b->func)
            b->func(widget->statusbar->screen, b->arg);
}

/** Common tell function for widget, which only warn user that widget
 * cannot be told anything.
 * \param widget The widget.
 * \param new_value Unused argument.
 * \return The status of the command, which is always an error in this case.
 */
static widget_tell_status_t
widget_common_tell(widget_t *widget, char *property __attribute__ ((unused)),
                   char *new_value __attribute__ ((unused)))
{
    warn("%s widget does not accept commands.\n", widget->name);
    return WIDGET_ERROR_CUSTOM;
}

/** Common function for creating a widget.
 * \param widget The allocated widget.
 * \param statusbar The statusbar the widget is on.
 * \param config The cfg_t structure we will parse to set common info.
 */
void
widget_common_new(widget_t *widget, statusbar_t *statusbar, cfg_t *config)
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
 * external modifications. widget_t who watch flags will
 * be set to be refreshed.
 * \param screen Virtual screen number.
 * \param flags Cache flags to invalidate.
 */
void
widget_invalidate_cache(int screen, int flags)
{
    statusbar_t *statusbar;
    widget_t *widget;

    for(statusbar = globalconf.screens[screen].statusbar;
        statusbar;
        statusbar = statusbar->next)
        for(widget = statusbar->widgets; widget; widget = widget->next)
            if(widget->cache_flags & flags)
                statusbar->need_update = true;
}

/** Send commands to widgets.
 * \param screen Virtual screen number.
 * \param arg Widget command. Syntax depends on specific widget.
 * \ingroup ui_callback
 */
void
uicb_widget_tell(int screen, char *arg)
{
    statusbar_t *statusbar;
    widget_t *widget;
    char *p, *property = NULL, *new_value;
    ssize_t len;
    widget_tell_status_t status;

    if(!(len = a_strlen(arg)))
        return warn("must specify a statusbar and a widget.\n");

    if(!(p = strtok(arg, " ")))
        return warn("ignoring malformed widget command (missing statusbar name).\n");

    if(!(statusbar = statusbar_getbyname(screen, p)))
        return warn("no such statusbar: %s\n", p);

    if(!(p = strtok(NULL, " ")))
        return warn("ignoring malformed widget command (missing widget name).\n");

    if(!(widget = widget_getbyname(statusbar, p)))
        return warn("no such widget: %s in statusbar %s.\n", p, statusbar->name);

    if(!(p = strtok(NULL, " ")))
        return warn("ignoring malformed widget command (missing property name).\n");

    property = p;
    p += a_strlen(property) + 1; /* could be out of 'arg' now */

    /* arg + len points to the finishing \0.
     * p to the char right of the first space (strtok delimiter)
     *
     * \0 is on the right(>) of p pointer => some text (new_value) */
    if(arg + len > p)
    {
        len = a_strlen(p);
        new_value = p_new(char, len + 1);
        a_strncpy(new_value, len + 1, p, len);
        status = widget->tell(widget, property, new_value);
        p_delete(&new_value);
    }
    else
        status = widget->tell(widget, property, NULL);

    switch(status)
    {
      case WIDGET_ERROR:
        warn("error changing property %s of widget %s\n",
             property, widget->name);
        break;
      case WIDGET_ERROR_NOVALUE:
          warn("error changing property %s of widget %s, no value given\n",
                property, widget->name);
        break;
      case WIDGET_ERROR_FORMAT_FONT:
        warn("error changing property %s of widget %s, must be a valid font\n",
             property, widget->name);
        break;
      case WIDGET_ERROR_FORMAT_COLOR:
        warn("error changing property %s of widget %s, must be a valid color\n",
             property, widget->name);
        break;
      case WIDGET_ERROR_FORMAT_SECTION:
        warn("error changing property %s of widget %s, section/title not found\n",
             property, widget->name);
        break;
      case WIDGET_NOERROR:
          widget->statusbar->need_update = true;
          break;
      case WIDGET_ERROR_CUSTOM:
        break;
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
