#ifndef AWESOME_WIDGET_H
#define AWESOME_WIDGET_H

#include "config.h"
#include "draw.h"

enum { AlignLeft, AlignRight, AlignFlex };

typedef Widget_ptr (WidgetConstructor)(Statusbar *);

int calculate_offset(int, int, int, int);
void calculate_alignments(Widget *widget);

Widget_ptr layoutinfo_new(Statusbar*);
Widget_ptr taglist_new(Statusbar*);
Widget_ptr textbox_new(Statusbar*);
Widget_ptr focustitle_new(Statusbar*);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
