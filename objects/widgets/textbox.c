/*
 * textbox.c - text box widget
 *
 * Copyright © 2007-2009 Julien Danjou <julien@danjou.info>
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

#include "objects/widget.h"
#include "luaa.h"
#include "common/tokenize.h"

/** Padding type */
typedef struct
{
    /** Padding at top */
    int top;
    /** Padding at bottom */
    int bottom;
    /** Padding at left */
    int left;
    /** Padding at right */
    int right;
} padding_t;

/** The textbox private data structure */
typedef struct
{
    /** The actual text the textbox is set to */
    char *text;
    /** The length of text */
    size_t text_len;
    draw_text_context_t data;
    /** Textbox width and height */
    int width, height;
    /** Extents */
    area_t extents;
    PangoEllipsizeMode ellip;
    PangoWrapMode wrap;
    /** Border */
    struct
    {
        int width;
        color_t color;
    } border;
    /** Text alignment */
    alignment_t align, valign;
    /** Margin */
    padding_t margin;
    /** Background color */
    color_t bg;
    /** Background image */
    image_t *bg_image;
    /** Background resize to wibox height. */
    bool bg_resize;
    /** Background alignment */
    alignment_t bg_align;
} textbox_data_t;

/** Get an optional padding table from a Lua table.
 * \param L The Lua VM state.
 * \param idx The table index on the stack.
 * \param dpadding The default padding value to use.
 */
static inline padding_t
luaA_getopt_padding(lua_State *L, int idx, padding_t *dpadding)
{
    padding_t padding;

    luaA_checktable(L, idx);

    padding.right = luaA_getopt_number(L, idx, "right", dpadding->right);
    padding.left = luaA_getopt_number(L, idx, "left", dpadding->left);
    padding.top = luaA_getopt_number(L, idx, "top", dpadding->top);
    padding.bottom = luaA_getopt_number(L, idx, "bottom", dpadding->bottom);

    return padding;
}


/** Push a padding structure into a table on the Lua stack.
 * \param L The Lua VM state.
 * \param padding The padding struct pointer.
 * \return The number of elements pushed on stack.
 */
static inline int
luaA_pushpadding(lua_State *L, padding_t *padding)
{
    lua_createtable(L, 0, 4);
    lua_pushnumber(L, padding->right);
    lua_setfield(L, -2, "right");
    lua_pushnumber(L, padding->left);
    lua_setfield(L, -2, "left");
    lua_pushnumber(L, padding->top);
    lua_setfield(L, -2, "top");
    lua_pushnumber(L, padding->bottom);
    lua_setfield(L, -2, "bottom");
    return 1;
}

static area_t
textbox_extents(lua_State *L, widget_t *widget)
{
    textbox_data_t *d = widget->data;
    area_t geometry = d->extents;
    geometry.width += d->margin.left + d->margin.left;
    geometry.height += d->margin.bottom + d->margin.top;

    if(d->width)
        geometry.width = d->width;

    if(d->height)
        geometry.width = d->height;

    if(d->bg_image)
    {
        int bgi_height = image_getheight(d->bg_image);
        int bgi_width = image_getwidth(d->bg_image);
        double ratio = d->bg_resize ? (double) geometry.height / bgi_height : 1;
        geometry.width = MAX(d->extents.width + d->margin.left + d->margin.right, MAX(d->width, bgi_width * ratio));
    }

    geometry.x = geometry.y = 0;

    return geometry;
}

/** Draw a textbox widget.
 * \param widget The widget.
 * \param ctx The draw context.
 * \param p A pointer to the object we're draw onto.
 */
static void
textbox_draw(widget_t *widget, draw_context_t *ctx, area_t geometry, wibox_t *p)
{
    textbox_data_t *d = widget->data;

    if(d->bg.initialized)
        draw_rectangle(ctx, geometry, 1.0, true, &d->bg);

    if(d->border.width > 0)
        draw_rectangle(ctx, geometry, d->border.width, false, &d->border.color);

    if(d->bg_image)
    {
        int bgi_height = image_getheight(d->bg_image);
        int bgi_width = image_getwidth(d->bg_image);
        double ratio = d->bg_resize ? (double) geometry.height / bgi_height : 1;
        /* check there is enough space to draw the image */
        if(ratio * bgi_width <= geometry.width)
        {
            int x = geometry.x;
            int y = geometry.y;
            switch(d->bg_align)
            {
              case AlignCenter:
                x += (geometry.width - bgi_width * ratio) / 2;
                break;
              case AlignRight:
                x += geometry.width - bgi_width * ratio;
                break;
              case AlignBottom:
                y += geometry.height - bgi_height * ratio;
                break;
              case AlignMiddle:
                y += (geometry.height - bgi_height * ratio) / 2;
                break;
              default:
                break;
            }
            draw_image(ctx, x, y, ratio, d->bg_image);
        }
    }

    geometry.x += d->margin.left;
    geometry.y += d->margin.top;

    int margin_width = d->margin.left + d->margin.right;
    if(margin_width <= geometry.width)
        geometry.width -= d->margin.left + d->margin.right;
    else
        geometry.width = 0;

    int margin_height = d->margin.top + d->margin.bottom;
    if(margin_height <= geometry.height)
        geometry.height -= d->margin.top + d->margin.bottom;
    else
        geometry.height = 0;

    draw_text(ctx, &d->data, d->ellip, d->wrap, d->align, d->valign, geometry);
}

