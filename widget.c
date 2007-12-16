#include "util.h"
#include "widget.h"

extern awesome_config globalconf;

const NameFuncLink WidgetList[] =
{
    {"taglist", taglist_new},
    {"layoutinfo", layoutinfo_new},
    {"focustitle", focustitle_new},
    {"textbox", textbox_new},
    {NULL, NULL}
};

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
        for(; widget; widget = widget->next){
            if (widget->alignment == AlignFlex)
                warn("Multiple flex widgets in panel -"
                     " ignoring flex for all but the first.");
            widget->alignment = AlignRight;
        }
}

int
calculate_offset(int barwidth, int widgetwidth, int offset, int alignment)
{
    if (alignment == AlignLeft || alignment == AlignFlex)
        return offset;
    return barwidth - offset - widgetwidth;
}


static Widget *
find_widget(char *name, int screen)
{
    Widget *widget;
    widget = globalconf.screens[screen].statusbar.widgets;
    for(; widget; widget = widget->next)
        if (strcmp(name, widget->name) == 0)
            return widget;
    return NULL;
}

void
common_new(Widget *widget, Statusbar *statusbar, const char *name)
{
    widget->statusbar = statusbar;
    widget->name = p_new(char, strlen(name)+1);
    strncpy(widget->name, name, strlen(name));
}

void
uicb_widget_tell(int screen, char *arg)
{
    Widget *widget;
    char *p;
    p = strtok(arg, " ");
    if (!p){
        warn("Ignoring malformed widget command.");
        return;
    }
    widget = find_widget(p, screen);
    if (!widget){
        warn("No such widget: %s\n", p);
        return;
    }

    /* To be continued... */

}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
