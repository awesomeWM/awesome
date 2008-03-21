/*
 * tag.c - tag management
 *
 * Copyright © 2007-2008 Julien Danjou <julien@danjou.info>
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
#include "tag.h"
#include "rules.h"
#include "client.h"
#include "ewmh.h"
#include "widget.h"

extern AwesomeConf globalconf;

Tag *
tag_new(const char *name, Layout *layout, double mwfact, int nmaster, int ncol)
{
    Tag *tag;

    tag = p_new(Tag, 1);
    tag->name = a_strdup(name);
    tag->layout = layout;

    tag->mwfact = mwfact;
    if(tag->mwfact <= 0 || tag->mwfact >= 1)
        tag->mwfact = 0.5;

    if((tag->nmaster = nmaster) < 0)
        tag->nmaster = 1;

    if((tag->ncol = ncol) < 1)
        tag->ncol = 1;

    return tag;
}

static void
tag_append_to_screen(Tag *tag, int screen)
{
    tag->screen = screen;
    tag_list_append(&globalconf.screens[screen].tags, tag);
    widget_invalidate_cache(screen, WIDGET_CACHE_TAGS);
}

void
tag_push_to_screen(Tag *tag, int screen)
{
    tag->screen = screen;
    tag_list_push(&globalconf.screens[screen].tags, tag);
    widget_invalidate_cache(screen, WIDGET_CACHE_TAGS);
}

void
tag_client(Client *c, Tag *t)
{
    tag_client_node_t *tc;

    /* don't tag twice */
    if(is_client_tagged(c, t))
        return;

    tc = p_new(tag_client_node_t, 1);
    tc->client = c;
    tc->tag = t;
    tag_client_node_list_push(&globalconf.tclink, tc);

    client_saveprops(c);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    globalconf.screens[c->screen].need_arrange = True;
}

void
untag_client(Client *c, Tag *t)
{
    tag_client_node_t *tc;

    for(tc = globalconf.tclink; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
        {
            tag_client_node_list_detach(&globalconf.tclink, tc);
            p_delete(&tc);
            break;
        }

    client_saveprops(c);
    widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
    globalconf.screens[c->screen].need_arrange = True;
}

Bool
is_client_tagged(Client *c, Tag *t)
{
    tag_client_node_t *tc;

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
tag_client_with_rule(Client *c, Rule *r)
{
    Tag *tag;
    Bool matched = False;

    if(!r) return;

    /* check if at least one tag match */
    for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
        if(tag_match_rule(tag, r))
        {
            matched = True;
            break;
        }

    if(matched)
        for(tag = globalconf.screens[c->screen].tags; tag; tag = tag->next)
            if(tag_match_rule(tag, r))
                tag_client(c, tag);
            else
                untag_client(c, tag);
}

Tag **
tags_get_current(int screen)
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
            && !is_client_tagged(sel, tag); tag = tag->next);

        if(!tag)
            tag_client(sel, target_tag);
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            if(is_client_tagged(sel, tag))
                tag_client(sel, tag);
            else
                untag_client(sel, tag);
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
            tag_view(target_tag, !target_tag->selected);

        /* check that there's at least one tag selected */
        for(tag = globalconf.screens[screen].tags; tag && !tag->selected; tag = tag->next);
        if(!tag)
            tag_view(target_tag, True);
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag_view(tag, !tag->selected);
}

static void
tag_view_only(Tag *target)
{
    Tag *tag;

    if(!target) return;

    for(tag = globalconf.screens[target->screen].tags; tag; tag = tag->next)
        tag_view(tag, tag == target);
}

void
tag_view_byindex(int screen, int dindex, Bool view)
{
    Tag *tag;

    if(dindex < 0)
        return;

    for(tag = globalconf.screens[screen].tags; tag && dindex > 0;
        tag = tag->next, dindex--);
    tag_view(tag, view);
}

void
tag_view_only_byindex(int screen, int dindex)
{
    Tag *tag;

    if(dindex < 0)
        return;

    for(tag = globalconf.screens[screen].tags; tag && dindex > 0;
        tag = tag->next, dindex--);
    tag_view_only(tag);
}

void
tag_view(Tag *tag, Bool view)
{
    tag->was_selected = tag->selected;
    tag->selected = view;
    ewmh_update_net_current_desktop(screen_virttophys(tag->screen));
    widget_invalidate_cache(tag->screen, WIDGET_CACHE_TAGS);
    globalconf.screens[tag->screen].need_arrange = True;
}

/** View tag
 * \param screen Screen ID
 * \param arg tag to view
 * \ingroup ui_callback
 */
void
uicb_tag_view(int screen, char *arg)
{
    Tag *tag;

    if(arg)
	tag_view_only_byindex(screen, atoi(arg) - 1);
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag_view(tag, True);
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

    for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
        tag_view(tag, tag->was_selected);
}

/** View next tag
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewnext(int screen, char *arg __attribute__ ((unused)))
{
    Tag *tag, **curtags = tags_get_current(screen);

    tag = tag_list_next_cycle(&globalconf.screens[screen].tags, curtags[0]);

    tag_view(curtags[0], False);
    tag_view(tag, True);

    p_delete(&curtags);
}

/** View previous tag
 * \param screen Screen ID
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewprev(int screen, char *arg __attribute__ ((unused)))
{
    Tag *tag, **curtags = tags_get_current(screen);

    tag = tag_list_prev_cycle(&globalconf.screens[screen].tags, curtags[0]);

    tag_view(curtags[0], False);
    tag_view(tag, True);

    p_delete(&curtags);
}

void
uicb_tag_create(int screen, char *arg)
{
    Tag *tag;

    if(!a_strlen(arg))
        return;

    tag = tag_new(arg, globalconf.screens[screen].layouts, 0.5, 1, 1);
    tag_append_to_screen(tag, screen);
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
