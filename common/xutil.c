/*
 * common/xutil.c - X-related useful functions
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

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "common/util.h"
#include "common/xutil.h"

Bool
xgettextprop(Display *disp, Window w, Atom atom, char *text, ssize_t textlen)
{
    char **list = NULL;
    int n;
    XTextProperty name;

    if(!text || !textlen)
        return False;

    text[0] = '\0';
    XGetTextProperty(disp, w, &name, atom);

    if(!name.nitems)
        return False;

    if(name.encoding == XA_STRING)
        a_strncpy(text, textlen, (char *) name.value, textlen - 1);

    else if(XmbTextPropertyToTextList(disp, &name, &list, &n) >= Success && n > 0 && *list)
    {
        a_strncpy(text, textlen, *list, textlen - 1);
        XFreeStringList(list);
    }

    text[textlen - 1] = '\0';
    XFree(name.value);

    return True;
}

unsigned int
xgetnumlockmask(Display *disp)
{
    XModifierKeymap *modmap;
    unsigned int mask = 0;
    int i, j;

    modmap = XGetModifierMapping(disp);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
            if(modmap->modifiermap[i * modmap->max_keypermod + j]
               == XKeysymToKeycode(disp, XK_Num_Lock))
                mask = (1 << i);

    XFreeModifiermap(modmap);

    return mask;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
