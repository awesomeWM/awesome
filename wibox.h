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

#include "widget.h"
#include "swindow.h"
#include "common/util.h"
#include "common/refcount.h"
#include "common/array.h"

static inline void
wibox_delete(wibox_t **wibox)
{
    simplewindow_wipe(&(*wibox)->sw);
    widget_node_list_wipe(&(*wibox)->widgets);
    p_delete(wibox);
}

DO_RCNT(wibox_t, wibox, wibox_delete)
ARRAY_FUNCS(wibox_t *, wibox, wibox_unref)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
