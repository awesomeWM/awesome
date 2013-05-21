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

#include <cairo-xlib.h>
#ifdef HAVE_GTK
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#else
#include <Imlib2.h>
#endif

#include <langinfo.h>
#include <iconv.h>
#include <errno.h>

#include <math.h>

#include "draw.h"
#include "common/util.h"

/** Convert text from any charset to UTF-8 using iconv
 * \param iso the ISO string to convert
 * \return NULL if error, otherwise pointer to the new converted string
 */
static char *
draw_iso2utf8(char *iso)
{
    iconv_t iso2utf8;
    size_t len, utf8len;
    char *utf8, *utf8p;

    if(!(len = a_strlen(iso)))
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
            warn("unable to convert text: %s\n", strerror(errno));

        return NULL;
    }

    utf8len = 2 * len + 1;
    utf8 = utf8p = p_new(char, utf8len);

    if(iconv(iso2utf8, &iso, &len, &utf8, &utf8len) == (size_t) -1)
    {
        warn("text conversion failed: %s\n", strerror(errno));
        p_delete(&utf8p);
    }

    if(iconv_close(iso2utf8))
        warn("error closing iconv");

    return utf8p;
}

/** Get a draw context
 * \param disp Display ref
 * \param phys_screen physical screen id
 * \param width width
 * \param height height
 * \param dw Drawable object to store in DrawCtx
 * \return draw context ref
 */
DrawCtx *
draw_context_new(Display *disp, int phys_screen, int width, int height, Drawable dw)
{
    DrawCtx *d = p_new(DrawCtx, 1);

    d->display = disp;
    d->phys_screen = phys_screen;
    d->width = width;
    d->height = height;
    d->depth = DefaultDepth(disp, phys_screen);
    d->visual = DefaultVisual(disp, phys_screen);
    d->drawable = dw;
    d->surface = cairo_xlib_surface_create(disp, dw, d->visual, width, height);
    d->cr = cairo_create(d->surface);
    d->layout = pango_cairo_create_layout(d->cr);

    return d;
};

/** Delete a draw context
 * \param ctx DrawCtx to delete
 */
void
draw_context_delete(DrawCtx **ctx)
{
    g_object_unref((*ctx)->layout);
    cairo_surface_destroy((*ctx)->surface);
    cairo_destroy((*ctx)->cr);
    p_delete(ctx);
}

/** Create a new Pango font
 * \param disp Display ref
 * \param fontname Pango fontname (e.g. [FAMILY-LIST] [STYLE-OPTIONS] [SIZE])
 * \return a new font
 */
font_t *
draw_font_new(Display *disp, char *fontname)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    PangoLayout *layout;
    font_t *font = p_new(font_t, 1);

    /* Create a dummy cairo surface, cairo context and pango layout in
     * order to get font informations */
    surface = cairo_xlib_surface_create(disp,
                                        DefaultScreen(disp),
                                        DefaultVisual(disp, DefaultScreen(disp)),
                                        DisplayWidth(disp, DefaultScreen(disp)),
                                        DisplayHeight(disp, DefaultScreen(disp)));

    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);

    /* Get the font description used to set text on a PangoLayout */
    font->desc = pango_font_description_from_string(fontname);
    pango_layout_set_font_description(layout, font->desc);

    /* Get height */
    pango_layout_get_pixel_size(layout, NULL, &font->height);

    /* At the moment, we don't need ascent/descent but maybe it could
     * be useful in the future... */
#if 0
    PangoContext *context;
    PangoFontMetrics *font_metrics;

    /* Get ascent and descent */
    context = pango_layout_get_context(layout);
    font_metrics = pango_context_get_metrics(context, font->desc, NULL);

    /* Values in PangoFontMetrics are given in Pango units */
    font->ascent = PANGO_PIXELS(pango_font_metrics_get_ascent(font_metrics));
    font->descent = PANGO_PIXELS(pango_font_metrics_get_descent(font_metrics));

    pango_font_metrics_unref(font_metrics);
