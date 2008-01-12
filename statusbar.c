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
#include "util.h"
#include "tag.h"
#include "widget.h"

extern AwesomeConf globalconf;

static void
statusbar_update_position(Statusbar *statusbar)
{
    Area area = get_screen_area(statusbar->screen,
                                NULL,
                                &globalconf.screens[statusbar->screen].padding);

    XMapRaised(globalconf.display, statusbar->window);
    switch(statusbar->position)
    {
      default:
        XMoveWindow(globalconf.display, statusbar->window,
                    area.x, area.y);
        break;
      case Left:
        XMoveWindow(globalconf.display, statusbar->window,
                    area.x, (area.y + area.height) - statusbar->width);
        break;
      case Right:
        XMoveWindow(globalconf.display, statusbar->window,
                    area.x + (area.width - statusbar->height), area.y);
        break;
      case Bottom:
        XMoveWindow(globalconf.display, statusbar->window,
                    area.x, area.height - statusbar->height);
        break;
      case Off:
        XUnmapWindow(globalconf.display, statusbar->window);
        break;
    }
}

static void
statusbar_draw(Statusbar *statusbar)
{
    int phys_screen = get_phys_screen(statusbar->screen);
    Widget *widget, *last_drawn = NULL;
    int left = 0, right = 0;

    /* don't waste our time */
    if(statusbar->position == Off)
        return;

    XFreePixmap(globalconf.display, statusbar->drawable);

    DrawCtx *ctx = draw_get_context(phys_screen,
                                    statusbar->width,
                                    statusbar->height);

    draw_rectangle(ctx, 0, 0, statusbar->width, statusbar->height, True,
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

    if(statusbar->position == Right
       || statusbar->position == Left)
    {
        if(statusbar->position == Right)
            statusbar->drawable = draw_rotate(ctx, phys_screen, M_PI_2, statusbar->height, 0);
        else
            statusbar->drawable = draw_rotate(ctx, phys_screen, - M_PI_2, 0, statusbar->width);

        draw_free_context(ctx);
    }
    else
    {
        statusbar->drawable = ctx->drawable;
        /* just delete the struct, don't delete the drawable */
        p_delete(&ctx);
    }

    statusbar_display(statusbar);
}

void
statusbar_display(Statusbar *statusbar)
{
    int phys_screen = get_phys_screen(statusbar->screen);

    /* don't waste our time */
    if(statusbar->position == Off)
        return;

    if(statusbar->position == Right
       || statusbar->position == Left)
        XCopyArea(globalconf.display, statusbar->drawable,
                  statusbar->window,
                  DefaultGC(globalconf.display, phys_screen), 0, 0,
                  statusbar->height,
                  statusbar->width, 0, 0);
    else
        XCopyArea(globalconf.display, statusbar->drawable,
                  statusbar->window,
                  DefaultGC(globalconf.display, phys_screen), 0, 0,
                  statusbar->width, statusbar->height, 0, 0);
}

void
statusbar_init(Statusbar *statusbar, int screen)
{
    Widget *widget;
    XSetWindowAttributes wa;
    int phys_screen = get_phys_screen(screen);
    Area area = get_screen_area(screen,
                                globalconf.screens[screen].statusbar,
                                &globalconf.screens[screen].padding);

    if(statusbar->height <= 0)
    {
        /* 1.5 as default factor, it fits nice but no one know why */
        statusbar->height = globalconf.screens[screen].font->height * 1.5;

        for(widget = statusbar->widgets; widget; widget = widget->next)
            if(widget->font)
                statusbar->height = MAX(statusbar->height, widget->font->height * 1.5);
    }

    if(statusbar->width <= 0)
    {
        if(statusbar->position == Right || statusbar->position == Left)
            statusbar->width = area.height;
        else
            statusbar->width = area.width;
    }

    statusbar->screen = screen;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = globalconf.cursor[CurNormal];
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    if(statusbar->dposition == Right || statusbar->dposition == Left)
        statusbar->window = XCreateWindow(globalconf.display,
                                          RootWindow(globalconf.display,
                                          phys_screen),
                                          0, 0,
                                          statusbar->height,
                                          statusbar->width,
                                          0,
                                          DefaultDepth(globalconf.display,
                                                       phys_screen),
                                          CopyFromParent,
                                          DefaultVisual(globalconf.display,
                                                        phys_screen),
                                          CWOverrideRedirect |
                                          CWBackPixmap |
                                          CWEventMask,
                                          &wa);
    else
        statusbar->window = XCreateWindow(globalconf.display,
                                          RootWindow(globalconf.display,
                                                     phys_screen),
                                          0, 0,
                                          statusbar->width,
                                          statusbar->height,
                                          0,
                                          DefaultDepth(globalconf.display,
                                                       phys_screen),
                                          CopyFromParent,
                                          DefaultVisual(globalconf.display,
                                                        phys_screen),
                                          CWOverrideRedirect |
                                          CWBackPixmap |
                                          CWEventMask,
                                          &wa);

    statusbar->drawable = XCreatePixmap(globalconf.display,
                                        RootWindow(globalconf.display, phys_screen),
                                        statusbar->width, statusbar->height,
                                        DefaultDepth(globalconf.display, phys_screen));


    XDefineCursor(globalconf.display,
                  statusbar->window,
                  globalconf.cursor[CurNormal]);

    widget_calculate_alignments(statusbar->widgets);

    statusbar_update_position(statusbar);
    XMapRaised(globalconf.display, statusbar->window);

    statusbar_draw(statusbar);
}

void
statusbar_refresh()
{
    int screen;
    Statusbar *statusbar;
    Widget *widget;

    for(screen = 0; screen < globalconf.nscreens; screen++)
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

    arrange(screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
