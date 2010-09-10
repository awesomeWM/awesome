/*
 * color.c - color functions
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
 * Copyright © 2009 Uli Schlachter <psychon@znc.in>
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

#include <ctype.h>

#include "color.h"
#include "globalconf.h"
#include "common/xutil.h"

#define RGB_8TO16(i)   (0xffff * ((i) & 0xff) / 0xff)
#define RGB_16TO8(i)   (0xff * ((i) & 0xffff) / 0xffff)

/** Parse an hexadecimal color string to its component.
 * \param colstr The color string.
 * \param len The color string length.
 * \param red A pointer to the red color to fill.
 * \param green A pointer to the green color to fill.
 * \param blue A pointer to the blue color to fill.
 * \param alpha A pointer to the alpha color to fill.
 * \return True if everything alright.
 */
static bool
color_parse(const char *colstr, ssize_t len,
            uint8_t *red, uint8_t *green, uint8_t *blue, uint8_t *alpha)
{
    unsigned long colnum;
    char *p;

    if(len == 7)
    {
        colnum = strtoul(colstr + 1, &p, 16);
        if(p - colstr != 7)
            goto invalid;
        *alpha = 0xff;
    }
    /* we have alpha */
    else if(len == 9)
    {
        colnum = strtoul(colstr + 1, &p, 16);
        if(p - colstr != 9)
            goto invalid;
        *alpha = colnum & 0xff;
        colnum >>= 8;
    }
    else
    {
      invalid:
        warn("awesome: error, invalid color '%s'", colstr);
        return false;
    }

    *red   = (colnum >> 16) & 0xff;
    *green = (colnum >> 8) & 0xff;
    *blue  = colnum & 0xff;

    return true;
}

/** Send a request to initialize a color.
 * If you are only interested in the color's pixel value or need both, the pixel
 * value and the rgba components, use xcolor_init_unchecked() and/or
 * xcolor_to_color() instead.
 * \param color color_t struct to store color into.
 * \param colstr Color specification.
 * \param len The length of colstr (which still MUST be NULL terminated).
 * \return True if color allocation was successful.
 */
bool
color_init_unchecked(color_t *color, const char *colstr, ssize_t len)
{
    if(!len)
        return false;

    /* The color is given in RGB value */
    if(!color_parse(colstr, len, &color->red, &color->green, &color->blue, &color->alpha))
        return false;

    color->initialized = true;
    return true;
}

/** Send a request to initialize a X color.
 * If you are only interested in the rgba values and don't need the color's
 * pixel value, you should use color_init_unchecked() instead.
 * \param color xcolor_t struct to store color into.
 * \param colstr Color specification.
 * \param len The length of colstr (which still MUST be NULL terminated).
 * \return request informations.
 */
xcolor_init_request_t
xcolor_init_unchecked(xcolor_t *color, const char *colstr, ssize_t len)
{
    xcolor_init_request_t req;
    uint8_t red, green, blue, alpha;

    p_clear(&req, 1);

    if(!len)
    {
        req.has_error = true;
        return req;
    }

    req.alpha = 0xffff;
    req.color = color;

    /* The color is given in RGB value */
    if(!color_parse(colstr, len, &red, &green, &blue, &alpha))
    {
        warn("awesome: error, invalid color '%s'", colstr);
        req.has_error = true;
        return req;
    }

    req.alpha = RGB_8TO16(alpha);

    req.cookie_hexa = xcb_alloc_color_unchecked(globalconf.connection,
                                                globalconf.screen->default_colormap,
                                                RGB_8TO16(red),
                                                RGB_8TO16(green),
                                                RGB_8TO16(blue));

    req.has_error = false;
    req.colstr = colstr;

    return req;
}

/** Initialize a X color.
 * \param req xcolor_init request.
 * \return True if color allocation was successful.
 */
bool
xcolor_init_reply(xcolor_init_request_t req)
{
    if(req.has_error)
        return false;

    xcb_alloc_color_reply_t *hexa_color;

    if((hexa_color = xcb_alloc_color_reply(globalconf.connection,
                                           req.cookie_hexa, NULL)))
    {
        req.color->pixel = hexa_color->pixel;
        req.color->red   = hexa_color->red;
        req.color->green = hexa_color->green;
        req.color->blue  = hexa_color->blue;
        req.color->alpha = req.alpha;
        req.color->initialized = true;
        p_delete(&hexa_color);
        return true;
    }

    warn("awesome: error, cannot allocate color '%s'", req.colstr);
    return false;
}

/** Convert a xcolor struct to a color one.
 * \param xcol The X color.
 * \param col The color.
 * \return True if everything has been converted.
 */
bool
xcolor_to_color(const xcolor_t *xcol, color_t *col)
{
    if (!xcol->initialized)
    {
        col->initialized = false;
        return false;
    }

    col->initialized = true;
    col->red   = RGB_16TO8(xcol->red);
    col->green = RGB_16TO8(xcol->green);
    col->blue  = RGB_16TO8(xcol->blue);
    col->alpha = RGB_16TO8(xcol->alpha);

    return true;
}

/** Push a color as a string onto the stack
 * \param L The Lua VM state.
 * \param c The color to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_pushxcolor(lua_State *L, const xcolor_t c)
{
    uint8_t r = (unsigned) c.red   * 0xff / 0xffff;
    uint8_t g = (unsigned) c.green * 0xff / 0xffff;
    uint8_t b = (unsigned) c.blue  * 0xff / 0xffff;
    uint8_t a = (unsigned) c.alpha * 0xff / 0xffff;
    char s[10];
    int len;
    /* do not print alpha if it's full */
    if(a == 0xff)
        len = snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
    else
        len = snprintf(s, sizeof(s), "#%02x%02x%02x%02x", r, g, b, a);
    lua_pushlstring(L, s, len);
    return 1;
}

/** Push a color as a string onto the stack
 * \param L The Lua VM state.
 * \param c The color to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_pushcolor(lua_State *L, const color_t *c)
{
    uint8_t r = c->red;
    uint8_t g = c->green;
    uint8_t b = c->blue;
    uint8_t a = c->alpha;
    char s[10];
    int len;
    /* do not print alpha if it's full */
    if(a == 0xff)
        len = snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
    else
        len = snprintf(s, sizeof(s), "#%02x%02x%02x%02x", r, g, b, a);
    lua_pushlstring(L, s, len);
    return 1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
