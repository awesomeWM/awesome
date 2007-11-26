/*
 * awesome-client.c - awesome client, communicate with socket
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "awesome-client.h"
#include "util.h"

/* GNU/Hurd workaround */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

int
main(void)
{
    struct sockaddr_un *addr;
    char buf[1024], *display, *tmp;
    int csfd, ret_value = EXIT_SUCCESS;

    csfd = get_client_socket();
    display = a_strdup(getenv("DISPLAY"));
    if(display && (tmp = strrchr(display, '.')))
       *tmp = '\0';
    addr = get_client_addr(display);

    if(!addr || csfd < 0)
        return EXIT_FAILURE;

    while(fgets(buf, sizeof(buf), stdin))
        if(sendto(csfd, buf, a_strlen(buf), MSG_NOSIGNAL,
                  (const struct sockaddr *) addr, sizeof(struct sockaddr_un)) == -1)
        {
            switch (errno)
            {
                case ENOENT:
                    fprintf(stderr, "Can't write to %s\n", addr->sun_path);
                    break;
                default:
                    perror("error sending datagram");
            }
            ret_value = errno;
            break;
        }

    close(csfd);

    p_delete(&addr);
    p_delete(&display);

    return ret_value;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
