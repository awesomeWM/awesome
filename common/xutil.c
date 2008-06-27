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

/* asprintf() */
#define _GNU_SOURCE

#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>

#include "common/util.h"
#include "common/xutil.h"

/** Get the string value of an atom.
 * \param conn X connection
 * \param w window
 * \param atoms atoms cache
 * \param atom the atom
 * \param text buffer to fill
 * \return true on sucess, falsse on failure
 */
bool
xutil_gettextprop(xcb_connection_t *conn, xcb_window_t w,
                  xutil_atom_cache_array_t *atoms,
                  xcb_atom_t atom, char **text)
{
    xcb_get_property_cookie_t prop_c;
    xcb_get_property_reply_t *prop_r;
    void *prop_val;

    if(!text)
        return false;

    prop_c = xcb_get_property_unchecked(conn, false,
                                        w, atom,
                                        XCB_GET_PROPERTY_TYPE_ANY,
                                        0L, 1000000L);

    prop_r = xcb_get_property_reply(conn, prop_c, NULL);

    if(!prop_r || !prop_r->value_len || prop_r->format != 8)
    {
        p_delete(&prop_r);
        return false;
    }

    prop_val = xcb_get_property_value(prop_r);

    /* Check whether the returned property value is just an ascii
     * string or utf8 string.  At the moment it doesn't handle
     * COMPOUND_TEXT and multibyte but it's not needed...  */
    if(prop_r->type == STRING ||
       prop_r->type == xutil_intern_atom_reply(conn, atoms,
                                               xutil_intern_atom(conn, atoms,
                                                                 "UTF8_STRING")))
    {
        *text = p_new(char, prop_r->value_len + 1);
        /* use memcpy() because prop_val may not be \0 terminated */
        memcpy(*text, prop_val, prop_r->value_len);
        (*text)[prop_r->value_len] = '\0';
    }

    p_delete(&prop_r);

    return true;
}

void
xutil_getlockmask(xcb_connection_t *conn, xcb_key_symbols_t *keysyms,
                  unsigned int *numlockmask, unsigned int *shiftlockmask,
                  unsigned int *capslockmask)
{
    xcb_get_modifier_mapping_reply_t *modmap_r;
    xcb_keycode_t *modmap, kc;
    unsigned int mask;
    int i, j;

    modmap_r = xcb_get_modifier_mapping_reply(conn,
                                              xcb_get_modifier_mapping_unchecked(conn),
                                              NULL);

    modmap = xcb_get_modifier_mapping_keycodes(modmap_r);

    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap_r->keycodes_per_modifier; j++)
        {
            kc = modmap[i * modmap_r->keycodes_per_modifier + j];
            mask = (1 << i);

            if(numlockmask != NULL
               && kc == xcb_key_symbols_get_keycode(keysyms, XK_Num_Lock))
                *numlockmask = mask;
            else if(shiftlockmask != NULL
                    && kc == xcb_key_symbols_get_keycode(keysyms, XK_Shift_Lock))
                *shiftlockmask = mask;
            else if(capslockmask != NULL
                    && kc == xcb_key_symbols_get_keycode(keysyms, XK_Caps_Lock))
                *capslockmask = mask;
        }

    p_delete(&modmap_r);
}

/** Equivalent to 'XGetTransientForHint' which is actually a
 * 'XGetWindowProperty' which gets the WM_TRANSIENT_FOR property of
 * the specified window
 * \param c X connection
 * \param win get the property from this window
 * \param prop_win returns the WM_TRANSIENT_FOR property of win
 * \return return true if successfull
 */
bool
xutil_get_transient_for_hint(xcb_connection_t *c, xcb_window_t win,
                             xcb_window_t *prop_win)
{
    xcb_get_property_reply_t *t_hint_r;

    /* Use checked because the error handler should not take care of
     * this error as we only return a boolean */
    t_hint_r = xcb_get_property_reply(c,
                                      xcb_get_property_unchecked(c, false, win,
                                                                 WM_TRANSIENT_FOR,
                                                                 WINDOW, 0, 1),
                                      NULL);

    if(!t_hint_r || t_hint_r->type != WINDOW || t_hint_r->format != 32 ||
       t_hint_r->length == 0)
    {
        *prop_win = XCB_NONE;
        p_delete(&t_hint_r);
        return false;
    }

    *prop_win = *((xcb_window_t *) xcb_get_property_value(t_hint_r));
    p_delete(&t_hint_r);

    return true;
}

