/*
 * util.c - useful functions header
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

#ifndef AWESOME_MEM_H
#define AWESOME_MEM_H

#include <string.h>
#include <stdlib.h>
#include "common.h"

/** Link a name to a function */
typedef struct
{
    const char *name;
    void *func;
} NameFuncLink;

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
 * Unlinke it's libc sibling, a_strlen returns a ssize_t, and supports its
 * argument beeing NULL.
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
 * Unlinke it's GNU libc sibling, a_strnlen returns a ssize_t, and supports
 * its argument beeing NULL.
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

void die(const char *, ...) __attribute__ ((noreturn)) __attribute__ ((format(printf, 1, 2)));
void eprint(const char *, ...) __attribute__ ((noreturn)) __attribute__ ((format(printf, 1, 2)));
Bool xgettextprop(Display *, Window, Atom, char *, ssize_t);
double compute_new_value_from_arg(const char *, double);
void *name_func_lookup(const char *, const NameFuncLink *);

UICB_PROTO(uicb_spawn);
UICB_PROTO(uicb_exec);
#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
