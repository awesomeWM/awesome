/*
 * taglist.c - tag list widget
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

#include "client.h"
#include "widget.h"
#include "tag.h"
#include "luaa.h"
#include "common/tokenize.h"

extern awesome_t globalconf;

/** Data type used to store where we draw the taglist for a particular object.
 * This is filled in the draw function, and use later when we get a click.
 */
typedef struct taglist_drawn_area_t taglist_drawn_area_t;
struct taglist_drawn_area_t
{
    void *object;
    area_array_t areas;
    taglist_drawn_area_t *next, *prev;
};

/** Delete a taglist_drawn_area_t object.
 * \param a The drawn area to delete.
 */
static void
taglist_drawn_area_delete(taglist_drawn_area_t **a)
{
    area_array_wipe(&(*a)->areas);
    p_delete(a);
}

DO_SLIST(taglist_drawn_area_t, taglist_drawn_area, taglist_drawn_area_delete);

/** Taglist widget private data */
typedef struct
{
    taglist_drawn_area_t *drawn_area;
    luaA_ref label;
} taglist_data_t;

static taglist_drawn_area_t *
taglist_drawn_area_getbyobj(taglist_drawn_area_t *list, void *p)
{
    taglist_drawn_area_t *t;
    
    for(t = list; t; t = t->next)
        if(t->object == p)
            return t;

    return NULL;
}

/** Draw a taglist.
 * \param ctx The draw context.
 * \param screen The screen we're drawing for.
 * \param w The widget node we're drawing for.
 * \param offset Offset to draw at.
 * \param used Space already used.
 * \param object The object pointer we're drawing onto.
 * \param type The object type.
 * \return The width used.
 */
static int
taglist_draw(draw_context_t *ctx, int screen, widget_node_t *w,
             int offset,
             int used __attribute__ ((unused)),
             void *object,
             awesome_type_t type)
{
    taglist_data_t *data = w->widget->data;
    area_t area;
    taglist_drawn_area_t *tda;
    int prev_width = 0;

    tag_array_t *tags = &globalconf.screens[screen].tags;
    const char **text = p_alloca(const char *, tags->len);
    size_t *len = p_alloca(size_t, tags->len);
    draw_parser_data_t *pdata = p_alloca(draw_parser_data_t, tags->len);

    w->area.width = w->area.y = 0;

    /* Lookup for our taglist_drawn_area.
     * This will be used to store area where we draw tag list for each object.  */
    if(!(tda = taglist_drawn_area_getbyobj(data->drawn_area, object)))
    {
        /* Oh, we did not find a drawn area for our object. First time? */
        tda = p_new(taglist_drawn_area_t, 1);
        tda->object = object;
        taglist_drawn_area_list_push(&data->drawn_area, tda);
    }

    area_array_wipe(&tda->areas);
    area_array_init(&tda->areas);

    /* First compute text and widget width */
    for(int i = 0; i < tags->len; i++)
    {
        tag_t *tag = tags->tab[i];
        p_clear(&area, 1);

        luaA_tag_userdata_new(globalconf.L, tag);
        if(luaA_dofunction(globalconf.L, data->label, 1, 1))
        {
            if(lua_isstring(globalconf.L, -1))
                text[i] = lua_tolstring(globalconf.L, -1, &len[i]);

            lua_pop(globalconf.L, 1);

            draw_parser_data_init(&pdata[i]);
            area = draw_text_extents(ctx->phys_screen,
                                     globalconf.font, text[i], len[i], &pdata[i]);

            if(pdata[i].bg_image)
                area.width = MAX(area.width,
                                 pdata[i].bg_resize ? ((double) pdata[i].bg_image->width / (double) pdata[i].bg_image->height) * w->area.height : pdata[i].bg_image->width);

            w->area.width += area.width;
        }
        area_array_append(&tda->areas, area);
    }

    /* Now that we have widget width we can compute widget x coordinate */
    w->area.x = widget_calculate_offset(ctx->width, w->area.width,
                                        offset, w->widget->align);

    for(int i = 0; i < tags->len; i++)
    {
        area_t *r = &tda->areas.tab[i];

        r->x = w->area.x + prev_width;
        prev_width += r->width;
        draw_text(ctx, globalconf.font, *r, text[i], len[i], &pdata[i]);
        draw_parser_data_wipe(&pdata[i]);
    }

    w->area.height = ctx->height;
    return w->area.width;
}

