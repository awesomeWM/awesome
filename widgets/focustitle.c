#include <stdio.h>
#include "util.h"
#include "widget.h"


extern awesome_config globalconf;

static int
focustitle_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    Client *sel = globalconf.focus->client;
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int location = calculate_offset(vscreen.statusbar.width,
                                    0,
                                    offset,
                                    widget->alignment);

    if(sel)
    {
        drawtext(ctx, location, 0, vscreen.statusbar.width - used,
                 vscreen.statusbar.height, vscreen.font, sel->name,
                 vscreen.colors_selected);
        if(sel->isfloating)
            drawcircle(ctx, location, 0,
                       (vscreen.font->height + 2) / 4,
                       sel->ismax,
                       vscreen.colors_selected[ColFG]);
    }
    else
        drawtext(ctx, location, 0, vscreen.statusbar.width - used,
                 vscreen.statusbar.height, vscreen.font, NULL,
                 vscreen.colors_normal);
    return vscreen.statusbar.width - used;
}

Widget *
focustitle_new(Statusbar *statusbar, const char *name)
{
    Widget *w;
    w = p_new(Widget, 1);
    w->draw = focustitle_draw;
    common_new(w, statusbar, name);
    w->alignment = AlignFlex;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
