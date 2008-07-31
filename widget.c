/*
 * widget.c - widget managing
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

#include <math.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "widget.h"
#include "titlebar.h"
#include "common/atoms.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, widget_t, widget, "widget", widget_ref)
DO_LUA_GC(widget_t, widget, "widget", widget_unref)
DO_LUA_EQ(widget_t, widget, "widget")

#include "widgetgen.h"

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

/** Common function for button press event on widget.
 * It will look into configuration to find the callback function to call.
 * \param w The widget node.
 * \param ev The button press event the widget received.
 * \param screen The screen number.
 * \param p The object where user clicked.
 * \param type The object type.
 */
static void
widget_common_button_press(widget_node_t *w,
                           xcb_button_press_event_t *ev,
                           int screen __attribute__ ((unused)),
                           void *p,
                           awesome_type_t type)
{
    button_t *b;
    
    for(b = w->widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
        {
            luaA_pushpointer(globalconf.L, p, type);
            luaA_dofunction(globalconf.L, b->fct, 1, 0);
        }
}

/** Render a list of widgets.
 * \param wnode The list of widgets.
 * \param ctx The draw context where to render.
 * \param rotate_px The rotate pixmap: where to rotate and render the final
 * \param screen The logical screen used to render.
 * \param position The object position.
 * \param x The x coordinates of the object.
 * \param y The y coordinates of the object.
 * pixmap when the object position is right or left.
 * \param object The object pointer.
 * \param type The object type.
 * \todo Remove GC.
 */
void
widget_render(widget_node_t *wnode, draw_context_t *ctx, xcb_gcontext_t gc, xcb_pixmap_t rotate_px,
              int screen, position_t position,
              int x, int y, void *object, awesome_type_t type)
{
    xcb_pixmap_t rootpix;
    xcb_screen_t *s;
    widget_node_t *w;
    int left = 0, right = 0;
    char *data;
    xcb_get_property_reply_t *prop_r;
    xcb_get_property_cookie_t prop_c;
    area_t rectangle = { 0, 0, 0, 0 };

    rectangle.width = ctx->width;
    rectangle.height = ctx->height;

    if(ctx->bg.alpha != 0xffff)
    {
        s = xutil_screen_get(globalconf.connection, ctx->phys_screen);
        prop_c = xcb_get_property_unchecked(globalconf.connection, false, s->root, _XROOTPMAP_ID,
                                            PIXMAP, 0, 1);
        if((prop_r = xcb_get_property_reply(globalconf.connection, prop_c, NULL)))
        {
            if((data = xcb_get_property_value(prop_r))
               && (rootpix = *(xcb_pixmap_t *) data))
               switch(position)
               {
                 case Left:
                   draw_rotate(ctx,
                               rootpix, ctx->pixmap,
                               s->width_in_pixels, s->height_in_pixels,
                               ctx->width, ctx->height,
                               M_PI_2,
                               y + ctx->width,
                               - x);
                   break;
                 case Right:
                   draw_rotate(ctx,
                               rootpix, ctx->pixmap,
                               s->width_in_pixels, s->height_in_pixels,
                               ctx->width, ctx->height,
                               - M_PI_2,
                               - y,
                               x + ctx->height);
                   break;
                 default:
                   xcb_copy_area(globalconf.connection, rootpix,
                                 rotate_px, gc,
                                 x, y,
                                 0, 0, 
                                 ctx->width, ctx->height);
                   break;
               }
            p_delete(&prop_r);
        }
    }

    draw_rectangle(ctx, rectangle, 1.0, true, &ctx->bg);

    for(w = wnode; w; w = w->next)
        if(w->widget->isvisible && w->widget->align == AlignLeft)
            left += w->widget->draw(ctx, screen, w, left, (left + right), object, type);

    /* renders right widget from last to first */
    for(w = *widget_node_list_last(&wnode); w; w = w->prev)
        if(w->widget->isvisible && w->widget->align == AlignRight)
            right += w->widget->draw(ctx, screen, w, right, (left + right), object, type);

    for(w = wnode; w; w = w->next)
        if(w->widget->isvisible && w->widget->align == AlignFlex)
            left += w->widget->draw(ctx, screen, w, left, (left + right), object, type);

    switch(position)
    {
        case Right:
          draw_rotate(ctx, ctx->pixmap, rotate_px,
                      ctx->width, ctx->height,
                      ctx->height, ctx->width,
                      M_PI_2, ctx->height, 0);
          break;
        case Left:
          draw_rotate(ctx, ctx->pixmap, rotate_px,
                      ctx->width, ctx->height,
                      ctx->height, ctx->width,
                      - M_PI_2, 0, ctx->width);
          break;
        default:
          break;
    }
}


/** Common function for creating a widget.
 * \param widget The allocated widget.
 */
void
widget_common_new(widget_t *widget)
{
    widget->align = AlignLeft;
    widget->button_press = widget_common_button_press;
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
    widget_node_t *widget;

    for(statusbar = globalconf.screens[screen].statusbar;
        statusbar;
        statusbar = statusbar->next)
        for(widget = statusbar->widgets; widget; widget = widget->next)
            if(widget->widget->cache_flags & flags)
            {
                statusbar->need_update = true;
                break;
            }
}

/** Set a statusbar needs update because it has widget, or redraw a titlebar.
 * \todo Probably needs more optimization.
 * \param widget The widget to look for.
 */
void
widget_invalidate_bywidget(widget_t *widget)
{
    int screen;
    statusbar_t *statusbar;
    widget_node_t *witer;
    client_t *c;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(statusbar = globalconf.screens[screen].statusbar;
            statusbar;
            statusbar = statusbar->next)
            for(witer = statusbar->widgets; witer; witer = witer->next)
                if(witer->widget == widget)
                {
                    statusbar->need_update = true;
                    break;
                }

    for(c = globalconf.clients; c; c = c->next)
        if(c->titlebar)
            for(witer = c->titlebar->widgets; witer; witer = witer->next)
                if(witer->widget == widget)
                    c->titlebar->need_update = true;
}

/** Create a new widget.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with at least a name and a type value. Optional attributes
 * are: align.
 * \lreturn A brand new widget.
 */
static int
luaA_widget_new(lua_State *L)
{
    const char *buf, *type;
    widget_t *w = NULL;
    widget_constructor_t *wc;
    alignment_t align;
    size_t len;

    luaA_checktable(L, 2);

    buf = luaA_getopt_lstring(L, 2, "align", "left", &len);
    align = draw_align_fromstr(buf, len);

    if(!(buf = luaA_getopt_string(L, 2, "name", NULL)))
        luaL_error(L, "object widget must have a name");

    type = luaA_getopt_string(L, 2, "type", NULL);

    if((wc = name_func_lookup(type, WidgetList)))
        w = wc(align);
    else
        luaL_error(L, "unkown widget type: %s", type);

    w->type = wc;

    /* Set visible by default. */
    w->isvisible = true;

    w->name = a_strdup(buf);

    return luaA_widget_userdata_new(L, w);
}

/** Add a mouse button bindings to a widget.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A widget.
 * \lparam A mouse button bindings object.
 */
static int
luaA_widget_mouse_add(lua_State *L)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    button_t **b = luaA_checkudata(L, 2, "mouse");

    button_list_push(&(*widget)->buttons, *b);
    button_ref(b);

    return 0;
}

