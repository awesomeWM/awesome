/*
 * awesome-client-common.c - awesome client, communicate with socket, common functions
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#include <sys/socket.h>
#include <sys/un.h>

#include "awesome-client.h"
#include "util.h"

#define CONTROL_UNIX_SOCKET_PATH ".awesome_ctl."

struct sockaddr_un *
get_client_addr(const char *display)
{
    char *homedir;
    ssize_t path_len;
    struct sockaddr_un *addr;

    addr = p_new(struct sockaddr_un, 1);
    homedir = getenv("HOME");
    /* (a_strlen(display) - 1) because we strcat on display + 1 and
     * + 3 for /, \0 and possibly 0 if display is NULL */
    path_len = a_strlen(homedir) + a_strlen(CONTROL_UNIX_SOCKET_PATH) + (a_strlen(display) - 1) + 3;
    if(path_len >= ssizeof(addr->sun_path))
    {
        fprintf(stderr, "error: path of control UNIX domain socket is too long");
        return NULL;
    }
    a_strcpy(addr->sun_path, path_len, homedir);
    a_strcat(addr->sun_path, path_len, "/");
    a_strcat(addr->sun_path, path_len, CONTROL_UNIX_SOCKET_PATH);
    if(display)
        a_strcat(addr->sun_path, path_len, display + 1);
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
