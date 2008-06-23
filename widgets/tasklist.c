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
#include "client.h"
#include "focus.h"
#include "screen.h"
#include "event.h"
#include "ewmh.h"
#include "tag.h"
#include "common/configopts.h"
#include "common/markup.h"

extern awesome_t globalconf;

typedef enum
{
    ShowFocus,
    ShowTags,
    ShowAll,
} showclient_t;

typedef struct
{
    showclient_t show;
    bool show_icons;
    char *text_normal, *text_urgent, *text_focus;
} tasklist_data_t;

static inline bool
tasklist_isvisible(client_t *c, int screen, showclient_t show)
{
    if(c->skip || c->skiptb)
        return false;

    switch(show)
    {
      case ShowAll:
        return (c->screen == screen);
      case ShowTags:
        return client_isvisible(c, screen);
      case ShowFocus:
        return (c == focus_get_current_client(screen));
    }
    return false;
}

struct tasklist_hook_data
{
    draw_context_t *ctx;
    area_t *area;
};

static void
tasklist_markup_on_elem(markup_parser_data_t *p, const char *elem,
                        const char **names, const char **values)
{
    struct tasklist_hook_data *data = p->priv;
    draw_context_t *ctx = data->ctx;

    assert(!strcmp(elem, "bg"));
    for(; *names; names++, values++)
    {
        if(!a_strcmp(*names, "color"))
        {
            xcolor_t bg_color;
            xcolor_new(ctx->connection, ctx->phys_screen, *values, &bg_color);
            draw_rectangle(ctx, *data->area, 1.0, true, bg_color);
            break;
        }
    }
}


static int
tasklist_draw(draw_context_t *ctx, int screen,
              widget_node_t *w,
              int offset, int used, void *q __attribute__ ((unused)))
{
    client_t *c;
    tasklist_data_t *d = w->widget->data;
    area_t area;
    char *text;
    int n = 0, i = 0, box_width = 0, icon_width = 0, box_width_rest = 0;
    netwm_icon_t *icon;
    draw_image_t *image;

    if(used >= ctx->width)
        return (w->area.width = 0);

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, screen, d->show))
            n++;

    if(!n)
        return (w->area.width = 0);

    box_width = (ctx->width - used) / n;
    /* compute how many pixel we left empty */
    box_width_rest = (ctx->width - used) % n;

    w->area.x = widget_calculate_offset(ctx->width,
                                        0, offset, w->widget->align);

    w->area.y = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, screen, d->show))
        {
            icon_width = 0;

            if(globalconf.focus->client == c)
                text = d->text_focus;
            else if(c->isurgent)
                text = d->text_urgent;
            else
                text = d->text_normal;

            text = client_markup_parse(c, text, a_strlen(text));

            if(d->show_icons)
            {
                static char const * const elements[] = { "bg", NULL };
                struct tasklist_hook_data data = { .ctx = ctx, .area = &area };
                markup_parser_data_t p =
                {
                    .elements   = elements,
                    .on_element = &tasklist_markup_on_elem,
                    .priv       = &data,
                };

                /* draw a background for icons */
                area.x = w->area.x + box_width * i;
                area.y = w->area.y;
                area.height = ctx->height;
                area.width = box_width;

                /* Actually look for the proper background color, since
                 * otherwise the background statusbar color is used instead */
                markup_parser_data_init(&p);
                markup_parse(&p, text, a_strlen(text));
                markup_parser_data_wipe(&p);

                if((image = draw_image_new(c->icon_path)))
                {
                    icon_width = ((double) ctx->height / (double) image->height) * image->width;
                    draw_image(ctx, w->area.x + box_width * i,
                               w->area.y, ctx->height, image);
                    draw_image_delete(&image);
                }

                if(!icon_width && (icon = ewmh_get_window_icon(c->win)))
                {
                    icon_width = ((double) ctx->height / (double) icon->height)
                        * icon->width;
                    draw_image_from_argb_data(ctx,
                                              w->area.x + box_width * i,
                                              w->area.y,
                                              icon->width, icon->height,
                                              ctx->height, icon->image);
                    p_delete(&icon->image);
                    p_delete(&icon);
                }
            }

            area.x = w->area.x + icon_width + box_width * i;
            area.y = w->area.y;
            area.width = box_width - icon_width;
            area.height = ctx->height;

            /* if we're on last elem, it has the last pixels left */
            if(i == n - 1)
                area.width += box_width_rest;

            draw_text(ctx, globalconf.font, area, text, NULL);

            p_delete(&text);

            if(c->isfloating || c->ismax)
                draw_circle(ctx, w->area.x + icon_width + box_width * i,
                            w->area.y,
                            (globalconf.font->height + 2) / 4,
                            c->ismax, ctx->fg);
            i++;
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
    client_t *c;
    tasklist_data_t *d = w->widget->data;
    int n = 0, box_width = 0, i, ci = 0;

    for(c = globalconf.clients; c; c = c->next)
        if(tasklist_isvisible(c, screen, d->show))
            n++;

    if(!n)
        return;

    box_width = w->area.width / n;

    ci = (ev->event_x - w->area.x) / box_width;

    /* found first visible client */
    for(c = globalconf.clients;
        c && !tasklist_isvisible(c, screen, d->show);
        c = c->next);
    /* found ci-th visible client */
    for(i = 0; c ; c = c->next)
        if(tasklist_isvisible(c, screen, d->show))
            if(i++ >= ci)
                break;

    if(c)
        for(b = w->widget->buttons; b; b = b->next)
            if(ev->detail == b->button && CLEANMASK(ev->state) == b->mod && b->fct)
            {
                luaA_pushpointer(globalconf.L, object, type);
                luaA_client_userdata_new(globalconf.L, c);
                luaA_dofunction(globalconf.L, b->fct, 2);
            }
}

static widget_tell_status_t
tasklist_tell(widget_t *widget, const char *property, const char *new_value)
{
    tasklist_data_t *d = widget->data;

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
    else if(!a_strcmp(property, "show_icons"))
        d->show_icons = a_strtobool(new_value);
    else if(!a_strcmp(property, "show"))
    {
        if(!a_strcmp(new_value, "tags"))
            d->show = ShowTags;
        else if(!a_strcmp(new_value, "focus"))
            d->show = ShowFocus;
        else if(!a_strcmp(new_value, "all"))
            d->show = ShowAll;
        else
            return WIDGET_ERROR;
    }
    else
        return WIDGET_ERROR;

    return WIDGET_NOERROR;
}

static void
tasklist_destructor(widget_t *widget)
{
    tasklist_data_t *d = widget->data;

    p_delete(&d->text_normal);
    p_delete(&d->text_focus);
    p_delete(&d->text_urgent);
    p_delete(&d);
}

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
    w->data = d = p_new(tasklist_data_t, 1);
    w->tell = tasklist_tell;
    w->destructor = tasklist_destructor;

    d->text_normal = a_strdup(" <title/> ");
    d->text_focus = a_strdup(" <title/> ");
    d->text_urgent = a_strdup(" <title/> ");
    d->show_icons = true;
    d->show = ShowTags;

    /* Set cache property */
    w->cache_flags = WIDGET_CACHE_CLIENTS;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
