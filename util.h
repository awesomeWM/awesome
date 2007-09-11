/* See LICENSE file for copyright and license details. */

#ifndef AWESOME_MEM_H
#define AWESOME_MEM_H

#include <string.h>
#include "config.h"

#define ssizeof(foo)            (ssize_t)sizeof(foo)
#define countof(foo)            (ssizeof(foo) / ssizeof(foo[0]))

#define p_new(type, count)      ((type *)xmalloc(sizeof(type) * (count)))
#define p_clear(p, count)       ((void)memset((p), 0, sizeof(*(p)) * (count)))
#define p_realloc(pp, count)    xrealloc((void*)(pp) sizeof(**(pp) * (count)))
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

void eprint(const char *, ...) __attribute__ ((noreturn)) __attribute__ ((format(printf, 1, 2)));
void spawn(Display *, awesome_config *, const char *);

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


#endif
