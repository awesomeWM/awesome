/*
 * draw.c - draw functions
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

#include <cairo-xcb.h>

#include "config.h"

#include <langinfo.h>
#include <iconv.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include "structs.h"

#include "common/tokenize.h"

extern awesome_t globalconf;

/** Convert text from any charset to UTF-8 using iconv.
 * \param iso The ISO string to convert.
 * \param len The string size.
 * \param dest The destination pointer. Memory will be allocated, up to you to
 * free, like any char *.
 * \param dlen The destination length, can be NULL.
 * \return True if conversion was done.
 */
bool
draw_iso2utf8(const char *iso, size_t len, char **dest, ssize_t *dlen)
{
    static iconv_t iso2utf8 = (iconv_t) -1;
    static int8_t dont_need_convert = -1;

    if(dont_need_convert == -1)
        dont_need_convert = !a_strcmp(nl_langinfo(CODESET), "UTF-8");

    if(!len || dont_need_convert)
        return false;

    if(iso2utf8 == (iconv_t) -1)
    {
        iso2utf8 = iconv_open("UTF-8", nl_langinfo(CODESET));
        if(iso2utf8 == (iconv_t) -1)
        {
            if(errno == EINVAL)
                warn("unable to convert text from %s to UTF-8, not available",
                     nl_langinfo(CODESET));
            else
                warn("unable to convert text: %s", strerror(errno));

            return false;
        }
    }

    size_t orig_utf8len, utf8len;
    char *utf8;

    orig_utf8len = utf8len = 2 * len + 1;
    utf8 = *dest = p_new(char, utf8len);

    if(iconv(iso2utf8, (char **) &iso, &len, &utf8, &utf8len) == (size_t) -1)
    {
        warn("text conversion failed: %s", strerror(errno));
        p_delete(dest);
    }

    if(dlen)
        *dlen = orig_utf8len - utf8len;

    return true;
}

static xcb_visualtype_t *
draw_screen_default_visual(xcb_screen_t *s)
{
    xcb_depth_iterator_t depth_iter;
    xcb_visualtype_iterator_t visual_iter;

    if(!s)
        return NULL;

    for(depth_iter = xcb_screen_allowed_depths_iterator(s);
        depth_iter.rem; xcb_depth_next (&depth_iter))
        for(visual_iter = xcb_depth_visuals_iterator (depth_iter.data);
             visual_iter.rem; xcb_visualtype_next (&visual_iter))
            if (s->root_visual == visual_iter.data->visual_id)
                return visual_iter.data;

    return NULL;
}

/** Create a new Pango font.
 * \param fontname Pango fontname (e.g. [FAMILY-LIST] [STYLE-OPTIONS] [SIZE]).
 * \return A new font.
 */
font_t *
draw_font_new(const char *fontname)
{
    cairo_surface_t *surface;
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, globalconf.default_screen);
    cairo_t *cr;
    PangoLayout *layout;
    font_t *font = p_new(font_t, 1);

    /* Create a dummy cairo surface, cairo context and pango layout in
     * order to get font informations */
    surface = cairo_xcb_surface_create(globalconf.connection,
                                       globalconf.default_screen,
                                       draw_screen_default_visual(s),
                                       s->width_in_pixels,
                                       s->height_in_pixels);

    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    /* Get the font description used to set text on a PangoLayout */
    font->desc = pango_font_description_from_string(fontname);
    pango_layout_set_font_description(layout, font->desc);

    /* Get height */
    pango_layout_get_pixel_size(layout, NULL, &font->height);

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return font;
}

/** Delete a font.
 * \param font Font to delete.
 */
void
draw_font_delete(font_t **font)
{
    if(*font)
    {
        pango_font_description_free((*font)->desc);
        p_delete(font);
    }
}

/** Initialize a draw_text_context_t with text data.
 * \param data The draw text context to init.
 * \param str The text string to render.
 * \param slen The text string length.
 * \return True if everything is ok, false otherwise.
 */
bool
draw_text_context_init(draw_text_context_t *data, const char *str, ssize_t slen)
{
    GError *error = NULL;

    if(!pango_parse_markup(str, slen, 0, &data->attr_list, &data->text, NULL, &error))
    {
        warn("cannot parse pango markup: %s", error ? error->message : "unknown error");
        if(error)
            g_error_free(error);
        return false;
    }

    data->len = a_strlen(data->text);

    return true;
}

