/*
 * statusbar.c - statusbar functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include "statusbar.h"
#include "screen.h"
#include "tag.h"
#include "widget.h"
#include "window.h"

extern AwesomeConf globalconf;

static void
statusbar_position_update(Statusbar *statusbar)
{
    Statusbar *sb;
    area_t area;

    if(statusbar->position == Off)
    {
        xcb_unmap_window(globalconf.connection, statusbar->sw->window);
        return;
    }

    xutil_map_raised(globalconf.connection, statusbar->sw->window);

    /* Top and Bottom Statusbar have prio */
    if(statusbar->position == Top || statusbar->position == Bottom)
        area = screen_get_area(statusbar->screen,
                               NULL,
                               &globalconf.screens[statusbar->screen].padding);
    else
        area = screen_get_area(statusbar->screen,
                               globalconf.screens[statusbar->screen].statusbar,
                               &globalconf.screens[statusbar->screen].padding);

    for(sb = globalconf.screens[statusbar->screen].statusbar; sb && sb != statusbar; sb = sb->next)
        switch(sb->position)
        {
          case Top:
            if(sb->position == statusbar->position)
                area.y += sb->height;
            break;
          case Bottom:
            if(sb->position == statusbar->position)
                area.height -= sb->height;
            break;
          case Left:
            /* we need to re-add our own value removed in the
            * screen_get_area computation */
            if(statusbar->position == Left
               || statusbar->position == Right)
            {
                area.x -= statusbar->sw->geometry.width;
                area.width += statusbar->sw->geometry.width;
            }
            break;
          case Right:
            if(statusbar->position == Left
               || statusbar->position == Right)
                area.width += statusbar->sw->geometry.width;
            break;
          default:
            break;
        }

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
      default:
        break;
    }
}

static void
statusbar_draw(Statusbar *statusbar)
{
    Widget *widget;
    int left = 0, right = 0;
    area_t rectangle = { 0, 0, 0, 0, NULL, NULL };

    rectangle.width = statusbar->width;
    rectangle.height = statusbar->height;
    draw_rectangle(statusbar->ctx, rectangle, 1.0, true,
                   globalconf.screens[statusbar->screen].styles.normal.bg);

    for(widget = statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignLeft)
        {
            widget->cache.needs_update = false;
            left += widget->draw(widget, statusbar->ctx, left, (left + right));
        }

    /* renders right widget from last to first */
    for(widget = *widget_list_last(&statusbar->widgets);
        widget;
        widget = widget_list_prev(&statusbar->widgets, widget))
        if (widget->alignment == AlignRight)
        {
            widget->cache.needs_update = false;
            right += widget->draw(widget, statusbar->ctx, right, (left + right));
        }

    for(widget = statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignFlex)
        {
            widget->cache.needs_update = false;
            left += widget->draw(widget, statusbar->ctx, left, (left + right));
        }

    switch(statusbar->position)
    {
        case Right:
          draw_rotate(statusbar->ctx, statusbar->sw->drawable,
                      statusbar->ctx->height, statusbar->ctx->width,
                      M_PI_2, statusbar->height, 0);
          break;
        case Left:
          draw_rotate(statusbar->ctx, statusbar->sw->drawable,
                      statusbar->ctx->height, statusbar->ctx->width,
                      - M_PI_2, 0, statusbar->width);
          break;
        default:
          break;
    }

    statusbar_display(statusbar);
}

void
statusbar_display(Statusbar *statusbar)
{
    /* don't waste our time */
    if(statusbar->position != Off)
        simplewindow_refresh_drawable(statusbar->sw, statusbar->phys_screen);
}

void
statusbar_preinit(Statusbar *statusbar)
{
    if(statusbar->height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        statusbar->height = 1.5 * MAX(globalconf.screens[statusbar->screen].styles.normal.font->height,
                                      MAX(globalconf.screens[statusbar->screen].styles.focus.font->height,
                                          globalconf.screens[statusbar->screen].styles.urgent.font->height));
}

void
statusbar_init(Statusbar *statusbar)
{
    Statusbar *sb;
    xcb_drawable_t dw;
    xcb_screen_t *s = NULL;
    int phys_screen = screen_virttophys(statusbar->screen);
    area_t area = screen_get_area(statusbar->screen,
                                  globalconf.screens[statusbar->screen].statusbar,
                                  &globalconf.screens[statusbar->screen].padding);

    statusbar->phys_screen = phys_screen;

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

    switch(statusbar->position)
    {
      case Right:
      case Left:
            statusbar->sw =
                 simplewindow_new(globalconf.connection, phys_screen, 0, 0,
                                  statusbar->height, statusbar->width, 0);
            break;
      default:
            statusbar->sw =
                simplewindow_new(globalconf.connection, phys_screen, 0, 0,
                                 statusbar->width, statusbar->height, 0);
            break;
    }

    widget_calculate_alignments(statusbar->widgets);

    statusbar_position_update(statusbar);

    switch(statusbar->position)
    {
      case Off:
        return;
      case Right:
      case Left:
        s = xcb_aux_get_screen(globalconf.connection, phys_screen);

        /* we need a new pixmap this way [     ] to render */
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection,
                          s->root_depth, dw,
                          xutil_root_window(globalconf.connection, phys_screen),
                          statusbar->width, statusbar->height);
        statusbar->ctx = draw_context_new(globalconf.connection,
                                          phys_screen,
                                          statusbar->width,
                                          statusbar->height,
                                          dw);
        break;
      default:
        statusbar->ctx = draw_context_new(globalconf.connection,
                                          phys_screen,
                                          statusbar->width,
                                          statusbar->height,
                                          statusbar->sw->drawable);
        break;
    }
    

    statusbar_draw(statusbar);
}

void
statusbar_refresh()
{
    int screen;
    Statusbar *statusbar;
    Widget *widget;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
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

Statusbar *
statusbar_getbyname(int screen, const char *name)
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
    
    globalconf.screens[statusbar->screen].need_arrange = true;
}

/** Toggle the statusbar on or off.
 * Argument must be a statusbar name, or no argument for all statusbars.
 * \param screen Screen ID
 * \param arg statusbar name
 * \ingroup ui_callback
 */
void
uicb_statusbar_toggle(int screen, char *arg)
{
    Statusbar *sb = statusbar_getbyname(screen, arg);

    if(sb)
        statusbar_toggle(sb);
    else
        for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
            statusbar_toggle(sb);

    for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
        statusbar_position_update(sb);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
