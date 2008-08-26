/*
 * socket.c - awesome client, communicate with socket, common functions
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
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "common/socket.h"
#include "common/util.h"

#define CONTROL_UNIX_SOCKET_PATH ".awesome_ctl."

/** Get a sockaddr_un struct with information feeded for opening a
 * communication to the awesome socket for given display
 * \param display the display number
 * \return sockaddr_un struct ready to be used or NULL if a problem occured
 */
struct sockaddr_un *
socket_getaddr(const char *display)
{
    char *homedir, *tmp, *dot;
    char *hostname = NULL, *screen = NULL;
    ssize_t path_len, hostname_len, screen_len;
    struct sockaddr_un *addr;

    addr = p_new(struct sockaddr_un, 1);
    homedir = getenv("HOME");

    if(a_strlen(display))
    {
        /* find hostname */
        if((tmp = strchr(display, ':')))
        {
            /* if display starts with : */
            if(tmp == display)
            {
                hostname = a_strdup("localhost");
                hostname_len = 9;
            }
            else
            {
                hostname_len = tmp - display;
                hostname = a_strndup(display, hostname_len);
            }

            tmp++;
            if((dot = strchr(tmp, '.')))
            {
                screen_len = dot - tmp;
                screen = a_strndup(tmp, screen_len); 
            }
            else
            {
                screen = a_strdup(tmp);
                screen_len = a_strlen(screen);
            }
        }
        else
        {
            hostname = a_strdup("unknown");
            hostname_len = 7;
            screen = a_strdup("0");
            screen_len = 1;
        }
    }
    else
    {
        hostname = a_strdup("localhost");
        hostname_len = 9;
        screen = a_strdup("0");
        screen_len = 1;
    }

    /* + 2 for / and . and \0 */
    path_len = a_strlen(homedir) + sizeof(CONTROL_UNIX_SOCKET_PATH) - 1
               + screen_len + hostname_len + 3;

    if(path_len >= ssizeof(addr->sun_path))
    {
        fprintf(stderr, "error: path of control UNIX domain socket is too long");
        return NULL;
    }
    a_strcpy(addr->sun_path, path_len, homedir);
    a_strcat(addr->sun_path, path_len, "/");
    a_strcat(addr->sun_path, path_len, CONTROL_UNIX_SOCKET_PATH);
    a_strcat(addr->sun_path, path_len, hostname);
    a_strcat(addr->sun_path, path_len, "|");
    a_strcat(addr->sun_path, path_len, screen);

    p_delete(&hostname);
    p_delete(&screen);

    addr->sun_family = AF_UNIX;

    return addr;
}

/** Get a AF_UNIX socket for communicating with awesome
 * \return the socket file descriptor
 */
int
socket_getclient(void)
{
    int csfd;

    csfd = socket(AF_UNIX, SOCK_DGRAM, 0);

    if(csfd < 0)
        warn("error opening UNIX domain socket: %s", strerror(errno));

    return csfd;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
