/*
 * awesome-client.c - awesome client, communicate with socket
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

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "common/socket.h"
#include "common/version.h"
#include "common/util.h"

/* GNU/Hurd workaround */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

struct sockaddr_un *addr;
int csfd;

/** Initialize the client and server socket connections.
 * If something goes wrong, preserves errno.
 * \return 0 if everything worked, 1 otherwise.
 */
static int
sockets_init(void)
{
    return (csfd = socket_getclient()) < 0 ||
      !(addr = socket_getaddr(getenv("DISPLAY"))) ||
      connect(csfd, addr, sizeof(struct sockaddr_un)) == -1;
}


/** Close the client and server socket connections.
 */
static void
sockets_close(void)
{
    close(csfd);
    p_delete(&addr);
}


/** Send a message to awesome.
 * \param msg The message.
 * \param msg_len The message length.
 * \return The errno of sendto().
 */
static int
send_msg(const char *msg, ssize_t msg_len)
{
    if(send(csfd, msg, msg_len, MSG_NOSIGNAL | MSG_EOR) == -1)
    {
        switch (errno)
        {
          case ENOENT:
            warn("can't write to %s", addr->sun_path);
            break;
          default:
            warn("error sending packet: %s", strerror(errno));
        }
        return errno;
    }

    return EXIT_SUCCESS;
}


/** Recieve a message from awesome.
 */
static void
recv_msg(void)
{
    ssize_t r;
    char buf[1024];

    r = recv(csfd, buf, sizeof(buf) - 1, MSG_TRUNC);
    if (r < 0)
    {
        warn("error recieving from UNIX domain socket: %s", strerror(errno));
        return;
    }
    else if(r > 0)
    {
        buf[r] = '\0';
        puts(buf);
    }
}


/** Print help and exit(2) with given exit_code.
 * \param exit_code The exit code.
 * \return Never return.
 */
static void __attribute__ ((noreturn))
exit_help(int exit_code)
{
    FILE *outfile = (exit_code == EXIT_SUCCESS) ? stdout : stderr;
    fprintf(outfile, "Usage: awesome-client [--version|--help]\n"
	    "In normal operation, give no parameters and issue commands "
	    "on standard input.\n");
    exit(exit_code);
}

/** Main function of awesome-client.
 * \param argc Number of args.
 * \param argv Args array.
 * \return Value returned by send_msg().
 */
int
main(int argc, char **argv)
{
    char buf[1024], *msg, *prompt;
    int ret_value = EXIT_SUCCESS;
    ssize_t len, msg_len = 1;

    if(argc == 2)
    {
        if(!a_strcmp("-v", argv[1]) || !a_strcmp("--version", argv[1]))
	    eprint_version("awesome-client");
        else if(!a_strcmp("-h", argv[1]) || !a_strcmp("--help", argv[1]))
            exit_help(EXIT_SUCCESS);
    }
    else if(argc > 2)
        exit_help(EXIT_SUCCESS);

    if (sockets_init())
    {
        warn("can't connect to UNIX domain socket: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    if(isatty(STDIN_FILENO))
    {
        char *display = getenv("DISPLAY");
        asprintf(&prompt, "awesome@%s%% ", display ? display : "unknown");
        while((msg = readline(prompt)))
            if((msg_len = a_strlen(msg)))
            {
                add_history (msg);
                p_realloc(&msg, msg_len + 2);
                msg[msg_len] = '\n';
                msg[msg_len + 1] = '\0';
                send_msg(msg, msg_len + 2);
                recv_msg();
                p_delete(&msg);
            }
    }
    else
    {
        msg = p_new(char, 1);
        while(fgets(buf, sizeof(buf), stdin))
        {
            len = a_strlen(buf);
            if(len < 2 && msg_len > 1)
            {
                ret_value = send_msg(msg, msg_len);
                p_delete(&msg);
                if (ret_value != EXIT_SUCCESS)
                    return ret_value;
                msg = p_new(char, 1);
                msg_len = 1;
            }
            else if (len > 1)
            {
                msg_len += len;
                p_realloc(&msg, msg_len);
                a_strncat(msg, msg_len, buf, len);
            }
        }
        if(msg_len > 1)
        {
            ret_value = send_msg(msg, msg_len);
            recv_msg();
        }
        p_delete(&msg);
    }

    sockets_close();
    return ret_value;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
