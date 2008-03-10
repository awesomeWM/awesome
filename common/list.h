/*
 * list.h - useful list handling header
 *
 * Copyright © 2008 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_COMMON_LIST_H
#define AWESOME_COMMON_LIST_H

#define DO_SLIST(type, prefix, dtor)                                             \
    static inline type *prefix##_list_next(type **list __attribute__ ((unused)), \
                                           type *item)                           \
    {                                                                            \
        return item->next;                                                       \
    }                                                                            \
                                                                                 \
    static inline type *prefix##_list_pop(type **list)                           \
    {                                                                            \
        if (*list)                                                               \
        {                                                                        \
            type *res = *list;                                                   \
            *list = res->next;                                                   \
            if(res->next)                                                        \
                res->next->prev = NULL;                                          \
            res->next = NULL;                                                    \
            return res;                                                          \
        }                                                                        \
        return NULL;                                                             \
    }                                                                            \
                                                                                 \
    static inline void prefix##_list_push(type **list, type *item)               \
    {                                                                            \
        if(*list)                                                                \
            (*list)->prev = item;                                                \
        item->next = *list;                                                      \
        *list = item;                                                            \
    }                                                                            \
                                                                                 \
    static inline type **prefix##_list_last(type **list)                         \
    {                                                                            \
        while (*list && (*list)->next)                                           \
            list = &(*list)->next;                                               \
        return list;                                                             \
    }                                                                            \
                                                                                 \
    static inline void prefix##_list_append(type **list, type *item)             \
    {                                                                            \
        if(*(list = prefix##_list_last(list)))                                   \
        {                                                                        \
            (*list)->next = item;                                                \
            item->prev = *list;                                                  \
        }                                                                        \
        else                                                                     \
            (*list) = item;                                                      \
    }                                                                            \
                                                                                 \
    static inline type *prefix##_list_rev(type *list)                            \
    {                                                                            \
        type *l = NULL;                                                          \
        while (list)                                                             \
            prefix##_list_push(&l, prefix##_list_pop(&list));                    \
        return l;                                                                \
    }                                                                            \
                                                                                 \
    static inline type **prefix##_list_init(type **list)                         \
    {                                                                            \
        *list = NULL;                                                            \
        return list;                                                             \
    }                                                                            \
                                                                                 \
    static inline void prefix##_list_wipe(type **list)                           \
    {                                                                            \
        while (*list)                                                            \
        {                                                                        \
            type *item = prefix##_list_pop(list);                                \
            dtor(&item);                                                         \
        }                                                                        \
    }                                                                            \
                                                                                 \
    static inline type *prefix##_list_prev(type **list __attribute__ ((unused)), \
                                           type *item)                           \
    {                                                                            \
        return item->prev;                                                       \
    }                                                                            \
                                                                                 \
    static inline void prefix##_list_swap(type **list, type *item1, type *item2) \
    {                                                                            \
        if(!item1 || !item2) return;                                             \
                                                                                 \
        type *i1n = item1->next;                                                 \
        type *i2n = item2->next;                                                 \
        type *i1p = item1->prev;                                                 \
        type *i2p = item2->prev;                                                 \
                                                                                 \
        if(item1 == i2n)                                                         \
        {                                                                        \
            item1->next = item2;                                                 \
            item2->prev = item1;                                                 \
            if((item1->prev = i2p))                                              \
                i2p->next = item1;                                               \
            if((item2->next = i1n))                                              \
                i1n->prev = item2;                                               \
        }                                                                        \
        else if(item2 == i1n)                                                    \
        {                                                                        \
            item2->next = item1;                                                 \
            item1->prev = item2;                                                 \
            if((item2->prev = i1p))                                              \
                i1p->next = item2;                                               \
            if((item1->next = i2n))                                              \
                i2n->prev = item1;                                               \
        }                                                                        \
        else                                                                     \
        {                                                                        \
            if((item1->next = i2n))                                              \
                item1->next->prev = item1;                                       \
            if((item1->prev = i2p))                                              \
                item1->prev->next = item1;                                       \
            if((item2->prev = i1p))                                              \
                item2->prev->next = item2;                                       \
            if((item2->next = i1n))                                              \
                item2->next->prev = item2;                                       \
        }                                                                        \
        if(*list == item1)                                                       \
            *list = item2;                                                       \
        else if(*list == item2)                                                  \
            *list = item1;                                                       \
    }                                                                            \
                                                                                 \
    static inline type *prefix##_list_prev_cycle(type **list, type *item)        \
    {                                                                            \
        if(!item->prev)                                                          \
            return *prefix##_list_last(list);                                    \
        return item->prev;                                                       \
    }                                                                            \
                                                                                 \
    static inline type *prefix##_list_next_cycle(type **list, type *item)        \
    {                                                                            \
        if(item->next)                                                           \
            return item->next;                                                   \
        else                                                                     \
            return *list;                                                        \
        return NULL;                                                             \
    }                                                                            \
                                                                                 \
    static inline void prefix##_list_detach(type **list, type *item)             \
    {                                                                            \
        if(item == *list)                                                        \
            *list = item->next;                                                  \
        else if(item->prev)                                                      \
             item->prev->next = item->next;                                      \
        if(item->next)                                                           \
             item->next->prev = item->prev;                                      \
        item->next = NULL;                                                       \
        item->prev = NULL;                                                       \
    }                                                                            \

#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
