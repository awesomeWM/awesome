#include  <confuse.h>
#include "util.h"
#include "widget.h"
#include "statusbar.h"

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
    widget = globalconf.screens[screen].statusbar->widgets;
    for(; widget; widget = widget->next)
        if (strcmp(name, widget->name) == 0)
            return widget;
    return NULL;
}


static void
common_tell(Widget *widget, char *command __attribute__ ((unused)))
{
    warn("%s widget does not accept commands.\n", widget->name);
}


void
common_new(Widget *widget, Statusbar *statusbar, cfg_t* config)
{
    const char *name;
    widget->statusbar = statusbar;

    name = cfg_title(config);
    widget->name = p_new(char, strlen(name)+1);
    widget->tell = common_tell;
    strncpy(widget->name, name, strlen(name));
}


void
uicb_widget_tell(int screen, char *arg)
{
    Widget *widget;
    char *p, *command;
    int len;

    if (!arg){
        warn("Must specify a widget.\n");
        return;
    }

    len = strlen(arg);
    p = strtok(arg, " ");
    if (!p){
        warn("Ignoring malformed widget command.\n");
        return;
    }
    widget = find_widget(p, screen);
    if (!widget){
        warn("No such widget: %s\n", p);
        return;
    }

    if (p+strlen(p) < arg+len)
    {
        p = p + strlen(p) + 1;
        command = p_new(char, strlen(p)+1);
        strncpy(command, p, strlen(p));
        widget->tell(widget, command);
        p_delete(&command);
    }
    else
        widget->tell(widget, NULL);
    statusbar_draw(screen);
    return;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
