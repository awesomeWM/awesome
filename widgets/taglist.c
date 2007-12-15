#include "config.h"
#include "util.h"
#include "widget.h"
#include "tag.h"

static char name[] = "taglist";

/** Check if at least a client is tagged with tag number t and is on screen
 * screen
 * \param t tag number
 * \param screen screen number
 * \return True or False
 */
static Bool
isoccupied(TagClientLink *tc, int screen, Client *head, Tag *t)
{
    Client *c;

    for(c = head; c; c = c->next)
        if(c->screen == screen && is_client_tagged(tc, c, t))
            return True;
    return False;
}


static int
taglist_draw(DrawCtx *ctx,
               awesome_config *awesomeconf,
               VirtScreen vscreen,
               int screen,
               int offset,
               int used __attribute__ ((unused)),
               int align __attribute__ ((unused)))
{
    Tag *tag;
    Client *sel = awesomeconf->focus->client;
    int w = 0, width = 0, location;
    int flagsize;
    XColor *colors;

    flagsize = (vscreen.font->height + 2) / 4;

    for(tag = vscreen.tags; tag; tag = tag->next)
    {
        width  += textwidth(ctx, vscreen.font, tag->name);
    }
    location = calculate_offset(vscreen.statusbar.width, width, offset, align);

    width = 0;
    for(tag = vscreen.tags; tag; tag = tag->next)
    {
        w = textwidth(ctx, vscreen.font, tag->name);
        if(tag->selected)
            colors = vscreen.colors_selected;
        else
            colors = vscreen.colors_normal;
        drawtext(ctx, location + width, 0, w,
                 vscreen.statusbar.height, vscreen.font, tag->name, colors);
        if(isoccupied(vscreen.tclink, screen, awesomeconf->clients, tag))
            drawrectangle(ctx, location + width, 0, flagsize, flagsize,
                          sel && is_client_tagged(vscreen.tclink, sel, tag),
                          colors[ColFG]);
        width += w;
    }
    return width;
}


Widget *
taglist_new(Statusbar *statusbar)
{
    Widget *w;
    w = p_new(Widget, 1);
    w->draw = (void*) taglist_draw;
    w->statusbar = statusbar;
    w->name = name;
    return w;
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