#endif

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return font;
}

/** Delete a font
 * \param font font_t to delete
 */
void
draw_font_delete(font_t **font)
{
    pango_font_description_free((*font)->desc);
    p_delete(font);
}

/** Draw text into a draw context
 * \param ctx DrawCtx to draw to
 * \param area area to draw to
 * \param align alignment
 * \param padding padding to add before drawing the text
 * \param font font to use
 * \param text text to draw
 * \param enable shadow
 * \param fg foreground color
 * \param bg background color
 */
void
draw_text(DrawCtx *ctx,
          area_t area,
          Alignment align,
          int padding,
          char *text,
          style_t style)
{
    int nw = 0, x, y;
    ssize_t len, olen;
    char *buf = NULL, *utf8 = NULL;

    draw_rectangle(ctx, area, 1.0, True, style.bg);

    if(!(len = olen = a_strlen(text)))
        return;

    /* try to convert it to UTF-8 */
    if((utf8 = draw_iso2utf8(text)))
    {
        buf = utf8;
        len = olen = a_strlen(buf);
    }
    else
        buf = a_strdup(text);

    /* check that the text is not too long */
    while(len && (nw = (draw_textwidth(ctx->display, style.font, buf)) + padding * 2) > area.width)
    {
        len--;
        /* we can't blindly null the char, we need to check if it's not part of
         * a multi byte char: if mbtowc return -1, we know that we must go back
         * in the string to find the beginning of the multi byte char */
        while(len && mbtowc(NULL, buf + len, a_strlen(buf + len)) < 0)
            len--;
        buf[len] = '\0';
    }
    if(nw > area.width)
        return;                 /* too long */
    if(len < olen)
    {
        if(len > 1)
            buf[len - 1] = '.';
        if(len > 2)
            buf[len - 2] = '.';
        if(len > 3)
            buf[len - 3] = '.';
    }

    pango_layout_set_text(ctx->layout, buf, -1);
    pango_layout_set_font_description(ctx->layout, style.font->desc);

    x = area.x + padding;
    /* + 1 is added for rounding, so that in any case of doubt we rather draw
     * the text 1px lower than too high which usually results in a better type
     * face */
    y = area.y + (ctx->height - style.font->height + 1) / 2;

    switch(align)
    {
      case AlignCenter:
        x += (area.width - nw) / 2;
        break;
      case AlignRight:
        x += area.width - nw;
        break;
      default:
        break;
    }

    if(style.shadow_offset > 0)
    {
        cairo_set_source_rgb(ctx->cr,
                             style.shadow.red / 65535.0,
                             style.shadow.green / 65535.0,
                             style.shadow.blue / 65535.0);
        cairo_move_to(ctx->cr, x + style.shadow_offset, y + style.shadow_offset);
        pango_cairo_update_layout(ctx->cr, ctx->layout);
        pango_cairo_show_layout(ctx->cr, ctx->layout);
    }

    cairo_set_source_rgb(ctx->cr,
                         style.fg.red / 65535.0,
                         style.fg.green / 65535.0,
                         style.fg.blue / 65535.0);
    cairo_move_to(ctx->cr, x, y);
    pango_cairo_update_layout(ctx->cr, ctx->layout);
    pango_cairo_show_layout(ctx->cr, ctx->layout);

    p_delete(&buf);
}

/** Setup color-source for cairo (gradient or mono)
 * \param ctx Draw context
 * \param rect x,y to x+x_offset,y+y_offset
 * \param color color to use at start (x,y)
 * \param pcolor_center color at 50% of width
 * \param pcolor_end color at pattern end (x + x_offset, y + y_offset)
 * \return pat pattern or NULL; needs to get cairo_pattern_destroy()'ed;
 */
