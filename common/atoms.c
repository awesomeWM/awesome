/*
 * atoms.c - X atoms functions
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

#include "common/atoms.h"
#include "common/util.h"

typedef struct
{
    const char *name;
    size_t len;
    xcb_atom_t *atom;
} atom_item_t;

#include "common/atoms-intern.h"

void
atoms_init(xcb_connection_t *conn)
{
    unsigned int i;
    xcb_intern_atom_cookie_t cs[countof(ATOM_LIST)];
    xcb_intern_atom_reply_t *r;

    /* Create the atom and get the reply in a XCB way (e.g. send all
     * the requests at the same time and then get the replies) */
    for(i = 0; i < countof(ATOM_LIST); i++)
	cs[i] = xcb_intern_atom_unchecked(conn,
					  false,
					  ATOM_LIST[i].len,
					  ATOM_LIST[i].name);

    for(i = 0; i < countof(ATOM_LIST); i++)
    {
	if(!(r = xcb_intern_atom_reply(conn, cs[i], NULL)))
	    /* An error occurred, get reply for next atom */
	    continue;

	*ATOM_LIST[i].atom = r->atom;
        p_delete(&r);
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