/** Initialize a new draw context.
 * \param d The draw context to initialize.
 * \param phys_screen Physical screen id.
 * \param width Width.
 * \param height Height.
 * \param px Pixmap object to store.
 * \param fg Foreground color.
 * \param bg Background color.
 */
void
draw_context_init(draw_context_t *d, int phys_screen,
                  int width, int height, xcb_pixmap_t px,
                  const xcolor_t *fg, const xcolor_t *bg)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, phys_screen);

    d->phys_screen = phys_screen;
    d->width = width;
    d->height = height;
    d->depth = s->root_depth;
    d->visual = draw_screen_default_visual(s);
    d->pixmap = px;
    d->surface = cairo_xcb_surface_create(globalconf.connection, px, d->visual, width, height);
    d->cr = cairo_create(d->surface);
    d->layout = pango_cairo_create_layout(d->cr);
    d->fg = *fg;
    d->bg = *bg;
};

/** Draw text into a draw context.
 * \param ctx Draw context  to draw to.
 * \param font The font to use.
 * \param elip Ellipsize mode.
 * \param wrap Wrap mode.
 * \param align Text alignment.
 * \param margin Margin to respect when drawing text.
 * \param area Area to draw to.
 * \param data Draw text context data.
 * \param ext Text extents.
 */
void
draw_text(draw_context_t *ctx, draw_text_context_t *data, font_t *font,
          PangoEllipsizeMode ellip, PangoWrapMode wrap,
          alignment_t align, padding_t *margin, area_t area, area_t *ext)
{
    int x, y;

    pango_layout_set_text(ctx->layout, data->text, data->len);
    pango_layout_set_width(ctx->layout,
                           pango_units_from_double(area.width
                                                   - (margin->left
                                                      + margin->right)));
    pango_layout_set_height(ctx->layout, pango_units_from_double(area.height)
                                         - (margin->top + margin->bottom));
    pango_layout_set_ellipsize(ctx->layout, ellip);
    pango_layout_set_wrap(ctx->layout, wrap);
    pango_layout_set_attributes(ctx->layout, data->attr_list);
    pango_layout_set_font_description(ctx->layout, font->desc);

    x = area.x + margin->left;
    /* + 1 is added for rounding, so that in any case of doubt we rather draw
     * the text 1px lower than too high which usually results in a better type
     * face */
    y = area.y + (ctx->height - ext->height + 1) / 2 + margin->top;

    /* only honors alignment if enough space */
    if(ext->width < area.width)
        switch(align)
        {
          case AlignCenter:
            x += (area.width - ext->width) / 2;
            break;
          case AlignRight:
            x += area.width - ext->width;
            break;
          default:
            break;
        }

    cairo_move_to(ctx->cr, x, y);

    cairo_set_source_rgba(ctx->cr,
                          ctx->fg.red / 65535.0,
                          ctx->fg.green / 65535.0,
                          ctx->fg.blue / 65535.0,
                          ctx->fg.alpha / 65535.0);
    pango_cairo_update_layout(ctx->cr, ctx->layout);
    pango_cairo_show_layout(ctx->cr, ctx->layout);
}

/** Setup color-source for cairo (gradient or mono).
 * \param ctx Draw context.
 * \param gradient_vector x, y to x + x_offset, y + y_offset.
 * \param pcolor Color to use at start of gradient_vector.
 * \param pcolor_center Color at center of gradient_vector.
 * \param pcolor_end Color at end of gradient_vector.
 * \return pat Pattern or NULL, needs to get cairo_pattern_destroy()'ed.
 */
