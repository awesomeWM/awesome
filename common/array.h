/*
 * array.h - useful array handling header
 *
 * Copyright © 2008 Pierre Habouzit <madcoder@debian.org>
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

#ifndef AWESOME_COMMON_ARRAY_H
#define AWESOME_COMMON_ARRAY_H

#include <assert.h>
#include "common/util.h"

#define ARRAY_TYPE(type_t, pfx)                                             \
    typedef struct pfx##_array_t {                                          \
        type_t *tab;                                                        \
        int len, size;                                                      \
    } pfx##_array_t

#define ARRAY_FUNCS(type_t, pfx, dtor)                                      \
    static inline pfx##_array_t * pfx##_array_new(void) {                   \
        return p_new(pfx##_array_t, 1);                                     \
    }                                                                       \
    static inline void pfx##_array_init(pfx##_array_t *arr) {               \
        p_clear(arr, 1);                                                    \
    }                                                                       \
    static inline void pfx##_array_wipe(pfx##_array_t *arr) {               \
        for (int i = 0; i < arr->len; i++) {                                \
            dtor(&arr->tab[i]);                                             \
        }                                                                   \
        p_delete(&arr->tab);                                                \
    }                                                                       \
    static inline void pfx##_array_delete(pfx##_array_t **arrp) {           \
        if (*arrp) {                                                        \
            pfx##_array_wipe(*arrp);                                        \
            p_delete(arrp);                                                 \
        }                                                                   \
    }                                                                       \
                                                                            \
    static inline void pfx##_array_grow(pfx##_array_t *arr, int newlen) {   \
        p_grow(&arr->tab, newlen, &arr->size);                              \
    }                                                                       \
    static inline void                                                      \
    pfx##_array_splice(pfx##_array_t *arr, int pos, int len,                \
                       type_t items[], int count)                           \
    {                                                                       \
        assert (pos >= 0 && len >= 0 && count >= 0);                        \
        assert (pos <= arr->len && pos + len < arr->len);                   \
        if (len != count) {                                                 \
            pfx##_array_grow(arr, arr->len + count - len);                  \
            for (int i = pos; i < pos + len; i++) {                         \
                dtor(&arr->tab[i]);                                         \
            }                                                               \
            memmove(arr->tab + pos + count, arr->tab + pos + len,           \
                    arr->len - pos - len);                                  \
            arr->len += count - len;                                        \
        }                                                                   \
        memcpy(arr->tab + pos, items, count * sizeof(*items));              \
    }                                                                       \
    static inline type_t pfx##_array_take(pfx##_array_t *arr, int pos) {    \
        type_t res = arr->tab[pos];                                         \
        pfx##_array_splice(arr, pos, 1, NULL, 0);                           \
        return res;                                                         \
    }                                                                       \
    static inline void pfx##_array_append(pfx##_array_t *arr, type_t e) {   \
        pfx##_array_splice(arr, arr->len, 0, &e, 1);                        \
    }

#define DO_ARRAY(type_t, pfx, dtor)                                         \
    ARRAY_TYPE(type_t, pfx); ARRAY_FUNCS(type_t, pfx, dtor)

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
