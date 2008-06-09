/*
 * workspacelist.c - workspace list widget
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
 * Copyright © 2007 Aldo Cortesi <aldo@nullcube.com>
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

#include "client.h"
#include "widget.h"
#include "workspace.h"
#include "lua.h"
#include "event.h"
#include "common/markup.h"
#include "common/configopts.h"

extern awesome_t globalconf;

typedef struct workspacelist_drawn_area_t workspacelist_drawn_area_t;
struct workspacelist_drawn_area_t
{
    void *object;
    area_t *area;
    workspacelist_drawn_area_t *next, *prev;
};

DO_SLIST(workspacelist_drawn_area_t, workspacelist_drawn_area, p_delete);

typedef struct
{
    char *text_normal, *text_focus, *text_urgent;
    bool show_empty;
    workspacelist_drawn_area_t *drawn_area;

} workspacelist_data_t;

static char *
workspace_markup_parse(workspace_t *t, const char *str, ssize_t len)
{
    const char *elements[] = { "title", NULL };
    char *title_esc = g_markup_escape_text(t->name, -1);
    const char *elements_sub[] = { title_esc , NULL };
    markup_parser_data_t *p;
    char *ret;

    p = markup_parser_data_new(elements, elements_sub, countof(elements));

    if(markup_parse(p, str, len))
    {
        ret = p->text;
        p->text = NULL;
    }
    else
        ret = a_strdup(str);

    markup_parser_data_delete(&p);
    p_delete(&title_esc);

    return ret;
}

/** Check if at least one client is on the workspace.
 * \param ws The workspace.
 * \return True or false.
 */
static bool
workspace_isoccupied(workspace_t *ws)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(workspace_client_get(c) == ws)
            return true;

    return false;
}

static bool
workspace_isurgent(workspace_t *ws)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(c->isurgent && workspace_client_get(c) == ws)
            return true;

    return false;
}

static char *
workspacelist_text_get(workspace_t *ws, screen_t *screen, workspacelist_data_t *data)
{
    if(screen->workspace == ws)
        return data->text_focus;

    if(workspace_isurgent(ws))
        return data->text_urgent;

    return data->text_normal;
}

static int
workspacelist_draw(draw_context_t *ctx, int screen,
                   widget_node_t *w,
                   int offset,
                   int used __attribute__ ((unused)),
                   void *object)
{
    workspace_t *ws;
    workspacelist_data_t *data = w->widget->data;
    int i = 0, prev_width = 0;
    area_t *area, rectangle = { 0, 0, 0, 0, NULL, NULL };
    char **text = NULL;
    workspacelist_drawn_area_t *tda;

    w->area.width = w->area.y = 0;

    /* Lookup for our workspacelist_drawn_area.
     * This will be used to store area where we draw ws list for each object.  */
    for(tda = data->drawn_area; tda && tda->object != object; tda = tda->next);

    /* Oh, we did not find a drawn area for our object. First time? */
    if(!tda)
    {
        /** \todo delete this when the widget is removed from the object */
        tda = p_new(workspacelist_drawn_area_t, 1);
        tda->object = object;
        workspacelist_drawn_area_list_push(&data->drawn_area, tda);
    }

    area_list_wipe(&tda->area);

    /* First compute text and widget width */
    for(ws = globalconf.workspaces; ws; ws = ws->next, i++)
    {
        p_realloc(&text, i + 1);
        area = p_new(area_t, 1);
        text[i] = workspacelist_text_get(ws, &globalconf.screens[screen], data);
        text[i] = workspace_markup_parse(ws, text[i], a_strlen(text[i]));
        *area = draw_text_extents(ctx->connection, ctx->phys_screen,
                                  globalconf.font, text[i]);

        if (data->show_empty || workspace_isoccupied(ws))
            w->area.width += area->width;

        area_list_append(&tda->area, area);
    }

    /* Now that we have widget width we can compute widget x coordinate */
    w->area.x = widget_calculate_offset(ctx->width, w->area.width,
                                        offset, w->widget->align); 

    for(area = tda->area, ws = globalconf.workspaces, i = 0;
        ws && area;
        ws = ws->next, area = area->next, i++)
    {
        if (!data->show_empty && !workspace_isoccupied(ws))
            continue;

        area->x = w->area.x + prev_width;
        prev_width += area->width;
        draw_text(ctx, globalconf.font, *area, text[i]);
        p_delete(&text[i]);

        if(workspace_isoccupied(ws))
        {
            rectangle.width = rectangle.height = (globalconf.font->height + 2) / 3;
            rectangle.x = area->x;
            rectangle.y = area->y;
            draw_rectangle(ctx, rectangle, 1.0, false, ctx->fg);
        }
    }

    p_delete(&text);

    w->area.height = ctx->height;
    return w->area.width;
}

/** Handle button click on tasklist.
 * \param w The widget node.
 * \param ev The button press event.
 * \param screen The screen where the click was.
 * \param object The object we're onto.
 * \param type The type object.
 */
static void
workspacelist_button_press(widget_node_t *w,
                           xcb_button_press_event_t *ev,
                           int screen __attribute__ ((unused)),
                           void *object,
                           awesome_type_t type)
{
    button_t *b;
    workspacelist_data_t *data = w->widget->data;
    workspacelist_drawn_area_t *tda;
    area_t *area;
    workspace_t *ws;

    /* Find the good drawn area list */
    for(tda = data->drawn_area; tda && tda->object != object; tda = tda->next);
    area = tda->area;

    for(b = w->widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
            for(ws = globalconf.workspaces; ws && area; ws = ws->next, area = area->next)
                if(ev->event_x >= AREA_LEFT(*area)
                   && ev->event_x < AREA_RIGHT(*area)
                   && (data->show_empty || workspace_isoccupied(ws)) )
                {
                    luaA_pushpointer(object, type);
                    luaA_workspace_userdata_new(ws);
                    luaA_dofunction(globalconf.L, b->fct, 2);
                    return;
                }
}

static widget_tell_status_t
workspacelist_tell(widget_t *widget, const char *property, const char *new_value)
{
    workspacelist_data_t *d = widget->data;

    if(!a_strcmp(property, "text_normal"))
    {
        p_delete(&d->text_normal);
        d->text_normal = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "text_focus"))
    {
        p_delete(&d->text_focus);
        d->text_focus = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "text_urgent"))
    {
        p_delete(&d->text_urgent);
        d->text_urgent = a_strdup(new_value);
    }
    else if(!a_strcmp(property, "show_empty"))
        d->show_empty = a_strtobool(new_value);
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

widget_t *
workspacelist_new(alignment_t align)
{
    widget_t *w;
    workspacelist_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = workspacelist_draw;
    w->button_press = workspacelist_button_press;
    w->tell = workspacelist_tell;

    w->data = d = p_new(workspacelist_data_t, 1);
    d->text_normal = a_strdup(" <text align=\"center\"/><title/> ");
    d->text_focus = a_strdup(" <text align=\"center\"/><title/> ");
    d->text_urgent = a_strdup(" <text align=\"center\"/><title/> ");
    d->show_empty = true;

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_WORKSPACES | WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
