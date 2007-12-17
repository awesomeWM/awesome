/*
 * tag.c - tag management
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

#include <stdio.h>
#include <X11/Xutil.h>

#include "screen.h"
#include "layout.h"
#include "tag.h"
#include "util.h"
#include "rules.h"

extern awesome_config globalconf;

static void
detach_tagclientlink(int screen, TagClientLink *tc)
{
    TagClientLink *tmp;

    if(globalconf.screens[screen].tclink == tc)
        globalconf.screens[screen].tclink = tc->next;
    else
    {
        for(tmp = globalconf.screens[screen].tclink; tmp && tmp->next != tc; tmp = tmp->next);
        tmp->next = tc->next;
    }
    
    p_delete(&tc);
}

void
tag_client(Client *c, Tag *t, int screen)
{
    TagClientLink *tc, *new_tc;

    /* don't tag twice */
    if(is_client_tagged(c, t, screen))
        return;

    new_tc = p_new(TagClientLink, 1);

    if(!globalconf.screens[screen].tclink)
        globalconf.screens[screen].tclink = new_tc;
    else
    {
        for(tc = globalconf.screens[screen].tclink; tc->next; tc = tc->next);
        tc->next = new_tc;
    }

    new_tc->client = c;
    new_tc->tag = t;
}

void
untag_client(Client *c, Tag *t, int screen)
{
    TagClientLink *tc;

    for(tc = globalconf.screens[screen].tclink; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
            detach_tagclientlink(screen, tc);
}

Bool
is_client_tagged(Client *c, Tag *t, int screen)
{
    TagClientLink *tc;

    for(tc = globalconf.screens[screen].tclink; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
            return True;
    return False;
}

void
tag_client_with_current_selected(Client *c, int screen)
{
    Tag *tag;
    VirtScreen vscreen;

    vscreen = globalconf.screens[screen];
    for(tag = vscreen.tags; tag; tag = tag->next)
        if(tag->selected)
            tag_client(c, tag, screen);
        else
            untag_client(c, tag, screen);
}

void
tag_client_with_rules(Client *c)
{
    Rule *r;
    Tag *tag;
    Bool matched = False;

    for(r = globalconf.rules; r; r = r->next)
        if(client_match_rule(c, r))
        {
            c->isfloating = r->isfloating;

            if(r->screen != RULE_NOSCREEN && r->screen != c->screen)
                move_client_to_screen(c, r->screen, True);

            for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
                if(is_tag_match_rules(tag, r))
                {
                    matched = True;
                    tag_client(c, tag, c->screen);
                }
                else
                    untag_client(c, tag, c->screen);

            if(!matched)
                tag_client_with_current_selected(c, c->screen);
            break;
        }
}

/** Tag selected window with tag
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_client_tag(int screen, char *arg)
{
    int tag_id = -1;
    Tag *tag, *target_tag;
    Client *sel = globalconf.focus->client;

    if(!sel)
        return;

    if(arg)
    {
	tag_id = atoi(arg) - 1;
        if(tag_id != -1)
        {
            for(target_tag = globalconf.screens[screen].tags; target_tag && tag_id > 0;
                target_tag = target_tag->next, tag_id--);
            if(target_tag)
            {
                for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
                    untag_client(sel, tag, screen);
                tag_client(sel, target_tag, screen);
            }
        }
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag_client(sel, tag, screen);

    client_saveprops(sel, screen);
    arrange(screen);
}

/** Toggle floating state of a client
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_togglefloating(int screen, char *arg __attribute__ ((unused)))
{
    Client *sel = globalconf.focus->client;
    
    if(!sel)
        return;

    sel->isfloating = !sel->isfloating;

    if (arg == NULL)
        client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, True, False);
    else
        client_resize(sel, sel->x, sel->y, sel->w, sel->h, True, True);

    client_saveprops(sel, screen);
    arrange(screen);
}

/** Toggle a tag on client
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_client_toggletag(int screen, char *arg)
{
    Client *sel = globalconf.focus->client;
    int i;
    Tag *tag, *target_tag;

    if(!sel)
        return;

    if(arg)
    {
        i = atoi(arg) - 1;
        for(target_tag = globalconf.screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);
        if(target_tag)
        {
            if(is_client_tagged(sel, target_tag, screen))
                untag_client(sel, target_tag, screen);
            else
                tag_client(sel, target_tag, screen);
        }

        /* check that there's at least one tag selected for this client*/
        for(tag = globalconf.screens[screen].tags; tag
            && !is_client_tagged(sel, tag, screen); tag = tag->next)

        if(!tag)
            tag_client(sel, target_tag, screen);
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            if(is_client_tagged(sel, tag, screen))
                tag_client(sel, tag, screen);
            else
                untag_client(sel, tag, screen);

    client_saveprops(sel, screen);
    arrange(screen);
}

/** Add a tag to viewed tags
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_tag_toggleview(int screen, char *arg)
{
    int i;
    Tag *tag, *target_tag;

    if(arg)
    {
        i = atoi(arg) - 1;
        for(target_tag = globalconf.screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);

        if(target_tag)
            target_tag->selected = !target_tag->selected;

        /* check that there's at least one tag selected */
        for(tag = globalconf.screens[screen].tags; tag && !tag->selected; tag = tag->next);
        if(!tag)
            target_tag->selected = True;
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag->selected = !tag->selected;

    saveawesomeprops(screen);
    arrange(screen);
}

/** View tag
 * \param arg tag to view
 * \ingroup ui_callback
 */
void
uicb_tag_view(int screen, char *arg)
{
    int i;
    Tag *tag, *target_tag;

    if(arg)
    {
	i = atoi(arg) - 1;
        for(target_tag = globalconf.screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);
        if(target_tag)
        {
            for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
                tag->selected = False;
            target_tag->selected = True;
        }
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag->selected = True;

    saveawesomeprops(screen);
    arrange(screen);
}

/** View previously selected tags
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_prev_selected(int screen, char *arg __attribute__ ((unused)))
{
    Tag *tag;
    Bool t;

    for(tag =  globalconf.screens[screen].tags; tag; tag = tag->next)
    {
        t = tag->selected;
        tag->selected = tag->was_selected;
        tag->was_selected = t;
    }
    arrange(screen);
}

/** View next tag
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewnext(int screen, char *arg __attribute__ ((unused)))
{
    Tag *curtag = get_current_tag(screen);

    if(!curtag->next)
        return;

    curtag->selected = False;
    curtag->next->selected = True;

    saveawesomeprops(screen);
    arrange(screen);
}

/** View previous tag
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewprev(int screen, char *arg __attribute__ ((unused)))
{
    Tag *tag, *curtag = get_current_tag(screen);

    for(tag = globalconf.screens[screen].tags; tag && tag->next != curtag; tag = tag->next);
    if(tag)
    {
        tag->selected = True;
        curtag->selected = False;
        saveawesomeprops(screen);
        arrange(screen);
    }
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
