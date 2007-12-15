#include "util.h"
#include "widget.h"

static char name[] = "textbox";

static int
textbox_draw(DrawCtx *ctx,
             awesome_config *awesomeconf __attribute__ ((unused)),
             VirtScreen vscreen,
             int screen __attribute__ ((unused)),
             int offset, 
             int used __attribute__ ((unused)),
             int align)
{
    int width = textwidth(ctx, vscreen.font, vscreen.statustext);
    int location = calculate_offset(vscreen.statusbar.width, width, offset, align);
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