static cairo_pattern_t *
draw_setup_cairo_color_source(draw_context_t *ctx, vector_t gradient_vector,
                              const xcolor_t *pcolor, const xcolor_t *pcolor_center,
                              const xcolor_t *pcolor_end)
{
    cairo_pattern_t *pat = NULL;
    bool has_center = pcolor_center->initialized;
    bool has_end    = pcolor_end->initialized;

    /* no need for a real pattern: */
    if(!has_end && !has_center)
        cairo_set_source_rgba(ctx->cr,
                              pcolor->red / 65535.0,
                              pcolor->green / 65535.0,
                              pcolor->blue / 65535.0,
                              pcolor->alpha / 65535.0);
    else
    {
        pat = cairo_pattern_create_linear(gradient_vector.x,
                                          gradient_vector.y,
                                          gradient_vector.x + gradient_vector.x_offset,
                                          gradient_vector.y + gradient_vector.y_offset);

        /* pcolor is always set (so far in awesome) */
        cairo_pattern_add_color_stop_rgba(pat, 0.0,
                                          pcolor->red / 65535.0,
                                          pcolor->green / 65535.0,
                                          pcolor->blue / 65535.0,
                                          pcolor->alpha / 65535.0);

        if(has_center)
            cairo_pattern_add_color_stop_rgba(pat, 0.5,
                                              pcolor_center->red / 65535.0,
                                              pcolor_center->green / 65535.0,
                                              pcolor_center->blue / 65535.0,
                                              pcolor_center->alpha / 65535.0);

        if(has_end)
            cairo_pattern_add_color_stop_rgba(pat, 1.0,
                                              pcolor_end->red / 65535.0,
                                              pcolor_end->green / 65535.0,
                                              pcolor_end->blue / 65535.0,
                                              pcolor_end->alpha / 65535.0);
        else
            cairo_pattern_add_color_stop_rgba(pat, 1.0,
                                              pcolor->red / 65535.0,
                                              pcolor->green / 65535.0,
                                              pcolor->blue / 65535.0,
                                              pcolor->alpha / 65535.0);
        cairo_set_source(ctx->cr, pat);
    }
    return pat;
}

/** Draw rectangle inside the coordinates
 * \param ctx Draw context
 * \param geometry geometry
 * \param line_width line width
 * \param filled fill rectangle?
 * \param color color to use
 */
void
draw_rectangle(draw_context_t *ctx, area_t geometry,
               float line_width, bool filled, const xcolor_t *color)
{
    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, line_width);
    cairo_set_miter_limit(ctx->cr, 10.0);
    cairo_set_line_join(ctx->cr, CAIRO_LINE_JOIN_MITER);
    cairo_set_source_rgba(ctx->cr,
                          color->red / 65535.0,
                          color->green / 65535.0,
                          color->blue / 65535.0,
                          color->alpha / 65535.0);
    if(filled)
    {
        cairo_rectangle(ctx->cr, geometry.x, geometry.y,
                        geometry.width, geometry.height);
        cairo_fill(ctx->cr);
    }
    else
    {
        cairo_rectangle(ctx->cr, geometry.x + line_width / 2.0, geometry.y + line_width / 2.0,
                        geometry.width - line_width, geometry.height - line_width);
        cairo_stroke(ctx->cr);
    }
}

/** Draw rectangle with gradient colors
 * \param ctx Draw context.
 * \param geometry Geometry.
 * \param line_width Line width.
 * \param filled Filled rectangle?
 * \param gradient_vector Color-gradient course.
 * \param pcolor Color at start of gradient_vector.
 * \param pcolor_center Color in the center.
 * \param pcolor_end Color at end of gradient_vector.
 */
void
draw_rectangle_gradient(draw_context_t *ctx, area_t geometry, float line_width, bool filled,
                        vector_t gradient_vector, const xcolor_t *pcolor,
                        const xcolor_t *pcolor_center, const xcolor_t *pcolor_end)
{
    cairo_pattern_t *pat;

    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, line_width);
    cairo_set_miter_limit(ctx->cr, 10.0);
    cairo_set_line_join(ctx->cr, CAIRO_LINE_JOIN_MITER);

    pat = draw_setup_cairo_color_source(ctx, gradient_vector, pcolor, pcolor_center, pcolor_end);

    if(filled)
    {
        cairo_rectangle(ctx->cr, geometry.x, geometry.y, geometry.width, geometry.height);
        cairo_fill(ctx->cr);
    }
    else
    {
        cairo_rectangle(ctx->cr, geometry.x + 1, geometry.y, geometry.width - 1, geometry.height - 1);
        cairo_stroke(ctx->cr);
    }

    if(pat)
        cairo_pattern_destroy(pat);
}

