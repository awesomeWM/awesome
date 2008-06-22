/*
 * markup.c - markup functions
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

/* asprintf */
#define _GNU_SOURCE

#include <glib.h>

#include <stdio.h>

#include "common/markup.h"

/** Callback to invoke when the opening tag of an element is seen.
 * Here is just copies element elements we do not care about, and copies
 * values we care.
 * \param context the context of GMarkup
 * \param element_name element name
 * \param attribute_names array of attribute names
 * \param attribute_values array of attribute values
 * \param user_data pointer to user data, here a markup_parser_data_t pointer
 * \param error a GError
 */
static void
markup_parse_start_element(GMarkupParseContext *context __attribute__ ((unused)),
                           const gchar *element_name,
                           const gchar **attribute_names,
                           const gchar **attribute_values,
                           gpointer user_data,
                           GError **error __attribute__ ((unused)))
{
    markup_parser_data_t *p = (markup_parser_data_t *) user_data;
    int i, j;

    for(i = 0; p->elements[i]; i++)
        if(!a_strcmp(element_name, p->elements[i]))
        {
            for(j = 0; attribute_names[j]; j++);
            p->attribute_names[i] = p_new(char *, ++j);
            p->attribute_values[i] = p_new(char *, j);

            for(j = 0; attribute_names[j]; j++)
            {
                p->attribute_names[i][j] = a_strdup(attribute_names[j]);
                p->attribute_values[i][j] = a_strdup(attribute_values[j]);
            }

            if(p->elements_sub && p->elements_sub[i])
            {
                buffer_add_xmlescaped(&p->text, p->elements_sub[i]);
            }

            return;
        }

    if(a_strcmp(element_name, "markup"))
    {
        buffer_addf(&p->text, "<%s", element_name);
        for(i = 0; attribute_names[i]; i++)
        {
            buffer_addf(&p->text, " %s=\"%s\"", attribute_names[i],
                        attribute_values[i]);
        }
        buffer_addc(&p->text, '>');
    }
}

/** Callback to invoke when the closing tag of an element is seen. Note that
 * this is also called for empty tags like \<empty/\>.
 * Here is just copies element elements we do not care about.
 * \param context the context of GMarkup
 * \param element_name element name
 * \param user_data pointer to user data, here a markup_parser_data_t pointer
 * \param error a GError
 */
static void
markup_parse_end_element(GMarkupParseContext *context __attribute__ ((unused)),
                         const gchar *element_name,
                         gpointer user_data,
                         GError **error __attribute__ ((unused)))
{
    markup_parser_data_t *p = (markup_parser_data_t *) user_data;
    int i;

    for(i = 0; p->elements[i]; i++)
        if(!a_strcmp(element_name, p->elements[i]))
            return;

    if(a_strcmp(element_name, "markup"))
        buffer_addf(&p->text, "</%s>", element_name);
}

/** Callback to invoke when some text is seen (text is always inside an
 * element). Note that the text of an element may be spread over multiple calls
 * of this function. If the G_MARKUP_TREAT_CDATA_AS_TEXT  flag is set, this
 * function is also called for the content of CDATA marked sections.
 * Here it recopies blindly the text in the text attribute of user_data.
 * \param context the context of GMarkup
 * \param text the text
 * \param text_len the text length
 * \param user_data pointer to user data, here a markup_parser_data_t pointer
 * \param error a GError
 */
static void
markup_parse_text(GMarkupParseContext *context __attribute__ ((unused)),
                  const gchar *text,
                  gsize text_len,
                  gpointer user_data,
                  GError **error __attribute__ ((unused)))
{
    markup_parser_data_t *p = (markup_parser_data_t *) user_data;

    buffer_grow(&p->text, text_len);
    buffer_add_xmlescaped(&p->text, text);
}

/** Create a markup_parser_data_t structure with elements list.
 * \param a pointer to an allocated markup_parser_data_t which must be wiped
 *         with markup_parser_data_wipe()
 * \param elements an array of elements to look for, NULL terminated
 * \param elements_sub an optional array of values to substitude to elements, NULL
 *        terminated, or NULL if not needed
 * \param nelem number of elements in the array (without NULL)
 */
void
markup_parser_data_init(markup_parser_data_t *p, const char **elements,
                        const char **elements_sub, ssize_t nelem)
{
    buffer_init(&p->text);
    p->elements = elements;
    p->elements_sub = elements_sub;
    p->attribute_names = p_new(char **, nelem);
    p->attribute_values = p_new(char **, nelem);
}

/** Wipe a markup_parser_data_t initialized with markup_parser_data_init.
 * \param p markup_parser_data_t address
 */
void
markup_parser_data_wipe(markup_parser_data_t *p)
{
    int i, j;

    for(i = 0; p->elements[i]; i++)
        if(p->attribute_names[i])
        {
            for(j = 0; p->attribute_names[i][j]; j++)
            {
                p_delete(&p->attribute_names[i][j]);
                p_delete(&p->attribute_values[i][j]);
            }
            p_delete(&p->attribute_names[i]);
            p_delete(&p->attribute_values[i]);
        }

    p_delete(&p->attribute_names);
    p_delete(&p->attribute_values);

    buffer_wipe(&p->text);
}

/** Parse markup defined in data on the string str.
 * \param data A markup_parser_data_t allocated by markup_parser_data_new().
 * \param str A string to parse markup from.
 * \param slen String length.
 * \return True if success, false otherwise.
 */
bool
markup_parse(markup_parser_data_t *data, const char *str, ssize_t slen)
{
    static GMarkupParser const parser =
    {
        /* start_element */
        markup_parse_start_element,
        /* end_element */
        markup_parse_end_element,
        /* text */
        markup_parse_text,
        /* passthrough */
        NULL,
        /* error */
        NULL
    };
    GMarkupParseContext *mkp_ctx;
    GError *error = NULL;

    if(slen <= 0)
        return false;

    mkp_ctx = g_markup_parse_context_new(&parser, 0, data, NULL);

    if(!g_markup_parse_context_parse(mkp_ctx, "<markup>", -1, &error)
       || !g_markup_parse_context_parse(mkp_ctx, str, slen, &error)
       || !g_markup_parse_context_parse(mkp_ctx, "</markup>", -1, &error)
       || !g_markup_parse_context_end_parse(mkp_ctx, &error))
    {
        warn("unable to parse text \"%s\": %s", str, error ? error->message : "unknown error");
        if(error)
            g_error_free(error);
        g_markup_parse_context_free(mkp_ctx);
        return false;
    }

    g_markup_parse_context_free(mkp_ctx);

    return true;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
