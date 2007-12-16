#include "util.h"
#include "widget.h"

extern awesome_config globalconf;

static char name[] = "textbox";

static int
textbox_draw(Widget *widget,
             DrawCtx *ctx,
             int offset, 
             int used __attribute__ ((unused)))
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int width = textwidth(ctx, vscreen.font, vscreen.statustext);
    int location = calculate_offset(vscreen.statusbar.width, width, offset, widget->alignment);
    drawtext(ctx, location, 0, width, vscreen.statusbar.height,
             vscreen.font, vscreen.statustext, vscreen.colors_normal);
    return width;
}

Widget *
textbox_new(Statusbar *statusbar)
{
    Widget *w;
    w = p_new(Widget, 1);
    w->draw = textbox_draw;
    w->statusbar = statusbar;
    w->name = name;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
