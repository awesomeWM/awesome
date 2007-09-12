/*  
 * util.c - useful functions
 *  
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
 * Copyright © 2006 Pierre Habouzit <madcoder@debian.org>
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

#include <stdarg.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

void
eprint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void
spawn(Display * disp,
      awesome_config * awesomeconf __attribute__ ((unused)),
      const char *arg)
{
    static char *shell = NULL;

    if(!shell && !(shell = getenv("SHELL")))
        shell = strdup("/bin/sh");
    if(!arg)
        return;
    /* The double-fork construct avoids zombie processes and keeps the code
     * * clean from stupid signal handlers. */
    if(fork() == 0)
    {
        if(fork() == 0)
        {
            if(disp)
                close(ConnectionNumber(disp));
            setsid();
            execl(shell, shell, "-c", arg, (char *) NULL);
            fprintf(stderr, "awesome: execl '%s -c %s'", shell, arg);
            perror(" failed");
        }
        exit(EXIT_SUCCESS);
    }
    wait(0);
}