/** Setup some cairo-things for drawing a graph
 * \param ctx Draw context
 */
void
draw_graph_setup(draw_context_t *ctx)
{
    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, 1.0);
    /* without it, it can draw over the path on sharp angles
     * ...too long lines * (...graph_line) */
    cairo_set_miter_limit(ctx->cr, 0.0);
    cairo_set_line_join(ctx->cr, CAIRO_LINE_JOIN_MITER);
}

/** Draw a graph.
 * \param ctx Draw context.
 * \param rect The area to draw into.
 * \param from Array of starting-point offsets to draw a graph lines.
 * \param to Array of end-point offsets to draw a graph lines.
 * \param cur_index Current position in data-array (cycles around).
 * \param grow Put new values to the left or to the right.
 * \param gradient_vector Color-Gradient course.
 * \param pcolor Color at start of gradient_vector.
 * \param pcolor_center Color in the center.
 * \param pcolor_end Color at end of gradient_vector.
 */
void
draw_graph(draw_context_t *ctx, area_t rect, int *from, int *to, int cur_index,
           position_t grow, vector_t gradient_vector, const xcolor_t *pcolor,
           const xcolor_t *pcolor_center, const xcolor_t *pcolor_end)
{
    int i = -1;
    float x = rect.x + 0.5; /* middle of a pixel */
    cairo_pattern_t *pat;

    pat = draw_setup_cairo_color_source(ctx, gradient_vector,
                                        pcolor, pcolor_center, pcolor_end);

    if(grow == Right) /* draw from right to left */
    {
        x += rect.width - 1;
        while(++i < rect.width)
        {
            cairo_move_to(ctx->cr, x, rect.y - from[cur_index]);
            cairo_line_to(ctx->cr, x, rect.y - to[cur_index]);
            x -= 1.0;

            if (--cur_index < 0)
                cur_index = rect.width - 1;
        }
    }
    else /* draw from left to right */
        while(++i < rect.width)
        {
            cairo_move_to(ctx->cr, x, rect.y - from[cur_index]);
            cairo_line_to(ctx->cr, x, rect.y - to[cur_index]);
            x += 1.0;

            if (--cur_index < 0)
                cur_index = rect.width - 1;
        }

    cairo_stroke(ctx->cr);

    if(pat)
        cairo_pattern_destroy(pat);
}

/** Draw a line into a graph-widget.
 * \param ctx Draw context.
 * \param rect The area to draw into.
 * \param to array of offsets to draw the line through...
 * \param cur_index current position in data-array (cycles around)
 * \param grow put new values to the left or to the right
 * \param gradient_vector Color-gradient course.
 * \param pcolor Color at start of gradient_vector.
 * \param pcolor_center Color in the center.
 * \param pcolor_end Color at end of gradient_vector.
 */
void
draw_graph_line(draw_context_t *ctx, area_t rect, int *to, int cur_index,
                position_t grow, vector_t gradient_vector, const xcolor_t *pcolor,
                const xcolor_t *pcolor_center, const xcolor_t *pcolor_end)
{
    int i, w;
    float x, y;
    cairo_pattern_t *pat;

    /* NOTE: without, it sometimes won't draw to some path (centered in a pixel)
     * ... it won't fill some pixels! It also looks much nicer so.
     * (since line-drawing is the last on the graph, no need to reset to _NONE) */
    /* Not much difference to CAIRO_ANTIALIAS_DEFAULT, but recommend for LCD */
    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_SUBPIXEL);
    /* a nicer, better visible line compared to 1.0 */
    cairo_set_line_width(ctx->cr, 1.25);

    pat = draw_setup_cairo_color_source(ctx, gradient_vector, pcolor, pcolor_center, pcolor_end);

    /* path through the centers of pixels */
    x = rect.x + 0.5;
    y = rect.y + 0.5;
    w = rect.width;

    if(grow == Right)
    {
        /* go through the values from old to new. Begin with the oldest. */
        if(++cur_index > w - 1)
            cur_index = 0;

        cairo_move_to(ctx->cr, x, y - to[cur_index]);
    }
    else
        /* on the left border: fills a pixel also when there's only one value */
        cairo_move_to(ctx->cr, x - 1.0, y - to[cur_index]);

    for(i = 0; i < w; i++)
    {
        cairo_line_to(ctx->cr, x, y - to[cur_index]);
        x += 1.0;

        /* cycles around the index */
        if(grow == Right)
        {
            if (++cur_index > w - 1)
                cur_index = 0;
        }
        else
        {
            if(--cur_index < 0)
                cur_index = w - 1;
        }
    }

    /* onto the right border: fills a pixel also when there's only one value */
    if(grow == Right)
        cairo_line_to(ctx->cr, x, y - to[cur_index]);

    cairo_stroke(ctx->cr);

    if(pat)
        cairo_pattern_destroy(pat);
    /* reset line-width */
    cairo_set_line_width(ctx->cr, 1.0);
}

