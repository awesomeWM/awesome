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

#include "color.h"
#include "globalconf.h"

#include <ctype.h>

/* 0xFFFF / 0xFF == 0x101 (257) */
#define RGB_8TO16(i) (((i) & 0xff)   * 0x101)
#define RGB_16TO8(i) (((i) & 0xffff) / 0x101)

/** Parse an hexadecimal color string to its component.
 * \param colstr The color string.
 * \param len The color string length.
 * \param red A pointer to the red color to fill.
 * \param green A pointer to the green color to fill.
 * \param blue A pointer to the blue color to fill.
 * \return True if everything alright.
 */
static bool
color_parse(const char *colstr, ssize_t len,
            uint8_t *red, uint8_t *green, uint8_t *blue, uint8_t *alpha)
{
    unsigned long colnum;
    char *p;

    *alpha = 0xff;

    colnum = strtoul(colstr + 1, &p, 16);
    if(len == 9 && (p - colstr) == 9)
    {
        *alpha = colnum & 0xff;
        colnum >>= 8;
        len -= 2;
        p -= 2;
    }
    if(len != 7 || colstr[0] != '#' || (p - colstr) != 7)
    {
        warn("awesome: error, invalid color '%s'", colstr);
        return false;
    }

    *red   = (colnum >> 16) & 0xff;
    *green = (colnum >> 8) & 0xff;
    *blue  = colnum & 0xff;

    return true;
}

/** Given a color component value in the range from 0x00 to 0xff and a mask that
 * specifies where the component is, move the component into place. For example,
 * component=0x12 and mask=0xff00 return 0x1200. Note that the mask can have a
 * different width, for example component=0x12 and mask=0xf00 return 0x100.
 * \param component The intensity of a color component.
 * \param mask A mask containing a consecutive number of bits set to 1 defining
 * where the component is.
 * \return A pixel value containing the giving component in the given component.
 */
static uint32_t
apply_mask(uint8_t component, uint32_t mask)
{
    unsigned int shift = 0;
    unsigned int width = 0;

    // Shift the mask right until the first set bit appears
    while (mask != 0 && (mask & 0x1) == 0) {
        shift++;
        mask >>= 1;
    }
    // Shift the mask right until we saw all set bits
    while ((mask & 0x1) == 1) {
        width++;
        mask >>= 1;
    }

    // The mask consists of [width] set bits followed by [shift] unset bits.
    // Modify the component accordingly.
    uint32_t result = component;

    // Scale the result up to the desired width
    if (width < 8)
        result >>= (8 - width);
    if (width > 8)
        result <<= (width - 8);
    return result << shift;
}

/** Send a request to initialize a X color.
 * \param color color_t struct to store color into.
 * \param colstr Color specification.
 * \param len The length of colstr (which still MUST be NULL terminated).
 * \param visual The visual for which the color is to be allocated.
 * \return request information.
 */
color_init_request_t
color_init_unchecked(color_t *color, const char *colstr, ssize_t len, xcb_visualtype_t *visual)
{
    color_init_request_t req;
    uint8_t red, green, blue, alpha;

    p_clear(&req, 1);

    if(!len)
    {
        req.has_error = true;
        return req;
    }

    req.color = color;

    /* The color is given in RGB value */
    if(!color_parse(colstr, len, &red, &green, &blue, &alpha))
    {
        warn("awesome: error, invalid color '%s'", colstr);
        req.has_error = true;
        return req;
    }

    req.colstr = colstr;
    req.has_error = false;

    if (visual->_class == XCB_VISUAL_CLASS_TRUE_COLOR || visual->_class == XCB_VISUAL_CLASS_DIRECT_COLOR) {
        uint32_t pixel = 0;
        pixel |= apply_mask(red, visual->red_mask);
        pixel |= apply_mask(blue, visual->blue_mask);
        pixel |= apply_mask(green, visual->green_mask);
        if (draw_visual_depth(globalconf.screen, visual->visual_id) == 32) {
            /* This is not actually in the X11 protocol, but we assume that this
             * is an ARGB visual and everything unset in some mask is alpha.
             */
            pixel |= apply_mask(alpha, ~(visual->red_mask | visual->blue_mask | visual->green_mask));
        }
        req.color->pixel = pixel;
        req.color->red   = RGB_8TO16(red);
        req.color->green = RGB_8TO16(green);
        req.color->blue  = RGB_8TO16(blue);
        req.color->alpha = RGB_8TO16(alpha);
        req.color->initialized = true;
        return req;
    }
    req.cookie_hexa = xcb_alloc_color_unchecked(globalconf.connection,
                                                globalconf.screen_cmap,
                                                RGB_8TO16(red),
                                                RGB_8TO16(green),
                                                RGB_8TO16(blue));


    return req;
}

/** Initialize a X color.
 * \param req color_init request.
 * \return True if color allocation was successful.
 */
bool
color_init_reply(color_init_request_t req)
{
    if(req.has_error)
        return false;
    if(req.color->initialized)
        return true;

    xcb_alloc_color_reply_t *hexa_color;

    if((hexa_color = xcb_alloc_color_reply(globalconf.connection,
                                           req.cookie_hexa, NULL)))
    {
        req.color->pixel = hexa_color->pixel;
        req.color->red   = hexa_color->red;
        req.color->green = hexa_color->green;
        req.color->blue  = hexa_color->blue;
        req.color->alpha = 0xffff;
        req.color->initialized = true;
        p_delete(&hexa_color);
        return true;
    }

    warn("awesome: error, cannot allocate color '%s'", req.colstr);
    return false;
}

/** Push a color as a string onto the stack
 * \param L The Lua VM state.
 * \param c The color to push.
 * \return The number of elements pushed on stack.
 */
int
luaA_pushcolor(lua_State *L, const color_t c)
{
    uint8_t r = RGB_16TO8(c.red);
    uint8_t g = RGB_16TO8(c.green);
    uint8_t b = RGB_16TO8(c.blue);
    uint8_t a = RGB_16TO8(c.alpha);

    char s[1 + 4*2 + 1];
    int len;
    if (a >= 0xff)
        len = snprintf(s, sizeof(s), "#%02x%02x%02x", r, g, b);
    else
        len = snprintf(s, sizeof(s), "#%02x%02x%02x%02x", r, g, b, a);
    lua_pushlstring(L, s, len);
    return 1;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
