/*
 * textbox.c - text box widget
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
#include "common/tokenize.h"

extern awesome_t globalconf;

/** The textbox private data structure */
typedef struct
{
    /** Textbox text */
    char *text;
    /** Textbox text length */
    size_t len;
    /** Textbox width */
    int width;
    /** Extents */
    int extents;
    /** Draw parser data */
    draw_parser_data_t pdata;
} textbox_data_t;

/** Draw a textbox widget.
 * \param ctx The draw context.
 * \param screen The screen.
 * \param w The widget node we are linked from.
 * \param offset Offset to draw at.
 * \param used The size used on the element.
 * \param p A pointer to the object we're draw onto.
 * \param type The object type.
 * \return The width used.
 */
static int
textbox_draw(draw_context_t *ctx, int screen __attribute__ ((unused)),
             widget_node_t *w,
             int offset, int used,
             void *p __attribute__ ((unused)),
             awesome_type_t type)
{
    textbox_data_t *d = w->widget->data;

    w->area.height = ctx->height;

    if(d->width)
        w->area.width = d->width;
    else if(w->widget->align == AlignFlex)
        w->area.width = ctx->width - used;
    else
    {
        w->area.width = MIN(d->extents, ctx->width - used);

        if(d->pdata.bg_image)
            w->area.width = MAX(w->area.width,
                                d->pdata.bg_resize ? ((double) d->pdata.bg_image->width / (double) d->pdata.bg_image->height) * w->area.height : d->pdata.bg_image->width);
    }

    w->area.x = widget_calculate_offset(ctx->width,
                                        w->area.width,
                                        offset,
                                        w->widget->align);
    w->area.y = 0;

    draw_text(ctx, globalconf.font, w->area, d->text, d->len, &d->pdata);

    return w->area.width;
}

/** Delete a textbox widget.
 * \param w The widget to destroy.
 */
static void
textbox_destructor(widget_t *w)
{
    textbox_data_t *d = w->data;
    draw_parser_data_wipe(&d->pdata);
    p_delete(&d->text);
    p_delete(&d);
}

/** Textbox widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield text The text to display.
 * \lfield width The width of the textbox. Set to 0 for auto.
 */
static int
luaA_textbox_index(lua_State *L, awesome_token_t token)
{
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    textbox_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_TEXT:
        lua_pushstring(L, d->text);
        return 1;
      case A_TK_WIDTH:
        lua_pushnumber(L, d->width);
        return 1;
      default:
        return 0;
    }
}

/** The __newindex method for a textbox object.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 */
static int
luaA_textbox_newindex(lua_State *L, awesome_token_t token)
{
    size_t len = 0;
    widget_t **widget = luaA_checkudata(L, 1, "widget");
    const char *buf = NULL;
    textbox_data_t *d = (*widget)->data;

    switch(token)
    {
      case A_TK_TEXT:
        if(lua_isnil(L, 3)
           || (buf = luaL_checklstring(L, 3, &len)))
        {
            /* delete */
            draw_parser_data_wipe(&d->pdata);
            /* reinit since we are giving it as arg to draw_text unconditionally */
            draw_parser_data_init(&d->pdata);
            p_delete(&d->text);

            /* re-init */
            d->len = len;
            if(buf)
            {
                a_iso2utf8(&d->text, buf, len);
                d->extents = draw_text_extents(globalconf.connection, globalconf.default_screen,
                                               globalconf.font, d->text, d->len, &d->pdata).width;
            }
            else
                d->extents = 0;
        }
        break;
      case A_TK_WIDTH:
        d->width = luaL_checknumber(L, 3);
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(*widget);

    return 0;
}

/** Create a new textbox widget.
 * \param align Widget alignment.
 * \return A brand new widget.
 */
widget_t *
textbox_new(alignment_t align)
{
    widget_t *w;
    textbox_data_t *d;

    w = p_new(widget_t, 1);
    widget_common_new(w);
    w->align = align;
    w->draw = textbox_draw;
    w->index = luaA_textbox_index;
    w->newindex = luaA_textbox_newindex;
    w->destructor = textbox_destructor;
    w->data = d = p_new(textbox_data_t, 1);

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
