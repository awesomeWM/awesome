/* See LICENSE file for copyright and license details. */

#ifndef JDWM_MEM_H
#define JDWM_MEM_H

#include "config.h"

#define ssizeof(foo)            (ssize_t)sizeof(foo)
#define countof(foo)            (ssizeof(foo) / ssizeof(foo[0]))

#define p_new(type, count)      ((type *)xmalloc(sizeof(type) * (count)))
#define p_clear(p, count)       ((void)memset((p), 0, sizeof(*(p)) * (count)))
#define p_realloc(pp, count)    xrealloc((void*)(pp) sizeof(**(pp) * (count)))

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
void spawn(Display *, jdwm_config *, const char *);

#endif
