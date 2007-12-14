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
#include "layouts/tile.h"

/** Check if at least a client is tagged with tag number t and is on screen
 * screen
 * \param t tag number
 * \param screen screen number
 * \return True or False
 */
static Bool
isoccupied(TagClientLink *tc, int screen, Client *head, Tag *t)
{
    Client *c;

    for(c = head; c; c = c->next)
        if(c->screen == screen && is_client_tagged(tc, c, t))
            return True;
    return False;
}

void
statusbar_draw(awesome_config *awesomeconf, int screen)
{
    int z, x = 0, y = 0, w;
    Client *sel = awesomeconf->focus->client;
    int phys_screen = get_phys_screen(awesomeconf->display, screen);
    Statusbar sbar;
    Tag *tag;
    Layout *layout;
    
    sbar = awesomeconf->screens[screen].statusbar;
    DrawCtx *ctx = draw_get_context(awesomeconf->display, phys_screen, sbar.width, sbar.height);
    /* don't waste our time */
    if(sbar.position == BarOff)
        return;

    for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
    {
        w = textwidth(ctx, awesomeconf->screens[screen].font, tag->name);
        if(tag->selected)
        {
            drawtext(ctx, x, y, w,
                     sbar.height,
                     awesomeconf->screens[screen].font,
                     tag->name,
                     awesomeconf->screens[screen].colors_selected);
            if(isoccupied(awesomeconf->screens[screen].tclink, screen, awesomeconf->clients, tag))
                drawrectangle(ctx, x, y,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              sel && is_client_tagged(awesomeconf->screens[screen].tclink, sel, tag),
                              awesomeconf->screens[screen].colors_selected[ColFG]);
        }
        else
        {
            drawtext(ctx, x, y, w,
                     sbar.height,
                     awesomeconf->screens[screen].font,
                     tag->name,
                     awesomeconf->screens[screen].colors_normal);
            if(isoccupied(awesomeconf->screens[screen].tclink, screen, awesomeconf->clients, tag))
                drawrectangle(ctx, x, y,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              (awesomeconf->screens[screen].font->height + 2) / 4,
                              sel && is_client_tagged(awesomeconf->screens[screen].tclink, sel, tag),
                              awesomeconf->screens[screen].colors_normal[ColFG]);
        }
        x += w;
    }

    /* This is only here until we refactor this function. */
    for(layout = awesomeconf->screens[screen].layouts; layout; layout = layout->next)
        sbar.txtlayoutwidth = MAX(sbar.txtlayoutwidth,
                                  textwidth(ctx, awesomeconf->screens[screen].font, layout->symbol));
    drawtext(ctx, x, y,
             sbar.txtlayoutwidth,
             sbar.height,
             awesomeconf->screens[screen].font,
             get_current_layout(awesomeconf->screens[screen])->symbol,
             awesomeconf->screens[screen].colors_normal);
    z = x + sbar.txtlayoutwidth;
    w = textwidth(ctx, awesomeconf->screens[screen].font, awesomeconf->screens[screen].statustext);
    x = sbar.width - w;
    if(x < z)
    {
        x = z;
        w = sbar.width - z;
    }
    drawtext(ctx, x, y, w,
             sbar.height,
             awesomeconf->screens[screen].font,
             awesomeconf->screens[screen].statustext,
             awesomeconf->screens[screen].colors_normal);
    if((w = x - z) > sbar.height)
    {
        x = z;
        if(sel && sel->screen == screen)
        {
            drawtext(ctx, x, y, w,
                     sbar.height,
                     awesomeconf->screens[screen].font,
                     sel->name,
                     awesomeconf->screens[screen].colors_selected);
            if(sel->isfloating)
                drawcircle(ctx, x, y,
                           (awesomeconf->screens[screen].font->height + 2) / 4,
                           sel->ismax,
                           awesomeconf->screens[screen].colors_selected[ColFG]);
        }
        else
            drawtext(ctx, x, y, w,
                     sbar.height,
                     awesomeconf->screens[screen].font,
                     NULL, awesomeconf->screens[screen].colors_normal);
    }
    if(sbar.position == BarRight || sbar.position == BarLeft)
    {
        Drawable d;
        if(sbar.position == BarRight)
            d = draw_rotate(ctx, phys_screen, M_PI_2, sbar.height, 0);
        else
            d = draw_rotate(ctx, phys_screen, - M_PI_2, 0, sbar.width);
        XCopyArea(awesomeconf->display, d,
                  sbar.window,
                  DefaultGC(awesomeconf->display, phys_screen), 0, 0,
                  sbar.height,
                  sbar.width, 0, 0);
        XFreePixmap(awesomeconf->display, d);
    }
    else
        XCopyArea(awesomeconf->display, ctx->drawable,
                  sbar.window,
                  DefaultGC(awesomeconf->display, phys_screen), 0, 0,
                  sbar.width, sbar.height, 0, 0);
    draw_free_context(ctx);
    XSync(awesomeconf->display, False);
}

