/*
 * taglist.c - tag list widget
 *
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
#include "tag.h"
#include "lua.h"
#include "event.h"
#include "common/markup.h"
#include "common/configopts.h"
#include "common/tokenize.h"

extern awesome_t globalconf;

typedef struct taglist_drawn_area_t taglist_drawn_area_t;
struct taglist_drawn_area_t
{
    void *object;
    area_array_t areas;
    taglist_drawn_area_t *next, *prev;
};

static void
taglist_drawn_area_delete(taglist_drawn_area_t **a)
{
    area_array_wipe(&(*a)->areas);
    p_delete(a);
}

DO_SLIST(taglist_drawn_area_t, taglist_drawn_area, taglist_drawn_area_delete);

typedef struct
{
    char *text_normal, *text_focus, *text_urgent;
    bool show_empty;
    taglist_drawn_area_t *drawn_area;

} taglist_data_t;

static void
tag_markup_on_elem(markup_parser_data_t *p, const char *elem,
                      const char **names, const char **values)
{
    assert (!strcmp(elem, "title"));
    buffer_add_xmlescaped(&p->text, p->priv);
}


static char *
tag_markup_parse(tag_t *t, const char *str, ssize_t len)
{
    static char const * const elements[] = { "title", NULL };
    markup_parser_data_t p =
    {
        .elements   = elements,
        .on_element = &tag_markup_on_elem,
        .priv       = t->name,
    };
    char *ret;

    markup_parser_data_init(&p);

    if(markup_parse(&p, str, len))
        ret = buffer_detach(&p.text);
    else
        ret = a_strdup(str);

    markup_parser_data_wipe(&p);

    return ret;
}

/** Check if at least one client is tagged with tag number t and is on screen
 * screen
 * \param t tag
 * \return true or false
 */
static bool
tag_isoccupied(tag_t *t)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t) && !c->skip)
            return true;

    return false;
}

static bool
tag_isurgent(tag_t *t)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t) && c->isurgent)
            return true;

    return false;
}

static char *
taglist_text_get(tag_t *tag, taglist_data_t *data)
{
    if(tag->selected)
        return data->text_focus;
    else if(tag_isurgent(tag))
        return data->text_urgent;

    return data->text_normal;
}

