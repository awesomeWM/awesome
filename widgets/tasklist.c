/*
 * tasklist.c - task list widget
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#include "widget.h"
#include "focus.h"
#include "ewmh.h"
#include "tag.h"
#include "common/markup.h"
#include "common/tokenize.h"

extern awesome_t globalconf;

/** Link a client and a label */
typedef struct
{
    /** A client */
    client_t *client;
    /** The client label */
    char *label;
    /** The client label len */
    size_t label_len;
} client_label_t;

/** Delete a client label.
 * \param l The client label.
 */
static void
client_label_wipe(client_label_t *l)
{
    p_delete(&l->label);
}

DO_ARRAY(client_label_t, client_label, client_label_wipe)

typedef struct tasklist_object_data_t tasklist_object_data_t;
/** Link an object with a client label array and other infos */
struct tasklist_object_data_t
{
    /** The object */
    void *object;
    /** The box width for each client */
    int box_width;
    /** The client label array for the object */
    client_label_array_t client_labels;
    /** This is a list */
    tasklist_object_data_t *prev, *next;
};

static void
tasklist_object_data_delete(tasklist_object_data_t **l)
{
    client_label_array_wipe(&(*l)->client_labels);
    p_delete(l);
}

DO_SLIST(tasklist_object_data_t, tasklist_object_data, tasklist_object_data_delete)

/** The tasklist private data structure. */
typedef struct
{
    bool show_icons;
    luaA_function label;
    tasklist_object_data_t *objects_data;
} tasklist_data_t;

/** Get an object data by its object.
 * \param od The object data list.
 * \param p The object.
 * \return A object data or NULL if not found.
 */
static tasklist_object_data_t *
tasklist_object_data_getbyobj(tasklist_object_data_t *od, void *p)
{
    tasklist_object_data_t *o;

    for(o = od; o; o = o->next)
        if(o->object == p)
            return o;

    return NULL;
}

/** Draw a tasklist widget.
 * \param ctx The draw context.
 * \param screen The screen number.
 * \param w The widget node we are called from.
 * \param offset The offset to draw at.
 * \param used The already used width.
 * \param p A pointer to the object we're drawing onto.
 */
