/*
 * widgets/common.h - some functions headers used by widgets
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Marco Candrian <mac@calmar.ws>
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

#ifndef AWESOME_WIDGETS_COMMON_H
#define AWESOME_WIDGETS_COMMON_H
#include "widget.h"

widget_tell_status_t widget_set_color_for_data(widget_t *, xcolor_t *, char *, int, char **);
widget_tell_status_t widget_set_color_pointer_for_data(widget_t *, xcolor_t **, char *, int, char **);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