/** Send an unchecked InternAtom request if it is not already in the
 * cache, in the second case it stores the cache entry (an ordered
 * array).
 * \param c X connection.
 * \param atoms Atoms cache, or NULL if no cache.
 * \param name Atom name.
 * \return A request structure.
 */
xutil_intern_atom_request_t
xutil_intern_atom(xcb_connection_t *c,
                  xutil_atom_cache_array_t *atoms,
                  const char *name)
{
    xutil_intern_atom_request_t atom_req;
    int l = 0, r;

    p_clear(&atom_req, 1);

    /* Check if this atom is present in the cache ordered array */
    if(atoms)
    {
        r = atoms->len;
        while (l < r)
        {
            int i = (r + l) / 2;
            switch (a_strcmp(name, atoms->tab[i]->name))
            {
              case -1: /* ev < atoms->tab[i] */
                r = i;
                break;
              case 0: /* ev == atoms->tab[i] */
                atom_req.cache_hit = true;
                atom_req.cache = atoms->tab[i];
                return atom_req;
              case 1: /* ev > atoms->tab[i] */
                l = i + 1;
                break;
            }
        }
    }

    /* Otherwise send an InternAtom request to the server */
    atom_req.name = a_strdup(name);
    atom_req.cookie = xcb_intern_atom_unchecked(c, false, a_strlen(name), name);

    return atom_req;
}

/** Treat the reply which may be a cache entry or a reply from
 * InternAtom request (cookie), in the second case, add the atom to
 * the cache.
 * \param c X connection.
 * \param atoms Atoms cache or NULL if no cache.
 * \param atom_req Atom request.
 * \return A brand new xcb_atom_t.
 */
xcb_atom_t
xutil_intern_atom_reply(xcb_connection_t *c,
                        xutil_atom_cache_array_t *atoms,
                        xutil_intern_atom_request_t atom_req)
{
    xcb_intern_atom_reply_t *atom_rep;
    xutil_atom_cache_t *atom_cache;
    xcb_atom_t atom = 0;
    int l = 0, r;

    /* If the atom is present in the cache, just returns the
     * atom... */
    if(atom_req.cache_hit)
        return atom_req.cache->atom;

    /* Get the reply from InternAtom request */
    if(!(atom_rep = xcb_intern_atom_reply(c, atom_req.cookie, NULL)))
        goto bailout;

    atom = atom_rep->atom;

    if(atoms)
    {
        r = atoms->len;

        /* Create a new atom cache entry */
        atom_cache = p_new(xutil_atom_cache_t, 1);
        atom_cache->atom = atom_rep->atom;
        atom_cache->name = atom_req.name;

        while (l < r)
        {
            int i = (r + l) / 2;
            switch(a_strcmp(atom_cache->name, atoms->tab[i]->name))
            {
              case -1: /* k < atoms->tab[i] */
                r = i;
                break;
              case 0: /* k == atoms->tab[i] cannot append */
                assert(0);
              case 1: /* k > atoms->tab[i] */
                l = i + 1;
                break;
            }
        }

        xutil_atom_cache_array_splice(atoms, r, 0, &atom_cache, 1);
    }

bailout:
    p_delete(&atom_rep);

    return atom;
}

/* Delete a atom cache entry.
 * \param entry A cache entry.
 */
void
xutil_atom_cache_delete(xutil_atom_cache_t **entry)
{
    p_delete(&(*entry)->name);
    p_delete(entry);
}

class_hint_t *
xutil_get_class_hint(xcb_connection_t *conn, xcb_window_t win)
{
    xcb_get_property_reply_t *class_hint_r;
    xcb_get_property_cookie_t class_hint_c;
    char *data;
    int len_name, len_class;
    class_hint_t *ch;

    class_hint_c = xcb_get_property_unchecked(conn, false, win, WM_CLASS,
                                              STRING, 0L, 2048L);
    ch = p_new(class_hint_t, 1);

    class_hint_r = xcb_get_property_reply(conn, class_hint_c, NULL);

    if(!class_hint_r
       || class_hint_r->type != STRING
       || class_hint_r->format != 8)
    {
        p_delete(&class_hint_r);
	return NULL;
    }

    data = xcb_get_property_value(class_hint_r);

    len_name = a_strlen(data);
    len_class = a_strlen(data + len_name + 1);

    ch->res_name = a_strndup(data, len_name);
    ch->res_class = a_strndup(data + len_name + 1, len_class);

    p_delete(&class_hint_r);

    return ch;
}

/* Number of different errors */
#define ERRORS_NBR 256

/* Number of different events */
#define EVENTS_NBR 126

