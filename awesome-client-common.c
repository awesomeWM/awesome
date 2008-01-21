/*
 * awesome-client-common.c - awesome client, communicate with socket, common functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007 daniel@brinkers.de
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

#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "awesome-client.h"
#include "common/util.h"

#define CONTROL_UNIX_SOCKET_PATH ".awesome_ctl."

struct sockaddr_un *
get_client_addr(const char *display)
{
    char *homedir, *tmp;
    const char *real_display = NULL;
    ssize_t path_len;
    struct sockaddr_un *addr;

    addr = p_new(struct sockaddr_un, 1);
    homedir = getenv("HOME");
    if(a_strlen(display))
    {
        if((tmp = strchr(display, ':')))
            real_display = tmp + 1;
        else
            real_display = display;
        if((tmp = strrchr(display, '.')))
            *tmp = '\0';
    }

    /* a_strlen(display) because we strcat on display and
     * + 2 for / and \0 */
    path_len = a_strlen(homedir) + a_strlen(CONTROL_UNIX_SOCKET_PATH)
               + (a_strlen(display) ? (a_strlen(real_display)) : 1) + 2;

    if(path_len >= ssizeof(addr->sun_path))
    {
        fprintf(stderr, "error: path of control UNIX domain socket is too long");
        return NULL;
    }
    a_strcpy(addr->sun_path, path_len, homedir);
    a_strcat(addr->sun_path, path_len, "/");
    a_strcat(addr->sun_path, path_len, CONTROL_UNIX_SOCKET_PATH);
    if(a_strlen(display))
        a_strcat(addr->sun_path, path_len, real_display);
    else
        a_strcat(addr->sun_path, path_len, "0");

    addr->sun_family = AF_UNIX;

    return addr;
}

int
get_client_socket(void)
{
    int csfd;

    csfd = socket(AF_UNIX, SOCK_DGRAM, 0);

    if(csfd < 0)
        perror("error opening UNIX domain socket");

    return csfd;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
