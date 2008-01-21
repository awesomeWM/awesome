/*
 * xutil.c - X-related useful functions
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
#include <sys/wait.h>

#include "config.h"
#include "xutil.h"
#include "screen.h"
#include "common/util.h"

extern AwesomeConf globalconf;

/** Execute another process, replacing the current instance of Awesome
 * \param screen Screen ID
 * \param arg Command
 * \ingroup ui_callback
 */
void
uicb_exec(int screen __attribute__ ((unused)), char *arg)
{
    char path[PATH_MAX];
    if(globalconf.display)
        close(ConnectionNumber(globalconf.display));

    sscanf(arg, "%s", path);
    execlp(path, arg, NULL);
}

/** Spawn another process
 * \param screen Screen ID
 * \param arg Command
 * \ingroup ui_callback
 */
void
uicb_spawn(int screen, char *arg)
{
    static char *shell = NULL;
    char *display = NULL;
    char *tmp, newdisplay[128];

    if(!shell && !(shell = getenv("SHELL")))
        shell = a_strdup("/bin/sh");
    if(!arg)
        return;

    if(!XineramaIsActive(globalconf.display) && (tmp = getenv("DISPLAY")))
    {
        display = a_strdup(tmp);
        if((tmp = strrchr(display, '.')))
            *tmp = '\0';
        snprintf(newdisplay, sizeof(newdisplay), "%s.%d", display, screen);
        setenv("DISPLAY", newdisplay, 1);
    }


    /* The double-fork construct avoids zombie processes and keeps the code
     * clean from stupid signal handlers. */
    if(fork() == 0)
    {
        if(fork() == 0)
        {
            if(globalconf.display)
                close(ConnectionNumber(globalconf.display));
            setsid();
            execl(shell, shell, "-c", arg, (char *) NULL);
            warn("execl '%s -c %s'", shell, arg);
            perror(" failed");
        }
        exit(EXIT_SUCCESS);
    }
    wait(0);
}

Bool
xgettextprop(Window w, Atom atom, char *text, ssize_t textlen)
{
    char **list = NULL;
    int n;

    XTextProperty name;

    if(!text || !textlen)
        return False;

    text[0] = '\0';
    XGetTextProperty(globalconf.display, w, &name, atom);

    if(!name.nitems)
        return False;

    if(name.encoding == XA_STRING)
        a_strncpy(text, textlen, (char *) name.value, textlen - 1);

    else if(XmbTextPropertyToTextList(globalconf.display, &name, &list, &n) >= Success && n > 0 && *list)
    {
        a_strncpy(text, textlen, *list, textlen - 1);
        XFreeStringList(list);
    }

    text[textlen - 1] = '\0';
    XFree(name.value);

    return True;
}

/** Initialize an X color
 * \param screen Physical screen number
 * \param colstr Color specification
 */
XColor
initxcolor(Display *disp, int phys_screen, const char *colstr)
{
    XColor screenColor, exactColor;

    if(!XAllocNamedColor(disp,
                         DefaultColormap(disp, phys_screen),
                         colstr,
                         &screenColor,
                         &exactColor))
        eprint("awesome: error, cannot allocate color '%s'\n", colstr);

    return screenColor;
}

unsigned int
get_numlockmask(Display *disp)
{
    XModifierKeymap *modmap;
    unsigned int mask = 0;
    int i, j;

    modmap = XGetModifierMapping(disp);
    for(i = 0; i < 8; i++)
        for(j = 0; j < modmap->max_keypermod; j++)
            if(modmap->modifiermap[i * modmap->max_keypermod + j]
               == XKeysymToKeycode(disp, XK_Num_Lock))
                mask = (1 << i);

    XFreeModifiermap(modmap);

    return mask;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
