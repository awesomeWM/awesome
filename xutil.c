#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>
#include <sys/wait.h>

#include "xutil.h"

void
uicb_exec(awesome_config * awesomeconf,
          int screen __attribute__ ((unused)),
          const char *arg)
{
    char path[PATH_MAX];
    if(awesomeconf->display)
        close(ConnectionNumber(awesomeconf->display));

    sscanf(arg, "%s", path);
    execlp(path, arg, NULL);
}

void
uicb_spawn(awesome_config * awesomeconf,
           int screen,
           const char *arg)
{
    static char *shell = NULL;
    char *display = NULL;
    char *tmp, newdisplay[128];

    if(!shell && !(shell = getenv("SHELL")))
        shell = a_strdup("/bin/sh");
    if(!arg)
        return;

    if(!XineramaIsActive(awesomeconf->display) && (tmp = getenv("DISPLAY")))
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
            if(awesomeconf->display)
                close(ConnectionNumber(awesomeconf->display));
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
xgettextprop(Display *disp, Window w, Atom atom, char *text, ssize_t textlen)
{
    char **list = NULL;
    int n;

    XTextProperty name;

    if(!text || !textlen)
        return False;

    text[0] = '\0';
    XGetTextProperty(disp, w, &name, atom);

    if(!name.nitems)
        return False;

    if(name.encoding == XA_STRING)
        a_strncpy(text, textlen, (char *) name.value, textlen - 1);

    else if(XmbTextPropertyToTextList(disp, &name, &list, &n) >= Success && n > 0 && *list)
    {
        a_strncpy(text, textlen, *list, textlen - 1);
        XFreeStringList(list);
    }

    text[textlen - 1] = '\0';
    XFree(name.value);

    return True;
}

