/*
 * keybinding.c - Key bindings configuration management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Pierre Habouzit <madcoder@debian.org>
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

/* XStringToKeysym() */
#include <X11/Xlib.h>

#include "structs.h"
#include "common/refcount.h"
#include "common/array.h"
#include "keybinding.h"

ARRAY_TYPE(keybinding_t *, keybinding)

extern awesome_t globalconf;

static struct {
    keybinding_array_t by_code;
    keybinding_array_t by_sym;
} keys_g;

static void
keybinding_delete(keybinding_t **kbp)
{
    luaL_unref(globalconf.L, LUA_REGISTRYINDEX, (*kbp)->fct);
    p_delete(kbp);
}

DO_RCNT(keybinding_t, keybinding, keybinding_delete)
ARRAY_FUNCS(keybinding_t *, keybinding, keybinding_unref)
DO_LUA_NEW(static, keybinding_t, keybinding, "keybinding", keybinding_ref)
DO_LUA_GC(keybinding_t, keybinding, "keybinding", keybinding_unref)

static int
keybinding_ev_cmp(xcb_keysym_t keysym, xcb_keycode_t keycode,
                  unsigned long mod, const keybinding_t *k)
{
    if (k->keysym) {
        if (k->keysym != keysym)
            return k->keysym > keysym ? 1 : -1;
    }
    if (k->keycode) {
        if (k->keycode != keycode)
            return k->keycode > keycode ? 1 : -1;
    }
    return k->mod == mod ? 0 : (k->mod > mod ? 1 : -1);
}

static int
keybinding_cmp(const keybinding_t *k1, const keybinding_t *k2)
{
    assert ((k1->keysym && k2->keysym) || (k1->keycode && k2->keycode));
    assert ((!k1->keysym && !k2->keysym) || (!k1->keycode && !k2->keycode));

    if (k1->keysym != k2->keysym)
        return k2->keysym > k1->keysym ? 1 : -1;
    if (k1->keycode != k2->keycode)
        return k2->keycode > k1->keycode ? 1 : -1;
    return k1->mod == k2->mod ? 0 : (k2->mod > k1->mod ? 1 : -1);
}

/** Grab key on the root windows.
 * \param k The keybinding.
 */
static void
window_root_grabkey(keybinding_t *k)
{
    int phys_screen = 0;
    int nscreen = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
    xcb_screen_t *s;
    xcb_keycode_t kc;

    if((kc = k->keycode)
       || (k->keysym
           && (kc = xcb_key_symbols_get_keycode(globalconf.keysyms, k->keysym))))
        do
        {
            s = xutil_screen_get(globalconf.connection, phys_screen);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod | XCB_MOD_MASK_LOCK, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod | globalconf.numlockmask, kc, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
            xcb_grab_key(globalconf.connection, true, s->root,
                         k->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK, kc, XCB_GRAB_MODE_ASYNC,
                         XCB_GRAB_MODE_ASYNC);
        phys_screen++;
        } while(phys_screen < nscreen);
}

/** Ungrab key on the root windows.
 * \param k The keybinding.
 */
static void
window_root_ungrabkey(keybinding_t *k)
{
    int phys_screen = 0;
    int nscreen = xcb_setup_roots_length(xcb_get_setup(globalconf.connection));
    xcb_screen_t *s;
    xcb_keycode_t kc;

    if((kc = k->keycode)
       || (k->keysym && (kc = xcb_key_symbols_get_keycode(globalconf.keysyms, k->keysym))))
        do
        {
            s = xutil_screen_get(globalconf.connection, phys_screen);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod | XCB_MOD_MASK_LOCK);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod | globalconf.numlockmask);
            xcb_ungrab_key(globalconf.connection, kc, s->root,
                           k->mod | globalconf.numlockmask | XCB_MOD_MASK_LOCK);
        phys_screen++;
        } while(phys_screen < nscreen);
}