void
xutil_set_error_handler_catch_all(xcb_event_handlers_t *evenths,
                                  xcb_generic_error_handler_t handler,
                                  void *data)
{
    int err_num;
    for(err_num = 0; err_num < ERRORS_NBR; err_num++)
	xcb_set_error_handler(evenths, err_num, handler, data);
}

const char *
xutil_error_label[] =
{
    "Success",
    "BadRequest",
    "BadValue",
    "BadWindow",
    "BadPixmap",
    "BadAtom",
    "BadCursor",
    "BadFont",
    "BadMatch",
    "BadDrawable",
    "BadAccess",
    "BadAlloc",
    "BadColor",
    "BadGC",
    "BadIDChoice",
    "BadName",
    "BadLength",
    "BadImplementation",
};

const char *
xutil_request_label[] =
{
    "None",
    "CreateWindow",
    "ChangeWindowAttributes",
    "GetWindowAttributes",
    "DestroyWindow",
    "DestroySubwindows",
    "ChangeSaveSet",
    "ReparentWindow",
    "MapWindow",
    "MapSubwindows",
    "UnmapWindow",
    "UnmapSubwindows",
    "ConfigureWindow",
    "CirculateWindow",
    "GetGeometry",
    "QueryTree",
    "InternAtom",
    "GetAtomName",
    "ChangeProperty",
    "DeleteProperty",
    "GetProperty",
    "ListProperties",
    "SetSelectionOwner",
    "GetSelectionOwner",
    "ConvertSelection",
    "SendEvent",
    "GrabPointer",
    "UngrabPointer",
    "GrabButton",
    "UngrabButton",
    "ChangeActivePointerGrab",
    "GrabKeyboard",
    "UngrabKeyboard",
    "GrabKey",
    "UngrabKey",
    "AllowEvents",
    "GrabServer",
    "UngrabServer",
    "QueryPointer",
    "GetMotionEvents",
    "TranslateCoords",
    "WarpPointer",
    "SetInputFocus",
    "GetInputFocus",
    "QueryKeymap",
    "OpenFont",
    "CloseFont",
    "QueryFont",
    "QueryTextExtents",
    "ListFonts",
    "ListFontsWithInfo",
    "SetFontPath",
    "GetFontPath",
    "CreatePixmap",
    "FreePixmap",
    "CreateGC",
    "ChangeGC",
    "CopyGC",
    "SetDashes",
    "SetClipRectangles",
    "FreeGC",
    "ClearArea",
    "CopyArea",
    "CopyPlane",
    "PolyPoint",
    "PolyLine",
    "PolySegment",
    "PolyRectangle",
    "PolyArc",
    "FillPoly",
    "PolyFillRectangle",
    "PolyFillArc",
    "PutImage",
    "GetImage",
    "PolyText",
    "PolyText",
    "ImageText",
    "ImageText",
    "CreateColormap",
    "FreeColormap",
    "CopyColormapAndFree",
    "InstallColormap",
    "UninstallColormap",
    "ListInstalledColormaps",
    "AllocColor",
    "AllocNamedColor",
    "AllocColorCells",
    "AllocColorPlanes",
    "FreeColors",
    "StoreColors",
    "StoreNamedColor",
    "QueryColors",
    "LookupColor",
    "CreateCursor",
    "CreateGlyphCursor",
    "FreeCursor",
    "RecolorCursor",
    "QueryBestSize",
    "QueryExtension",
    "ListExtensions",
    "ChangeKeyboardMapping",
    "GetKeyboardMapping",
    "ChangeKeyboardControl",
    "GetKeyboardControl",
    "Bell",
    "ChangePointerControl",
    "GetPointerControl",
    "SetScreenSaver",
    "GetScreenSaver",
    "ChangeHosts",
    "ListHosts",
    "SetAccessControl",
    "SetCloseDownMode",
    "KillClient",
    "RotateProperties",
    "ForceScreenSaver",
    "SetPointerMapping",
    "GetPointerMapping",
    "SetModifierMapping",
    "GetModifierMapping",
    "major 120",
    "major 121",
    "major 122",
    "major 123",
    "major 124",
    "major 125",
    "major 126",
    "NoOperation",
};

