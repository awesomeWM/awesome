/* See LICENSE file for copyright and license details. */

#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include "jdwm.h"
#include "layout.h"
#include "tag.h"
#include "layouts/floating.h"
#include "util.h"

/* extern */
extern int wax, way, wah, waw;  /* windowarea geometry */
extern Window barwin;
extern Client *clients, *sel;   /* global client list */
extern Atom jdwmprops;
extern DC dc;

void
arrange(Display * disp, jdwm_config *jdwmconf)
{
    Client *c;

    for(c = clients; c; c = c->next)
        if(isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags))
            unban(c);
        else
            ban(c);
    jdwmconf->current_layout->arrange(disp, jdwmconf);
    focus(disp, &dc, NULL, True, jdwmconf);
    restack(disp, jdwmconf);
}

void
uicb_focusnext(Display *disp __attribute__ ((unused)),
               jdwm_config * jdwmconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!sel)
        return;
    for(c = sel->next; c && !isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags); c = c->next);
    if(!c)
        for(c = clients; c && !isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags); c = c->next);
    if(c)
    {
        focus(c->display, &dc, c, True, jdwmconf);
        restack(c->display, jdwmconf);
    }
}

void
uicb_focusprev(Display *disp __attribute__ ((unused)),
               jdwm_config *jdwmconf,
               const char *arg __attribute__ ((unused)))
{
    Client *c;

    if(!sel)
        return;
    for(c = sel->prev; c && !isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags); c = c->prev);
    if(!c)
    {
        for(c = clients; c && c->next; c = c->next);
        for(; c && !isvisible(c, jdwmconf->selected_tags, jdwmconf->ntags); c = c->prev);
    }
    if(c)
    {
        focus(c->display, &dc, c, True, jdwmconf);
        restack(c->display, jdwmconf);
    }
}

void
loadjdwmprops(Display *disp, jdwm_config * jdwmconf)
{
    int i;
    char *prop;

    prop = p_new(char, jdwmconf->ntags + 1);

    if(gettextprop(disp, DefaultRootWindow(disp), jdwmprops, prop, jdwmconf->ntags + 1))
        for(i = 0; i < jdwmconf->ntags && prop[i]; i++)
            jdwmconf->selected_tags[i] = prop[i] == '1';

    p_delete(&prop);
}

void
restack(Display * disp, jdwm_config *jdwmconf)
{
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    drawstatus(disp, jdwmconf);
    if(!sel)
        return;
    if(sel->isfloating || IS_ARRANGE(floating))
        XRaiseWindow(disp, sel->win);
    if(!IS_ARRANGE(floating))
    {
        wc.stack_mode = Below;
        wc.sibling = barwin;
        if(!sel->isfloating)
        {
            XConfigureWindow(disp, sel->win, CWSibling | CWStackMode, &wc);
            wc.sibling = sel->win;
        }
        for(c = clients; c; c = c->next)
        {
            if(IS_TILED(c, jdwmconf->selected_tags, jdwmconf->ntags) || c == sel)
                continue;
            XConfigureWindow(disp, c->win, CWSibling | CWStackMode, &wc);
            wc.sibling = c->win;
        }
    }
    XSync(disp, False);
    while(XCheckMaskEvent(disp, EnterWindowMask, &ev));
}

void
savejdwmprops(Display *disp, jdwm_config *jdwmconf)
{
    int i;
    char *prop;

    prop = p_new(char, jdwmconf->ntags + 1);
    for(i = 0; i < jdwmconf->ntags; i++)
        prop[i] = jdwmconf->selected_tags[i] ? '1' : '0';
    prop[i] = '\0';
    XChangeProperty(disp, DefaultRootWindow(disp), jdwmprops, XA_STRING, 8, PropModeReplace, (unsigned char *) prop, i);
    p_delete(&prop);
}

void
uicb_setlayout(Display *disp, jdwm_config * jdwmconf, const char *arg)
{
    int i, j;
    Client *c;

    if(!arg)
    {
        if(!(++jdwmconf->current_layout)->symbol)
            jdwmconf->current_layout = &jdwmconf->layouts[0];
    }
    else
    {
        i = strtol(arg, NULL, 10);
        if(i < 0 || i >= jdwmconf->nlayouts)
            return;
        jdwmconf->current_layout = &jdwmconf->layouts[i];
    }

    for(c = clients; c; c = c->next)
        c->ftview = True;

    if(sel)
        arrange(disp, jdwmconf);
    else
        drawstatus(disp, jdwmconf);

    savejdwmprops(disp, jdwmconf);

    for(j = 0; j < jdwmconf->ntags; j++)
        if (jdwmconf->selected_tags[j])
            jdwmconf->tag_layouts[j] = jdwmconf->current_layout;
}

void
uicb_togglebar(Display *disp,
               jdwm_config *jdwmconf,
               const char *arg __attribute__ ((unused)))
{
    if(jdwmconf->current_bpos == BarOff)
        jdwmconf->current_bpos = (jdwmconf->bpos == BarOff) ? BarTop : jdwmconf->bpos;
    else
        jdwmconf->current_bpos = BarOff;
    updatebarpos(disp, jdwmconf->current_bpos);
    arrange(disp, jdwmconf);
}

static void
maximize(int x, int y, int w, int h, jdwm_config *jdwmconf)
{
    XEvent ev;

    if(!sel)
        return;

    if((sel->ismax = !sel->ismax))
    {
        sel->wasfloating = sel->isfloating;
        sel->isfloating = True;
        sel->rx = sel->x;
        sel->ry = sel->y;
        sel->rw = sel->w;
        sel->rh = sel->h;
        resize(sel, x, y, w, h, True);
    }
    else if(sel->isfloating)
        resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, True);
    else
        sel->isfloating = False;

    drawstatus(sel->display, jdwmconf);

    while(XCheckMaskEvent(sel->display, EnterWindowMask, &ev));
}

void
uicb_togglemax(Display *disp __attribute__ ((unused)),
               jdwm_config *jdwmconf,
               const char *arg __attribute__ ((unused)))
{
    maximize(wax, way, waw - 2 * jdwmconf->borderpx, wah - 2 * jdwmconf->borderpx, jdwmconf);
}

void
uicb_toggleverticalmax(Display *disp __attribute__ ((unused)),
                       jdwm_config *jdwmconf,
                       const char *arg __attribute__ ((unused)))
{
    if(sel)
        maximize(sel->x, way, sel->w, wah - 2 * jdwmconf->borderpx, jdwmconf);
}


void
uicb_togglehorizontalmax(Display *disp __attribute__ ((unused)),
                         jdwm_config *jdwmconf,
                         const char *arg __attribute__ ((unused)))
{
    if(sel)
        maximize(wax, sel->y, waw - 2 * jdwmconf->borderpx, sel->h, jdwmconf);
}

void 
uicb_zoom(Display *disp __attribute__ ((unused)), 
          jdwm_config *jdwmconf,
          const char *arg __attribute__ ((unused))) 
{ 
    if(!sel)
        return;
    detach(sel);
    attach(sel);
    focus(sel->display, &dc, sel, True, jdwmconf);
    arrange(sel->display, jdwmconf);
} 

