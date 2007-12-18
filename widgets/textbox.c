#include <confuse.h>
#include "util.h"
#include "widget.h"

extern awesome_config globalconf;


typedef struct Data Data;
struct Data
{
    char *text;
};


static void
update(Widget *widget, char *text){
    Data *d;
    d = (Data*) widget->data;
    if (d->text)
        p_delete(&d->text);
    d->text = a_strdup(text);
}


static int
textbox_draw(Widget *widget, DrawCtx *ctx, int offset,
             int used __attribute__ ((unused)))
{
    VirtScreen vscreen = globalconf.screens[widget->statusbar->screen];
    int width, location;
    Data *d;
    d = (Data*) widget->data;
    width = textwidth(ctx, vscreen.font, d->text);
    location = calculate_offset(vscreen.statusbar.width,
                                width,
                                offset,
                                widget->alignment);
    drawtext(ctx, location, 0, width, vscreen.statusbar.height,
             vscreen.font, d->text,
             vscreen.colors_normal[ColFG], 
             vscreen.colors_normal[ColBG]);
    return width;
}


static void
textbox_tell(Widget *widget, char *command)
{
    update(widget, command);
}


Widget *
textbox_new(Statusbar *statusbar, cfg_t *config)
{
    Widget *w;
    Data *d;

    w = p_new(Widget, 1);
    common_new(w, statusbar, config);
    w->draw = textbox_draw;
    w->tell = textbox_tell;

    d = p_new(Data, 1);
    w->data = (void*) d;

    update(w, cfg_getstr(config, "default"));
    return w;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
