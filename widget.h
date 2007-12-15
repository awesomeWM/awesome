#ifndef AWESOME_WIDGET_H
#define AWESOME_WIDGET_H

#include "config.h"
#include "draw.h"

enum { AlignLeft, AlignRight, AlignFlex };

int calculate_offset(int, int, int, int);
void calculate_alignments(Widget *widget);

Widget *layoutinfo_new(Statusbar*);
Widget *taglist_new(Statusbar*);
Widget *textbox_new(Statusbar*);
Widget *focustitle_new(Statusbar*);

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
