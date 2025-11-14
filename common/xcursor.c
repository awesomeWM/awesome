/*
 * xcursor.c - X cursors management
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include "common/xcursor.h"
#include "common/util.h"

struct cursor_cache_entry_t {
    char const * name;
    xcb_cursor_t cursor;
};

static int
cursor_cache_entry_cmp(const void *a, const void *b) {
    return a_strcmp(((cursor_cache_entry_t *) a)->name, ((cursor_cache_entry_t *) b)->name);
}

static void
cursor_cache_entry_wipe(cursor_cache_entry_t *entry)
{
    p_delete(&entry->name);
}

BARRAY_FUNCS(cursor_cache_entry_t, cursors, cursor_cache_entry_wipe, cursor_cache_entry_cmp)

/** Equivalent to 'XCreateFontCursor()', error are handled by the
 * default current error handler.
 * \param ctx The xcb-cursor context.
 * \param cursor_font Type of cursor to use.
 * \return Allocated cursor font.
 */
xcb_cursor_t
xcursor_new(cursors_array_t *array, xcb_cursor_context_t *ctx, const char *cursor_name)
{
    cursor_cache_entry_t entry;
    entry.name = cursor_name;

    cursor_cache_entry_t *found = cursors_array_lookup(array, &entry);
    if (NULL == found) {
        entry.name = a_strdup(cursor_name);
        entry.cursor = xcb_cursor_load_cursor(ctx, cursor_name);
        cursors_array_insert(array, entry);
        found = &entry;
    }

    return found->cursor;
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
