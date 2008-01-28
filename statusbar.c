/*
 * statusbar.c - statusbar functions
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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
#include <math.h>

#include "statusbar.h"
#include "screen.h"
#include "tag.h"
#include "widget.h"
#include "window.h"
#include "common/util.h"

extern AwesomeConf globalconf;

static void
statusbar_update_position(Statusbar *statusbar)
{
    Area area;

    XMapRaised(globalconf.display, statusbar->sw->window);

    /* Top and Bottom Statusbar have prio */
    if(statusbar->position == Top || statusbar->position == Bottom)
        area = get_screen_area(statusbar->screen,
                               NULL,
                               &globalconf.screens[statusbar->screen].padding);
    else
       area = get_screen_area(statusbar->screen,
                              globalconf.screens[statusbar->screen].statusbar,
                              &globalconf.screens[statusbar->screen].padding);

    switch(statusbar->position)
    {
      case Top:
        simplewindow_move(statusbar->sw, area.x, area.y);
        break;
      case Bottom:
        simplewindow_move(statusbar->sw, area.x, (area.y + area.height) - statusbar->sw->geometry.height);
        break;
      case Left:
        simplewindow_move(statusbar->sw, area.x - statusbar->sw->geometry.width,
                          (area.y + area.height) - statusbar->sw->geometry.height);
        break;
      case Right:
        simplewindow_move(statusbar->sw, area.x + area.width, area.y);
        break;
      case Off:
        XUnmapWindow(globalconf.display, statusbar->sw->window);
        break;
    }
}

static void
statusbar_draw(Statusbar *statusbar)
{
    int phys_screen = get_phys_screen(statusbar->screen);
    Widget *widget, *last_drawn = NULL;
    int left = 0, right = 0;
    Area rectangle = { 0, 0, 0, 0, NULL };
    Drawable d;

    /* don't waste our time */
    switch(statusbar->position)
    {
      case Off:
        return;
        break;
      case Right:
      case Left:
        /* we need a new pixmap this way [     ] to render */
        XFreePixmap(globalconf.display, statusbar->sw->drawable);
        d = XCreatePixmap(globalconf.display,
                          RootWindow(globalconf.display, phys_screen),
                          statusbar->width, statusbar->height,
                          DefaultDepth(globalconf.display, phys_screen));
        statusbar->sw->drawable = d;
        break;
      default:
        break;
    }

    DrawCtx *ctx = draw_context_new(globalconf.display,
                                    phys_screen,
                                    statusbar->width,
                                    statusbar->height,
                                    statusbar->sw->drawable);

    rectangle.width = statusbar->width;
    rectangle.height = statusbar->height;
    draw_rectangle(ctx, rectangle, True,
                   globalconf.screens[statusbar->screen].colors_normal[ColBG]);

    for(widget = statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignLeft)
        {
            widget->cache.needs_update = False;
            left += widget->draw(widget, ctx, left, (left + right));
        }

    /* renders right widget from last to first */
    for(widget = statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignRight && last_drawn == widget->next)
        {
            widget->cache.needs_update = False;
            right += widget->draw(widget, ctx, right, (left + right));
            last_drawn = widget;
            widget = statusbar->widgets;
        }

    for(widget = statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignFlex)
        {
            widget->cache.needs_update = False;
            left += widget->draw(widget, ctx, left, (left + right));
        }

    switch(statusbar->position)
    {
        case Right:
          d = draw_rotate(ctx, phys_screen, M_PI_2,
                          statusbar->height, 0);
          XFreePixmap(globalconf.display, statusbar->sw->drawable);
          statusbar->sw->drawable = d;
          break;
        case Left:
          d = draw_rotate(ctx, phys_screen, - M_PI_2,
                          0, statusbar->width);
          XFreePixmap(globalconf.display, statusbar->sw->drawable);
          statusbar->sw->drawable = d;
          break;
        default:
          break;
    }

    p_delete(&ctx);

    statusbar_display(statusbar);
}

