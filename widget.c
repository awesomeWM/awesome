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
#include "lua.h"

extern AwesomeConf globalconf;

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

/** Find a widget on a screen by its name.
 * \param name The widget name.
 * \return A widget pointer.
 */
widget_t *
widget_getbyname(const char *name)
{
    widget_node_t *widget;
    statusbar_t *sb;
    int screen;

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
            for(widget = sb->widgets; widget; widget = widget->next)
                if(!a_strcmp(name, widget->widget->name))
                    return widget->widget;

    return NULL;
}

/** Common function for button press event on widget.
 * It will look into configuration to find the callback function to call.
 * \param w The widget node.
 * \param statusbar The statusbar.
 * \param ev The button press event the widget received.
 */
static void
widget_common_button_press(widget_node_t *w,
                           statusbar_t *statusbar __attribute__ ((unused)),
                           xcb_button_press_event_t *ev)
{
    button_t *b;

    for(b = w->widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
            luaA_dofunction(globalconf.L, b->fct, 0);
}

/** Common tell function for widget, which only warn user that widget
 * cannot be told anything.
 * \param widget The widget.
 * \param new_value Unused argument.
 * \return The status of the command, which is always an error in this case.
 */
static widget_tell_status_t
widget_common_tell(widget_t *widget,
                   const char *property __attribute__ ((unused)),
                   const char *new_value __attribute__ ((unused)))
{
    warn("%s widget does not accept commands.\n", widget->name);
    return WIDGET_ERROR_CUSTOM;
}

/** Common function for creating a widget.
 * \param widget The allocated widget.
 */
void
widget_common_new(widget_t *widget)
{
    widget->align = AlignLeft;
    widget->tell = widget_common_tell;
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

void
widget_invalidate_statusbar_bywidget(widget_t *widget)
{
    int screen;
    statusbar_t *statusbar;
    widget_node_t *witer;

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
}

static int
luaA_widget_new(lua_State *L)
{
    const char *type;
    widget_t **widget, *w;
    WidgetConstructor *wc;
    int objpos;
    alignment_t align;

    luaA_checktable(L, 1);
    align = draw_align_get_from_str(luaA_getopt_string(L, 1, "align", "left"));

    type = luaA_getopt_string(L, 1, "type", NULL);

    /* \todo use type to call the WidgetConstructor and set ->tell*/
    if((wc = name_func_lookup(type, WidgetList)))
        w = wc(align);
    else
        return 0;

    widget = lua_newuserdata(L, sizeof(widget_t *));
    objpos = lua_gettop(L);
    *widget = w;

    /* \todo check that the name is unique */
    (*widget)->name = luaA_name_init(L);

    widget_ref(widget);

    /* repush obj on top */
    lua_pushvalue(L, objpos);
    return luaA_settype(L, "widget");
}

static int
luaA_widget_mouse(lua_State *L)
{
    size_t i, len;
    /* arg 1 is object */
    widget_t **widget = luaL_checkudata(L, 1, "widget");
    int b;
    button_t *button;

    /* arg 2 is modkey table */
    luaA_checktable(L, 2);
    /* arg 3 is mouse button */
    b = luaL_checknumber(L, 3);
    /* arg 4 is cmd to run */
    luaA_checkfunction(L, 4);

    button = p_new(button_t, 1);
    button->button = xutil_button_fromint(b);
    button->fct = luaL_ref(L, LUA_REGISTRYINDEX);

    len = lua_objlen(L, 2);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 2, i);
        button->mod |= xutil_keymask_fromstr(luaL_checkstring(L, -1));
    }

    button_list_push(&(*widget)->buttons, button);

    return 0;
}

void
widget_tell_managestatus(widget_t *widget, widget_tell_status_t status, const char *property)
{
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
        widget_invalidate_statusbar_bywidget(widget);
        break;
      case WIDGET_ERROR_CUSTOM:
        break;
    }
}

static int
luaA_widget_set(lua_State *L)
{
    widget_t **widget = luaL_checkudata(L, 1, "widget");
    const char *property, *value;
    widget_tell_status_t status;

    property = luaL_checkstring(L, 2);
    value = luaL_checkstring(L, 3);

    status = (*widget)->tell(*widget, property, value);
    widget_tell_managestatus(*widget, status, property);
    return 0;
}

static int
luaA_widget_gc(lua_State *L)
{
    widget_t **widget = luaL_checkudata(L, 1, "widget");
    widget_unref(widget);
    return 0;
}

static int
luaA_widget_tostring(lua_State *L)
{
    widget_t **p = luaL_checkudata(L, 1, "widget");
    lua_pushfstring(L, "[widget udata(%p) name(%s)]", *p, (*p)->name);
    return 1;
}

static int
luaA_widget_eq(lua_State *L)
{
    widget_t **t1 = luaL_checkudata(L, 1, "widget");
    widget_t **t2 = luaL_checkudata(L, 2, "widget");
    lua_pushboolean(L, (*t1 == *t2));
    return 1;
}

static int
luaA_widget_get(lua_State *L)
{
    statusbar_t *sb;
    widget_t **wobj;
    widget_node_t *widget;
    int i = 1, screen;
    bool add = true;
    widget_node_t *wlist = NULL, *witer;

    lua_newtable(L);

    for(screen = 0; screen < globalconf.screens_info->nscreen; screen++)
        for(sb = globalconf.screens[screen].statusbar; sb; sb = sb->next)
            for(widget = sb->widgets; widget; widget = widget->next)
            {
                for(witer = wlist; witer; witer = witer->next)
                    if(witer->widget == widget->widget)
                    {
                        add = false;
                        break;
                    }
                if(add)
                {
                    witer = p_new(widget_node_t, 1);
                    wobj = lua_newuserdata(L, sizeof(tag_t *));
                    witer->widget = *wobj = widget->widget;
                    widget_ref(&widget->widget);
                    widget_node_list_push(&wlist, witer);
                    /* ref again for the list */
                    widget_ref(&widget->widget);
                    luaA_settype(L, "widget");
                    lua_rawseti(L, -2, i++);
                }
                add = true;
            }

    widget_node_list_wipe(&wlist);

    return 1;
}

static int
luaA_widget_name_set(lua_State *L)
{
    widget_t **widget = luaL_checkudata(L, 1, "widget");
    const char *name = luaL_checkstring(L, 2);
    p_delete(&(*widget)->name);
    (*widget)->name = a_strdup(name);
    return 0;
}

static int
luaA_widget_name_get(lua_State *L)
{
    widget_t **widget = luaL_checkudata(L, 1, "widget");
    lua_pushstring(L, (*widget)->name);
    return 1;
}

const struct luaL_reg awesome_widget_methods[] =
{
    { "new", luaA_widget_new },
    { "get", luaA_widget_get },
    { NULL, NULL }
};
const struct luaL_reg awesome_widget_meta[] =
{
    { "mouse", luaA_widget_mouse },
    { "set", luaA_widget_set },
    { "name_set", luaA_widget_name_set },
    { "name_get", luaA_widget_name_get },
    { "__gc", luaA_widget_gc },
    { "__eq", luaA_widget_eq },
    { "__tostring", luaA_widget_tostring },
    { NULL, NULL }
};

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
