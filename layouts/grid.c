/* See LICENSE file for copyright and license details. */

#include "grid.h"
#include "layout.h"
#include "tag.h"

extern int wah, waw;            /* windowarea geometry */
extern Client *clients;         /* global client list and stack */
extern DC dc;

void
grid(Display *disp, awesome_config *awesomeconf)
{
    unsigned int i, n, cx, cy, cw, ch, aw, ah, cols, rows;
    Client *c;

    for(n = 0, c = clients; c; c = c->next)
        if(IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            n++;

    /* grid dimensions */
    for(rows = 0; rows <= n / 2; rows++)
        if(rows * rows >= n)
            break;
    cols = (rows && (rows - 1) * rows >= n) ? rows - 1 : rows;

    /* window geoms (cell height/width) */
    ch = wah / (rows ? rows : 1);
    cw = waw / (cols ? cols : 1);

    for(i = 0, c = clients; c; c = c->next)
    {
        if(!IS_TILED(c, awesomeconf->selected_tags, awesomeconf->ntags))
            continue;
        c->ismax = False;
        cx = (i / rows) * cw;
        cy = (i % rows) * ch + (awesomeconf->statusbar.position == BarTop ? awesomeconf->statusbar.height : 0);   // bh? adjust
        /* adjust height/width of last row/column's windows */
        ah = ((i + 1) % rows == 0) ? wah - ch * rows : 0;
        aw = (i >= rows * (cols - 1)) ? waw - cw * cols : 0;
        resize(c, cx, cy, cw - 2 * c->border + aw, ch - 2 * c->border + ah, False);
        i++;
    }

    focus(disp, &dc, NULL, True, awesomeconf);
    restack(disp, awesomeconf);
}
