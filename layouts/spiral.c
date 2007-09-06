/* See LICENSE file for copyright and license details. */

#include "layout.h"
#include "tag.h"
#include "spiral.h"

extern int wax, way, wah, waw;  /* windowarea geometry */
extern Client *clients;         /* global client list */
extern DC dc;

static void
fibonacci(Display *disp, jdwm_config *jdwmconf, int shape)
{
    int n, nx, ny, nh, nw, i;
    Client *c;

    nx = wax;
    ny = 0;
    nw = waw;
    nh = wah;
    for(n = 0, c = nexttiled(clients, jdwmconf->selected_tags, jdwmconf->ntags); c; c = nexttiled(c->next, jdwmconf->selected_tags, jdwmconf->ntags))
        n++;
    for(i = 0, c = clients; c; c = c->next)
    {
        c->ismax = False;
        if((i % 2 && nh / 2 > 2 * c->border)
           || (!(i % 2) && nw / 2 > 2 * c->border))
        {
            if(i < n - 1)
            {
                if(i % 2)
                    nh /= 2;
                else
                    nw /= 2;
                if((i % 4) == 2 && !shape)
                    ny += nh;
            }
            if((i % 4) == 0)
            {
                if(shape)
                    ny += nh;
                else
                    ny -= nh;
            }
            else if((i % 4) == 1)
                nx += nw;
            else if((i % 4) == 2)
                ny += nh;
            else if((i % 4) == 3)
            {
                if(shape)
                    nx += nw;
                else
                    nx -= nw;
            }
            if(i == 0)
                ny = way;
            i++;
        }
        resize(c, nx, ny, nw - 2 * c->border, nh - 2 * c->border, False);
    }
    focus(disp, &dc, NULL, jdwmconf);
    restack(disp, jdwmconf);
}


void
dwindle(Display *disp, jdwm_config *jdwmconf)
{
    fibonacci(disp, jdwmconf, 1);
}

void
spiral(Display *disp, jdwm_config *jdwmconf)
{
    fibonacci(disp, jdwmconf, 0);
}
