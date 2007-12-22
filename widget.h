#ifndef AWESOME_WIDGET_H
#define AWESOME_WIDGET_H

#include <confuse.h>
#include "config.h"
#include "draw.h"
#include "common.h"

enum { AlignLeft, AlignRight, AlignFlex };

typedef Widget *(WidgetConstructor)(Statusbar*, cfg_t*);

int calculate_offset(int, int, int, int);
void calculate_alignments(Widget *widget);
void common_new(Widget*, Statusbar*, cfg_t*);

WidgetConstructor layoutinfo_new;
WidgetConstructor taglist_new;
WidgetConstructor textbox_new;
WidgetConstructor focustitle_new;
WidgetConstructor iconbox_new;

UICB_PROTO(uicb_widget_tell);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
