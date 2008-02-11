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

#include <cairo.h>
#include <cairo-ft.h>
#include <cairo-xlib.h>

#include <langinfo.h>
#include <iconv.h>
#include <errno.h>

#include <math.h>

#include "draw.h"
#include "common/util.h"

static char *
draw_iso2utf8(char *iso)
{
    iconv_t iso2utf8;
    size_t len, utf8len;
    char *utf8;

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
            perror("awesome: unable to convert text");

        return NULL;
    }

    utf8len = (3 * len) / 2 + 1;
    utf8 = p_new(char, utf8len);

    if(iconv(iso2utf8, &iso, &len, &utf8, &utf8len) == (size_t) -1)
    {
        perror("awesome: text conversion failed");
        p_delete(&utf8);
        return NULL;
    }

    if(iconv_close(iso2utf8))
        warn("error closing iconv");

    return utf8;
}

/** Get a draw context
 * \param phys_screen physical screen id
 * \param width width
 * \param height height
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

    return d;
};

void
draw_context_delete(DrawCtx *ctx)
{
    cairo_surface_destroy(ctx->surface);
    cairo_destroy(ctx->cr);
    p_delete(&ctx);
}

/** Draw text into a draw context
 * \param x x coord
 * \param y y coord
 * \param w width
 * \param h height
 * \param align alignment
 * \param padding padding to add before drawing the text
 * \param font font to use
 * \param text text to draw
 * \param fg foreground color
 * \param bg background color
 */
void
draw_text(DrawCtx *ctx,
          Area area,
          Alignment align,
          int padding,
          XftFont *font, char *text,
          XColor fg, XColor bg)
{
    int nw = 0;
    ssize_t len, olen;
    char *buf = NULL, *utf8 = NULL;
    cairo_font_face_t *font_face;

    draw_rectangle(ctx, area, True, bg);

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
    while(len && (nw = (draw_textwidth(ctx->display, font, buf)) + padding * 2) > area.width)
    {
        len--;
        /* we can't blindly null the char, we need to check if it's not part of
         * a multi byte char: if mbtowc return -1, we know that we must go back
         * in the string to find the beginning of the multi byte char */
        while(mbtowc(NULL, buf + len, a_strlen(buf + len)) < 0)
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

    font_face = cairo_ft_font_face_create_for_pattern(font->pattern);
    cairo_set_font_face(ctx->cr, font_face);
    cairo_set_font_size(ctx->cr, font->height);
    cairo_set_source_rgb(ctx->cr, fg.red / 65535.0, fg.green / 65535.0, fg.blue / 65535.0);

    switch(align)
    {
      case AlignLeft:
        cairo_move_to(ctx->cr, area.x + padding, area.y + font->ascent + (ctx->height - font->height) / 2);
        break;
      case AlignRight:
        cairo_move_to(ctx->cr, area.x + (area.width - nw) + padding,
                      area.y + font->ascent + (ctx->height - font->height) / 2);
        break;
      default:
        cairo_move_to(ctx->cr, area.x + ((area.width - nw) / 2) + padding,
                      area.y + font->ascent + (ctx->height - font->height) / 2);
        break;
    }

    cairo_show_text(ctx->cr, buf);

    cairo_font_face_destroy(font_face);

    p_delete(&buf);
}

/** Setup color-source for cairo (gradient or mono)
 * \param ctx Draw context
 * \param x x-offset of widget
 * \param y y-offset of widget
 * \param width width in pixels
 * \param color color to use from 0%
 * \param pcolor_center color at 50% of width
 * \param pcolor_end color at 100% of width
 * \return pat pattern or NULL; needs to get cairo_pattern_destroy()'ed;
 */
cairo_pattern_t *
setup_cairo_color_source(DrawCtx *ctx, int x, int y, int width,
                         XColor color, XColor *pcolor_center, XColor *pcolor_end)
{
    cairo_pattern_t *pat;

    if(pcolor_center || pcolor_end) /* draw a gradient */
    {
        pat = cairo_pattern_create_linear(x, y, x + width, y);

        cairo_pattern_add_color_stop_rgb(pat, 0, color.red / 65535.0,
                                         color.green / 65535.0, color.blue / 65535.0);
        if(pcolor_center)
            cairo_pattern_add_color_stop_rgb(pat, 0.5, pcolor_center->red / 65535.0,
                                             pcolor_center->green / 65535.0, pcolor_center->blue / 65535.0);
        if(pcolor_end)
            cairo_pattern_add_color_stop_rgb(pat, 1, pcolor_end->red / 65535.0,
                                             pcolor_end->green / 65535.0, pcolor_end->blue / 65535.0);
        else
            cairo_pattern_add_color_stop_rgb(pat, 1, color.red / 65535.0,
                                             color.green / 65535.0, color.blue / 65535.0);
        cairo_set_source(ctx->cr, pat);
        return pat;
    }
    else /* no gradient */
    {
        cairo_set_source_rgb(ctx->cr, color.red / 65535.0, color.green / 65535.0, color.blue / 65535.0);
        return NULL;
    }
}

/** Draw rectangle
 * \param ctx Draw context
 * \param geometry geometry
 * \param filled filled rectangle?
 * \param color color to use
 */
void
draw_rectangle(DrawCtx *ctx, Area geometry, Bool filled, XColor color)
{
    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, 1.0);
    cairo_set_source_rgb(ctx->cr, color.red / 65535.0, color.green / 65535.0, color.blue / 65535.0);
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
}
/** Draw rectangle with gradient colors
 * \param ctx Draw context
 * \param geometry geometry
 * \param fullwidth width of full bar in pixels
 * \param filled filled rectangle?
 * \param color color to use from 0%
 * \param pcolor_center color at 50%
 * \param pcolor_end color at 100%
 */
