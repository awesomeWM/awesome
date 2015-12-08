#include "system.h"

#include <glibtop.h>
#include <glibtop/mem.h>

#include <glib.h>
#include <unistd.h>

int luaA_sys_get_meminfo(lua_State *L)
{
    glibtop_mem mem;
    glibtop_init();
    glibtop_get_mem(&mem);
    lua_pushinteger(L, mem.cached);
    return 1;
}
