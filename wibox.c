/*
 * wibox.c - statusbar functions
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

#include "screen.h"
#include "wibox.h"
#include "statusbar.h"
#include "titlebar.h"
#include "client.h"

extern awesome_t globalconf;

DO_LUA_NEW(extern, wibox_t, wibox, "wibox", wibox_ref)
DO_LUA_GC(wibox_t, wibox, "wibox", wibox_unref)
DO_LUA_EQ(wibox_t, wibox, "wibox")

/** Refresh all wiboxes.
 */
void
wibox_refresh(void)
{
    statusbar_refresh();
    titlebar_refresh();
}

/** Convert a wibox to a printable string.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A wibox.
 */
static int
luaA_wibox_tostring(lua_State *L)
{
    wibox_t **p = luaA_checkudata(L, 1, "wibox");
    lua_pushfstring(L, "[wibox udata(%p)]", *p);
    return 1;
}

/** Create a new wibox.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with optionaly defined values:
 * position, align, fg, bg, border_widht, border_color, width and height.
 * \lreturn A brand new wibox.
 */
int
luaA_wibox_new(lua_State *L)
{
    wibox_t *w;
    const char *buf;
    size_t len;
    xcolor_init_request_t reqs[3];
    int8_t i, reqs_nbr = -1;

    luaA_checktable(L, 2);

    w = p_new(wibox_t, 1);

    w->colors.fg = globalconf.colors.fg;
    if((buf = luaA_getopt_lstring(L, 2, "fg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->colors.fg, buf, len);

    w->colors.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->colors.bg, buf, len);

    w->colors.bg = globalconf.colors.bg;
    if((buf = luaA_getopt_lstring(L, 2, "bg", NULL, &len)))
        reqs[++reqs_nbr] = xcolor_init_unchecked(&w->colors.bg, buf, len);

    buf = luaA_getopt_lstring(L, 2, "align", "left", &len);
    w->align = draw_align_fromstr(buf, len);

    w->geometry.width = luaA_getopt_number(L, 2, "width", 0);
    w->geometry.height = luaA_getopt_number(L, 2, "height", 0);
    if(w->geometry.height <= 0)
        /* 1.5 as default factor, it fits nice but no one knows why */
        w->geometry.height = 1.5 * globalconf.font->height;

    buf = luaA_getopt_lstring(L, 2, "position", "top", &len);
    w->position = position_fromstr(buf, len);

    w->screen = SCREEN_UNDEF;
    w->isvisible = true;

    for(i = 0; i <= reqs_nbr; i++)
        xcolor_init_reply(reqs[i]);

    return luaA_wibox_userdata_new(L, w);
}

/** Wibox object.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield geometry Geometry table.
 * \lfield screen Screen number.
 * \lfield client The client attached to this titlebar.
 * \lfield border_width Border width.
 * \lfield border_color Border color.
 * \lfield align The alignment.
 * \lfield fg Foreground color.
 * \lfield bg Background color.
 * \lfield position The position.
 * \lfield ontop On top of other windows.
 */
static int
luaA_wibox_index(lua_State *L)
{
    size_t len;
    wibox_t **wibox = luaA_checkudata(L, 1, "wibox");
    const char *attr = luaL_checklstring(L, 2, &len);

    if(luaA_usemetatable(L, 1, 2))
        return 1;

    switch(a_tokenize(attr, len))
    {
        client_t *c;

      case A_TK_GEOMETRY:
        luaA_pusharea(L, (*wibox)->geometry);
        break;
      case A_TK_VISIBLE:
        lua_pushboolean(L, (*wibox)->isvisible);
        break;
      case A_TK_CLIENT:
        if((c = client_getbytitlebar(*wibox)))
            return luaA_client_userdata_new(L, c);
        else
            return 0;
      case A_TK_SCREEN:
        if((*wibox)->screen == SCREEN_UNDEF)
            return 0;
        lua_pushnumber(L, (*wibox)->screen + 1);
        break;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, (*wibox)->border.width);
        break;
      case A_TK_BORDER_COLOR:
        luaA_pushcolor(L, &(*wibox)->border.color);
        break;
      case A_TK_ALIGN:
        lua_pushstring(L, draw_align_tostr((*wibox)->align));
        break;
      case A_TK_FG:
        luaA_pushcolor(L, &(*wibox)->colors.fg);
        break;
      case A_TK_BG:
        luaA_pushcolor(L, &(*wibox)->colors.bg);
        break;
      case A_TK_POSITION:
        lua_pushstring(L, position_tostr((*wibox)->position));
        break;
      case A_TK_ONTOP:
        lua_pushboolean(L, (*wibox)->ontop);
        break;
      default:
        return 0;
    }

    return 1;
}

/** Get or set the wibox widgets.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam None, or a table of widgets to set.
 * \lreturn The current wibox widgets.
*/
static int
luaA_wibox_widgets(lua_State *L)
{
    wibox_t **wibox = luaA_checkudata(L, 1, "wibox");

    if(lua_gettop(L) == 2)
    {
        luaA_widget_set(L, 2, *wibox, &(*wibox)->widgets);
        (*wibox)->need_update = true;
        (*wibox)->mouse_over = NULL;
        return 1;
    }
    return luaA_widget_get(L, (*wibox)->widgets);
}

/** Wibox newindex.
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 */
static int
luaA_wibox_newindex(lua_State *L)
{
    size_t len;
    wibox_t **wibox = luaA_checkudata(L, 1, "wibox");
    const char *buf, *attr = luaL_checklstring(L, 2, &len);
    awesome_token_t tok;

    switch((tok = a_tokenize(attr, len)))
    {
        bool b;

      case A_TK_FG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*wibox)->colors.fg, buf, len)))
            {
                (*wibox)->sw.ctx.fg = (*wibox)->colors.fg;
                (*wibox)->need_update = true;
            }
        break;
      case A_TK_BG:
        if((buf = luaL_checklstring(L, 3, &len)))
            if(xcolor_init_reply(xcolor_init_unchecked(&(*wibox)->colors.bg, buf, len)))
            {
                (*wibox)->sw.ctx.bg = (*wibox)->colors.bg;
                (*wibox)->need_update = true;
            }
        break;
      case A_TK_POSITION:
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            return luaA_titlebar_newindex(L, *wibox, tok);
          case WIBOX_TYPE_STATUSBAR:
            return luaA_statusbar_newindex(L, *wibox, tok);
          case WIBOX_TYPE_NONE:
            if((buf = luaL_checklstring(L, 3, &len)))
                (*wibox)->position = position_fromstr(buf, len);
            break;
        }
        break;
      case A_TK_CLIENT:
        /* first detach */
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_NONE:
          case WIBOX_TYPE_TITLEBAR:
            break;
          case WIBOX_TYPE_STATUSBAR:
            statusbar_detach(*wibox);
            break;
        }
        if(lua_isnil(L, 3))
            titlebar_client_detach(client_getbytitlebar(*wibox));
        else
        {
            client_t **c = luaA_checkudata(L, 3, "client");
            titlebar_client_attach(*c, *wibox);
        }
        break;
      case A_TK_SCREEN:
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_NONE:
          case WIBOX_TYPE_STATUSBAR:
            break;
          case WIBOX_TYPE_TITLEBAR:
            titlebar_client_detach(client_getbytitlebar(*wibox));
            break;
        }
        if(lua_isnil(L, 3))
            statusbar_detach(*wibox);
        else
        {
            int screen = luaL_checknumber(L, 3) - 1;
            luaA_checkscreen(screen);
            statusbar_attach(*wibox, &globalconf.screens[screen]);
        }
        break;
      case A_TK_ONTOP:
        b = luaA_checkboolean(L, 3);
        if(b != (*wibox)->ontop)
        {
            (*wibox)->ontop = b;
            client_stack();
        }
        break;
      case A_TK_VISIBLE:
        b = luaA_checkboolean(L, 3);
        if(b != (*wibox)->isvisible)
        {
            (*wibox)->isvisible = b;
            switch((*wibox)->type)
            {
              case WIBOX_TYPE_STATUSBAR:
                statusbar_position_update(*wibox);
                break;
              case WIBOX_TYPE_NONE:
              case WIBOX_TYPE_TITLEBAR:
                break;
            }
        }
        break;
      default:
        switch((*wibox)->type)
        {
          case WIBOX_TYPE_TITLEBAR:
            return luaA_titlebar_newindex(L, *wibox, tok);
          case WIBOX_TYPE_STATUSBAR:
            return luaA_statusbar_newindex(L, *wibox, tok);
          case WIBOX_TYPE_NONE:
            break;
        }
    }

    return 0;
}

const struct luaL_reg awesome_wibox_methods[] =
{
    { "__call", luaA_wibox_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_wibox_meta[] =
{
    { "widgets", luaA_wibox_widgets },
    { "__index", luaA_wibox_index },
    { "__newindex", luaA_wibox_newindex },
    { "__gc", luaA_wibox_gc },
    { "__eq", luaA_wibox_eq },
    { "__tostring", luaA_wibox_tostring },
    { NULL, NULL },
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
