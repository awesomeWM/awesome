#ifndef _CONFIG_H_
#define _CONFIG_H_

#define AWESOME_LUA_LIB_PATH  "@AWESOME_LUA_LIB_PATH@"
#define XDG_CONFIG_DIR        "@XDG_CONFIG_DIR@"
#define AWESOME_IS_BIG_ENDIAN @AWESOME_IS_BIG_ENDIAN@

#cmakedefine WITH_DBUS
#cmakedefine HAS_EXECINFO
#cmakedefine HAS___BUILTIN_CLZ

#endif //_CONFIG_H_
