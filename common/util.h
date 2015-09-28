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

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

#if !(defined (__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined (__DragonFly__))
#include <alloca.h>
#endif

/** \brief replace \c NULL strings with empty strings */
#define NONULL(x)       (x ? x : "")

#define DO_NOTHING(...)

#undef MAX
#undef MIN
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define unsigned_subtract(a, b)  \
    do {                         \
        if (b > a)               \
            a = 0;               \
        else                     \
            a -= b;              \
    } while (0)

#define ssizeof(foo)            (ssize_t)sizeof(foo)
#define countof(foo)            (ssizeof(foo) / ssizeof(foo[0]))
#define fieldsizeof(type_t, m)  sizeof(((type_t *)0)->m)
#define fieldtypeof(type_t, m)  typeof(((type_t *)0)->m)

#define p_alloca(type, count)                                \
        ((type *)memset(alloca(sizeof(type) * (count)),      \
                        0, sizeof(type) * (count)))

#define p_alloc_nr(x)           (((x) + 16) * 3 / 2)
#define p_new(type, count)      ((type *)xmalloc(sizeof(type) * (count)))
#define p_clear(p, count)       ((void)memset((p), 0, sizeof(*(p)) * (count)))
#define p_realloc(pp, count)    xrealloc((void*)(pp), sizeof(**(pp)) * (count))
#define p_dup(p, count)         xmemdup((p), sizeof(*(p)) * (count))
#define p_grow(pp, goalnb, allocnb)                  \
    do {                                             \
        if ((goalnb) > *(allocnb)) {                 \
            if (p_alloc_nr(*(allocnb)) < (goalnb)) { \
                *(allocnb) = (goalnb);               \
            } else {                                 \
                *(allocnb) = p_alloc_nr(*(allocnb)); \
            }                                        \
            p_realloc(pp, *(allocnb));               \
        }                                            \
    } while (0)

#define p_delete(mem_p)                              \
    do {                                             \
        void **__ptr = (void **) (mem_p);            \
        free(*__ptr);                                \
        *(void **)__ptr = NULL;                      \
    } while (0)

#ifdef __GNUC__
#define likely(expr)    __builtin_expect(!!(expr), 1)
#define unlikely(expr)  __builtin_expect((expr), 0)
#else
#define likely(expr)    expr
#define unlikely(expr)  expr
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

/** Duplicate a memory zone.
 * \param src The source.
 * \param size The source size.
 * \return The memory address of the copy.
 */
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
 * The a_strdup() function returns a pointer to a new string, which is a
 * duplicate of \c s. Memory should be freed using p_delete().
 *
 * \warning when s is \c "", it returns NULL !
 *
 * \param[in] s the string to duplicate.
 * \return a pointer to the duplicated string.
 */
static inline
char *a_strdup(const char *s)
{
    ssize_t len = a_strlen(s);
    return len ? p_dup(s, len + 1) : NULL;
}

/** \brief safe limited strdup.
 *
 * Copies at most min(<tt>n-1</tt>, \c l) characters from \c src into a newly
 * allocated buffer, always adding a final \c \\0, and returns that buffer.
 *
 * \warning when s is \c "" or l is 0, it returns NULL !
 *
 * \param[in]  s        source string.
 * \param[in]  l        maximum number of chars to copy.
 * \return a newly allocated buffer containing the first \c l chars of \c src.
*/
static inline
char * a_strndup(const char *s, ssize_t l)
{
    ssize_t len = MIN(a_strlen(s), l);
    if(len)
    {
        char *p = p_dup(s, len + 1);
        p[len] = '\0';
        return p;
    }
    return NULL;
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

#define  A_STREQ(a, b) (((a) == (b)) || a_strcmp(a, b) == 0)
#define A_STRNEQ(a, b) (!A_STREQ(a, b))

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

#define  A_STREQ_CASE(a, b) (((a) == (b)) || a_strcasecmp(a, b) == 0)
#define A_STRNEQ_CASE(a, b) (!A_STREQ_CASE(a, b))

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

#define  A_STREQ_N(a, b, n) (((a) == (b)) || (n) == ((ssize_t) 0) || a_strncmp(a, b, n) == 0)
#define A_STRNEQ_N(a, b) (!A_STREQN(a, b))

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

/** Compute a hash for a string.
 * This is based on 'djb2' algorithm.
 */
static inline unsigned long __attribute__ ((nonnull(1)))
a_strhash(const unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

#define fatal(string, ...) _fatal(__LINE__, \
                                  __FUNCTION__, \
                                  string, ## __VA_ARGS__)
void _fatal(int, const char *, const char *, ...)
    __attribute__ ((noreturn)) __attribute__ ((format(printf, 3, 4)));

#define warn(string, ...) _warn(__LINE__, \
                                __FUNCTION__, \
                                string, ## __VA_ARGS__)
void _warn(int, const char *, const char *, ...)
    __attribute__ ((format(printf, 3, 4)));

void a_exec(const char *);

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
