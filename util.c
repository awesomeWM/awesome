/*
 * util.c - useful functions
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
 * Copyright © 2006 Pierre Habouzit <madcoder@debian.org>
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

#include <stdarg.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

#include "util.h"

void
die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}

void
eprint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "awesome: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void
warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "awesome: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

double
compute_new_value_from_arg(const char *arg, double current_value)
{
    double delta;

    if(arg && sscanf(arg, "%lf", &delta) == 1)
    {
        if(arg[0] == '+' || arg[0] == '-')
            current_value += delta;
        else
            current_value = delta;
    }

    return current_value;
}


/** Lookup for a function pointer from its name
 * in the given NameFuncLink list
 * \param funcname Function name
 * \param list Function and name link list
 * \return function pointer
 */
void *
name_func_lookup(const char *funcname, const NameFuncLink * list)
{
    int i;

    if(funcname && list)
        for(i = 0; list[i].name; i++)
            if(!a_strcmp(funcname, list[i].name))
                return list[i].func;

    return NULL;
}

/** \brief safe limited strcpy.
 *
 * Copies at most min(<tt>n-1</tt>, \c l) characters from \c src into \c dst,
 * always adding a final \c \\0 in \c dst.
 *
 * \param[in]  dst      destination buffer.
 * \param[in]  n        size of the buffer. Negative sizes are allowed.
 * \param[in]  src      source string.
 * \param[in]  l        maximum number of chars to copy.
 *
 * \return minimum of  \c src \e length and \c l.
*/
ssize_t a_strncpy(char *dst, ssize_t n, const char *src, ssize_t l)
{
    ssize_t len = MIN(a_strlen(src), l);

    if (n > 0)
    {
        ssize_t dlen = MIN(n - 1, len);
        memcpy(dst, src, dlen);
        dst[dlen] = '\0';
    }

    return len;
}

/** \brief safe strcpy.
 *
 * Copies at most <tt>n-1</tt> characters from \c src into \c dst, always
 * adding a final \c \\0 in \c dst.
 *
 * \param[in]  dst      destination buffer.
 * \param[in]  n        size of the buffer. Negative sizes are allowed.
 * \param[in]  src      source string.
 * \return \c src \e length. If this value is \>= \c n then the copy was
 *         truncated.
 */
ssize_t a_strcpy(char *dst, ssize_t n, const char *src)
{
    ssize_t len = a_strlen(src);

    if (n > 0)
    {
        ssize_t dlen = MIN(n - 1, len);
        memcpy(dst, src, dlen);
        dst[dlen] = '\0';
    }

    return len;
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
