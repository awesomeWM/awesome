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
#include <unistd.h>

#include <xcb/xcb.h>

#include "common/socket.h"
#include "common/util.h"

#define CONTROL_UNIX_SOCKET_PATH ".awesome_ctl."

/** Open a communication socket with awesome for a given display.
 * \param csfd The socket file descriptor.
 * \param display the display number
 * \param mode The open mode, either Bind or Connect.
 * \return sockaddr_un Struct ready to be used or NULL if a problem ocurred.
 */
struct sockaddr_un *
socket_open(int csfd, const char *display, const socket_mode_t mode)
{
    char *host = NULL;
    int screenp = 0, displayp = 0;
    bool is_socket_opened = false;
    ssize_t path_len, len;
    struct sockaddr_un *addr;
    const char *fallback[] = { ":HOME", ":TMPDIR", "/tmp", NULL }, *directory;

    addr = p_new(struct sockaddr_un, 1);
    addr->sun_family = AF_UNIX;

    xcb_parse_display(NULL, &host, &displayp, &screenp);
    len = a_strlen(host);

    for(int i = 0; fallback[i] && !is_socket_opened; i++)
    {
        if(fallback[i][0] == ':')
        {
            if(!(directory = getenv(fallback[i] + 1)))
                continue;
        }
        else
            directory = fallback[i];

        /* + 2 for / and . and \0 */
        path_len = snprintf(addr->sun_path, sizeof(addr->sun_path),
                            "%s/" CONTROL_UNIX_SOCKET_PATH "%s%s%d",
                            directory, len ? host : "", len ? "." : "",
                            displayp);

        if(path_len < ssizeof(addr->sun_path))
            switch(mode)
            {
              case SOCKET_MODE_BIND:
/* Needed for some OSes like Solaris */
#ifndef SUN_LEN
#define SUN_LEN(ptr) ((size_t) (((struct sockaddr_un *) 0)->sun_path) + strlen ((ptr)->sun_path))
#endif
                if(!bind(csfd, (const struct sockaddr *) addr, SUN_LEN(addr)))
                    is_socket_opened = true;
                else if(errno == EADDRINUSE)
                {
                    if(unlink(addr->sun_path))
                        warn("error unlinking existing file: %s", strerror(errno));
                    if(!bind(csfd, (const struct sockaddr *)addr, SUN_LEN(addr)))
                        is_socket_opened = true;
                }
                break;
              case SOCKET_MODE_CONNECT:
                if(!connect(csfd, (const struct sockaddr *)addr, sizeof(struct sockaddr_un)))
                    is_socket_opened = true;
                break;
            }
        else
            warn("error: path (using %s) of control UNIX domain socket is too long", directory);
    }

    p_delete(&host);

    if(!is_socket_opened)
        p_delete(&addr);

    return addr;
}

/** Get a AF_UNIX socket for communicating with awesome
 * \return the socket file descriptor
 */
int
socket_getclient(void)
{
    int csfd;
    #ifndef __FreeBSD__
    csfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    #else
    csfd = socket(PF_UNIX, SOCK_STREAM, 0);
    #endif

    if(csfd < 0)
        warn("error opening UNIX domain socket: %s", strerror(errno));

    return csfd;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
