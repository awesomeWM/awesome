/*
 * widgets/common.c - some functions used by widgets
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2008 Marco Candrian <mac@calmar.ws>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "widgets/common.h"

extern AwesomeConf globalconf;

widget_tell_status_t
widget_set_color_for_data(widget_t *widget, xcolor_t *color, char *new_value, int data_items, char ** data_title)
{
    char *title, *setting;
    int i;
    title = strtok(new_value, " ");
    if(!(setting = strtok(NULL, " ")))
        return WIDGET_ERROR_NOVALUE;
    for(i = 0; i < data_items; i++)
        if(!a_strcmp(title, data_title[i]))
        {
            if(draw_color_new(globalconf.connection,
                              widget->statusbar->phys_screen,
                              setting, &color[i]))
                return WIDGET_NOERROR;
            else
                return WIDGET_ERROR_FORMAT_COLOR;
        }
    return WIDGET_ERROR_FORMAT_SECTION;
}
widget_tell_status_t
widget_set_color_pointer_for_data(widget_t *widget, xcolor_t **color, char *new_value, int data_items, char ** data_title)
{
    char *title, *setting;
    int i;
    bool flag;
    title = strtok(new_value, " ");
    if(!(setting = strtok(NULL, " ")))
        return WIDGET_ERROR_NOVALUE;
    for(i = 0; i < data_items; i++)
        if(!a_strcmp(title, data_title[i]))
        {
            flag = false;
            if(!color[i])
            {
                flag = true; /* p_delete && restore to NULL, if draw_color_new unsuccessful */
                color[i] = p_new(xcolor_t, 1);
            }
            if(!(draw_color_new(globalconf.connection,
                            widget->statusbar->phys_screen,
                            setting, color[i])))
            {
                if(flag) /* restore */
                {
                    p_delete(&color[i]);
                    color[i] = NULL;
                }
                return WIDGET_ERROR_FORMAT_COLOR;
            }
            return WIDGET_NOERROR;
        }
    return WIDGET_ERROR_FORMAT_SECTION;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