void
draw_rectangle_gradient(DrawCtx *ctx, Area geometry, int fullwidth, Bool filled,
                        XColor color, XColor *pcolor_center, XColor *pcolor_end)
{
    cairo_pattern_t *pat;

    cairo_set_antialias(ctx->cr, CAIRO_ANTIALIAS_NONE);
    cairo_set_line_width(ctx->cr, 1.0);

    pat = setup_cairo_color_source(ctx, geometry.x, geometry.y, fullwidth,
                                   color, pcolor_center, pcolor_end);

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
    /* without it, it can draw over the path on sharp angles (...too long lines) */
    cairo_set_line_join (ctx->cr, CAIRO_LINE_JOIN_ROUND);
}
/** Draw a graph
 * \param ctx Draw context
 * \param x x-offset of widget
 * \param y y-offset of widget
 * \param w width in pixels
 * \param from array of starting-point offsets to draw a graph-lines
 * \param to array of end-point offsets to draw a graph-lines
 * \param cur_index current position in data-array (cycles around)
 * \param color color to use from 0%
 * \param pcolor_center color at 50%
 * \param pcolor_end color at 100%
 */
void
draw_graph(DrawCtx *ctx, int x, int y, int w, int *from, int *to, int cur_index,
           XColor color, XColor *pcolor_center, XColor *pcolor_end)
{
    int i;
    cairo_pattern_t *pat;
    pat = setup_cairo_color_source(ctx, x, y, w, color, pcolor_center, pcolor_end);

    i = -1;
    while(++i < w)
    {
        cairo_move_to(ctx->cr, x, y - from[cur_index]);
        cairo_line_to(ctx->cr, x, y - to[cur_index]);
        x++;

        if (--cur_index < 0)
            cur_index = w - 1;
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
 * \param color color to use from 0%
 * \param pcolor_center color at 50%
 * \param pcolor_end color at 100%
 */
void
draw_graph_line(DrawCtx *ctx, int x, int y, int w, int *to, int cur_index,
           XColor color, XColor *pcolor_center, XColor *pcolor_end)
{
    int i;
    int flag = 0; /* used to prevent drawing a line from 0 to 0 values */
    cairo_pattern_t *pat;

    pat = setup_cairo_color_source(ctx, x, y, w, color, pcolor_center, pcolor_end);

    /* x-1 (on the border), paints *from* the last point (... not included itself) */
    /* makes sense when you assume there is already some line drawn to it. */
    cairo_move_to(ctx->cr, x - 1, y - to[cur_index]);

    for (i = 0; i < w; i++)
    {
        if (to[cur_index] > 0)
        {
            cairo_line_to(ctx->cr, x, y - to[cur_index]);
            flag = 1;
        }
        else
        {
            if(flag) /* only draw from values > 0 to 0-values */
            {
                cairo_line_to(ctx->cr, x, y);
                flag = 0;
            }
            else
                cairo_move_to(ctx->cr, x, y);
        }

        if (--cur_index < 0) /* cycles around the index */
            cur_index = w - 1;
        x++;
    }
    cairo_stroke(ctx->cr);

    if(pat)
        cairo_pattern_destroy(pat);
}

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

void draw_image_from_argb_data(DrawCtx *ctx, int x, int y, int w, int h,
                               int wanted_h, unsigned char *data)
{
    double ratio;
    cairo_t *cr;
    cairo_surface_t *source;

    source = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, w, h, 0);
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

    cairo_surface_destroy(source);
}

void
draw_image(DrawCtx *ctx, int x, int y, int wanted_h, const char *filename)
{
    double ratio;
    int h;
    cairo_surface_t *source;
    cairo_t *cr;
    cairo_status_t cairo_st;

    source = cairo_image_surface_create_from_png(filename);
    if((cairo_st = cairo_surface_status(source)))
    {
        warn("failed to draw image %s: %s\n", filename, cairo_status_to_string(cairo_st));
        return;
    }
    cr = cairo_create (ctx->surface);
    if(wanted_h > 0 && (h = cairo_image_surface_get_height(source)) > 0)
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

Area
draw_get_image_size(const char *filename)
{
    Area size = { -1, -1, -1, -1, NULL };
    cairo_surface_t *surface;
    cairo_status_t cairo_st;

    surface = cairo_image_surface_create_from_png(filename);
    if((cairo_st = cairo_surface_status(surface)))
        warn("failed to get image size %s: %s\n", filename, cairo_status_to_string(cairo_st));
    else
    {
        cairo_image_surface_get_width(surface);
        size.x = 0;
        size.y = 0;
        size.width = cairo_image_surface_get_width(surface);
        size.height = cairo_image_surface_get_height(surface);
        cairo_surface_destroy(surface);
    }

    return size;
}

Drawable
draw_rotate(DrawCtx *ctx, int screen, double angle, int tx, int ty)
{
    cairo_surface_t *surface, *source;
    cairo_t *cr;
    Drawable newdrawable;

    newdrawable = XCreatePixmap(ctx->display,
                                RootWindow(ctx->display, screen),
                                ctx->height, ctx->width,
                                ctx->depth);
    surface = cairo_xlib_surface_create(ctx->display, newdrawable, ctx->visual, ctx->height, ctx->width);
    source = cairo_xlib_surface_create(ctx->display, ctx->drawable, ctx->visual, ctx->width, ctx->height);
    cr = cairo_create (surface);

    cairo_translate(cr, tx, ty);
    cairo_rotate(cr, angle);

    cairo_set_source_surface(cr, source, 0.0, 0.0);
    cairo_paint(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(source);
    cairo_surface_destroy(surface);

    return newdrawable;
}

unsigned short
draw_textwidth(Display *disp, XftFont *font, char *text)
{
    cairo_surface_t *surface;
    cairo_t *cr;
    cairo_font_face_t *font_face;
    cairo_text_extents_t te;

    if (!a_strlen(text))
        return 0;

    surface = cairo_xlib_surface_create(disp, DefaultScreen(disp),
                                        DefaultVisual(disp, DefaultScreen(disp)),
                                        DisplayWidth(disp, DefaultScreen(disp)),
                                        DisplayHeight(disp, DefaultScreen(disp)));
    cr = cairo_create(surface);
    font_face = cairo_ft_font_face_create_for_pattern(font->pattern);
    cairo_set_font_face(cr, font_face);
    cairo_set_font_size(cr, font->height);
    cairo_text_extents(cr, text, &te);
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    cairo_font_face_destroy(font_face);

    return MAX(te.x_advance, te.width);
}

Alignment
draw_get_align(const char *align)
{
    if(!a_strncmp(align, "left", 4))
        return AlignLeft;
    else if(!a_strncmp(align, "center", 6))
        return AlignCenter;
    else if(!a_strncmp(align, "right", 5))
        return AlignRight;

    return AlignAuto;
}

/** Initialize an X color
 * \param disp display ref
 * \param screen Physical screen number
 * \param colstr Color specification
 * \return XColor struct
 */
XColor
draw_color_new(Display *disp, int phys_screen, const char *colstr)
{
    XColor screenColor, exactColor;

    if(!XAllocNamedColor(disp,
                         DefaultColormap(disp, phys_screen),
                         colstr,
                         &screenColor,
                         &exactColor))
        eprint("awesome: error, cannot allocate color '%s'\n", colstr);

    return screenColor;
}

/** Remove a area from a list of them,
 * spliting the space between several area that
 * can overlaps
 * \param head list head
 * \param elem area to remove
 */
void
area_list_remove(Area **head, Area *elem)
{
    Area *r, inter, *extra;

    area_list_detach(head, elem);

    for(r = *head; r; r = r->next)
        if(area_intersect_area(*r, *elem))
        {
            /* remove it from the list */
            area_list_detach(head, r);

            inter = area_get_intersect_area(*r, *elem);

            if(AREA_LEFT(inter) > AREA_LEFT(*r))
            {
                extra = p_new(Area, 1);
                extra->x = r->x;
                extra->y = r->y;
                extra->width = AREA_LEFT(inter) - r->x;
                extra->height = r->height;
                area_list_push(head, extra);
            }

            if(AREA_TOP(inter) > AREA_TOP(*r))
            {
                extra = p_new(Area, 1);
                extra->x = r->x;
                extra->y = r->y;
                extra->width = r->width;
                extra->height = AREA_TOP(inter) - r->y;
                area_list_push(head, extra);
            }

            if(AREA_RIGHT(inter) < AREA_RIGHT(*r))
            {
                extra = p_new(Area, 1);
                extra->x = AREA_RIGHT(inter);
                extra->y = r->y;
                extra->width = AREA_RIGHT(*r) - AREA_RIGHT(inter);
                extra->height = r->height;
                area_list_push(head, extra);
            }

            if(AREA_BOTTOM(inter) < AREA_BOTTOM(*r))
            {
                extra = p_new(Area, 1);
                extra->x = r->x;
                extra->y = AREA_BOTTOM(inter);
                extra->width = r->width;
                extra->height = AREA_BOTTOM(*r) - AREA_BOTTOM(inter);
                area_list_push(head, extra);
            }
        }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
