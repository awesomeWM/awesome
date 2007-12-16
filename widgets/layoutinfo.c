#include "widget.h"
#include "util.h"
#include "layout.h"

extern awesome_config globalconf;

static char name[] = "layoutinfo";

static int
layoutinfo_draw(DrawCtx *ctx, int screen, int offset, 
                  int used __attribute__ ((unused)),
                  int align __attribute__ ((unused)))
{
    int width = 0, location; 
    VirtScreen vscreen = globalconf.screens[screen];
    Layout *l;
    for(l = vscreen.layouts ; l; l = l->next)
        width = MAX(width, (textwidth(ctx, vscreen.font, l->symbol)));
    location = calculate_offset(vscreen.statusbar.width, width, offset, align);
    drawtext(ctx, location, 0,
             width,
             vscreen.statusbar.height,
             vscreen.font,
             get_current_layout(screen)->symbol,
             vscreen.colors_normal);
    return width;
}


Widget *
layoutinfo_new(Statusbar *statusbar)
{
    Widget *w;
    w = p_new(Widget, 1);
    w->draw = (void*) layoutinfo_draw;
    w->statusbar = statusbar;
    w->name = name;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