void
statusbar_init(Display *disp, int screen, Statusbar *statusbar, Cursor cursor, XftFont *font, Padding *padding)
{
    XSetWindowAttributes wa;
    int phys_screen = get_phys_screen(disp, screen);
    ScreenInfo *si = get_screen_info(disp, screen, NULL, padding);

    statusbar->height = font->height * 1.5;

    if(statusbar->position == BarRight || statusbar->position == BarLeft)
        statusbar->width = si[screen].height;
    else
        statusbar->width = si[screen].width;

    p_delete(&si);

    statusbar->screen = screen;

    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask
        | EnterWindowMask | LeaveWindowMask | StructureNotifyMask;
    wa.cursor = cursor;
    wa.override_redirect = 1;
    wa.background_pixmap = ParentRelative;
    wa.event_mask = ButtonPressMask | ExposureMask;
    if(statusbar->dposition == BarRight || statusbar->dposition == BarLeft)
        statusbar->window = XCreateWindow(disp, RootWindow(disp, phys_screen), 0, 0,
                                          statusbar->height,
                                          statusbar->width,
                                          0, DefaultDepth(disp, phys_screen), CopyFromParent,
                                          DefaultVisual(disp, phys_screen),
                                          CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    else
        statusbar->window = XCreateWindow(disp, RootWindow(disp, phys_screen), 0, 0,
                                          statusbar->width,
                                          statusbar->height,
                                          0, DefaultDepth(disp, phys_screen), CopyFromParent,
                                          DefaultVisual(disp, phys_screen),
                                          CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
    XDefineCursor(disp, statusbar->window, cursor);
    statusbar_update_position(disp, *statusbar, padding);
    XMapRaised(disp, statusbar->window);
}

void
statusbar_update_position(Display *disp, Statusbar statusbar, Padding *padding)
{
    XEvent ev;
    ScreenInfo *si = get_screen_info(disp, statusbar.screen, NULL, padding);

    XMapRaised(disp, statusbar.window);
    switch (statusbar.position)
    {
      default:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].y_org);
        break;
      case BarRight:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org + (si[statusbar.screen].width - statusbar.height), si[statusbar.screen].y_org);
        break;
      case BarBot:
        XMoveWindow(disp, statusbar.window, si[statusbar.screen].x_org, si[statusbar.screen].height - statusbar.height);
        break;
      case BarOff:
        XUnmapWindow(disp, statusbar.window);
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

void
uicb_statusbar_toggle(awesome_config *awesomeconf,
               int screen,
               const char *arg __attribute__ ((unused)))
{
    if(awesomeconf->screens[screen].statusbar.position == BarOff)
        awesomeconf->screens[screen].statusbar.position = (awesomeconf->screens[screen].statusbar.dposition == BarOff) ? BarTop : awesomeconf->screens[screen].statusbar.dposition;
    else
        awesomeconf->screens[screen].statusbar.position = BarOff;
    statusbar_update_position(awesomeconf->display, awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);
    arrange(awesomeconf, screen);
}

void
uicb_statusbar_set_position(awesome_config *awesomeconf,
                    int screen,
                    const char *arg)
{
    awesomeconf->screens[screen].statusbar.dposition = 
        awesomeconf->screens[screen].statusbar.position =
            statusbar_get_position_from_str(arg);
    statusbar_update_position(awesomeconf->display, awesomeconf->screens[screen].statusbar, &awesomeconf->screens[screen].padding);
}

void
uicb_statusbar_set_text(awesome_config *awesomeconf, int screen, const char *arg)
{
    if(!arg)
        return;
    a_strncpy(awesomeconf->screens[screen].statustext,
              sizeof(awesomeconf->screens[screen].statustext), arg, a_strlen(arg));

    statusbar_draw(awesomeconf, screen);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
