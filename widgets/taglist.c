#include "config.h"
#include "util.h"
#include "widget.h"
#include "tag.h"

extern awesome_config globalconf;

/** Check if at least one client is tagged with tag number t and is on screen
 * screen
 * \param screen screen number
 * \param t tag
 * \return True or False
 */
static Bool
isoccupied(int screen, Tag *t)
{
    Client *c;

    for(c = globalconf.clients; c; c = c->next)
        if(c->screen == screen && is_client_tagged(c, t, screen))
            return True;
    return False;
}

static int
taglist_draw(Widget *widget,
             DrawCtx *ctx,
             int offset,
             int used __attribute__ ((unused)))
{
    Tag *tag;
    Client *sel = globalconf.focus->client;
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int w = 0, width = 0, location;
    int flagsize;
    XColor *colors;

    flagsize = (vscreen.font->height + 2) / 4;

    for(tag = vscreen.tags; tag; tag = tag->next)
    {
        width  += textwidth(ctx, vscreen.font, tag->name);
    }
    location = calculate_offset(vscreen.statusbar.width,
                                width,
                                offset,
                                widget->alignment);

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
        if(isoccupied(widget->statusbar->screen, tag))
            drawrectangle(ctx, location + width, 0, flagsize, flagsize,
                          sel && is_client_tagged(sel,
                                                  tag,
                                                  widget->statusbar->screen),
                          colors[ColFG]);
        width += w;
    }
    return width;
}

Widget *
taglist_new(Statusbar *statusbar, const char *name)
{
    Widget *w;
    w = p_new(Widget, 1);
    w->draw = taglist_draw;
    common_new(w, statusbar, name);
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