static cairo_pattern_t *
draw_setup_cairo_color_source(DrawCtx *ctx, area_t rect,
                              XColor *pcolor, XColor *pcolor_center,
                              XColor *pcolor_end)
{
    cairo_pattern_t *pat = NULL;

    /* no need for a real pattern: */
    if(!pcolor_end && !pcolor_center)
        cairo_set_source_rgb(ctx->cr, pcolor->red / 65535.0, pcolor->green / 65535.0, pcolor->blue / 65535.0);
    else
    {
        pat = cairo_pattern_create_linear(rect.x, rect.y, rect.x + rect.width, rect.y + rect.height);

        /* pcolor is always set (so far in awesome) */
        cairo_pattern_add_color_stop_rgb(pat, 0.0, pcolor->red / 65535.0,
                                             pcolor->green / 65535.0, pcolor->blue / 65535.0);

        if(pcolor_center)
            cairo_pattern_add_color_stop_rgb(pat, 0.5, pcolor_center->red / 65535.0,
                                             pcolor_center->green / 65535.0, pcolor_center->blue / 65535.0);

        if(pcolor_end)
            cairo_pattern_add_color_stop_rgb(pat, 1.0, pcolor_end->red / 65535.0,
                                             pcolor_end->green / 65535.0, pcolor_end->blue / 65535.0);
        else
            cairo_pattern_add_color_stop_rgb(pat, 1.0, pcolor->red / 65535.0,
                                             pcolor->green / 65535.0, pcolor->blue / 65535.0);
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
draw_rectangle(DrawCtx *ctx, area_t geometry, float line_width, Bool filled, XColor color)
{
    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, line_width);
    cairo_set_miter_limit(ctx->cr, 10.0);
    cairo_set_line_join(ctx->cr, CAIRO_LINE_JOIN_MITER);
    cairo_set_source_rgb(ctx->cr,
                         color.red / 65535.0,
                         color.green / 65535.0,
                         color.blue / 65535.0);
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
 * \param pattern__x pattern start x coord
 * \param pattern_width pattern width
 * \param color color to use at start
 * \param pcolor_center color at 50% of width
 * \param pcolor_end color at pattern_start + pattern_width
 */
void
draw_rectangle_gradient(DrawCtx *ctx, area_t geometry, float line_width, Bool filled,
                        area_t pattern_rect, XColor *pcolor,
                        XColor *pcolor_center, XColor *pcolor_end)
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
draw_graph_setup(DrawCtx *ctx)
{
    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, 1.0);
    /* without it, it can draw over the path on sharp angles
     * ...too long lines * (...graph_line) */
    cairo_set_miter_limit(ctx->cr, 0.0);
    cairo_set_line_join(ctx->cr, CAIRO_LINE_JOIN_MITER);
}

/** Draw a graph
 * \param ctx Draw context
 * \param x x-offset of widget
 * \param y y-offset of widget
 * \param w width in pixels
 * \param from array of starting-point offsets to draw a graph-lines
 * \param to array of end-point offsets to draw a graph-lines
 * \param cur_index current position in data-array (cycles around)
 * \param grow put new values to the left or to the right
 * \param pcolor color at the left
 * \param pcolor_center color in the center
 * \param pcolor_end color at the right
 */
void
draw_graph(DrawCtx *ctx, area_t rect, int *from, int *to, int cur_index,
           Position grow, area_t patt_rect,
           XColor *pcolor, XColor *pcolor_center, XColor *pcolor_end)
{
    int i, y, w;
    float x;
    cairo_pattern_t *pat;

    pat = draw_setup_cairo_color_source(ctx, patt_rect,
                                        pcolor, pcolor_center, pcolor_end);

    /* middle of a pixel */
    x = rect.x + 0.5;
    y = rect.y;
    w = rect.width;

    i = -1;
    if(grow == Right) /* draw from right to left */
    {
        x += w - 1;
        while(++i < w)
        {
            cairo_move_to(ctx->cr, x, y - from[cur_index]);
            cairo_line_to(ctx->cr, x, y - to[cur_index]);
            x -= 1.0;

            if (--cur_index < 0)
                cur_index = w - 1;
        }
    }
    else /* draw from left to right */
    {
        while(++i < w)
        {
            cairo_move_to(ctx->cr, x, y - from[cur_index]);
            cairo_line_to(ctx->cr, x, y - to[cur_index]);
            x += 1.0;

            if (--cur_index < 0)
                cur_index = w - 1;
        }
    }

    cairo_stroke(ctx->cr);

    if(pat)
        cairo_pattern_destroy(pat);
}

/** Draw a line into a graph-widget
 * \param ctx Draw context
 * \param x x-offset of widget
 * \param y y-offset of widget
 * \param w width in pixels
 * \param to array of offsets to draw the line through...
 * \param cur_index current position in data-array (cycles around)
 * \param grow put new values to the left or to the right
 * \param pcolor color at the left
 * \param pcolor_center color in the center
 * \param pcolor_end color at the right
 */
void
draw_graph_line(DrawCtx *ctx, area_t rect, int *to, int cur_index,
                Position grow, area_t patt_rect,
                XColor *pcolor, XColor *pcolor_center, XColor *pcolor_end)
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

/** Draw a circle
 * \param ctx Draw context to draw to
 * \param x x coordinate
 * \param y y coordinate
 * \param r size of the circle
 * \param filled fill circle?
 * \param color color to use
 */
void
draw_circle(DrawCtx *ctx, int x, int y, int r, Bool filled, XColor color)
{
    cairo_set_line_width(ctx->cr, 1.0);
    cairo_set_source_rgb(ctx->cr, color.red / 65535.0, color.green / 65535.0, color.blue / 65535.0);

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
 * \param ctx Draw context to draw to
 * \param x x coordinate
 * \param y y coordinate
 * \param w width
 * \param h height
 * \param wanted_h wanted height: if > 0, image will be resized
 * \param data the image pixels array
 */
void draw_image_from_argb_data(DrawCtx *ctx, int x, int y, int w, int h,
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

#ifdef HAVE_GTK

/** Draw an image (PNG format only) from a file to a draw context
 * \param ctx Draw context to draw to
 * \param x x coordinate
 * \param y y coordinate
 * \param wanted_h wanted height: if > 0, image will be resized
 * \param filename file name to draw
 */
void
draw_image(DrawCtx *ctx, int x, int y, int wanted_h, const char *filename)
{

    double ratio;
    int w, h;
    cairo_t *cr;
    GdkPixbuf *pixbuf;
    GError *error=NULL;

    if(!(pixbuf = gdk_pixbuf_new_from_file(filename,&error)))
        return warn("cannot load image %s: %s\n", filename, error->message);

    w = gdk_pixbuf_get_width(pixbuf);
    h = gdk_pixbuf_get_height(pixbuf);

    cr = cairo_create(ctx->surface);
    if(wanted_h > 0 && h > 0)
    {
        ratio = (double) wanted_h / (double) h;
        cairo_scale(cr, ratio, ratio);
        gdk_cairo_set_source_pixbuf(cr, pixbuf, x/ratio, y/ratio);
    }
    else
        gdk_cairo_set_source_pixbuf(cr, pixbuf, (double) x, (double) y);

    cairo_paint(cr);

    gdk_pixbuf_unref(pixbuf);

    cairo_destroy(cr);
}

/** get an image size
 * \param filename file name
 * \return area_t structure with width and height set to image size
 */
area_t
draw_get_image_size(const char *filename)
{
    area_t size = { -1, -1, -1, -1, NULL, NULL };
    gint width, height;

    if(gdk_pixbuf_get_file_info(filename, &width, &height))
    {
        size.width = width;
        size.height = height;
    }
    else
        warn("cannot load image %s: %s\n", filename, "format unrecognized");

    return size;
}

#else /* HAVE_GTK */

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

/** Draw an image (PNG format only) from a file to a draw context
 * \param ctx Draw context to draw to
 * \param x x coordinate
 * \param y y coordinate
 * \param wanted_h wanted height: if > 0, image will be resized
 * \param filename file name to draw
 */
void
draw_image(DrawCtx *ctx, int x, int y, int wanted_h, const char *filename)
{
    int w, h, size, i;
    DATA32 *data;
    double alpha;
    unsigned char *dataimg, *rdataimg;
    Imlib_Image image;
    Imlib_Load_Error e = IMLIB_LOAD_ERROR_NONE;

    if (!filename) {
        warn("cannot load image");
        return;
    }
    if(!(image = imlib_load_image_with_error_return(filename, &e))) {
        warn("cannot load image %s: %s\n", filename, draw_imlib_load_strerror(e));
        return;
    }

    imlib_context_set_image(image);
    h = imlib_image_get_height();
    w = imlib_image_get_width();

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

    draw_image_from_argb_data(ctx, x, y, w, h, wanted_h, rdataimg);

    imlib_free_image();
    p_delete(&rdataimg);
}

/** Get an image size
 * \param filename file name
 * \return area_t structure with width and height set to image size
 */
area_t
draw_get_image_size(const char *filename)
{
    area_t size = { -1, -1, -1, -1, NULL, NULL };
    Imlib_Image image;
    Imlib_Load_Error e = IMLIB_LOAD_ERROR_NONE;

	 if (!filename)
		 return size;

    if((image = imlib_load_image_with_error_return(filename, &e)))
    {
        imlib_context_set_image(image);

        size.width = imlib_image_get_width();
        size.height = imlib_image_get_height();

        imlib_free_image();
    }
    else
        warn("cannot load image %s: %s\n", filename, draw_imlib_load_strerror(e));

    return size;
}
#endif /* HAVE_GTK */

/** Rotate a drawable
 * \param ctx Draw context to draw to
 * \param dest Drawable to draw the result
 * \param dest_w Drawable width
 * \param dest_h Drawable height
 * \param angle angle to rotate
 * \param tx translate to this x coordinate
 * \param ty translate to this y coordinate
 * \return new rotated drawable
 */
void
draw_rotate(DrawCtx *ctx, Drawable dest, int dest_w, int dest_h,
            double angle, int tx, int ty)
{
    cairo_surface_t *surface, *source;
    cairo_t *cr;

    surface = cairo_xlib_surface_create(ctx->display, dest,
                                        ctx->visual, dest_w, dest_h);
    source = cairo_xlib_surface_create(ctx->display, ctx->drawable,
                                       ctx->visual, ctx->width, ctx->height);
    cr = cairo_create (surface);

    cairo_translate(cr, tx, ty);
    cairo_rotate(cr, angle);

    cairo_set_source_surface(cr, source, 0.0, 0.0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
    cairo_surface_destroy(surface);
}

/** Return the width of a text in pixel
 * \param disp Display ref
 * \param font font to use
 * \param text the text
 * \return text width
 */
unsigned short
draw_textwidth(Display *disp, font_t *font, const char *text)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    PangoLayout *layout;
    PangoRectangle ext;

    if(!a_strlen(text))
        return 0;

    surface = cairo_xlib_surface_create(disp, DefaultScreen(disp),
                                        DefaultVisual(disp, DefaultScreen(disp)),
                                        DisplayWidth(disp, DefaultScreen(disp)),
                                        DisplayHeight(disp, DefaultScreen(disp)));
    cr = cairo_create(surface);
    layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_font_description(layout, font->desc);
    pango_layout_get_pixel_extents(layout, NULL, &ext);
    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return ext.width;
}

/** Transform a string to a Alignment type.
 * Recognized string are left, center or right. Everything else will be
 * recognized as AlignAuto.
 * \param align string with align text
 * \return Alignment type
 */
Alignment
draw_align_get_from_str(const char *align)
{
    if(!a_strcmp(align, "left"))
        return AlignLeft;
    else if(!a_strcmp(align, "center"))
        return AlignCenter;
    else if(!a_strcmp(align, "right"))
        return AlignRight;
    else if(!a_strcmp(align, "flex"))
        return AlignFlex;

    return AlignAuto;
}

/** Initialize an X color
 * \param disp display ref
 * \param phys_screen Physical screen number
 * \param colstr Color specification
 * \param color XColor struct to store color to
 * \return true if color allocation was successfull
 */
Bool
draw_color_new(Display *disp, int phys_screen, const char *colstr, XColor *color)
{
    Bool ret;
    XColor exactColor;

    if(!a_strlen(colstr))
        return False;

    if(!(ret = XAllocNamedColor(disp,
                                DefaultColormap(disp, phys_screen),
                                colstr,
                                color,
                                &exactColor)))
        warn("awesome: error, cannot allocate color '%s'\n", colstr);

    return ret;
}

/** Init a style struct. Every value will be inherited from m
 * if they are not set in the configuration section cfg.
 * \param disp Display ref
 * \param phys_screen Physical screen number
 * \param cfg style configuration section
 * \param c style to fill
 * \param m style to use as template
 */
void
draw_style_init(Display *disp, int phys_screen, cfg_t *cfg,
                style_t *c, style_t *m)
{
    char *buf;
    int shadow;

    if(m)
        *c = *m;

    if(!cfg)
        return;

    if((buf = cfg_getstr(cfg, "font")))
        c->font = draw_font_new(disp, buf);

    draw_color_new(disp, phys_screen,
                   cfg_getstr(cfg, "fg"), &c->fg);

    draw_color_new(disp, phys_screen,
                   cfg_getstr(cfg, "bg"), &c->bg);

    draw_color_new(disp, phys_screen,
                   cfg_getstr(cfg, "border"), &c->border);

    draw_color_new(disp, phys_screen,
                   cfg_getstr(cfg, "shadow"), &c->shadow);

    if((shadow = cfg_getint(cfg, "shadow_offset")) != (int) 0xffffffff)
        c->shadow_offset = shadow;
    else if(!m)
        c->shadow_offset = 0;
}

/** Remove a area from a list of them,
 * spliting the space between several area that can overlap
 * \param head list head
 * \param elem area to remove
 */
void
area_list_remove(area_t **head, area_t *elem)
{
    area_t *r, inter, *extra, *rnext;

    for(r = *head; r; r = rnext)
    {
        rnext = r->next;
        if(area_intersect_area(*r, *elem))
        {
            /* remove it from the list */
            area_list_detach(head, r);

            inter = area_get_intersect_area(*r, *elem);

            if(AREA_LEFT(inter) > AREA_LEFT(*r))
            {
                extra = p_new(area_t, 1);
                extra->x = r->x;
                extra->y = r->y;
                extra->width = AREA_LEFT(inter) - r->x;
                extra->height = r->height;
                area_list_append(head, extra);
            }

            if(AREA_TOP(inter) > AREA_TOP(*r))
            {
                extra = p_new(area_t, 1);
                extra->x = r->x;
                extra->y = r->y;
                extra->width = r->width;
                extra->height = AREA_TOP(inter) - r->y;
                area_list_append(head, extra);
            }

            if(AREA_RIGHT(inter) < AREA_RIGHT(*r))
            {
                extra = p_new(area_t, 1);
                extra->x = AREA_RIGHT(inter);
                extra->y = r->y;
                extra->width = AREA_RIGHT(*r) - AREA_RIGHT(inter);
                extra->height = r->height;
                area_list_append(head, extra);
            }

            if(AREA_BOTTOM(inter) < AREA_BOTTOM(*r))
            {
                extra = p_new(area_t, 1);
                extra->x = r->x;
                extra->y = AREA_BOTTOM(inter);
                extra->width = r->width;
                extra->height = AREA_BOTTOM(*r) - AREA_BOTTOM(inter);
                area_list_append(head, extra);
            }

            /* delete the elem since we removed it from the list */
            p_delete(&r);
        }
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
