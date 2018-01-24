/*
 * common/signal.h - Signal handling functions
 *
 * Copyright © 2009 Julien Danjou <julien@danjou.info>
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

#ifndef AWESOME_COMMON_SIGNAL
#define AWESOME_COMMON_SIGNAL

#include "common/array.h"

DO_ARRAY(const void *, cptr, DO_NOTHING)

typedef struct
{
    unsigned long id;
    cptr_array_t sigfuncs;
} signal_t;

static inline int
signal_cmp(const void *a, const void *b)
{
    const signal_t *x = a, *y = b;
    return x->id > y->id ? 1 : (x->id < y->id ? -1 : 0);
}

static inline void
signal_wipe(signal_t *sig)
{
    cptr_array_wipe(&sig->sigfuncs);
}

DO_BARRAY(signal_t, signal, signal_wipe, signal_cmp)

static inline signal_t *
signal_array_getbyid(signal_array_t *arr, unsigned long id)
{
    signal_t sig = { .id = id };
    return signal_array_lookup(arr, &sig);
}

static inline signal_t *
signal_array_getbyname(signal_array_t *arr, const char *name)
{
    signal_t sig = { .id = a_strhash((const unsigned char *) NONULL(name)) };
    return signal_array_lookup(arr, &sig);
}

/** Connect a signal inside a signal array.
 * You are in charge of reference counting.
 * \param arr The signal array.
 * \param name The signal name.
 * \param ref The reference to add.
 */
static inline void
signal_connect(signal_array_t *arr, const char *name, const void *ref)
{
    unsigned long tok = a_strhash((const unsigned char *) name);
    signal_t *sigfound = signal_array_getbyid(arr, tok);
    if(sigfound)
        cptr_array_append(&sigfound->sigfuncs, ref);
    else
    {
        signal_t sig = { .id = tok };
        cptr_array_append(&sig.sigfuncs, ref);
        signal_array_insert(arr, sig);
    }
}

/** Disconnect a signal inside a signal array.
 * You are in charge of reference counting.
 * \param arr The signal array.
 * \param name The signal name.
 * \param ref The reference to remove.
 */
static inline bool
signal_disconnect(signal_array_t *arr, const char *name, const void *ref)
{
    signal_t *sigfound = signal_array_getbyid(arr,
                                              a_strhash((const unsigned char *) name));
    if(sigfound)
    {
        foreach(func, sigfound->sigfuncs)
            if(ref == *func)
            {
                cptr_array_remove(&sigfound->sigfuncs, func);
                if(sigfound->sigfuncs.len == 0)
                    signal_array_remove(arr, sigfound);
                return true;
            }
    }
    return false;
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
