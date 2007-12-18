#include <confuse.h>
#include "widget.h"
#include "util.h"
#include "layout.h"

extern awesome_config globalconf;

static int
layoutinfo_draw(Widget *widget,
                DrawCtx *ctx,
                int offset,
                int used __attribute__ ((unused)))
{
    int width = 0, location; 
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    Layout *l;
    for(l = vscreen.layouts ; l; l = l->next)
        width = MAX(width, (textwidth(ctx, vscreen.font, l->symbol)));
    location = calculate_offset(vscreen.statusbar->width,
                                width,
                                offset,
                                widget->alignment);
    drawtext(ctx, location, 0,
             width,
             vscreen.statusbar->height,
             vscreen.font,
             get_current_layout(widget->statusbar->screen)->symbol,
             vscreen.colors_normal[ColFG],
             vscreen.colors_normal[ColBG]);
    return width;
}


Widget *
layoutinfo_new(Statusbar *statusbar, cfg_t* config)
{
    Widget *w;
    w = p_new(Widget, 1);
    common_new(w, statusbar, config);
    w->draw = (void*) layoutinfo_draw;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
