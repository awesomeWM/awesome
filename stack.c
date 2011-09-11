/*
 * stack.c - client stack management
 *
 * Copyright © 2008-2009 Julien Danjou <julien@danjou.info>
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

#include "ewmh.h"
#include "screen.h"
#include "stack.h"
#include "objects/client.h"
#include "objects/drawin.h"

void
stack_client_remove(client_t *c)
{
    foreach(client, globalconf.stack)
        if(*client == c)
        {
            client_array_remove(&globalconf.stack, client);
            break;
        }
    ewmh_update_net_client_list_stacking();
    stack_windows();
}

/** Push the client at the beginning of the client stack.
 * \param c The client to push.
 */
void
stack_client_push(client_t *c)
{
    stack_client_remove(c);
    client_array_push(&globalconf.stack, c);
    ewmh_update_net_client_list_stacking();
    stack_windows();
}

/** Push the client at the end of the client stack.
 * \param c The client to push.
 */
void
stack_client_append(client_t *c)
{
    stack_client_remove(c);
    client_array_append(&globalconf.stack, c);
    ewmh_update_net_client_list_stacking();
    stack_windows();
}

static bool need_stack_refresh = false;

void
stack_windows(void)
{
    need_stack_refresh = true;
}

/** Stack a window above another window, without causing errors.
 * \param w The window.
 * \param previous The window which should be below this window.
 */
static void
stack_window_above(xcb_window_t w, xcb_window_t previous)
{
    if (previous == XCB_NONE)
        /* This would cause an error from the X server. Also, if we really
         * changed the stacking order of all windows, they'd all have to redraw
         * themselves. Doing it like this is better. */
        return;

    xcb_configure_window(globalconf.connection, w,
                         XCB_CONFIG_WINDOW_SIBLING | XCB_CONFIG_WINDOW_STACK_MODE,
                         (uint32_t[]) { previous, XCB_STACK_MODE_ABOVE });
}

/** Stack a client above.
 * \param client The client.
 * \param previous The previous client on the stack.
 * \return The next-previous!
 */
static xcb_window_t
stack_client_above(client_t *c, xcb_window_t previous)
{
    stack_window_above(c->frame_window, previous);

    previous = c->frame_window;

    /* stack transient window on top of their parents */
    foreach(node, globalconf.stack)
        if((*node)->transient_for == c)
            previous = stack_client_above(*node, previous);

    return previous;
}

/** Stacking layout layers */
typedef enum
{
    /** This one is a special layer */
    WINDOW_LAYER_IGNORE,
    WINDOW_LAYER_DESKTOP,
    WINDOW_LAYER_BELOW,
    WINDOW_LAYER_NORMAL,
    WINDOW_LAYER_ABOVE,
    WINDOW_LAYER_FULLSCREEN,
    WINDOW_LAYER_ONTOP,
    /** This one only used for counting and is not a real layer */
    WINDOW_LAYER_COUNT
} window_layer_t;

/** Get the real layer of a client according to its attribute (fullscreen, …)
 * \param c The client.
 * \return The real layer.
 */
static window_layer_t
client_layer_translator(client_t *c)
{
    /* first deal with user set attributes */
    if(c->ontop)
        return WINDOW_LAYER_ONTOP;
    /* Fullscreen windows only get their own layer when they have the focus */
    else if(c->fullscreen && globalconf.focus.client == c)
        return WINDOW_LAYER_FULLSCREEN;
    else if(c->above)
        return WINDOW_LAYER_ABOVE;
    else if(c->below)
        return WINDOW_LAYER_BELOW;
    /* check for transient attr */
    else if(c->transient_for)
        return WINDOW_LAYER_IGNORE;

    /* then deal with windows type */
    switch(c->type)
    {
      case WINDOW_TYPE_DESKTOP:
        return WINDOW_LAYER_DESKTOP;
      default:
        break;
    }

    return WINDOW_LAYER_NORMAL;
}

/** Restack clients.
 * \todo It might be worth stopping to restack everyone and only stack `c'
 * relatively to the first matching in the list.
 */
void
stack_refresh()
{
    if(!need_stack_refresh)
        return;

    xcb_window_t next = XCB_NONE;

    /* stack desktop windows */
    for(window_layer_t layer = WINDOW_LAYER_DESKTOP; layer < WINDOW_LAYER_BELOW; layer++)
        foreach(node, globalconf.stack)
            if(client_layer_translator(*node) == layer)
                next = stack_client_above(*node, next);

    /* first stack not ontop drawin window */
    foreach(drawin, globalconf.drawins)
        if(!(*drawin)->ontop)
        {
            stack_window_above((*drawin)->window, next);
            next = (*drawin)->window;
        }

    /* then stack clients */
    for(window_layer_t layer = WINDOW_LAYER_BELOW; layer < WINDOW_LAYER_COUNT; layer++)
        foreach(node, globalconf.stack)
            if(client_layer_translator(*node) == layer)
                next = stack_client_above(*node, next);

    /* then stack ontop drawin window */
    foreach(drawin, globalconf.drawins)
        if((*drawin)->ontop)
        {
            stack_window_above((*drawin)->window, next);
            next = (*drawin)->window;
        }

    need_stack_refresh = false;
}


// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:textwidth=80
