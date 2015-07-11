/*
 * xrdb.c - X Resources DataBase communication functions
 *
 * Copyright © 2015 Yauhen Kirylau <yawghen@gmail.com>
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

#include "xrdb.h"
#include "globalconf.h"

#include <X11/Xresource.h>

#include <string.h>

/* \brief open X display and X Resources DB
 */
static void xrdb_init(void) {
  XrmInitialize(); // @TODO: it works without it but in docs it's said what it's
                   // needed
  if (!(globalconf.xrmdb = XrmGetDatabase(globalconf.display))) {

    /* taken from xpbiff: */
    /* >> what a hack; need to initialize dpy->db */
    (void)XGetDefault(globalconf.display, "", "");
    /**/

    if (!(globalconf.xrmdb = XrmGetDatabase(globalconf.display)))
      warn("Can't open xrdb\n");
  }
}

/* \brief get value from X Resources DataBase
 * \param L The Lua VM state.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lparam string xrdb class, ie "URxvt" or ""
 * \lparam string xrdb name, ie "background" or "color0"
 * \lreturn string xrdb value or nil if not exists. \
 */
int luaA_xrdb_get_value(lua_State *L) {
  if (!globalconf.xrmdb)
    xrdb_init();

  char *resource_type;
  int resource_code;
  XrmValue resource_value;
  const char *resource_class = luaL_checkstring(L, 1);
  const char *resource_name = luaL_checkstring(L, 2);

  resource_code = XrmGetResource(globalconf.xrmdb, resource_name, resource_class,
                                 &resource_type, &resource_value);
  if (resource_code && (strcmp(resource_type, "String") == 0)) {
    lua_pushstring(L, (char *)resource_value.addr);
    return 1;
  } else {
    if (strlen(resource_class))
      luaA_warn(L, "Failed to get xrdb value '%s' (class '%s').", resource_name, resource_class);
    else
      luaA_warn(L, "Failed to get xrdb value '%s'.", resource_name);
    return 0;
  }
}
