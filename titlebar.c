/*
 * titlebar.c - titlebar management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include <math.h>

#include "titlebar.h"
#include "screen.h"
#include "layouts/floating.h"

extern AwesomeConf globalconf;

static char *
titlebar_markup_parse(client_t *c, const char *str, ssize_t len)
{
    const char *ps;
    char *new;
    int i = 0;
    ssize_t clen;
   
    new = p_new(char, len++);

    for(ps = str; *ps; ps++, i++)
    {
        if(*ps == '%')
        {
            ps++;
            if(*ps == '%')
                new[i] = '%';
            else if(*ps == 't')
            {
                clen = a_strlen(c->name);
                len += clen;
                p_realloc(&new, len);
                a_strcat(new + i, len - i,  c->name);
                i += clen - 1;
            }
        }
        else
            new[i] = *ps;
    }

    return new;
}

static char *
titlebar_text(client_t *c)
{
    char *text;

    if(globalconf.focus->client == c)
        text = c->titlebar.text_focus;
    else if(c->isurgent)
        text = c->titlebar.text_urgent;
    else
        text = c->titlebar.text_normal;

    return titlebar_markup_parse(c, text, a_strlen(text));
}

static inline area_t
titlebar_size(client_t *c)
{
    return draw_text_extents(globalconf.connection, globalconf.default_screen,
                             globalconf.screens[c->screen].styles.normal.font, titlebar_text(c));
}

/** Initialize a titlebar: create the simple_window_t.
 * We still need to update its geometry to have it placed correctly.
 * \param c the client
 */
void
titlebar_init(client_t *c)
{
    int width = 0, height = 0;

    if(!c->titlebar.height)
        c->titlebar.height = MAX(MAX(draw_text_extents(globalconf.connection, globalconf.default_screen,
                                                       globalconf.screens[c->screen].styles.normal.font, c->titlebar.text_focus).height,
                                     draw_text_extents(globalconf.connection, globalconf.default_screen,
                                           globalconf.screens[c->screen].styles.normal.font, c->titlebar.text_normal).height),
                                 draw_text_extents(globalconf.connection, globalconf.default_screen,
                                                   globalconf.screens[c->screen].styles.normal.font, c->titlebar.text_urgent).height);

    switch(c->titlebar.position)
    {
      case Off:
        return;
      default:
        c->titlebar.position = Off;
        return;
      case Top:
      case Bottom:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        height = c->titlebar.height;
        break;
      case Left:
      case Right:
        if(!c->titlebar.width)
            height = c->geometry.height + 2 * c->border;
        else
            height = MIN(c->titlebar.width, c->geometry.height);
        width = c->titlebar.height;
        break;
    }
    c->titlebar.sw = simplewindow_new(globalconf.connection,
                                      c->phys_screen, 0, 0,
                                      width, height, 0);
    xcb_map_window(globalconf.connection, c->titlebar.sw->window);
}

/** Add the titlebar geometry to a geometry.
 * \param t the titlebar
 * \param geometry the geometry
 * \return a new geometry bigger if the titlebar is visible
 */
area_t
titlebar_geometry_add(titlebar_t *t, area_t geometry)
{
    if(!t->sw)
        return geometry;

    switch(t->position)
    {
      case Top:
        geometry.y -= t->sw->geometry.height;
        geometry.height += t->sw->geometry.height;
        break;
      case Bottom:
        geometry.height += t->sw->geometry.height;
        break;
      case Left:
        geometry.x -= t->sw->geometry.width;
        geometry.width += t->sw->geometry.width;
        break;
      case Right:
        geometry.width += t->sw->geometry.width;
        break;
      default:
        break;
    }

    return geometry;
}

/** Remove the titlebar geometry to a geometry.
 * \param t the titlebar
 * \param geometry the geometry
 * \return a new geometry smaller if the titlebar is visible
 */
area_t
titlebar_geometry_remove(titlebar_t *t, area_t geometry)
{
    if(!t->sw)
        return geometry;

    switch(t->position)
    {
      case Top:
        geometry.y += t->sw->geometry.height;
        geometry.height -= t->sw->geometry.height;
        break;
      case Bottom:
        geometry.height -= t->sw->geometry.height;
        break;
      case Left:
        geometry.x += t->sw->geometry.width;
        geometry.width -= t->sw->geometry.width;
        break;
      case Right:
        geometry.width -= t->sw->geometry.width;
        break;
      default:
        break;
    }

    return geometry;
}

/** Draw the titlebar content.
 * \param c the client
 */