static void
keybinding_register_root(keybinding_t *k)
{
    keybinding_array_t *arr = k->keysym ? &keys_g.by_sym : &keys_g.by_code;
    int l = 0, r = arr->len;

    keybinding_ref(&k);

    while (l < r) {
        int i = (r + l) / 2;
        switch (keybinding_cmp(k, arr->tab[i])) {
          case -1: /* k < arr->tab[i] */
            r = i;
            break;
          case 0: /* k == arr->tab[i] */
            keybinding_unref(&arr->tab[i]);
            arr->tab[i] = k;
            return;
          case 1: /* k > arr->tab[i] */
            l = i + 1;
            break;
        }
    }

    keybinding_array_splice(arr, r, 0, &k, 1);
    window_root_grabkey(k);
}

static void
keybinding_unregister_root(keybinding_t **k)
{
    keybinding_array_t *arr = (*k)->keysym ? &keys_g.by_sym : &keys_g.by_code;
    int l = 0, r = arr->len;

    while (l < r) {
        int i = (r + l) / 2;
        switch (keybinding_cmp(*k, arr->tab[i])) {
          case -1: /* k < arr->tab[i] */
            r = i;
            break;
          case 0: /* k == arr->tab[i] */
            keybinding_array_take(arr, i);
            window_root_ungrabkey(*k);
            keybinding_unref(k);
            return;
          case 1: /* k > arr->tab[i] */
            l = i + 1;
            break;
        }
    }
}

/** Return the keysym from keycode.
 * \param detail The keycode received.
 * \param state The modifier state.
 * \return A keysym.
 */
xcb_keysym_t
key_getkeysym(xcb_keycode_t detail, uint16_t state)
{
    xcb_keysym_t k0, k1;

    /* 'col' (third parameter) is used to get the proper KeySym
     * according to modifier (XCB doesn't provide an equivalent to
     * XLookupString()).
     *
     * If Mod5 is ON we look into second group.
     */
    if(state & XCB_MOD_MASK_5)
    {
        k0 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 2);
        k1 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 3);
    }
    else
    {
        k0 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 0);
        k1 = xcb_key_symbols_get_keysym(globalconf.keysyms, detail, 1);
    }

    /* The numlock modifier is on and the second KeySym is a keypad
     * KeySym */
    if((state & globalconf.numlockmask) && xcb_is_keypad_key(k1))
    {
        /* The Shift modifier is on, or if the Lock modifier is on and
         * is interpreted as ShiftLock, use the first KeySym */
        if((state & XCB_MOD_MASK_SHIFT) ||
           (state & XCB_MOD_MASK_LOCK && (state & globalconf.shiftlockmask)))
            return k0;
        else
            return k1;
    }

    /* The Shift and Lock modifers are both off, use the first
     * KeySym */
    else if(!(state & XCB_MOD_MASK_SHIFT) && !(state & XCB_MOD_MASK_LOCK))
        return k0;

    /* The Shift modifier is off and the Lock modifier is on and is
     * interpreted as CapsLock */
    else if(!(state & XCB_MOD_MASK_SHIFT) &&
            (state & XCB_MOD_MASK_LOCK && (state & globalconf.capslockmask)))
        /* The first Keysym is used but if that KeySym is lowercase
         * alphabetic, then the corresponding uppercase KeySym is used
         * instead */
        return k1;

    /* The Shift modifier is on, and the Lock modifier is on and is
     * interpreted as CapsLock */
    else if((state & XCB_MOD_MASK_SHIFT) &&
            (state & XCB_MOD_MASK_LOCK && (state & globalconf.capslockmask)))
        /* The second Keysym is used but if that KeySym is lowercase
         * alphabetic, then the corresponding uppercase KeySym is used
         * instead */
        return k1;

    /* The Shift modifer is on, or the Lock modifier is on and is
     * interpreted as ShiftLock, or both */
    else if((state & XCB_MOD_MASK_SHIFT) ||
            (state & XCB_MOD_MASK_LOCK && (state & globalconf.shiftlockmask)))
        return k1;

    return XCB_NO_SYMBOL;
}


