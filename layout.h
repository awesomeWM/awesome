/*
 * layout.h - layout management header
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

#ifndef AWESOME_LAYOUT_H
#define AWESOME_LAYOUT_H

#include "uicb.h"
#include "common/list.h"
#include "common/util.h"

typedef void (LayoutArrange)(int);

typedef struct Layout Layout;
struct Layout
{
    char *image;
    LayoutArrange *arrange;
    Layout *prev, *next;
};

DO_SLIST(Layout, layout, p_delete)

int layout_refresh(void);
Layout * layout_get_current(int);

Uicb uicb_tag_setlayout;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
