/*
 * layout.h - layout management header
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

typedef void (LayoutArrange)(int);

typedef struct Layout Layout;
struct Layout
{
    char *image;
    LayoutArrange *arrange;
    Layout *next;
};

void arrange(int);
Layout * get_current_layout(int);
void restack(int);
void loadawesomeprops(int);
void saveawesomeprops(int);

Uicb uicb_client_focusnext;
Uicb uicb_client_focusprev;
Uicb uicb_tag_setlayout;
Uicb uicb_client_togglefloating;

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
