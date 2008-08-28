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

#include <xcb/xcb.h>

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
    char *homedir, *host = NULL;
    int screenp = 0, displayp = 0;
    ssize_t path_len, len;
    struct sockaddr_un *addr;

    addr = p_new(struct sockaddr_un, 1);
    homedir = getenv("HOME");
    
    xcb_parse_display(NULL, &host, &displayp, &screenp);

    len = a_strlen(host);

    /* + 2 for / and . and \0 */
    path_len = snprintf(addr->sun_path, sizeof(addr->sun_path),
                        "%s/" CONTROL_UNIX_SOCKET_PATH "%s%s%d",
                        homedir, len ? host : "", len ? "." : "",
                        displayp);

    p_delete(&host);

    if(path_len >= ssizeof(addr->sun_path))
    {
        fprintf(stderr, "error: path of control UNIX domain socket is too long");
        return NULL;
    }

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
