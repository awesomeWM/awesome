/*
 * ewmh.h - EWMH header
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

#ifndef AWESOME_EWMH_H
#define AWESOME_EWMH_H

#include "structs.h"

typedef struct
{
    int height;
    int width;
    unsigned char *image;
} NetWMIcon;

void ewmh_init_atoms(void);
void ewmh_set_supported_hints(int);
void ewmh_update_net_client_list(int);
void ewmh_update_net_numbers_of_desktop(int);
void ewmh_update_net_current_desktop(int);
void ewmh_update_net_desktop_names(int);
void ewmh_update_net_active_window(int);
int ewmh_process_client_message(xcb_client_message_event_t *);
void ewmh_update_net_client_list_stacking(int);
void ewmh_check_client_hints(client_t *);
NetWMIcon * ewmh_get_window_icon(xcb_window_t);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