/** Remove a mouse button bindings from a widget.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A widget.
 * \lparam A mouse button bindings object.
 */
static int
luaA_widget_mouse_remove(lua_State *L)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    button_t **b = luaA_checkudata(L, 2, "mouse");

    button_list_detach(&(*widget)->buttons, *b);
    button_unref(b);

    return 0;
}

/** Convert a widget into a printable string.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A widget.
 */
static int
luaA_widget_tostring(lua_State *L)
{
    widget_t **p = luaA_checkudata(L, 1, "widget");
    lua_pushfstring(L, "[widget udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

/** Generic widget.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield visible The widget visibility.
 * \lfield name The widget name.
 */
static int
luaA_widget_index(lua_State *L)
{
    size_t len;
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    const char *buf = luaL_checklstring(L, 2, &len);
    awesome_token_t token;

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch((token = a_tokenize(buf, len)))
    {
      case A_TK_VISIBLE:
        lua_pushboolean(L, (*widget)->isvisible);
        return 1;
      case A_TK_NAME:
        lua_pushstring(L, (*widget)->name);
        return 1;
      default:
        break;
    }

    return (*widget)->index ? (*widget)->index(L, token) : 0;
}

/** Generic widget newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_widget_newindex(lua_State *L)
{
    size_t len;
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    const char *buf = luaL_checklstring(L, 2, &len);
    awesome_token_t token;

    switch((token = a_tokenize(buf, len)))
    {
      case A_TK_VISIBLE:
        (*widget)->isvisible = luaA_checkboolean(L, 3);
        return 0;
      default:
        break;
    }

    return (*widget)->newindex ? (*widget)->newindex(L, token) : 0;
}

const struct luaL_reg awesome_widget_methods[] =
{
    { "__call", luaA_widget_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_widget_meta[] =
{
    { "mouse_add", luaA_widget_mouse_add },
    { "mouse_remove", luaA_widget_mouse_remove },
    { "__index", luaA_widget_index },
    { "__newindex", luaA_widget_newindex },
    { "__gc", luaA_widget_gc },
    { "__eq", luaA_widget_eq },
    { "__tostring", luaA_widget_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
