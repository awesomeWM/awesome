#ifndef AWESOME_WIDGET_H
#define AWESOME_WIDGET_H

#include "config.h"
#include "draw.h"

enum { AlignLeft, AlignRight, AlignFlex };

typedef Widget *(WidgetConstructor)(Statusbar*, const char*);

int calculate_offset(int, int, int, int);
void calculate_alignments(Widget *widget);
void common_new(Widget *, Statusbar *, const char *);

WidgetConstructor layoutinfo_new;
WidgetConstructor taglist_new;
WidgetConstructor textbox_new;
WidgetConstructor focustitle_new;

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