keybinding_t *
keybinding_find(const xcb_key_press_event_t *ev)
{
    const keybinding_array_t *arr = &keys_g.by_sym;
    int l, r, mod = XUTIL_MASK_CLEAN(ev->state);
    xcb_keysym_t keysym;

    /* get keysym ignoring shift and mod5 */
    keysym = key_getkeysym(ev->detail, ev->state & ~(XCB_MOD_MASK_SHIFT | XCB_MOD_MASK_5));

  again:
    l = 0;
    r = arr->len;
    while (l < r)
    {
        int i = (r + l) / 2;
        switch (keybinding_ev_cmp(keysym, ev->detail, mod, arr->tab[i]))
        {
          case -1: /* ev < arr->tab[i] */
            r = i;
            break;
          case 0: /* ev == arr->tab[i] */
            return arr->tab[i];
          case 1: /* ev > arr->tab[i] */
            l = i + 1;
            break;
        }
    }
    if (arr != &keys_g.by_code)
    {
        arr = &keys_g.by_code;
        goto again;
    }
    return NULL;
}

static void
__luaA_keystore(keybinding_t *key, const char *str)
{
    if(a_strlen(str))
    {
        if(*str != '#')
            key->keysym = XStringToKeysym(str);
        else
            key->keycode = atoi(str + 1);
    }
}

/** Define a global key binding. This key binding will always be available.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lparam A table with modifier keys.
 * \lparam A key name.
 * \lparam A function to execute.
 * \lreturn The keybinding.
 */
static int
luaA_keybinding_new(lua_State *L)
{
    size_t i, len;
    keybinding_t *k;
    const char *key;

    /* arg 2 is key mod table */
    luaA_checktable(L, 2);
    /* arg 3 is key */
    key = luaL_checkstring(L, 3);
    /* arg 4 is cmd to run */
    luaA_checkfunction(L, 4);

    /* get the last arg as function */
    k = p_new(keybinding_t, 1);
    __luaA_keystore(k, key);
    luaA_registerfct(L, &k->fct);

    len = lua_objlen(L, 2);
    for(i = 1; i <= len; i++)
    {
        lua_rawgeti(L, 2, i);
        k->mod |= xutil_key_mask_fromstr(luaL_checkstring(L, -1));
    }

    return luaA_keybinding_userdata_new(L, k);
}

/** Add a global key binding. This key binding will always be available.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A keybinding.
 */
static int
luaA_keybinding_add(lua_State *L)
{
    keybinding_t **k = luaA_checkudata(L, 1, "keybinding");
    keybinding_register_root(*k);
    return 0;
}

/** Remove a global key binding.
 * \param L The Lua VM state.
 *
 * \luastack
 * \lvalue A keybinding.
 */
static int
luaA_keybinding_remove(lua_State *L)
{
    keybinding_t **k = luaA_checkudata(L, 1, "keybinding");
    keybinding_unregister_root(k);
    return 0;
}

/** Convert a keybinding to a printable string.
 * \return A string.
 */
static int
luaA_keybinding_tostring(lua_State *L)
{
    keybinding_t **p = luaA_checkudata(L, 1, "keybinding");
    lua_pushfstring(L, "[keybinding udata(%p)]", *p);
    return 1;
}

const struct luaL_reg awesome_keybinding_methods[] =
{
    { "__call", luaA_keybinding_new },
    { NULL, NULL }
};
const struct luaL_reg awesome_keybinding_meta[] =
{
    { "add", luaA_keybinding_add },
    { "remove", luaA_keybinding_remove },
    { "__tostring", luaA_keybinding_tostring },
    { "__gc", luaA_keybinding_gc },
    { NULL, NULL },
};

