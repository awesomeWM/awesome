
#include "widget.h"


void
calculate_alignments(Widget *widget)
{
    while (widget){
        printf("%s\n", widget->name);
        if (widget->alignment == AlignFlex){
            widget = widget->next;
            break;
        }
        widget->alignment = AlignLeft;
        widget = widget->next;
    }
    if (widget){
        while (widget){ 
            widget->alignment = AlignRight;
            widget = widget->next;
        }
    }
}

int
calculate_offset(int barwidth, int widgetwidth, int offset, int alignment)
{
    if (alignment == AlignLeft || alignment == AlignFlex)
        return offset;
    else
        return barwidth - offset - widgetwidth;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
