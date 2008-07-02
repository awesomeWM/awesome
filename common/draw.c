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
#ifdef WITH_IMLIB2
#include <Imlib2.h>
#else
#include <gdk/gdkcairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>

#include <langinfo.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "common/tokenize.h"
#include "common/draw.h"
#include "common/markup.h"
#include "common/xutil.h"

void draw_parser_data_init(draw_parser_data_t *pdata)
{
    p_clear(pdata, 1);
    buffer_init(&pdata->text);
}

void draw_parser_data_wipe(draw_parser_data_t *pdata)
{
    buffer_wipe(&pdata->text);
    draw_image_delete(&pdata->bg_image);
}

/** Convert text from any charset to UTF-8 using iconv.
 * \param iso The ISO string to convert.
 * \param len The string size.
 * \return NULL if error, otherwise pointer to the new converted string.
 */
char *
draw_iso2utf8(const char *iso, size_t len)
{
    iconv_t iso2utf8;
    size_t utf8len;
    char *utf8, *utf8p;

    if(!len)
        return NULL;

    if(!a_strcmp(nl_langinfo(CODESET), "UTF-8"))
        return NULL;

    iso2utf8 = iconv_open("UTF-8", nl_langinfo(CODESET));
    if(iso2utf8 == (iconv_t) -1)
    {
        if(errno == EINVAL)
            warn("unable to convert text from %s to UTF-8, not available",
                 nl_langinfo(CODESET));
        else
            warn("unable to convert text: %s", strerror(errno));

        return NULL;
    }

    utf8len = 2 * len + 1;
    utf8 = utf8p = p_new(char, utf8len);

    if(iconv(iso2utf8, (char **) &iso, &len, &utf8, &utf8len) == (size_t) -1)
    {
        warn("text conversion failed: %s", strerror(errno));
        p_delete(&utf8p);
    }

    if(iconv_close(iso2utf8))
        warn("error closing iconv");

    return utf8p;
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

/** Create a new draw context.
 * \param conn Connection ref.
 * \param phys_screen Physical screen id.
 * \param width Width.
 * \param height Height.
 * \param px Pixmap object to store.
 * \param fg Foreground color.
 * \param bg Background color.
 * \return A draw context pointer.
 */
draw_context_t *
draw_context_new(xcb_connection_t *conn, int phys_screen,
                 int width, int height, xcb_pixmap_t px,
                 const xcolor_t *fg, const xcolor_t *bg)
{
    draw_context_t *d = p_new(draw_context_t, 1);
    xcb_screen_t *s = xutil_screen_get(conn, phys_screen);

    d->connection = conn;
    d->phys_screen = phys_screen;
    d->width = width;
    d->height = height;
    d->depth = s->root_depth;
    d->visual = draw_screen_default_visual(s);
    d->pixmap = px;
    d->surface = cairo_xcb_surface_create(conn, px, d->visual, width, height);
    d->cr = cairo_create(d->surface);
    d->layout = pango_cairo_create_layout(d->cr);
    d->fg = *fg;
    d->bg = *bg;

    return d;
};

/** Create a new Pango font
 * \param conn Connection ref
 * \param phys_screen The physical screen number.
 * \param fontname Pango fontname (e.g. [FAMILY-LIST] [STYLE-OPTIONS] [SIZE])
 * \return a new font
 */
font_t *
draw_font_new(xcb_connection_t *conn, int phys_screen, const char *fontname)
{
    cairo_surface_t *surface;
    xcb_screen_t *s = xutil_screen_get(conn, phys_screen);
    cairo_t *cr;
    PangoLayout *layout;
    font_t *font = p_new(font_t, 1);

    /* Create a dummy cairo surface, cairo context and pango layout in
     * order to get font informations */
    surface = cairo_xcb_surface_create(conn,
                                       phys_screen,
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

static void
draw_markup_on_element(markup_parser_data_t *p, const char *elem,
                       const char **names, const char **values)
{
    draw_parser_data_t *data = p->priv;

    /* hack: markup.c validates tags so we can avoid strcmps here */
    switch (*elem) {
      case 'b': /* bg */
        for(; *names; names++, values++)
            switch(a_tokenize(*names, -1))
            {
              case A_TK_COLOR:
                data->has_bg_color = xcolor_init(&data->bg_color, data->connection, data->phys_screen, *values);
                break;
              case A_TK_IMAGE:
                if(data->bg_image)
                    draw_image_delete(&data->bg_image);
                data->bg_image = draw_image_new(*values);
                break;
              case A_TK_ALIGN:
                data->bg_align = draw_align_fromstr(*values, -1);
              case A_TK_RESIZE:
                data->bg_resize = a_strtobool(*values, -1);
              default:
                break;
            }
        break;
      case 't': /* text */
        for(; *names; names++, values++)
            switch(a_tokenize(*names, -1))
            {
              case A_TK_ALIGN:
                data->align = draw_align_fromstr(*values, -1);
                break;
              case A_TK_SHADOW:
                xcolor_init(&data->shadow.color, data->connection,
                            data->phys_screen, *values);
                break;
              case A_TK_SHADOW_OFFSET:
                data->shadow.offset = atoi(*values);
                break;
              default:
                break;
            }
        break;
      case 'm': /* margin */
        for (; *names; names++, values++)
            switch(a_tokenize(*names, -1))
            {
              case A_TK_LEFT:
                data->margin.left = atoi(*values);
                break;
              case A_TK_RIGHT:
                data->margin.right = atoi(*values);
                break;
              default:
                break;
            }
        break;
    }
}

static bool
draw_text_markup_expand(draw_parser_data_t *data,
                        const char *str, ssize_t slen)
{
    static char const * const elements[] = { "bg", "text", "margin", NULL };
    markup_parser_data_t p =
    {
        .elements   = elements,
        .priv       = data,
        .on_element = &draw_markup_on_element,
    };

    markup_parser_data_init(&p);

    if(!markup_parse(&p, str, slen))
    {
        markup_parser_data_wipe(&p);
        return false;
    }

    /* stole text */
    buffer_wipe(&data->text);
    data->text = p.text;
    buffer_init(&p.text);
    markup_parser_data_wipe(&p);

    return true;
}

/** Draw text into a draw context.
 * \param ctx Draw context  to draw to.
 * \param font The font to use.
 * \param area Area to draw to.
 * \param text Text to draw.
 * \param data Optional parser data.
 */
void
draw_text(draw_context_t *ctx, font_t *font,
          area_t area, const char *text, draw_parser_data_t *pdata)
{
    int x, y;
    ssize_t len = 0, olen;
    PangoRectangle ext;
    draw_parser_data_t parser_data;

    if(!(len = a_strlen(text)))
        return;

    if(!pdata)
    {
        draw_parser_data_init(&parser_data);
        parser_data.connection = ctx->connection;
        parser_data.phys_screen = ctx->phys_screen;
        if(draw_text_markup_expand(&parser_data, text, len))
        {
            text = parser_data.text.s;
            len = parser_data.text.len;
        }
        pdata = &parser_data;
    }
    else
    {
        text = pdata->text.s;
        len = pdata->text.len;
    }

    olen = len;

    if(pdata->has_bg_color)
        draw_rectangle(ctx, area, 1.0, true, &pdata->bg_color);

    if(pdata->bg_image)
    {
        x = area.x;
        y = area.y;
        switch(pdata->bg_align)
        {
          case AlignCenter:
            x += (area.width - pdata->bg_image->width) / 2;
            break;
          case AlignRight:
            x += area.width- pdata->bg_image->width;
            break;
          default:
            break;
        }
        draw_image(ctx, x, y, pdata->bg_resize ? area.height : 0, pdata->bg_image);
        draw_image_delete(&pdata->bg_image);
    }

    pango_layout_set_width(ctx->layout,
                           pango_units_from_double(area.width
                                                   - (pdata->margin.left
                                                      + pdata->margin.right)));
    pango_layout_set_ellipsize(ctx->layout, PANGO_ELLIPSIZE_END);
    pango_layout_set_markup(ctx->layout, text, len);
    pango_layout_set_font_description(ctx->layout, font->desc);
    pango_layout_get_pixel_extents(ctx->layout, NULL, &ext);

    x = area.x + pdata->margin.left;
    /* + 1 is added for rounding, so that in any case of doubt we rather draw
     * the text 1px lower than too high which usually results in a better type
     * face */
    y = area.y + (ctx->height - font->height + 1) / 2;

    switch(pdata->align)
    {
      case AlignCenter:
        x += (area.width - ext.width) / 2;
        break;
      case AlignRight:
        x += area.width - ext.width;
        break;
      default:
        break;
    }

    if(pdata->shadow.offset)
    {
        cairo_set_source_rgba(ctx->cr,
                              pdata->shadow.color.red / 65535.0,
                              pdata->shadow.color.green / 65535.0,
                              pdata->shadow.color.blue / 65535.0,
                              pdata->shadow.color.alpha / 65535.0);
        cairo_move_to(ctx->cr, x + pdata->shadow.offset, y + pdata->shadow.offset);
        pango_cairo_update_layout(ctx->cr, ctx->layout);
        pango_cairo_show_layout(ctx->cr, ctx->layout);
    }

    cairo_move_to(ctx->cr, x, y);

    cairo_set_source_rgba(ctx->cr,
                          ctx->fg.red / 65535.0,
                          ctx->fg.green / 65535.0,
                          ctx->fg.blue / 65535.0,
                          ctx->fg.alpha / 65535.0);
    pango_cairo_update_layout(ctx->cr, ctx->layout);
    pango_cairo_show_layout(ctx->cr, ctx->layout);

    if (pdata == &parser_data)
        draw_parser_data_wipe(&parser_data);
}

/** Setup color-source for cairo (gradient or mono).
 * \param ctx Draw context.
 * \param rect x, y to x + x_offset, y + y_offset.
 * \param pcolor Color to use at start (x, y).
 * \param pcolor_center Color at 50% of width.
 * \param pcolor_end Color at pattern end (x + x_offset, y + y_offset).
 * \return pat Pattern or NULL, needs to get cairo_pattern_destroy()'ed.
 */
static cairo_pattern_t *
draw_setup_cairo_color_source(draw_context_t *ctx, area_t rect,
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
        pat = cairo_pattern_create_linear(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);

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
 * \param ctx Draw context
 * \param geometry geometry
 * \param line_width line width
 * \param filled filled rectangle?
 * \param pattern_rect pattern geometry
 * \param pcolor color to use at start
 * \param pcolor_center color at 50% of width
 * \param pcolor_end color at pattern_start + pattern_width
 */
void
draw_rectangle_gradient(draw_context_t *ctx, area_t geometry, float line_width, bool filled,
                        area_t pattern_rect, const xcolor_t *pcolor,
                        const xcolor_t *pcolor_center, const xcolor_t *pcolor_end)
{
    cairo_pattern_t *pat;

    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, line_width);
    cairo_set_miter_limit(ctx->cr, 10.0);
    cairo_set_line_join(ctx->cr, CAIRO_LINE_JOIN_MITER);

    pat = draw_setup_cairo_color_source(ctx, pattern_rect, pcolor, pcolor_center, pcolor_end);

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
 * \param patt_rect Pattern geometry.
 * \param pcolor Color at the left.
 * \param pcolor_center Color in the center.
 * \param pcolor_end Color at the right.
 */
void
draw_graph(draw_context_t *ctx, area_t rect, int *from, int *to, int cur_index,
           position_t grow, area_t patt_rect, const xcolor_t *pcolor,
           const xcolor_t *pcolor_center, const xcolor_t *pcolor_end)
{
    int i = -1;
    float x = rect.x + 0.5; /* middle of a pixel */
    cairo_pattern_t *pat;

    pat = draw_setup_cairo_color_source(ctx, patt_rect,
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
 * \param patt_rect Pattern geometry.
 * \param pcolor color at the left
 * \param pcolor_center color in the center
 * \param pcolor_end color at the right
 */
void
draw_graph_line(draw_context_t *ctx, area_t rect, int *to, int cur_index,
                position_t grow, area_t patt_rect, const xcolor_t *pcolor,
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

    pat = draw_setup_cairo_color_source(ctx, patt_rect, pcolor, pcolor_center, pcolor_end);

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

/** Draw a circle.
 * \param ctx Draw context to draw to.
 * \param x X coordinate.
 * \param y Y coordinate.
 * \param r Size of the circle.
 * \param filled Fill circle, or not.
 * \param color Color to use.
 */
void
draw_circle(draw_context_t *ctx, int x, int y, int r, bool filled, const xcolor_t *color)
{
    cairo_set_line_width(ctx->cr, 1.0);
    cairo_set_source_rgba(ctx->cr,
                          color->red / 65535.0,
                          color->green / 65535.0,
                          color->blue / 65535.0,
                          color->alpha / 65535.0);

    cairo_new_sub_path(ctx->cr); /* don't draw from the old reference point to.. */

    if(filled)
    {
        cairo_arc (ctx->cr, x + r, y + r, r, 0, 2 * M_PI);
        cairo_fill(ctx->cr);
    }
    else
        cairo_arc (ctx->cr, x + r, y + r, r - 1, 0, 2 * M_PI);

    cairo_stroke(ctx->cr);
}

/** Draw an image from ARGB data to a draw context.
 * Data should be stored as an array of alpha, red, blue, green for each pixel
 * and the array size should be w * h elements long.
 * \param ctx Draw context to draw to.
 * \param x X coordinate.
 * \param y Y coordinate.
 * \param w Width.
 * \param h Height.
 * \param wanted_h Wanted height: if > 0, image will be resized.
 * \param data The image pixels array.
 */
void
draw_image_from_argb_data(draw_context_t *ctx, int x, int y, int w, int h,
                          int wanted_h, unsigned char *data)
{
    double ratio;
    cairo_t *cr;
    cairo_surface_t *source;

    source = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, w, h,
#if CAIRO_VERSION_MAJOR < 1 || (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR < 5) || (CAIRO_VERSION_MAJOR == 1 && CAIRO_VERSION_MINOR == 5 && CAIRO_VERSION_MICRO < 8)
                                                 sizeof(unsigned char) * 4 * w);
#else
                                                 cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w));
#endif
    cr = cairo_create(ctx->surface);
    if(wanted_h > 0 && h > 0)
    {
        ratio = (double) wanted_h / (double) h;
        cairo_scale(cr, ratio, ratio);
        cairo_set_source_surface(cr, source, x / ratio, y / ratio);
    }
    else
        cairo_set_source_surface(cr, source, x, y);

    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
}

#ifndef WITH_IMLIB2

/** Load an image from filename.
 * \param filename The image file to load.
 * \return A new image.
 */
draw_image_t *
draw_image_new(const char *filename)
{
    draw_image_t *image = NULL;
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    if(filename)
    {
        if(!(pixbuf = gdk_pixbuf_new_from_file(filename,&error)))
            warn("cannot load image %s: %s", filename, error->message);
        else
        {
            image = p_new(draw_image_t, 1);
            image->data = pixbuf;
            image->width = gdk_pixbuf_get_width(pixbuf);
            image->height = gdk_pixbuf_get_height(pixbuf);
        }
    }

    return image;
}

/** Delete an image.
 * \param image The image to delete.
 */
void
draw_image_delete(draw_image_t **image)
{
    if(*image)
    {
        gdk_pixbuf_unref((*image)->data);
        p_delete(image);
    }
}

/** Draw an image to a draw context.
 * \param ctx Draw context to draw to.
 * \param x x coordinate.
 * \param y y coordinate.
 * \param wanted_h Wanted height: if > 0, image will be resized.
 * \param image The image to draw.
 */
void
draw_image(draw_context_t *ctx, int x, int y, int wanted_h, draw_image_t *image)
{
    cairo_t *cr;

    cr = cairo_create(ctx->surface);
    if(wanted_h > 0 && image->height > 0)
    {
        double ratio = (double) wanted_h / (double) image->height;
        cairo_scale(cr, ratio, ratio);
        gdk_cairo_set_source_pixbuf(cr, image->data, x / ratio, y / ratio);
    }
    else
        gdk_cairo_set_source_pixbuf(cr, image->data, x, y);

    cairo_paint(cr);

    cairo_destroy(cr);
}

#else /* WITH_IMLIB2 */

static const char *
draw_imlib_load_strerror(Imlib_Load_Error e)
{
    switch(e)
    {
      case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
        return "no such file or directory";
      case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
        return "file is a directory";
      case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
        return "read permission denied";
      case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
        return "no loader for file format";
      case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
        return "path too long";
      case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
        return "path component non existant";
      case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
        return "path compoment not a directory";
      case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
        return "path points oustide address space";
      case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
        return "too many symbolic links";
      case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
        return "out of memory";
      case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
        return "out of file descriptors";
      case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
        return "write permission denied";
      case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
        return "out of disk space";
      case IMLIB_LOAD_ERROR_UNKNOWN:
        return "unknown error, that's really bad";
      case IMLIB_LOAD_ERROR_NONE:
        return "no error, oops";
    }

    return "unknown error";
}


/** Load an image from filename.
 * \param filename The image file to load.
 * \return A new image.
 */
draw_image_t *
draw_image_new(const char *filename)
{
    int w, h, size, i;
    DATA32 *data;
    double alpha;
    unsigned char *dataimg, *rdataimg;
    Imlib_Image imimage;
    Imlib_Load_Error e = IMLIB_LOAD_ERROR_NONE;
    draw_image_t *image;

    if(!filename)
        return NULL;

    if(!(imimage = imlib_load_image_with_error_return(filename, &e)))
    {
        warn("cannot load image %s: %s", filename, draw_imlib_load_strerror(e));
        return NULL;
    }

    imlib_context_set_image(imimage);

    w = imlib_image_get_width();
    h = imlib_image_get_height();

    size = w * h;

    data = imlib_image_get_data_for_reading_only();

    rdataimg = dataimg = p_new(unsigned char, size * 4);

    for(i = 0; i < size; i++, dataimg += 4)
    {
        dataimg[3] = (data[i] >> 24) & 0xff;           /* A */
        /* cairo wants pre-multiplied alpha */
        alpha = dataimg[3] / 255.0;
        dataimg[2] = ((data[i] >> 16) & 0xff) * alpha; /* R */
        dataimg[1] = ((data[i] >>  8) & 0xff) * alpha; /* G */
        dataimg[0] = (data[i]         & 0xff) * alpha; /* B */
    }

    image = p_new(draw_image_t, 1);

    image->data = rdataimg;
    image->width = w;
    image->height = h;

    imlib_free_image();

    return image;
}

/** Delete an image.
 * \param image The image to delete.
 */
void
draw_image_delete(draw_image_t **image)
{
    if (*image)
    {
        p_delete(&(*image)->data);
        p_delete(image);
    }
}

/** Draw an image to a draw context.
 * \param ctx Draw context to draw to.
 * \param x X coordinate.
 * \param y Y coordinate.
 * \param wanted_h Wanted height: if > 0, image will be resized.
 * \param image The image to draw.
 */
void
draw_image(draw_context_t *ctx, int x, int y, int wanted_h, draw_image_t *image)
{
    draw_image_from_argb_data(ctx, x, y, image->width, image->height, wanted_h, image->data);
}

#endif /* WITH_IMLIB2 */

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

    surface = cairo_xcb_surface_create(ctx->connection, dest,
                                       ctx->visual, dest_w, dest_h);
    source = cairo_xcb_surface_create(ctx->connection, src,
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
 * \param conn Connection ref.
 * \param phys_screen Physical screen number.
 * \param font Font to use.
 * \param text The text.
 * \param pdata The parser data to fill.
 * \return Text height and width.
 */
area_t
draw_text_extents(xcb_connection_t *conn, int phys_screen, font_t *font,
                  const char *text, draw_parser_data_t *parser_data)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    PangoLayout *layout;
    PangoRectangle ext;
    xcb_screen_t *s = xutil_screen_get(conn, phys_screen);
    area_t geom = { 0, 0, 0, 0 };
    ssize_t len;

    if(!(len = a_strlen(text)))
        return geom;

    parser_data->connection = conn;
    parser_data->phys_screen = phys_screen;
    if(draw_text_markup_expand(parser_data, text, len))
    {
        text = parser_data->text.s;
        len  = parser_data->text.len;
    }

    surface = cairo_xcb_surface_create(conn, phys_screen,
                                       draw_screen_default_visual(s),
                                       s->width_in_pixels,
                                       s->height_in_pixels);

    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);
    pango_layout_set_markup(layout, text, len);
    pango_layout_set_font_description(layout, font->desc);
    pango_layout_get_pixel_extents(layout, NULL, &ext);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    geom.width = ext.width;
    geom.height = ext.height * 1.5;

    return geom;
}

/** Transform a string to a alignment_t type.
 * Recognized string are left, center or right. Everything else will be
 * recognized as AlignAuto.
 * \param align Atring with align text.
 * \param len The string length.
 * \return An alignment_t type.
 */
alignment_t
draw_align_fromstr(const char *align, ssize_t len)
{
    switch (a_tokenize(align, len))
    {
      case A_TK_LEFT:   return AlignLeft;
      case A_TK_CENTER: return AlignCenter;
      case A_TK_RIGHT:  return AlignRight;
      case A_TK_FLEX:   return AlignFlex;
      default:          return AlignAuto;
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
      case AlignAuto:   return "auto";
      default:          return NULL;
    }
}

#define RGB_COLOR_8_TO_16(i) (65535 * ((i) & 0xff) / 255)

/** Initialize an X color.
 * \param color xcolor_t struct to store color into.
 * \param conn Connection ref.
 * \param phys_screen Physical screen number.
 * \param colstr Color specification.
 * \return True if color allocation was successfull.
 */
bool
xcolor_init(xcolor_t *color, xcb_connection_t *conn, int phys_screen,
            const char *colstr)
{
    xcb_screen_t *s = xutil_screen_get(conn, phys_screen);
    unsigned long colnum;
    uint16_t red, green, blue, alpha = 0xffff;
    ssize_t len;

    if(!(len = a_strlen(colstr)))
        return false;

    /* The color is given in RGB value */
    if(colstr[0] == '#')
    {
        xcb_alloc_color_cookie_t cookie;
        xcb_alloc_color_reply_t *hexa_color;
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
            alpha = RGB_COLOR_8_TO_16(colnum);
            colnum >>= 8;
        }
        else
        {
          invalid:
            warn("awesome: error, invalid color '%s'", colstr);
            return false;
        }

        red   = RGB_COLOR_8_TO_16(colnum >> 16);
        green = RGB_COLOR_8_TO_16(colnum >> 8);
        blue  = RGB_COLOR_8_TO_16(colnum);

        cookie = xcb_alloc_color_unchecked(conn, s->default_colormap,
                                           red, green, blue),
        hexa_color = xcb_alloc_color_reply(conn, cookie, NULL);

        if(hexa_color)
        {
            color->pixel = hexa_color->pixel;
            color->red   = hexa_color->red;
            color->green = hexa_color->green;
            color->blue  = hexa_color->blue;
            color->alpha = alpha;
            color->initialized = true;
            p_delete(&hexa_color);
            return true;
        }
    }
    else
    {
        xcb_alloc_named_color_reply_t *named_color = NULL;
        xcb_alloc_named_color_cookie_t cookie;

        cookie = xcb_alloc_named_color_unchecked(conn, s->default_colormap, len,
                                                 colstr),
        named_color = xcb_alloc_named_color_reply(conn, cookie, NULL);

        if(named_color)
        {
            color->pixel = named_color->pixel;
            color->red   = named_color->visual_red;
            color->green = named_color->visual_green;
            color->blue  = named_color->visual_blue;
            color->alpha = 0xffff;
            color->alpha = alpha;
            color->initialized = true;
            p_delete(&named_color);
            return true;
        }
    }

    warn("awesome: error, cannot allocate color '%s'", colstr);
    return false;
}

/** Remove a area from a list of them,
 * spliting the space between several area that can overlap
 * \param areas Array of areas.
 * \param elem Area to remove.
 */
void
area_array_remove(area_array_t *areas, area_t elem)
{
    /* loop from the end because:
     *  (1) we remove elements ;
     *  (2) the one we add to the end are okay wrt the invariants
     */
    for(int i = areas->len - 1; i >= 0; i--)
        if(area_intersect_area(areas->tab[i], elem))
        {
            /* remove it from the list */
            area_t r = area_array_take(areas, i);
            area_t inter = area_get_intersect_area(r, elem);

            if(AREA_LEFT(inter) > AREA_LEFT(r))
            {
                area_t extra =
                {
                    .x = r.x,
                    .y = r.y,
                    .width = AREA_LEFT(inter) - AREA_LEFT(r),
                    .height = r.height,
                };
                area_array_append(areas, extra);
            }

            if(AREA_TOP(inter) > AREA_TOP(r))
            {
                area_t extra =
                {
                    .x = r.x,
                    .y = r.y,
                    .width = r.width,
                    .height = AREA_TOP(inter) - AREA_LEFT(r),
                };
                area_array_append(areas, extra);
            }

            if(AREA_RIGHT(inter) < AREA_RIGHT(r))
            {
                area_t extra =
                {
                    .x = AREA_RIGHT(inter),
                    .y = r.y,
                    .width = AREA_RIGHT(r) - AREA_RIGHT(inter),
                    .height = r.height,
                };
                area_array_append(areas, extra);
            }

            if(AREA_BOTTOM(inter) < AREA_BOTTOM(r))
            {
                area_t extra =
                {
                    .x = r.x,
                    .y = AREA_BOTTOM(inter),
                    .width = r.width,
                    .height = AREA_BOTTOM(r) - AREA_BOTTOM(inter),
                };
                area_array_append(areas, extra);
            }
        }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