/** Draw an image from ARGB data to a draw context.
 * Data should be stored as an array of alpha, red, blue, green for each pixel
 * and the array size should be w * h elements long.
 * \param ctx Draw context to draw to.
 * \param x X coordinate.
 * \param y Y coordinate.
 * \param w Width.
 * \param h Height.
 * \param ratio The ratio to apply to the image.
 * \param data The image pixels array.
 */
static void
draw_image_from_argb_data(draw_context_t *ctx, int x, int y, int w, int h,
                          double ratio, unsigned char *data)
{
    cairo_t *cr;
    cairo_surface_t *source;

    source = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, w, h,
#if CAIRO_VERSION_MAJOR < 1 || (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR < 5) || (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR == 5 && CAIRO_VERSION_MICRO < 8)
                                                 sizeof(unsigned char) * 4 * w);
#else
                                                 cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w));
#endif
    cr = cairo_create(ctx->surface);
    cairo_scale(cr, ratio, ratio);
    cairo_set_source_surface(cr, source, x / ratio, y / ratio);

    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
}

/** Draw an image to a draw context.
 * \param ctx Draw context to draw to.
 * \param x X coordinate.
 * \param y Y coordinate.
 * \param ratio The ratio to apply to the image.
 * \param image The image to draw.
 */
void
draw_image(draw_context_t *ctx, int x, int y, double ratio, image_t *image)
{
    draw_image_from_argb_data(ctx, x, y, image->width, image->height, ratio, image->data);
}

/** Rotate a pixmap.
 * \param ctx Draw context to draw with.
 * \param src Drawable to draw from.
 * \param dest Drawable to draw to.
 * \param src_w Drawable width.
 * \param src_h Drawable height.
 * \param dest_w Drawable width.
 * \param dest_h Drawable height.
 * \param angle angle to rotate.
 * \param tx Translate to this x coordinate.
 * \param ty Translate to this y coordinate.
 */
void
draw_rotate(draw_context_t *ctx,
            xcb_pixmap_t src, xcb_pixmap_t dest,
            int src_w, int src_h,
            int dest_w, int dest_h,
            double angle, int tx, int ty)
{
    cairo_surface_t *surface, *source;
    cairo_t *cr;

    surface = cairo_xcb_surface_create(globalconf.connection, dest,
                                       ctx->visual, dest_w, dest_h);
    source = cairo_xcb_surface_create(globalconf.connection, src,
                                      ctx->visual, src_w, src_h);
    cr = cairo_create (surface);

    cairo_translate(cr, tx, ty);
    cairo_rotate(cr, angle);

    cairo_set_source_surface(cr, source, 0.0, 0.0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
    cairo_surface_destroy(surface);
}

/** Return the width and height of a text in pixel.
 * \param data The draw context text data.
 * \param font Font to use.
 * \return Text height and width.
 */
area_t
draw_text_extents(draw_text_context_t *data, font_t *font)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    PangoLayout *layout;
    PangoRectangle ext;
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, globalconf.default_screen);
    area_t geom = { 0, 0, 0, 0 };

    if(data->len <= 0)
        return geom;

    surface = cairo_xcb_surface_create(globalconf.connection,
                                       globalconf.default_screen,
                                       draw_screen_default_visual(s),
                                       s->width_in_pixels,
                                       s->height_in_pixels);

    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, data->text, data->len);
    pango_layout_set_attributes(layout, data->attr_list);
    pango_layout_set_font_description(layout, font->desc);
    pango_layout_get_pixel_extents(layout, NULL, &ext);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    geom.width = ext.width;
    geom.height = ext.height;

    return geom;
}

