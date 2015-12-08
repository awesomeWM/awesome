#ifndef AWESOME_SYSTEM_H
#define AWESOME_SYSTEM_H

#include <lua.h>

void sys_init(void);
int luaA_sys_get_meminfo(lua_State *L);

#endif