xutil_error_t *
xutil_get_error(const xcb_generic_error_t *e)
{
    if(e->response_type != 0)
        /* This is not an error, this _should_ not happen */
        return NULL;

    xutil_error_t *err = p_new(xutil_error_t, 1);

    /*
     * Get the request code,  taken from 'xcb-util/wm'. I can't figure
     * out  how it  works BTW,  seems to  get a  byte in  'pad' member
     * (second byte in second element of the array)
     */
    err->request_code = *((uint8_t *) e + 10);

    /* Extensions  generally provide  their  own requests  so we  just
     * store the request code */
    if(err->request_code >= (sizeof(xutil_request_label) / sizeof(char *)))
        asprintf(&err->request_label, "%d", err->request_code);
    else
        err->request_label = a_strdup(xutil_request_label[err->request_code]);

    /* Extensions may also define their  own errors, so just store the
     * error_code */
    if(e->error_code >= (sizeof(xutil_error_label) / sizeof(char *)))
        asprintf(&err->error_label, "%d", e->error_code);
    else
        err->error_label = a_strdup(xutil_error_label[e->error_code]);

    return err;
}

void
xutil_delete_error(xutil_error_t *err)
{
    p_delete(&err->error_label);
    p_delete(&err->request_label);
    p_delete(&err);
}

/** Link a name to a key symbol */
typedef struct
{
    const char *name;
    xcb_keysym_t keysym;
} keymod_t;

xcb_keysym_t
xutil_keymask_fromstr(const char *keyname)
{
    /** List of keyname and corresponding X11 mask codes */
    static const keymod_t KeyModList[] =
    {
        { "Shift", XCB_MOD_MASK_SHIFT },
        { "Lock", XCB_MOD_MASK_LOCK },
        { "Control", XCB_MOD_MASK_CONTROL },
        { "Ctrl", XCB_MOD_MASK_CONTROL },
        { "Mod1", XCB_MOD_MASK_1 },
        { "Mod2", XCB_MOD_MASK_2 },
        { "Mod3", XCB_MOD_MASK_3 },
        { "Mod4", XCB_MOD_MASK_4 },
        { "Mod5", XCB_MOD_MASK_5 },
        { NULL, XCB_NO_SYMBOL }
    };
    int i;

    if(keyname)
        for(i = 0; KeyModList[i].name; i++)
            if(!a_strcmp(keyname, KeyModList[i].name))
                 return KeyModList[i].keysym;

    return XCB_NO_SYMBOL;

}

/** Permit to use mouse with many more buttons */
#ifndef XCB_BUTTON_INDEX_6
#define XCB_BUTTON_INDEX_6 6
#endif
#ifndef XCB_BUTTON_INDEX_7
#define XCB_BUTTON_INDEX_7 7
#endif
#ifndef XCB_BUTTON_INDEX_8
#define XCB_BUTTON_INDEX_8 8
#endif
#ifndef XCB_BUTTON_INDEX_9
#define XCB_BUTTON_INDEX_9 9
#endif

/** Link a name to a mouse button symbol */
typedef struct
{
    int id;
    unsigned int button;
} mouse_button_t;

/** Lookup for a mouse button from its index.
 * \param button Mouse button index.
 * \return Mouse button or 0 if not found.
 */
unsigned int
xutil_button_fromint(int button)
{
    /** List of button name and corresponding X11 mask codes */
    static const mouse_button_t mouse_button_list[] =
    {
        { 1, XCB_BUTTON_INDEX_1 },
        { 2, XCB_BUTTON_INDEX_2 },
        { 3, XCB_BUTTON_INDEX_3 },
        { 4, XCB_BUTTON_INDEX_4 },
        { 5, XCB_BUTTON_INDEX_5 },
        { 6, XCB_BUTTON_INDEX_6 },
        { 7, XCB_BUTTON_INDEX_7 },
        { 8, XCB_BUTTON_INDEX_8 },
        { 9, XCB_BUTTON_INDEX_9 }
    };

    if(button >= 1 && button <= countof(mouse_button_list))
        return mouse_button_list[button - 1].button;

    return 0;
}

/** Equivalent to 'XCreateFontCursor()', error are handled by the
 * default current error handler.
 * \param conn The connection to the X server.
 * \param cursor_font Type of cursor to use.
 * \return Allocated cursor font.
 */
xcb_cursor_t
xutil_cursor_new(xcb_connection_t *conn, unsigned int cursor_font)
{
    xcb_font_t font;
    xcb_cursor_t cursor;

    /* Get the font for the cursor*/
    font = xcb_generate_id(conn);
    xcb_open_font(conn, font, a_strlen("cursor"), "cursor");

    cursor = xcb_generate_id(conn);
    xcb_create_glyph_cursor(conn, cursor, font, font,
                            cursor_font, cursor_font + 1,
                            0, 0, 0,
                            65535, 65535, 65535);

    return cursor;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
