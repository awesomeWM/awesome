/*
 * util.c - useful functions
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
#include "tokenize.h"

/** Print error and exit with EXIT_FAILURE code.
 */
void
_fatal(int line, const char *fct, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "E: awesome: %s:%d: ", fct, line);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/** Print error message on stderr.
 */
void
_warn(int line, const char *fct, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "W: awesome: %s:%d: ", fct, line);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
}

/** Lookup for a function pointer from its name
 * in the given name_func_link_t list.
 * \param funcname Function name.
 * \param list Function and name link list.
 * \return Function pointer.
 */
void *
name_func_lookup(const char *funcname, const name_func_link_t *list)
{
    int i;

    if(funcname && list)
        for(i = 0; list[i].name; i++)
            if(!a_strcmp(funcname, list[i].name))
                return list[i].func;

    return NULL;
}

/** Lookup for a function name from its pointer
 * in the given name_func_link_t list.
 * \param funcp Function pointer.
 * \param list Function and name link list.
 * \return Name of the function.
 */
const char *
name_func_rlookup(void * funcp, const name_func_link_t *list)
{
    int i;
    
    if(funcp && list)
        for(i = 0; list[i].name; i++)
            if(funcp == list[i].func)
                return list[i].name;

    return NULL;
}

/** Get a position type from a string.
 * \param pos The position.
 * \param len The string length, -1 if unknown.
 * \return A position.
 */
position_t
position_fromstr(const char *pos, ssize_t len)
{
    switch(a_tokenize(pos, len))
    {
      default:
        return Top;
      case A_TK_BOTTOM:
        return Bottom;
      case A_TK_RIGHT:
        return Right;
      case A_TK_LEFT:
        return Left;
    }
}

/** Convert a position type to a string.
 * \param p The position.
 * \return A position string.
 */
const char *
position_tostr(position_t p)
{
    switch(p)
    {
      case Top:    return "top";
      case Bottom: return "bottom";
      case Right:  return "right";
      case Left:   return "left";
      default:     return NULL;
    }
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
ssize_t
a_strncpy(char *dst, ssize_t n, const char *src, ssize_t l)
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
ssize_t
a_strcpy(char *dst, ssize_t n, const char *src)
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

/** Execute a command and replace the current process.
 * \param cmd The command to execute.
 */
void
a_exec(const char *cmd)
{
    static const char *shell = NULL;

    if(!shell && !(shell = getenv("SHELL")))
        shell = "/bin/sh";

    execl(shell, shell, "-c", cmd, NULL);
}

/** Split a string in substring.
 * \param str The string to split.
 * \param len The string length.
 * \param delim The string delimiter.
 * \return A list of string NULL terminted.
 */
char **
a_strsplit(const char *str, ssize_t len, char delim)
{
    char **retval;
    int comp, i, j = 0, n = 1;

    /* count components */
    for(i = 0; i < len; i++)
        if(str[i] == delim)
            n++;

    retval = p_new(char *, n);

    for(i = 0, comp = 0; comp < n; comp++)
    {
        if(str[i] == delim)
            i++;

        for(j = i; str[j] != delim && j < len; j++);

        retval[comp] = p_dup(&str[i], j - i + 1);
        retval[comp][j - i] = '\0';

        i = j;
    }

    return retval;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