/** Delete a textbox widget.
 * \param w The widget to destroy.
 */
static void
textbox_destructor(widget_t *w)
{
    textbox_data_t *d = w->data;
    draw_text_context_wipe(&d->data);
    p_delete(&d->text);
    p_delete(&d);
}

static int
luaA_textbox_margin(lua_State *L)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    textbox_data_t *d = widget->data;

    if(lua_gettop(L) == 2)
    {
        d->margin = luaA_getopt_padding(L, 2, &d->margin);
        widget_invalidate_bywidget(widget);
    }

    return luaA_pushpadding(L, &d->margin);
}

/** Textbox widget.
 * \param L The Lua VM state.
 * \param token The key token.
 * \return The number of elements pushed on stack.
 * \luastack
 * \lfield text The text to display.
 * \lfield width The width of the textbox. Set to 0 for auto.
 * \lfield wrap The wrap mode: word, char, word_char.
 * \lfield ellipsize The ellipsize mode: start, middle or end.
 * \lfield border_width The border width to draw around.
 * \lfield border_color The border color.
 * \lfield align Text alignment, left, center or right.
 * \lfield margin Method to pass text margin: a table with top, left, right and bottom keys.
 * \lfield bg Background color.
 * \lfield bg_image Background image.
 * \lfield bg_align Background image alignment, left, center, right, bottom, top or middle
 * \lfield bg_resize Background resize.
 */
