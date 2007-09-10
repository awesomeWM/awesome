/* See LICENSE file for copyright and license details. */

#include "tag.h"
#include "layouts/floating.h"

/* extern */
extern Client *clients;         /* global client */

void
floating(Display *disp __attribute__ ((unused)), awesome_config *awesomeconf)
{                               /* default floating layout */
    Client *c;

    for(c = clients; c; c = c->next)
        if(isvisible(c, awesomeconf->selected_tags, awesomeconf->ntags))
        {
            if(c->ftview)
            {
                resize(c, c->rx, c->ry, c->rw, c->rh, True);
                c->ftview = False;
            }
            else
                resize(c, c->x, c->y, c->w, c->h, True);
        }
}
