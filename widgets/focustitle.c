#include <stdio.h>
#include <confuse.h>
#include "util.h"
#include "widget.h"
#include "layout.h"
#include "focus.h"

extern AwesomeConf globalconf;

static int
focustitle_draw(Widget *widget, DrawCtx *ctx, int offset, int used)
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    Client *sel = focus_get_latest_client_for_tag(widget->statusbar->screen, 
                                                  get_current_tag(widget->statusbar->screen));

    widget->location = widget_calculate_offset(vscreen.statusbar->width,
                                               0,
                                               offset,
                                               widget->alignment);

    if(sel)
    {
        draw_text(ctx, widget->location, 0, vscreen.statusbar->width - used,
                  vscreen.statusbar->height, vscreen.font, sel->name,
                  vscreen.colors_selected[ColFG],
                  vscreen.colors_selected[ColBG]);
        if(sel->isfloating)
            draw_circle(ctx, widget->location, 0,
                        (vscreen.font->height + 2) / 4,
                        sel->ismax,
                        vscreen.colors_selected[ColFG]);
    }
    else
        draw_text(ctx, widget->location, 0, vscreen.statusbar->width - used,
                  vscreen.statusbar->height, vscreen.font, NULL,
                  vscreen.colors_normal[ColFG],
                  vscreen.colors_normal[ColBG]);

    widget->width = vscreen.statusbar->width - used;

    return widget->width;
}

Widget *
focustitle_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    w = p_new(Widget, 1);
    widget_common_new(w, statusbar, config);
    w->draw = focustitle_draw;
    w->alignment = AlignFlex;
    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
