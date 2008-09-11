/*
 * imagebox.c - imagebox widget
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
#include "titlebar.h"

extern awesome_t globalconf;

/** The imagebox private data structure */
typedef struct
{
    /** Imagebox image */
    image_t *image;
} imagebox_data_t;

/** Draw an image.
 * \param ctx The draw context.
 * \param screen The screen.
 * \param w The widget node we are linked from.
 * \param offset Offset to draw at.
 * \param used The size used on the element.
 * \param p A pointer to the object we're draw onto.
 * \param type The type of the object.
 * \return The width used.
 */
static int
imagebox_draw(draw_context_t *ctx, int screen __attribute__ ((unused)),
              widget_node_t *w,
              int offset, int used,
              void *p, awesome_type_t type)
{
    imagebox_data_t *d = w->widget->data;

    w->area.y = 0;

    if(d->image)
    {
        w->area.height = ctx->height;
        w->area.width = ((double) ctx->height / (double) d->image->image->height) * d->image->image->width;
        w->area.x = widget_calculate_offset(ctx->width,
                                            w->area.width,
                                            offset,
                                            w->widget->align);

        draw_image(ctx, w->area.x, w->area.y, ctx->height, d->image->image);
    }
    else
    {
        w->area.width = 0;
        w->area.height = 0;
    }

    return w->area.width;
}

/** Delete a imagebox widget.
 * \param w The widget to destroy.
 */
static void
imagebox_destructor(widget_t *w)
{
    imagebox_data_t *d = w->data;
    image_unref(&d->image);
    p_delete(&d);
}

/** Imagebox widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield image The image to display.
 * \lfield width The width of the imagebox. Set to 0 for auto.
 */
static int
luaA_imagebox_index(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    imagebox_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_IMAGE:
        if(d->image)
            return luaA_image_userdata_new(L, d->image);
      default:
        break;
    }
    return 0;
}

/** The __newindex method for a imagebox object.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 */
static int
luaA_imagebox_newindex(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    image_t **image = NULL;
    imagebox_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_IMAGE:
        if(lua_isnil(L, 3)
           || (image = luaA_checkudata(L, 3, "image")))
        {
            /* unref image */
            image_unref(&d->image);
            if(image)
                d->image = image_ref(image);
            else
                d->image = NULL;
        }
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(*widget);

    return 0;
}


/** Create a new imagebox widget.
 * \param align Widget alignment.
 * \return A brand new widget.
 */
widget_t *
imagebox_new(alignment_t align)
{
    widget_t *w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = imagebox_draw;
    w->index = luaA_imagebox_index;
    w->newindex = luaA_imagebox_newindex;
    w->destructor = imagebox_destructor;
    w->data = p_new(imagebox_data_t, 1);

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
