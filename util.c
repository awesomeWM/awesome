/* See LICENSE file for copyright and license details. */

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
