/*
 * common/xutil.c - X-related useful functions
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include "common/xutil.h"

/* XCB doesn't provide keysyms definition */
#include <X11/keysym.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>

uint16_t
xutil_key_mask_fromstr(const char *keyname)
{
    if (A_STREQ(keyname, "Shift"))
      return XCB_MOD_MASK_SHIFT;
    if (A_STREQ(keyname, "Lock"))
      return XCB_MOD_MASK_LOCK;
    if (A_STREQ(keyname, "Ctrl") || A_STREQ(keyname, "Control"))
      return XCB_MOD_MASK_CONTROL;
    if (A_STREQ(keyname, "Mod1"))
      return XCB_MOD_MASK_1;
    if(A_STREQ(keyname, "Mod2"))
      return XCB_MOD_MASK_2;
    if(A_STREQ(keyname, "Mod3"))
      return XCB_MOD_MASK_3;
    if(A_STREQ(keyname, "Mod4"))
      return XCB_MOD_MASK_4;
    if(A_STREQ(keyname, "Mod5"))
      return XCB_MOD_MASK_5;
    if(A_STREQ(keyname, "Any"))
      /* this is misnamed but correct */
      return XCB_BUTTON_MASK_ANY;
    return XCB_NO_SYMBOL;
}

void
xutil_key_mask_tostr(uint16_t mask, const char **name, size_t *len)
{
    switch(mask)
    {
#define SET_RESULT(res) \
        *name = #res; \
        *len = sizeof(#res) - 1; \
        return;
      case XCB_MOD_MASK_SHIFT:   SET_RESULT(Shift)
      case XCB_MOD_MASK_LOCK:    SET_RESULT(Lock)
      case XCB_MOD_MASK_CONTROL: SET_RESULT(Control)
      case XCB_MOD_MASK_1:       SET_RESULT(Mod1)
      case XCB_MOD_MASK_2:       SET_RESULT(Mod2)
      case XCB_MOD_MASK_3:       SET_RESULT(Mod3)
      case XCB_MOD_MASK_4:       SET_RESULT(Mod4)
      case XCB_MOD_MASK_5:       SET_RESULT(Mod5)
      case XCB_BUTTON_MASK_ANY:  SET_RESULT(Any)
      default:                   SET_RESULT(Unknown)
#undef SET_RESULT
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