static int
taglist_draw(draw_context_t *ctx, int screen, widget_node_t *w,
             int offset,
             int used __attribute__ ((unused)),
             void *object)
{
    tag_t *tag;
    taglist_data_t *data = w->widget->data;
    client_t *sel = globalconf.focus->client;
    screen_t *vscreen = &globalconf.screens[screen];
    int i = 0, prev_width = 0;
    area_t area, rectangle = { 0, 0, 0, 0 };
    char **text = NULL;
    taglist_drawn_area_t *tda;
    draw_parser_data_t *pdata = NULL;

    w->area.width = w->area.y = 0;

    /* Lookup for our taglist_drawn_area.
     * This will be used to store area where we draw tag list for each object.  */
    for(tda = data->drawn_area; tda && tda->object != object; tda = tda->next);

    /* Oh, we did not find a drawn area for our object. First time? */
    if(!tda)
    {
        /** \todo delete this when the widget is removed from the object */
        tda = p_new(taglist_drawn_area_t, 1);
        tda->object = object;
        taglist_drawn_area_list_push(&data->drawn_area, tda);
    }

    tda->areas.len = 0;

    /* First compute text and widget width */
    for(tag = vscreen->tags; tag; tag = tag->next, i++)
    {
        p_realloc(&text, i + 1);
        p_realloc(&pdata, i + 1);
        text[i] = taglist_text_get(tag, data);
        text[i] = tag_markup_parse(tag, text[i], a_strlen(text[i]));
        area = draw_text_extents(ctx->connection, ctx->phys_screen,
                                  globalconf.font, text[i], &pdata[i]);

        if (data->show_empty || tag->selected || tag_isoccupied(tag))
            w->area.width += area.width;

        area_array_append(&tda->areas, area);
    }

    /* Now that we have widget width we can compute widget x coordinate */
    w->area.x = widget_calculate_offset(ctx->width, w->area.width,
                                        offset, w->widget->align); 

    for(i = 0, tag = vscreen->tags; tag && i < tda->areas.len; i++, tag = tag->next)
    {
        area_t *r = &tda->areas.tab[i];

        if(!data->show_empty && !tag->selected && !tag_isoccupied(tag))
        {
            p_delete(&text[i]);
            continue;
        }

        r->x = w->area.x + prev_width;
        prev_width += r->width;
        draw_text(ctx, globalconf.font, *r, text[i], &pdata[i]);
        p_delete(&text[i]);

        if(tag_isoccupied(tag))
        {
            rectangle.width = rectangle.height = (globalconf.font->height + 2) / 3;
            rectangle.x = r->x;
            rectangle.y = r->y;
            draw_rectangle(ctx, rectangle, 1.0,
                           sel && is_client_tagged(sel, tag), ctx->fg);
        }
    }

    p_delete(&text);
    p_delete(&pdata);

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
taglist_button_press(widget_node_t *w,
                     xcb_button_press_event_t *ev,
                     int screen,
                     void *object,
                     awesome_type_t type)
{
    screen_t *vscreen = &globalconf.screens[screen];
    button_t *b;
    taglist_data_t *data = w->widget->data;
    taglist_drawn_area_t *tda;
    tag_t *tag;
    int i;

    /* Find the good drawn area list */
    for(tda = data->drawn_area; tda && tda->object != object; tda = tda->next);

    for(b = w->widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
            for(i = 0, tag = vscreen->tags; tag && i < tda->areas.len; tag = tag->next, i++)
            {
                area_t *area = &tda->areas.tab[i];
                if(ev->event_x >= AREA_LEFT(*area)
                   && ev->event_x < AREA_RIGHT(*area)
                   && (data->show_empty || tag->selected || tag_isoccupied(tag)) )
                {
                    luaA_pushpointer(globalconf.L, object, type);
                    luaA_tag_userdata_new(globalconf.L, tag);
                    luaA_dofunction(globalconf.L, b->fct, 2);
                    return;
                }
            }
}

static widget_tell_status_t
taglist_tell(widget_t *widget, const char *property, const char *new_value)
{
    taglist_data_t *d = widget->data;

    switch(a_tokenize(property, -1))
    {
      case A_TK_TEXT_NORMAL:
        p_delete(&d->text_normal);
        d->text_normal = a_strdup(new_value);
        break;
      case A_TK_TEXT_FOCUS:
        p_delete(&d->text_focus);
        d->text_focus = a_strdup(new_value);
        break;
      case A_TK_TEXT_URGENT:
        p_delete(&d->text_urgent);
        d->text_urgent = a_strdup(new_value);
        break;
      case A_TK_SHOW_EMPTY:
        d->show_empty = a_strtobool(new_value);
        break;
      default:
        return WIDGET_ERROR;
    }
    return WIDGET_NOERROR;
}

static void
taglist_destructor(widget_t *widget)
{
    taglist_data_t *d = widget->data;

    p_delete(&d->text_normal);
    p_delete(&d->text_focus);
    p_delete(&d->text_urgent);
    taglist_drawn_area_list_wipe(&d->drawn_area);
    p_delete(&d);
}

widget_t *
taglist_new(alignment_t align)
{
    widget_t *w;
    taglist_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = taglist_draw;
    w->button_press = taglist_button_press;
    w->tell = taglist_tell;
    w->destructor = taglist_destructor;

    w->data = d = p_new(taglist_data_t, 1);
    d->text_normal = a_strdup(" <text align=\"center\"/><title/> ");
    d->text_focus = a_strdup(" <text align=\"center\"/><title/> ");
    d->text_urgent = a_strdup(" <text align=\"center\"/><title/> ");
    d->show_empty = true;

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_TAGS | WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
