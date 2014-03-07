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

#include "common/lualib.h"
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

ARRAY_TYPE_EXTRA(signal_t, signal, struct signal_array_t *inherits_from)
BARRAY_FUNCS(signal_t, signal, signal_wipe, signal_cmp)

static inline signal_t *
signal_array_getbyid(signal_array_t *arr, unsigned long id)
{
    signal_t sig = { .id = id };
    signal_t *result;

    result = signal_array_lookup(arr, &sig);
    if(result)
        return result;

    /* The signal doesn't exist yet. Check if some of our inherits_from have the
     * signal and if yes, add it.
     */
    signal_array_t *inherit = arr->inherits_from;
    while(inherit != NULL)
    {
        if(signal_array_lookup(inherit, &sig))
            break;
        inherit = inherit->inherits_from;
    }
    if(inherit)
    {
        /* Add the signal to this array to pretend it always existed */
        signal_array_insert(arr, sig);
        result = signal_array_lookup(arr, &sig);
        assert(result != NULL);
    }
    return result;
}

/** Add a signal to a signal array.
 * Signals have to be added before they can be added or emitted.
 * \param arr The signal array.
 * \param name The signal name.
 */
static inline void
signal_add(signal_array_t *arr, const char *name)
{
    unsigned long tok = a_strhash((const unsigned char *) name);
    signal_t *sigfound = signal_array_getbyid(arr, tok);
    if(!sigfound)
    {
        signal_t sig = { .id = tok };
        signal_array_insert(arr, sig);
    }
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
        warn("Trying to connect to unknown signal '%s'", name);
}

/** Disconnect a signal inside a signal array.
 * You are in charge of reference counting.
 * \param arr The signal array.
 * \param name The signal name.
 * \param ref The reference to remove.
 */
static inline void
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
                break;
            }
    } else
        warn("Trying to disconnect from unknown signal '%s'", name);
}

#endif

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
