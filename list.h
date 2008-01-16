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

#ifndef AWESOME_LIST_H
#define AWESOME_LIST_H

#define DO_SLIST(type, prefix, dtor)                                         \
    static inline type *prefix##_list_pop(type **list)                       \
    {                                                                        \
        if (*list)                                                           \
        {                                                                    \
            type *res = *list;                                               \
            *list = res->next;                                               \
            res->next = NULL;                                                \
            return res;                                                      \
        }                                                                    \
        return NULL;                                                         \
    }                                                                        \
    static inline void prefix##_list_push(type **list, type *item)           \
    {                                                                        \
        item->next = *list;                                                  \
        *list = item;                                                        \
    }                                                                        \
                                                                             \
    static inline type **prefix##_list_last(type **list)                     \
    {                                                                        \
        while (*list && (*list)->next)                                       \
            list = &(*list)->next;                                           \
        return list;                                                         \
    }                                                                        \
                                                                             \
    static inline void prefix##_list_append(type **list, type *item)         \
    {                                                                        \
        if(*(list = prefix##_list_last(list)))                               \
            (*list)->next = item;                                            \
        else                                                                 \
            (*list) = item;                                                  \
    }                                                                        \
    static inline type *prefix##_list_rev(type *list)                        \
    {                                                                        \
        type *l = NULL;                                                      \
        while (list)                                                         \
            prefix##_list_push(&l, prefix##_list_pop(&list));                \
        return l;                                                            \
    }                                                                        \
                                                                             \
    static inline type **prefix##_list_init(type **list)                     \
    {                                                                        \
        *list = NULL;                                                        \
        return list;                                                         \
    }                                                                        \
    static inline void prefix##_list_wipe(type **list)                       \
    {                                                                        \
        while (*list)                                                        \
        {                                                                    \
            type *item = prefix##_list_pop(list);                            \
            dtor(&item);                                                     \
        }                                                                    \
    }                                                                        \
    static inline type *prefix##_list_prev(type **list, type *item)          \
    {                                                                        \
        type *tmp;                                                           \
        if(*list == item) return NULL;                                       \
        for(tmp = *list; tmp && tmp->next != item; tmp = tmp->next);         \
        return tmp;                                                          \
    }                                                                        \
    static inline void prefix##_list_swap(type **list, type *item1,          \
                                          type *item2)                       \
    {                                                                        \
        type *i1p, *i2p;                                                     \
        if(!item1 || !item2) return;                                         \
        type *i1n = item1->next;                                             \
        type *i2n = item2->next;                                             \
        item1->next = i2n == item1 ? item2 : i2n;                            \
        item2->next = i1n == item2 ? item1 : i1n;                            \
                                                                             \
        i1p = prefix##_list_prev(list, item1);                               \
        i2p = prefix##_list_prev(list, item2);                               \
                                                                             \
        if(i1p && i1p != item2)                                              \
            i1p->next = item2;                                               \
        if(i2p && i2p != item1)                                              \
            i2p->next = item1;                                               \
                                                                             \
        if(*list == item1)                                                   \
            *list = item2;                                                   \
        else if(*list == item2)                                              \
            *list = item1;                                                   \
    }                                                                        \
    static inline type *prefix##_list_prev_cycle(type **list, type *item)    \
    {                                                                        \
        type *tmp = prefix##_list_prev(list, item);                          \
        if(!tmp)                                                             \
            tmp = *prefix##_list_last(list);                                 \
        return tmp;                                                          \
    }                                                                        \
    static inline void prefix##_list_detach(type **list, type *item)         \
    {                                                                        \
        type *prev = prefix##_list_prev(list, item);                         \
        if(prev)                                                             \
             prev->next = item->next;                                        \
        if(item == *list)                                                    \
            *list = item->next;                                              \
        item->next = NULL;                                                   \
    }                                                                        \
 
#endif
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
