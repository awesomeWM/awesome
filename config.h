#ifndef _CONFIG_H_
#define _CONFIG_H_

#define AWESOME_LUA_LIB_PATH  "@AWESOME_LUA_LIB_PATH@"
#define XDG_CONFIG_DIR        "@XDG_CONFIG_DIR@"
#define AWESOME_THEMES_PATH   "@AWESOME_THEMES_PATH@"
#define AWESOME_ICON_PATH     "@AWESOME_ICON_PATH@"
#define AWESOME_DEFAULT_CONF  "@AWESOME_SYSCONFDIR@/rc.lua"

#cmakedefine WITH_WAYLAND
#cmakedefine WITH_DBUS
#cmakedefine WITH_XCB_ERRORS
#cmakedefine HAS_EXECINFO

#endif //_CONFIG_H_

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