void
titlebar_draw(client_t *c)
{
    xcb_drawable_t dw = 0;
    DrawCtx *ctx;
    area_t geometry;
    xcb_screen_t *s;
    char *text;

    if(!c->titlebar.sw)
        return;

    s = xcb_aux_get_screen(globalconf.connection,
                           c->titlebar.sw->phys_screen);

    switch(c->titlebar.position)
    {
      case Off:
        return;
      case Right:
      case Left:
        dw = xcb_generate_id(globalconf.connection);
        xcb_create_pixmap(globalconf.connection, s->root_depth,
                          dw,
                          s->root,
                          c->titlebar.sw->geometry.height,
                          c->titlebar.sw->geometry.width);
        ctx = draw_context_new(globalconf.connection, c->titlebar.sw->phys_screen,
                               c->titlebar.sw->geometry.height,
                               c->titlebar.sw->geometry.width,
                               dw);
        geometry.width = c->titlebar.sw->geometry.height;
        geometry.height = c->titlebar.sw->geometry.width;
        break;
      default:
        ctx = draw_context_new(globalconf.connection, c->titlebar.sw->phys_screen,
                               c->titlebar.sw->geometry.width,
                               c->titlebar.sw->geometry.height,
                               c->titlebar.sw->drawable);
        geometry = c->titlebar.sw->geometry;
        break;
    }

    text = titlebar_text(c);
    geometry.x = geometry.y = 0;
    draw_text(ctx, geometry, c->titlebar.text_align, 0,
              text, globalconf.screens[c->screen].styles.normal);
    p_delete(&text);

    switch(c->titlebar.position)
    {
      case Left:
        draw_rotate(ctx, c->titlebar.sw->drawable, ctx->height, ctx->width,
                    - M_PI_2, 0, c->titlebar.sw->geometry.height);
        xcb_free_pixmap(globalconf.connection, dw);
        break;
      case Right:
        draw_rotate(ctx, c->titlebar.sw->drawable, ctx->height, ctx->width,
                    M_PI_2, c->titlebar.sw->geometry.width, 0);
        xcb_free_pixmap(globalconf.connection, dw);
      default:
        break;
    }

    simplewindow_refresh_drawable(c->titlebar.sw);

    draw_context_delete(&ctx);
}

/** Update the titlebar geometry for a floating client.
 * \param c the client
 */
void
titlebar_update_geometry_floating(client_t *c)
{
    int width, x_offset = 0, y_offset = 0;

    if(!c->titlebar.sw)
        return;

    switch(c->titlebar.position)
    {
      default:
        return;
      case Off:
        return;
      case Top:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + c->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (c->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y - c->titlebar.sw->geometry.height,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = c->geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + c->geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (c->geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + x_offset,
                                 c->geometry.y + c->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + c->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (c->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x - c->titlebar.sw->geometry.width,
                                 c->geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        break;
      case Right:
        if(!c->titlebar.width)
            width = c->geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, c->geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + c->geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (c->geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 c->geometry.x + c->geometry.width + 2 * c->border,
                                 c->geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        break;
    }

    titlebar_draw(c);
}


/** Update the titlebar geometry for a tiled client.
 * \param c the client
 * \param geometry the geometry the client will receive
 */
void
titlebar_update_geometry(client_t *c, area_t geometry)
{
    int width, x_offset = 0 , y_offset = 0;

    if(!c->titlebar.sw)
        return;

    switch(c->titlebar.position)
    {
      default:
        return;
      case Off:
        return;
      case Top:
        if(!c->titlebar.width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + x_offset,
                                 geometry.y,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Bottom:
        if(!c->titlebar.width)
            width = geometry.width + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.width);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            x_offset = 2 * c->border + geometry.width - width;
            break;
          case AlignCenter:
            x_offset = (geometry.width - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + x_offset,
                                 geometry.y + geometry.height
                                     - c->titlebar.sw->geometry.height + 2 * c->border,
                                 width,
                                 c->titlebar.sw->geometry.height);
        break;
      case Left:
        if(!c->titlebar.width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x,
                                 geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        break;
      case Right:
        if(!c->titlebar.width)
            width = geometry.height + 2 * c->border;
        else
            width = MIN(c->titlebar.width, geometry.height);
        switch(c->titlebar.align)
        {
          default:
            break;
          case AlignRight:
            y_offset = 2 * c->border + geometry.height - width;
            break;
          case AlignCenter:
            y_offset = (geometry.height - width) / 2;
            break;
        }
        simplewindow_move_resize(c->titlebar.sw,
                                 geometry.x + geometry.width
                                     - c->titlebar.sw->geometry.width + 2 * c->border,
                                 geometry.y + y_offset,
                                 c->titlebar.sw->geometry.width,
                                 width);
        break;
    }

    titlebar_draw(c);
}

void
titlebar_position_set(titlebar_t *t, position_t p)
{
    if(!t->sw)
        return;

    if((t->position = p))
        xcb_map_window(globalconf.connection, t->sw->window);
    else
        xcb_unmap_window(globalconf.connection, t->sw->window);
}

/** Toggle the visibility of the focused window's titlebar.
 * \param screen screen number (unused)
 * \param arg unused argument
 * \ingroup ui_callback
 */
void
uicb_client_toggletitlebar(int screen __attribute__ ((unused)), char *arg __attribute__ ((unused)))
{
    client_t *c = globalconf.focus->client;

    if(!c || !c->titlebar.sw)
        return;

    if(!c->titlebar.position)
        titlebar_position_set(&c->titlebar, c->titlebar.dposition);
    else
        titlebar_position_set(&c->titlebar, Off);

    if(c->isfloating || layout_get_current(screen)->arrange == layout_floating)
        titlebar_update_geometry_floating(c);
    else
        globalconf.screens[c->screen].need_arrange = true;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
