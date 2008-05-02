/*
 * util.h - useful functions header
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

#ifndef AWESOME_COMMON_UTIL_H
#define AWESOME_COMMON_UTIL_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/** A list of possible position, not sex related */
typedef enum
{
    Off = 0,
    Top,
    Bottom,
    Right,
    Left,
    Auto
} position_t;

/** fuzzy_t logic */
typedef enum
{
    No = false,
    Yes = true,
    Maybe
} fuzzy_t;

/** Link a name to a function */
typedef struct
{
    const char *name;
    void *func;
} name_func_link_t;

/** \brief replace \c NULL strings with emtpy strings */
#define NONULL(x)       (x ? x : "")

#undef MAX
#undef MIN
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define ssizeof(foo)            (ssize_t)sizeof(foo)
#define countof(foo)            (ssizeof(foo) / ssizeof(foo[0]))

#define p_new(type, count)      ((type *)xmalloc(sizeof(type) * (count)))
#define p_clear(p, count)       ((void)memset((p), 0, sizeof(*(p)) * (count)))
#define p_realloc(pp, count)    xrealloc((void*)(pp), sizeof(**(pp)) * (count))
#define p_dup(p, count)         xmemdup((p), sizeof(*(p)) * (count))

#ifdef __GNUC__

#define p_delete(mem_pp)                           \
    do {                                           \
        typeof(**(mem_pp)) **__ptr = (mem_pp);     \
        free(*__ptr);                              \
        *__ptr = NULL;                             \
    } while(0)

#else

#define p_delete(mem_p)                            \
    do {                                           \
        void *__ptr = (mem_p);                     \
        free(*__ptr);                              \
        *(void **)__ptr = NULL;                    \
    } while (0)

#endif

static inline void * __attribute__ ((malloc)) xmalloc(ssize_t size)
{
    void *ptr;

    if(size <= 0)
        return NULL;

    ptr = calloc(1, size);

    if(!ptr)
        abort();

    return ptr;
}

static inline void
xrealloc(void **ptr, ssize_t newsize)
{
    if(newsize <= 0)
        p_delete(ptr);
    else
    {
        *ptr = realloc(*ptr, newsize);
        if(!*ptr)
            abort();
    }
}

static inline void *xmemdup(const void *src, ssize_t size)
{
    return memcpy(xmalloc(size), src, size);
}

/** \brief \c NULL resistant strlen.
 *
 * Unlike it's libc sibling, a_strlen returns a ssize_t, and supports its
 * argument being NULL.
 *
 * \param[in] s the string.
 * \return the string length (or 0 if \c s is \c NULL).
 */
static inline ssize_t a_strlen(const char *s)
{
    return s ? strlen(s) : 0;
}

/** \brief \c NULL resistant strnlen.
 *
 * Unlike it's GNU libc sibling, a_strnlen returns a ssize_t, and supports
 * its argument being NULL.
 *
 * The a_strnlen() function returns the number of characters in the string
 * pointed to by \c s, not including the terminating \c \\0 character, but at
 * most \c n. In doing this, a_strnlen() looks only at the first \c n
 * characters at \c s and never beyond \c s+n.
 *
 * \param[in]  s    the string.
 * \param[in]  n    the maximum length to return.
 * \return \c a_strlen(s) if less than \c n, else \c n.
 */
static inline ssize_t a_strnlen(const char *s, ssize_t n)
{
    if (s)
    {
        const char *p = memchr(s, '\0', n);
        return p ? p - s : n;
    }
    return 0;
}

/** \brief \c NULL resistant strdup.
 *
 * the a_strdup() function returns a pointer to a new string, which is a
 * duplicate of \c s. Memory should be freed using p_delete().
 *
 * \warning when s is \c "", it returns NULL !
 *
 * \param[in] s the string to duplicate.
 * \return a pointer to the duplicated string.
 */
static inline char *a_strdup(const char *s)
{
    ssize_t len = a_strlen(s);
    return len ? p_dup(s, len + 1) : NULL;
}