/** Handle button click on tasklist.
 * \param w The widget node.
 * \param ev The button press event.
 * \param btype The button press event type.
 * \param screen The screen where the click was.
 * \param object The object we're onto.
 * \param type The type object.
 */
static void
taglist_button(widget_node_t *w,
               xcb_button_press_event_t *ev,
               int screen,
               void *object,
               awesome_type_t type)
{
    tag_array_t *tags = &globalconf.screens[screen].tags;
    taglist_data_t *data = w->widget->data;
    taglist_drawn_area_t *tda;
    button_array_t *barr = &w->widget->buttons;

    /* Find the good drawn area list */
    if((tda = taglist_drawn_area_getbyobj(data->drawn_area, object)))
        for(int i = 0; i < barr->len; i++)
            if(ev->detail == barr->tab[i]->button
               && XUTIL_MASK_CLEAN(ev->state) == barr->tab[i]->mod)
                for(int j = 0; i < MIN(tags->len, tda->areas.len); j++)
                {
                    tag_t *tag  = tags->tab[j];
                    area_t *area = &tda->areas.tab[j];
                    if(ev->event_x >= AREA_LEFT(*area)
                       && ev->event_x < AREA_RIGHT(*area))
                    {
                        luaA_pushpointer(globalconf.L, object, type);
                        luaA_tag_userdata_new(globalconf.L, tag);
                        luaA_dofunction(globalconf.L,
                                        ev->response_type == XCB_BUTTON_PRESS ?
                                        barr->tab[i]->press : barr->tab[i]->release,
                                        2, 0);
                        return;
                    }
                }
}

/** Taglist widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield label Function used to get the string to display as the tag title.
 * It gets the tag as argument, and must return a string.
 */
static int
luaA_taglist_index(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    taglist_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_LABEL:
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->label);
        return 1;
      default:
        return 0;
    }
}

/** Newindex function for taglist.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 */
static int
luaA_taglist_newindex(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    taglist_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_LABEL:
        luaA_registerfct(L, 3, &d->label);
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(*widget);

    return 0;
}

/** Taglist destructor.
 * \param widget The widget to destroy.
 */
static void
taglist_destructor(widget_t *widget)
{
    taglist_data_t *d = widget->data;

    taglist_drawn_area_list_wipe(&d->drawn_area);
    p_delete(&d);
}

/** Taglist detach function.
 * \param widget The widget which is detaching.
 * \param object The object we are leaving.
 */
static void
taglist_detach(widget_t *widget, void *object)
{
    taglist_data_t *d = widget->data;
    taglist_drawn_area_t *tda;
    
    if((tda = taglist_drawn_area_getbyobj(d->drawn_area, object)))
    {
        taglist_drawn_area_list_detach(&d->drawn_area, tda);
        taglist_drawn_area_delete(&tda);
    }
}

/** Create a brand new taglist widget.
 * \param align Widget alignment.
 * \return A taglist widget.
 */
widget_t *
taglist_new(alignment_t align)
{
    widget_t *w;
    taglist_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->index = luaA_taglist_index;
    w->newindex = luaA_taglist_newindex;
    w->align = align;
    w->draw = taglist_draw;
    w->button = taglist_button;
    w->destructor = taglist_destructor;
    w->detach = taglist_detach;

    w->data = d = p_new(taglist_data_t, 1);
    d->label = LUA_REFNIL;

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_TAGS | WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