static int
tasklist_draw(draw_context_t *ctx, int screen,
              widget_node_t *w,
              int offset, int used, void *p)
{
    client_t *c;
    tasklist_data_t *d = w->widget->data;
    area_t area;
    int i = 0, icon_width = 0, box_width_rest = 0;
    netwm_icon_t *icon;
    draw_image_t *image;
    draw_parser_data_t pdata, *parser_data;
    tasklist_object_data_t *odata;

    if(used >= ctx->width)
        return (w->area.width = 0);

    if(!(odata = tasklist_object_data_getbyobj(d->objects_data, p)))
    {
        odata = p_new(tasklist_object_data_t, 1);
        odata->object = p;
        tasklist_object_data_list_push(&d->objects_data, odata);
    }

    client_label_array_wipe(&odata->client_labels);
    client_label_array_init(&odata->client_labels);

    for(c = globalconf.clients; c; c = c->next)
        if(!c->skip && !c->skiptb)
        {
            /* push client */
            luaA_client_userdata_new(globalconf.L, c);
            /* push screen we're at */
            lua_pushnumber(globalconf.L, screen + 1);
            /* call label function with client as argument and wait for one
             * result */
            if(luaA_dofunction(globalconf.L, d->label, 2, 1))
            {
                /* If we got a string as returned value, we got something to write:
                 * a label. So we store it in a client_label_t structure, pushed
                 * into the client_label_array_t which is owned by the object. */
                if(lua_isstring(globalconf.L, -1))
                {
                    client_label_t cl;
                    cl.client = c;
                    cl.label = a_strdup(lua_tolstring(globalconf.L, -1, &cl.label_len));
                    client_label_array_append(&odata->client_labels, cl);
                }

                lua_pop(globalconf.L, 1);
            }
        }

    if(!odata->client_labels.len)
        return (w->area.width = 0);

    odata->box_width = (ctx->width - used) / odata->client_labels.len;
    /* compute how many pixel we left empty */
    box_width_rest = (ctx->width - used) % odata->client_labels.len;

    w->area.x = widget_calculate_offset(ctx->width,
                                        0, offset, w->widget->align);

    w->area.y = 0;

    for(i = 0; i < odata->client_labels.len; i++)
    {
        icon_width = 0;

        if(d->show_icons)
        {
            draw_parser_data_init(&pdata);

            pdata.connection = ctx->connection;
            pdata.phys_screen = ctx->phys_screen;

            /* Actually look for the proper background color, since
             * otherwise the background statusbar color is used instead */
            if(draw_text_markup_expand(&pdata,
                                       odata->client_labels.tab[i].label,
                                       odata->client_labels.tab[i].label_len))
            {
                parser_data = &pdata;
                if(pdata.has_bg_color)
                {
                    /* draw a background for icons */
                    area.x = w->area.x + odata->box_width * i;
                    area.y = w->area.y;
                    area.height = ctx->height;
                    area.width = odata->box_width;
                    draw_rectangle(ctx, area, 1.0, true, &pdata.bg_color);
                }
            }
            else
                parser_data = NULL;

            if((image = draw_image_new(odata->client_labels.tab[i].client->icon_path)))
            {
                icon_width = ((double) ctx->height / (double) image->height) * image->width;
                draw_image(ctx, w->area.x + odata->box_width * i,
                           w->area.y, ctx->height, image);
                draw_image_delete(&image);
            }

            if(!icon_width && (icon = ewmh_get_window_icon(odata->client_labels.tab[i].client->win)))
            {
                icon_width = ((double) ctx->height / (double) icon->height)
                    * icon->width;
                draw_image_from_argb_data(ctx,
                                          w->area.x + odata->box_width * i,
                                          w->area.y,
                                          icon->width, icon->height,
                                          ctx->height, icon->image);
                p_delete(&icon->image);
                p_delete(&icon);
            }
        }
        else
            parser_data = NULL;

        area.x = w->area.x + icon_width + odata->box_width * i;
        area.y = w->area.y;
        area.width = odata->box_width - icon_width;
        area.height = ctx->height;

        /* if we're on last elem, it has the last pixels left */
        if(i == odata->client_labels.len - 1)
            area.width += box_width_rest;

        draw_text(ctx, globalconf.font, area,
                  odata->client_labels.tab[i].label,
                  odata->client_labels.tab[i].label_len,
                  parser_data);
        draw_parser_data_wipe(parser_data);
    }

    w->area.width = ctx->width - used;
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
tasklist_button_press(widget_node_t *w,
                      xcb_button_press_event_t *ev,
                      int screen,
                      void *object,
                      awesome_type_t type)
{
    button_t *b;
    tasklist_data_t *d = w->widget->data;
    int ci = 0;
    tasklist_object_data_t *odata;

    odata = tasklist_object_data_getbyobj(d->objects_data, object);

    if(!odata || !odata->client_labels.len)
        return;

    ci = (ev->event_x - w->area.x) / odata->box_width;

    for(b = w->widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
        {
            luaA_pushpointer(globalconf.L, object, type);
            luaA_client_userdata_new(globalconf.L, odata->client_labels.tab[ci].client);
            luaA_dofunction(globalconf.L, b->fct, 2, 0);
        }
}

/** Tasklist widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield show_icons Show icons near client title.
 * \lfield label Function used to get the string to display as the window title.
 * It gets the client and a screen number as argument, and must return a string.
 */
static int
luaA_tasklist_index(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    tasklist_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_SHOW_ICONS:
        lua_pushboolean(L, d->show_icons);
        return 1;
      case A_TK_LABEL:
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->label);
        return 1;
      default:
        return 0;
    }
}

/** Newindex function for tasklist widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 */
static int
luaA_tasklist_newindex(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    tasklist_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_SHOW_ICONS:
        d->show_icons = luaA_checkboolean(L, 3);
        break;
      case A_TK_LABEL:
        luaA_registerfct(L, &d->label);
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(*widget);

    return 0;
}

/** Destructor for the tasklist widget.
 * \param widget The widget to destroy.
 */
static void
tasklist_destructor(widget_t *widget)
{
    tasklist_data_t *d = widget->data;
    tasklist_object_data_list_wipe(&d->objects_data);
    p_delete(&d);
}

/** Tasklist detach function.
 * \param widget The widget which is detaching.
 * \param object The object we are leaving.
 */
static void
tasklist_detach(widget_t *widget, void *object)
{
    tasklist_data_t *d = widget->data;
    tasklist_object_data_t *od;
    
    if((od = tasklist_object_data_getbyobj(d->objects_data, object)))
    {
        tasklist_object_data_list_detach(&d->objects_data, od);
        tasklist_object_data_delete(&od);
    }
}

/** Create a new widget tasklist.
 * \param align The widget alignment, which is flex anyway.
 * \return A brand new tasklist widget.
 */
widget_t *
tasklist_new(alignment_t align __attribute__ ((unused)))
{
    widget_t *w;
    tasklist_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->draw = tasklist_draw;
    w->button_press = tasklist_button_press;
    w->align = AlignFlex;
    w->index = luaA_tasklist_index;
    w->newindex = luaA_tasklist_newindex;
    w->data = d = p_new(tasklist_data_t, 1);
    w->destructor = tasklist_destructor;
    w->detach = tasklist_detach;

    d->show_icons = true;
    d->label = LUA_REFNIL;

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
