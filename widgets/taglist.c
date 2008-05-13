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

#include "config.h"
#include "client.h"
#include "widget.h"
#include "tag.h"
#include "event.h"
#include "common/markup.h"
#include "common/configopts.h"

extern AwesomeConf globalconf;

typedef struct
{
    char *text_normal, *text_focus, *text_urgent;
    area_t *draw_area;
} taglist_data_t;

static char *
tag_markup_parse(tag_t *t, const char *str, ssize_t len)
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

/** Check if at least one client is tagged with tag number t and is on screen
 * screen
 * \param screen screen number
 * \param t tag
 * \return true or false
 */
static bool
tag_isoccupied(tag_t *t)
{
    client_t *c;

    for(c = globalconf.clients; c; c = c->next)
        if(is_client_tagged(c, t) && !c->skip && c != globalconf.scratch.client)
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

static style_t *
taglist_style_get(tag_t *tag, VirtScreen *vs)
{
    if(tag->selected)
        return &vs->styles.focus;
    else if(tag_isurgent(tag))
        return &vs->styles.urgent;

    return &vs->styles.normal;
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
taglist_draw(widget_t *widget,
             draw_context_t *ctx,
             int offset,
             int used __attribute__ ((unused)))
{
    tag_t *tag;
    taglist_data_t *data = widget->data;
    client_t *sel = globalconf.focus->client;
    VirtScreen *vscreen = &globalconf.screens[widget->statusbar->screen];
    int i = 0;
    style_t **styles;
    area_t *area, rectangle = { 0, 0, 0, 0, NULL, NULL };
    char **text;

    widget->area.width = 0;

    area_list_wipe(&data->draw_area);

    text = p_new(char *, 1);
    styles = p_new(style_t *, 1);
    for(tag = vscreen->tags; tag; tag = tag->next, i++)
    {
        p_realloc(&text, i + 1);
        p_realloc(&styles, i + 1);
        styles[i] = taglist_style_get(tag, vscreen);
        area = p_new(area_t, 1);
        text[i] = taglist_text_get(tag, data);
        text[i] = tag_markup_parse(tag, text[i], a_strlen(text[i]));
        *area = draw_text_extents(ctx->connection, ctx->phys_screen,
                                  styles[i]->font, text[i]);
        area->x = widget->area.width;
        area->height = widget->statusbar->height;
        area_list_append(&data->draw_area, area);
        widget->area.width += area->width;
    }

    if(!widget->user_supplied_x)
        widget->area.x = widget_calculate_offset(widget->statusbar->width,
                                                 widget->area.width,
                                                 offset,
                                                 widget->alignment);

    if(!widget->user_supplied_y)
        widget->area.y = 0;

    for(area = data->draw_area, tag = vscreen->tags, i = 0;
        tag && area;
        tag = tag->next, area = area->next, i++)
    {
        draw_text(ctx, *area, text[i], styles[i]);
        p_delete(&text[i]);

        if(tag_isoccupied(tag))
        {
            rectangle.width = rectangle.height = (styles[i]->font->height + 2) / 3;
            rectangle.x = area->x;
            rectangle.y = area->y;
            draw_rectangle(ctx, rectangle, 1.0,
                           sel && is_client_tagged(sel, tag), styles[i]->fg);
        }
    }

    p_delete(&text);
    p_delete(&styles);

    widget->area.height = widget->statusbar->height;
    return widget->area.width;
}

static void
taglist_button_press(widget_t *widget, xcb_button_press_event_t *ev)
{
    VirtScreen *vscreen = &globalconf.screens[widget->statusbar->screen];
    Button *b;
    tag_t *tag;
    char buf[4];
    int i = 1;
    taglist_data_t *data = widget->data;
    area_t *area = data->draw_area;

    for(b = widget->buttons; b; b = b->next)
        if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->func)
            switch(widget->statusbar->position)
            {
              default:
                for(tag = vscreen->tags; tag; tag = tag->next, i++, area = area->next)
                    if(ev->event_x >= AREA_LEFT(*area)
                       && ev->event_x < AREA_RIGHT(*area))
                    {
                        snprintf(buf, sizeof(buf), "%d", i);
                        b->func(widget->statusbar->screen, buf);
                        return;
                    }
                break;
              case Right:
                for(tag = vscreen->tags; tag; tag = tag->next, i++, area = area->next)
                    if(ev->event_y >= AREA_LEFT(*area)
                       && ev->event_y < AREA_RIGHT(*area))
                    {
                        snprintf(buf, sizeof(buf), "%d", i);
                        b->func(widget->statusbar->screen, buf);
                        return;
                    }
                break;
              case Left:
                for(tag = vscreen->tags; tag; tag = tag->next, i++, area = area->next)
                    if(widget->statusbar->width - ev->event_y >= AREA_LEFT(*area)
                       && widget->statusbar->width - ev->event_y < AREA_RIGHT(*area))
                    {
                        snprintf(buf, sizeof(buf), "%d", i);
                        b->func(widget->statusbar->screen, buf);
                        return;
                    }
                break;
            }
}

widget_t *
taglist_new(statusbar_t *statusbar, cfg_t *config)
{
    widget_t *w;
    taglist_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w, statusbar, config);
    w->draw = taglist_draw;
    w->button_press = taglist_button_press;
    w->alignment = cfg_getalignment(config, "align");

    w->data = d = p_new(taglist_data_t, 1);
    d->text_normal = a_strdup(cfg_getstr(config, "text_normal"));
    d->text_focus = a_strdup(cfg_getstr(config, "text_focus"));
    d->text_urgent = a_strdup(cfg_getstr(config, "text_urgent"));

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_TAGS | WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