/** \brief \c NULL resistant strcmp.
 * \param[in]  a     the first string.
 * \param[in]  b     the second string.
 * \return <tt>strcmp(a, b)</tt>, and treats \c NULL strings like \c ""
 * ones.
 */
static inline int a_strcmp(const char *a, const char *b)
{
        return strcmp(NONULL(a), NONULL(b));
}

/** \brief \c NULL resistant strcasecmp.
 * \param[in]  a     the first string.
 * \param[in]  b     the second string.
 * \return <tt>strcasecmp(a, b)</tt>, and treats \c NULL strings like \c ""
 * ones.
 */
static inline int a_strcasecmp(const char *a, const char *b)
{
    return strcasecmp(NONULL(a), NONULL(b));
}

/** \brief \c NULL resistant strncmp.
 * \param[in]  a     the first string.
 * \param[in]  b     the second string.
 * \param[in]  n     the number of maximum chars to compare.
 * \return <tt>strncmp(a, b, n)</tt>, and treats \c NULL strings like \c ""
 * ones.
 */
static inline int a_strncmp(const char *a, const char *b, ssize_t n)
{
    return strncmp(NONULL(a), NONULL(b), n);
}

ssize_t a_strncpy(char *dst, ssize_t n, const char *src, ssize_t l) __attribute__((nonnull(1)));
ssize_t a_strcpy(char *dst, ssize_t n, const char *src) __attribute__((nonnull(1)));
char* a_strndup(const char *src, ssize_t l) __attribute__((nonnull(1)));

/** \brief safe strcat.
 *
 * The a_strcat() function appends the string \c src at the end of the buffer
 * \c dst if space is available.
 *
 * \param[in]  dst   destination buffer.
 * \param[in]  n     size of the buffer, Negative sizes are allowed.
 * \param[in]  src   the string to append.
 * \return <tt>a_strlen(dst) + a_strlen(src)</tt>
 */
static inline ssize_t a_strcat(char *dst, ssize_t n, const char *src)
{
    ssize_t dlen = a_strnlen(dst, n - 1);
    return dlen + a_strcpy(dst + dlen, n - dlen, src);
}

/** \brief safe strncat.
 *
 * The a_strncat() function appends at most \c n chars from the string \c src
 * at the end of the buffer \c dst if space is available.
 *
 * \param[in]  dst   destination buffer.
 * \param[in]  n     size of the buffer, Negative sizes are allowed.
 * \param[in]  src   the string to append.
 * \param[in]  l     maximum number of chars of src to consider.
 * \return the smallest value between <tt>a_strlen(dst) + a_strlen(src)</tt>
 * and <tt>a_strlen(dst) + l</tt>
 */
static inline ssize_t
a_strncat(char *dst, ssize_t n, const char *src, ssize_t l)
{
    ssize_t dlen = a_strnlen(dst, n - 1);
    return dlen + a_strncpy(dst + dlen, n - dlen, src, l);
}

/** \brief convert a string to a boolean value.
 *
 * The a_strtobool() function converts a string \c s into a boolean.
 * It recognizes the strings "true", "on", "yes" and "1".
 *
 * \param[in]  s     the string to convert
 * \return true if the string is recognized as possibly true, false otherwise.
 */
static inline bool
a_strtobool(const char *s)
{
    if(!strcasecmp(s, "true")
       || !strcasecmp(s, "on")
       || !strcasecmp(s, "yes")
       || !strcasecmp(s, "1"))
        return true;
    return false;
}

#define eprint(string, ...) _eprint(__LINE__, \
                                    __FUNCTION__, \
                                    string, ## __VA_ARGS__)
void _eprint(int, const char *, const char *, ...)
    __attribute__ ((noreturn)) __attribute__ ((format(printf, 3, 4)));

#define warn(string, ...) _warn(__LINE__, \
                                __FUNCTION__, \
                                string, ## __VA_ARGS__)
void _warn(int, const char *, const char *, ...)
    __attribute__ ((format(printf, 3, 4)));

position_t position_get_from_str(const char *);
char * position_to_str(position_t);
fuzzy_t fuzzy_get_from_str(const char *);
double compute_new_value_from_arg(const char *, double);
void *name_func_lookup(const char *, const name_func_link_t *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