static int
luaA_textbox_index(lua_State *L, awesome_token_t token)
{
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    textbox_data_t *d = widget->data;

    switch(token)
    {
      case A_TK_BG_RESIZE:
        lua_pushboolean(L, d->bg_resize);
        return 1;
      case A_TK_BG_ALIGN:
        lua_pushstring(L, draw_align_tostr(d->bg_align));
        return 1;
      case A_TK_BG_IMAGE:
        return luaA_object_push(L, d->bg_image);
      case A_TK_BG:
        return luaA_pushcolor(L, &d->bg);
      case A_TK_MARGIN:
        lua_pushcfunction(L, luaA_textbox_margin);
        return 1;
      case A_TK_ALIGN:
        lua_pushstring(L, draw_align_tostr(d->align));
        return 1;
      case A_TK_VALIGN:
        lua_pushstring(L, draw_align_tostr(d->valign));
        return 1;
      case A_TK_BORDER_WIDTH:
        lua_pushnumber(L, d->border.width);
        return 1;
      case A_TK_BORDER_COLOR:
        luaA_pushcolor(L, &d->border.color);
        return 1;
      case A_TK_TEXT:
        if(d->text_len > 0)
        {
            lua_pushlstring(L, d->text, d->text_len);
            return 1;
        }
        return 0;
      case A_TK_WIDTH:
        lua_pushnumber(L, d->width);
        return 1;
      case A_TK_HEIGHT:
        lua_pushnumber(L, d->height);
        return 1;
      case A_TK_WRAP:
        switch(d->wrap)
        {
          default:
            lua_pushliteral(L, "word");
            break;
          case PANGO_WRAP_CHAR:
            lua_pushliteral(L, "char");
            break;
          case PANGO_WRAP_WORD_CHAR:
            lua_pushliteral(L, "word_char");
            break;
        }
        return 1;
      case A_TK_ELLIPSIZE:
        switch(d->ellip)
        {
          case PANGO_ELLIPSIZE_START:
            lua_pushliteral(L, "start");
            break;
          case PANGO_ELLIPSIZE_MIDDLE:
            lua_pushliteral(L, "middle");
            break;
          default:
            lua_pushliteral(L, "end");
            break;
        }
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
    widget_t *widget = luaA_checkudata(L, 1, &widget_class);
    const char *buf = NULL;
    textbox_data_t *d = widget->data;

    switch(token)
    {
      case A_TK_BG_ALIGN:
        buf = luaL_checklstring(L, 3, &len);
        d->bg_align = draw_align_fromstr(buf, len);
        break;
      case A_TK_BG_RESIZE:
        d->bg_resize = luaA_checkboolean(L, 3);
        break;
      case A_TK_BG_IMAGE:
        luaA_checkudataornil(L, -1, &image_class);
        luaA_object_unref_item(L, 1, d->bg_image);
        d->bg_image = luaA_object_ref_item(L, 1, 3);
        break;
      case A_TK_BG:
        if(lua_isnil(L, 3))
            p_clear(&d->bg, 1);
        else if((buf = luaL_checklstring(L, 3, &len)))
            color_init_reply(color_init_unchecked(&d->bg, buf, len));
        break;
      case A_TK_ALIGN:
        if((buf = luaL_checklstring(L, 3, &len)))
            d->align = draw_align_fromstr(buf, len);
        break;
      case A_TK_VALIGN:
        if((buf = luaL_checklstring(L, 3, &len)))
            d->valign = draw_align_fromstr(buf, len);
        break;
      case A_TK_BORDER_COLOR:
        if((buf = luaL_checklstring(L, 3, &len)))
            color_init_reply(color_init_unchecked(&d->border.color, buf, len));
        break;
      case A_TK_BORDER_WIDTH:
        d->border.width = luaL_checknumber(L, 3);
        break;
      case A_TK_TEXT:
        if(lua_isnil(L, 3)
           || (buf = luaL_checklstring(L, 3, &len)))
        {
            /* delete */
            draw_text_context_wipe(&d->data);
            p_delete(&d->text);
            d->text_len = 0;
            p_clear(&d->data, 1);

            if(buf)
            {
                char *text;
                ssize_t tlen;
                bool success;

                /* if text has been converted to UTF-8 */
                if(draw_iso2utf8(buf, len, &text, &tlen))
                {
                    success = draw_text_context_init(&d->data, text, tlen);
                    if(success)
                    {
                        d->text = p_dup(text, tlen);
                        d->text_len = tlen;
                    }
                    p_delete(&text);
                }
                else
                {
                    success = draw_text_context_init(&d->data, buf, len);
                    if(success)
                    {
                        d->text = p_dup(buf, len);
                        d->text_len = len;
                    }
                }

                d->extents = draw_text_extents(&d->data);

                if(!success)
                    luaL_error(L, "Invalid markup in '%s'", buf);
            }
            else
                p_clear(&d->extents, 1);
        }
        break;
      case A_TK_WIDTH:
        d->width = luaL_checknumber(L, 3);
        break;
      case A_TK_HEIGHT:
        d->height = luaL_checknumber(L, 3);
        break;
      case A_TK_WRAP:
        if((buf = luaL_checkstring(L, 3)))
        {
            if(a_strcmp(buf, "word") == 0)
                d->wrap = PANGO_WRAP_WORD;
            else if(a_strcmp(buf, "char") == 0)
                d->wrap = PANGO_WRAP_CHAR;
            else if(a_strcmp(buf, "word_char") == 0)
                d->wrap = PANGO_WRAP_WORD_CHAR;
        }
        break;
      case A_TK_ELLIPSIZE:
        if((buf = luaL_checkstring(L, 3)))
        {
            if(a_strcmp(buf, "start") == 0)
                d->ellip = PANGO_ELLIPSIZE_START;
            else if(a_strcmp(buf, "middle") == 0)
                d->ellip = PANGO_ELLIPSIZE_MIDDLE;
            else if(a_strcmp(buf, "end") == 0)
                d->ellip = PANGO_ELLIPSIZE_END;
        }
        break;
      default:
        return 0;
    }

    widget_invalidate_bywidget(widget);

    return 0;
}

/** Create a new textbox widget.
 * \param w The widget to initialize.
 * \return A brand new widget.
 */
widget_t *
widget_textbox(widget_t *w)
{
    w->draw = textbox_draw;
    w->index = luaA_textbox_index;
    w->newindex = luaA_textbox_newindex;
    w->destructor = textbox_destructor;
    w->extents = textbox_extents;

    textbox_data_t *d = w->data = p_new(textbox_data_t, 1);
    d->ellip = PANGO_ELLIPSIZE_END;
    d->valign = AlignCenter;

    return w;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
