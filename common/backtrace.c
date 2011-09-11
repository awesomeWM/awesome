/*
 * common/backtrace.c - Backtrace handling functions
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

#include "config.h"
#include "common/backtrace.h"

#ifdef HAS_EXECINFO
#include <execinfo.h>
#endif

#define MAX_STACK_SIZE 32

/** Get a backtrace.
  * \param buf The buffer to fill with backtrace.
  */
void
backtrace_get(buffer_t *buf)
{
    buffer_init(buf);

#ifdef HAS_EXECINFO
    void *stack[MAX_STACK_SIZE];
    char **bt;
    int stack_size;

    stack_size = backtrace(stack, countof(stack));
    bt = backtrace_symbols(stack, stack_size);

    if(bt)
    {
        for(int i = 0; i < stack_size; i++)
        {
            if(i > 0)
                buffer_addsl(buf, "\n");
            buffer_adds(buf, bt[i]);
        }
        p_delete(&bt);
    }
    else
#endif
        buffer_addsl(buf, "Cannot get backtrace symbols.");
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
