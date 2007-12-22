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

#include "layout.h"
#include "statusbar.h"
#include "draw.h"
#include "screen.h"
#include "util.h"
#include "tag.h"
#include "widget.h"

extern AwesomeConf globalconf;

void
statusbar_draw(int screen)
{
    int phys_screen = get_phys_screen(screen);
    VirtScreen vscreen;
    Widget *widget, *last_drawn = NULL;
    int left = 0, right = 0;
    
    vscreen = globalconf.screens[screen];
    /* don't waste our time */
    if(vscreen.statusbar->position == BarOff)
        return;

    DrawCtx *ctx = draw_get_context(globalconf.display, phys_screen,
                                    vscreen.statusbar->width,
                                    vscreen.statusbar->height);
    drawrectangle(ctx,
                  0,
                  0,
                  vscreen.statusbar->width, 
                  vscreen.statusbar->height, 
                  True,
                  vscreen.colors_normal[ColBG]);
    for(widget = vscreen.statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignLeft)
            left += widget->draw(widget, ctx, left, (left + right));

    /* renders right widget from last to first */
    for(widget = vscreen.statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignRight && last_drawn == widget->next)
        {
            right += widget->draw(widget, ctx, right, (left + right));
            last_drawn = widget;
            widget = vscreen.statusbar->widgets;
        }

    for(widget = vscreen.statusbar->widgets; widget; widget = widget->next)
        if (widget->alignment == AlignFlex)
            left += widget->draw(widget, ctx, left, (left + right));

    if(vscreen.statusbar->position == BarRight ||
       vscreen.statusbar->position == BarLeft)
    {
        Drawable d;
        if(vscreen.statusbar->position == BarRight)
            d = draw_rotate(ctx,
                            phys_screen,
                            M_PI_2,
                            vscreen.statusbar->height,
                            0);
        else
            d = draw_rotate(ctx,
                            phys_screen,
                            - M_PI_2,
                            0,
                            vscreen.statusbar->width);
        XCopyArea(globalconf.display, d,
                  vscreen.statusbar->window,
                  DefaultGC(globalconf.display, phys_screen), 0, 0,
                  vscreen.statusbar->height,
                  vscreen.statusbar->width, 0, 0);
        XFreePixmap(globalconf.display, d);
    }
    else
        XCopyArea(globalconf.display, ctx->drawable,
                  vscreen.statusbar->window,
                  DefaultGC(globalconf.display, phys_screen), 0, 0,
                  vscreen.statusbar->width, vscreen.statusbar->height, 0, 0);

    draw_free_context(ctx);
    XSync(globalconf.display, False);
}

void
statusbar_init(int screen)
{
    XSetWindowAttributes wa;
    int phys_screen = get_phys_screen(screen);
    ScreenInfo *si = get_screen_info(screen,
                                     NULL,
                                     &globalconf.screens[screen].padding);
    Statusbar *statusbar = globalconf.screens[screen].statusbar;

    statusbar->height = globalconf.screens[screen].font->height * 1.5;

    if(statusbar->position == BarRight || statusbar->position == BarLeft)
        statusbar->width = si[screen].height;
    else
        statusbar->width = si[screen].width;

    p_delete(&si);

    statusbar->screen = screen;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = globalconf.cursor[CurNormal];
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    if(statusbar->dposition == BarRight || statusbar->dposition == BarLeft)
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
    XDefineCursor(globalconf.display,
                  statusbar->window,
                  globalconf.cursor[CurNormal]);

    calculate_alignments(statusbar->widgets);

    statusbar_update_position(globalconf.display,
                              statusbar,
                              &globalconf.screens[screen].padding);
    XMapRaised(globalconf.display, statusbar->window);
}

void
statusbar_update_position(Display *disp, Statusbar *statusbar, Padding *padding)
{
    XEvent ev;
    ScreenInfo *si = get_screen_info(statusbar->screen, NULL, padding);

    XMapRaised(disp, statusbar->window);
    switch (statusbar->position)
    {
      default:
        XMoveWindow(disp, statusbar->window, si[statusbar->screen].x_org, si[statusbar->screen].y_org);
        break;
      case BarRight:
        XMoveWindow(disp, statusbar->window, si[statusbar->screen].x_org + (si[statusbar->screen].width - statusbar->height), si[statusbar->screen].y_org);
        break;
      case BarBot:
        XMoveWindow(disp, statusbar->window, si[statusbar->screen].x_org, si[statusbar->screen].height - statusbar->height);
        break;
      case BarOff:
        XUnmapWindow(disp, statusbar->window);
        break;
    }
    p_delete(&si);
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
}

int
statusbar_get_position_from_str(const char * pos)
{
    if(!a_strncmp(pos, "off", 3)) 
        return BarOff;
    else if(!a_strncmp(pos, "bottom", 6))
        return BarBot;
    else if(!a_strncmp(pos, "right", 5))
        return BarRight;
    else if(!a_strncmp(pos, "left", 4))
        return BarLeft;
    return BarTop;
}

/** Toggle statusbar
 * \param screen Screen ID
 * \param arg Unused
 * \ingroup ui_callback
 */
void
uicb_statusbar_toggle(int screen, char *arg __attribute__ ((unused)))
{
    if(globalconf.screens[screen].statusbar->position == BarOff)
        globalconf.screens[screen].statusbar->position = (globalconf.screens[screen].statusbar->dposition == BarOff) ? BarTop : globalconf.screens[screen].statusbar->dposition;
    else
        globalconf.screens[screen].statusbar->position = BarOff;
    statusbar_update_position(globalconf.display, globalconf.screens[screen].statusbar, &globalconf.screens[screen].padding);
    arrange(screen);
}

/** Set statusbar position
 * \param screen Screen ID
 * \param arg off | bottom | right | left | top
 * \ingroup ui_callback
 */
void
uicb_statusbar_set_position(int screen, char *arg)
{
    globalconf.screens[screen].statusbar->dposition = 
        globalconf.screens[screen].statusbar->position =
            statusbar_get_position_from_str(arg);
    statusbar_update_position(globalconf.display,
                              globalconf.screens[screen].statusbar,
                              &globalconf.screens[screen].padding);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
