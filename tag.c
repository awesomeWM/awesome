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
#include "ewmh.h"

extern AwesomeConf globalconf;

static void
detach_tagclientlink(TagClientLink *tc)
{
    TagClientLink *tmp;

    if(globalconf.tclink == tc)
        globalconf.tclink = tc->next;
    else
    {
        for(tmp = globalconf.tclink; tmp && tmp->next != tc; tmp = tmp->next);
        tmp->next = tc->next;
    }
    
    p_delete(&tc);
}

void
tag_client(Client *c, Tag *t)
{
    TagClientLink *tc, *new_tc;

    /* don't tag twice */
    if(is_client_tagged(c, t))
        return;

    new_tc = p_new(TagClientLink, 1);

    if(!globalconf.tclink)
        globalconf.tclink = new_tc;
    else
    {
        for(tc = globalconf.tclink; tc->next; tc = tc->next);
        tc->next = new_tc;
    }

    new_tc->client = c;
    new_tc->tag = t;
}

void
untag_client(Client *c, Tag *t)
{
    TagClientLink *tc;

    for(tc = globalconf.tclink; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
            detach_tagclientlink(tc);
}

Bool
is_client_tagged(Client *c, Tag *t)
{
    TagClientLink *tc;

    if(!c)
        return False;

    for(tc = globalconf.tclink; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
            return True;

    return False;
}

void
tag_client_with_current_selected(Client *c)
{
    Tag *tag;
    VirtScreen vscreen = globalconf.screens[c->screen];

    for(tag = vscreen.tags; tag; tag = tag->next)
        if(tag->selected)
            tag_client(c, tag);
        else
            untag_client(c, tag);
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
                    tag_client(c, tag);
                }
                else
                    untag_client(c, tag);

            if(!matched)
                tag_client_with_current_selected(c);
            break;
        }
}


Tag **
get_current_tags(int screen)
{
    Tag *tag, **tags = NULL;
    int n = 1;

    tags = p_new(Tag *, n);
    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        if(tag->selected)
        {
            p_realloc(&tags, ++n);
            tags[n - 2] = tag;
        }

    /* finish with null */
    tags[n - 1] = NULL;

    return tags;
}

/** Tag selected window with tag
 * \param screen Screen ID
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
                    untag_client(sel, tag);
                tag_client(sel, target_tag);
            }
        }
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag_client(sel, tag);

    client_saveprops(sel, screen);
    arrange(screen);
}

/** Toggle floating state of a client
 * \param screen Screen ID
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
 * \param screen Screen ID
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
            if(is_client_tagged(sel, target_tag))
                untag_client(sel, target_tag);
            else
                tag_client(sel, target_tag);
        }

        /* check that there's at least one tag selected for this client*/
        for(tag = globalconf.screens[screen].tags; tag
            && !is_client_tagged(sel, tag); tag = tag->next)

        if(!tag)
            tag_client(sel, target_tag);
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            if(is_client_tagged(sel, tag))
                tag_client(sel, tag);
            else
                untag_client(sel, tag);

    client_saveprops(sel, screen);
    arrange(screen);
}

/** Add a tag to viewed tags
 * \param screen Screen ID
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
    ewmh_update_net_current_desktop(get_phys_screen(screen));
}

/** View tag
 * \param screen Screen ID
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
    ewmh_update_net_current_desktop(get_phys_screen(screen));
}

/** View previously selected tags
 * \param screen Screen ID
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
    ewmh_update_net_current_desktop(get_phys_screen(screen));
}

/** View next tag
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewnext(int screen, char *arg __attribute__ ((unused)))
{
    Tag **curtags = get_current_tags(screen);

    if(!curtags[0]->next)
        return;

    curtags[0]->selected = False;
    curtags[0]->next->selected = True;

    p_delete(&curtags);

    saveawesomeprops(screen);
    arrange(screen);
    ewmh_update_net_current_desktop(get_phys_screen(screen));
}

/** View previous tag
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewprev(int screen, char *arg __attribute__ ((unused)))
{
    Tag *tag, **curtags = get_current_tags(screen);

    for(tag = globalconf.screens[screen].tags; tag && tag->next != curtags[0]; tag = tag->next);
    if(tag)
    {
        tag->selected = True;
        curtags[0]->selected = False;
        saveawesomeprops(screen);
        arrange(screen);
    }
    p_delete(&curtags);
    ewmh_update_net_current_desktop(get_phys_screen(screen));
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
