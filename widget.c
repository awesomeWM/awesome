#include "widget.h"

void
calculate_alignments(Widget *widget)
{
    for(; widget; widget = widget->next)
    {
        if(widget->alignment == AlignFlex)
        {
            widget = widget->next;
            break;
        }
        widget->alignment = AlignLeft;
    }

    if(widget)
        for(; widget; widget = widget->next)
            widget->alignment = AlignRight;
}

int
calculate_offset(int barwidth, int widgetwidth, int offset, int alignment)
{
    if (alignment == AlignLeft || alignment == AlignFlex)
        return offset;

    return barwidth - offset - widgetwidth;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
