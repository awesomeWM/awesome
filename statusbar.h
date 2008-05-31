/*
 * statusbar.h - statusbar functions header
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

#ifndef AWESOME_STATUSBAR_H
#define AWESOME_STATUSBAR_H

#include "structs.h"
#include "common/refcount.h"
#include "common/swindow.h"

static inline void
statusbar_delete(statusbar_t **statusbar)
{
    simplewindow_delete(&(*statusbar)->sw);
    widget_node_list_wipe(&(*statusbar)->widgets);
    p_delete(&(*statusbar)->name);
    p_delete(statusbar);
}

void statusbar_refresh(void);

DO_RCNT(statusbar_t, statusbar, statusbar_delete)
DO_SLIST(statusbar_t, statusbar, statusbar_delete)
DO_SLIST_UNREF(statusbar_t, statusbar, statusbar_delete)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
