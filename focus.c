/*
 * focus.c - focus management
 *
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
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

#include "util.h"
#include "tag.h"
#include "layout.h"
#include "focus.h" 

extern awesome_config globalconf;

static FocusList *
focus_get_node_by_client(FocusList *head, Client *c)
{
    FocusList *fh;

    for(fh = head; fh; fh = fh->prev)
        if(fh->client == c)
            return fh;

    return NULL;
}

static FocusList *
focus_detach_node(FocusList **head, FocusList *fl)
{
    FocusList *tmp;

    if(*head == fl)
        *head = fl->prev;
    else
    {
        for(tmp = *head; tmp && tmp->prev != fl; tmp = tmp->prev);
        tmp->prev = fl->prev;
    }

    return fl;
}

static FocusList *
focus_attach_node(FocusList **head, FocusList *fl)
{
    FocusList *old_head;

    old_head = *head;
    *head = fl;
    fl->prev = old_head;

    return fl;
}

void
focus_add_client(FocusList **head, Client *c)
{
    FocusList *new_fh;

    /* if we don't find this node, create a new one */
    if(!(new_fh = focus_get_node_by_client(*head, c)))
    {
        new_fh = p_new(FocusList, 1);
        new_fh->client = c;
    }
    else /* if we've got a node, detach it */
        focus_detach_node(head, new_fh);

    focus_attach_node(head, new_fh);
}

void
focus_delete_client(FocusList **head, Client *c)
{
    FocusList *fc = focus_get_node_by_client(*head, c), *target;
    if (fc)
    {
        target = focus_detach_node(head, fc);
        p_delete(&target);
    }
}

Client *
focus_get_latest_client_for_tag(FocusList *head, int screen, Tag *t)
{
    FocusList *fl;

    for(fl = head; fl; fl = fl->prev)
        if(is_client_tagged(fl->client, t, screen))
           return fl->client;

    return NULL;
}

void
uicb_focus_history(int screen, char *arg)
{
    int i;
    FocusList *fl = globalconf.focus;
    Tag *curtag = get_current_tag(screen);

    if(arg)
    {
        i = atoi(arg);

        if(i < 0)
        {
            for(; fl && i < 0; fl = fl->prev)
                if(is_client_tagged(fl->client, curtag, screen))
                    i++;
            if(fl)
                focus(fl->client, True, screen);
        }
    }
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
