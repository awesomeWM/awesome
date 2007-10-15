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

#include "client.h"

/** Check if current layout is arranged with a layout */
#define IS_ARRANGE(screen, layout)          (layout == awesomeconf[screen].current_layout->arrange)

#define AWESOMEPROPS_ATOM(disp)                    XInternAtom(disp, "_AWESOME_PROPERTIES", False)

void arrange(Display *, awesome_config *);             /* arranges all windows depending on the layout in use */
void restack(Display *, awesome_config *);        /* restores z layers of all clients */
void loadawesomeprops(Display *, awesome_config *);
void saveawesomeprops(Display *, awesome_config *);

UICB_PROTO(uicb_focusnext);
UICB_PROTO(uicb_focusprev);
UICB_PROTO(uicb_setlayout);
UICB_PROTO(uicb_togglemax);
UICB_PROTO(uicb_toggleverticalmax);
UICB_PROTO(uicb_togglehorizontalmax);
UICB_PROTO(uicb_zoom);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