void
statusbar_display(Statusbar *statusbar)
{
    /* don't waste our time */
    if(statusbar->position != Off)
        simplewindow_refresh_drawable(statusbar->sw, get_phys_screen(statusbar->screen));
}

void
statusbar_preinit(Statusbar *statusbar)
{
    Widget *widget;

    if(statusbar->height <= 0)
    {
        /* 1.5 as default factor, it fits nice but no one know why */
        statusbar->height = globalconf.screens[statusbar->screen].font->height * 1.5;

        for(widget = statusbar->widgets; widget; widget = widget->next)
            if(widget->font)
                statusbar->height = MAX(statusbar->height, widget->font->height * 1.5);
    }
}

void
statusbar_init(Statusbar *statusbar)
{
    Statusbar *sb;
    int phys_screen = get_phys_screen(statusbar->screen);
    Area area = get_screen_area(statusbar->screen,
                                globalconf.screens[statusbar->screen].statusbar,
                                &globalconf.screens[statusbar->screen].padding);

    
    /* Top and Bottom Statusbar have prio */
    for(sb = globalconf.screens[statusbar->screen].statusbar; sb; sb = sb->next)
        switch(sb->position)
        {
          case Left:
          case Right:
            area.width += sb->height;
            break;
          default:
            break;
        }

    if(statusbar->width <= 0)
    {
        if(statusbar->position == Right || statusbar->position == Left)
            statusbar->width = area.height;
        else
            statusbar->width = area.width;
    }

    switch(statusbar->dposition)
    {
      case Right:
      case Left:
            statusbar->sw =
                 simplewindow_new(globalconf.display, phys_screen, 0, 0,
                                  statusbar->height, statusbar->width, 0);
            break;
      default:
            statusbar->sw =
                simplewindow_new(globalconf.display, phys_screen, 0, 0,
                                 statusbar->width, statusbar->height, 0);
            break;
    }

    widget_calculate_alignments(statusbar->widgets);

    statusbar_update_position(statusbar);

    statusbar_draw(statusbar);
}

void
statusbar_refresh()
{
    int screen;
    Statusbar *statusbar;
    Widget *widget;

    for(screen = 0; screen < globalconf.nscreen; screen++)
        for(statusbar = globalconf.screens[screen].statusbar;
            statusbar;
            statusbar = statusbar->next)
            for(widget = statusbar->widgets; widget; widget = widget->next)
                if(widget->cache.needs_update)
                {
                    statusbar_draw(statusbar);
                    break;
                }
}

Position
statusbar_get_position_from_str(const char *pos)
{
    if(!a_strncmp(pos, "off", 3)) 
        return Off;
    else if(!a_strncmp(pos, "bottom", 6))
        return Bottom;
    else if(!a_strncmp(pos, "right", 5))
        return Right;
    else if(!a_strncmp(pos, "left", 4))
        return Left;
    return Top;
}

static Statusbar *
get_statusbar_byname(int screen, const char *name)
{
    Statusbar *sb;

    for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
        if(!a_strcmp(sb->name, name))
            return sb;

    return NULL;
}

static void
statusbar_toggle(Statusbar *statusbar)
{
    if(statusbar->position == Off)
        statusbar->position = (statusbar->dposition == Off) ? Top : statusbar->dposition;
    else
        statusbar->position = Off;

    statusbar_update_position(statusbar);
    globalconf.screens[statusbar->screen].need_arrange = True;
}

/** Toggle statusbar
 * \param screen Screen ID
 * \param arg statusbar name
 * \ingroup ui_callback
 */
void
uicb_statusbar_toggle(int screen, char *arg)
{
    Statusbar *sb = get_statusbar_byname(screen, arg);

    if(sb)
        statusbar_toggle(sb);
    else
        for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
            statusbar_toggle(sb);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
