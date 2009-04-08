/*
 *  Copyright © 2006,2007,2008 Pierre Habouzit
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The names of its contributors may not be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
 *  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED.  IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 *  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <sysexits.h>
#include <stdio.h>

#include "common/buffer.h"

char buffer_slop[1];

void
buffer_ensure(buffer_t *buf, int newlen)
{
    if (newlen < 0)
        exit(EX_SOFTWARE);

    if (newlen < buf->size)
        return;

    if (newlen < buf->offs + buf->size && buf->offs > buf->size / 4)
    {
        /* Data fits in the current area, shift it left */
        memmove(buf->s - buf->offs, buf->s, buf->len + 1);
        buf->s    -= buf->offs;
        buf->size += buf->offs;
        buf->offs  = 0;
        return;
    }

    buf->size = p_alloc_nr(buf->size + buf->offs);
    if (buf->size < newlen + 1)
        buf->size = newlen + 1;
    if (buf->alloced && !buf->offs)
        p_realloc(&buf->s, buf->size);
    else
    {
        char *new_area = xmalloc(buf->size);
        memcpy(new_area, buf->s, buf->len + 1);
        if (buf->alloced)
            free(buf->s - buf->offs);
        buf->alloced = true;
        buf->s = new_area;
        buf->offs = 0;
    }
}

void
buffer_addvf(buffer_t *buf, const char *fmt, va_list args)
{
    int len;
    va_list ap;

    va_copy(ap, args);
    buffer_ensure(buf, BUFSIZ);

    len = vsnprintf(buf->s + buf->len, buf->size - buf->len, fmt, args);
    if (unlikely(len < 0))
        return;
    if (len >= buf->size - buf->len)
    {
        buffer_ensure(buf, len);
        vsnprintf(buf->s + buf->len, buf->size - buf->len, fmt, ap);
    }
    buf->len += len;
    buf->s[buf->len] = '\0';
}

void
buffer_addf(buffer_t *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    buffer_addvf(buf, fmt, args);
    va_end(args);
}

/** Detach the data from a buffer.
 * \param Buffer from which detach.
 * \return The data.
 */
char *
buffer_detach(buffer_t *buf)
{
    char *res = buf->s;
    if (!buf->alloced)
        res = a_strdup(buf->s);
    buffer_init(buf);
    return res;
}