/** Transform a string to a alignment_t type.
 * Recognized string are flex, left, center or right. Everything else will be
 * recognized as AlignLeft.
 * \param align Atring with align text.
 * \param len The string length.
 * \return An alignment_t type.
 */
alignment_t
draw_align_fromstr(const char *align, ssize_t len)
{
    switch (a_tokenize(align, len))
    {
      case A_TK_CENTER: return AlignCenter;
      case A_TK_RIGHT:  return AlignRight;
      case A_TK_FLEX:   return AlignFlex;
      case A_TK_TOP:    return AlignTop;
      case A_TK_BOTTOM: return AlignBottom;
      default:          return AlignLeft;
    }
}

/** Transform an alignment to a string.
 * \param a The alignment.
 * \return A string which must not be freed.
 */
const char *
draw_align_tostr(alignment_t a)
{
    switch(a)
    {
      case AlignLeft:   return "left";
      case AlignCenter: return "center";
      case AlignRight:  return "right";
      case AlignFlex:   return "flex";
      case AlignBottom: return "bottom";
      case AlignTop:    return "top";
      default:          return NULL;
    }
}

#define RGB_COLOR_8_TO_16(i) (65535 * ((i) & 0xff) / 255)

/** Send a request to initialize a X color.
 * \param color xcolor_t struct to store color into.
 * \param colstr Color specification.
 * \return request informations.
 */
xcolor_init_request_t
xcolor_init_unchecked(xcolor_t *color, const char *colstr, ssize_t len)
{
    xcb_screen_t *s = xutil_screen_get(globalconf.connection, globalconf.default_screen);
    xcolor_init_request_t req;
    unsigned long colnum;
    uint16_t red, green, blue;

    p_clear(&req, 1);

    if(!len)
    {
        req.has_error = true;
        return req;
    }

    req.alpha = 0xffff;
    req.color = color;

    /* The color is given in RGB value */
    if(colstr[0] == '#')
    {
        char *p;

        if(len == 7)
        {
            colnum = strtoul(colstr + 1, &p, 16);
            if(p - colstr != 7)
                goto invalid;
        }
        /* we have alpha */
        else if(len == 9)
        {
            colnum = strtoul(colstr + 1, &p, 16);
            if(p - colstr != 9)
                goto invalid;
            req.alpha = RGB_COLOR_8_TO_16(colnum);
            colnum >>= 8;
        }
        else
        {
          invalid:
            warn("awesome: error, invalid color '%s'", colstr);
            req.has_error = true;
            return req;
        }

        red   = RGB_COLOR_8_TO_16(colnum >> 16);
        green = RGB_COLOR_8_TO_16(colnum >> 8);
        blue  = RGB_COLOR_8_TO_16(colnum);

        req.is_hexa = true;
        req.cookie_hexa = xcb_alloc_color_unchecked(globalconf.connection,
                                                    s->default_colormap,
                                                    red, green, blue);
    }
    else
    {
        req.is_hexa = false;
        req.cookie_named = xcb_alloc_named_color_unchecked(globalconf.connection,
                                                           s->default_colormap, len,
                                                               colstr);
    }

    req.has_error = false;
    req.colstr = colstr;

    return req;
}

/** Initialize a X color.
 * \param req xcolor_init request.
 * \return True if color allocation was successfull.
 */
bool
xcolor_init_reply(xcolor_init_request_t req)
{
    if(req.has_error)
        return false;

    if(req.is_hexa)
    {
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
    }
    else
    {
        xcb_alloc_named_color_reply_t *named_color;

        if((named_color = xcb_alloc_named_color_reply(globalconf.connection,
                                                      req.cookie_named, NULL)))
        {
            req.color->pixel = named_color->pixel;
            req.color->red   = named_color->visual_red;
            req.color->green = named_color->visual_green;
            req.color->blue  = named_color->visual_blue;
            req.color->alpha = req.alpha;
            req.color->initialized = true;
            p_delete(&named_color);
            return true;
        }
    }

    warn("awesome: error, cannot allocate color '%s'", req.colstr);
    return false;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
