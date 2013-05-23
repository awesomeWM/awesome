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

/** Create a new tag. Parameteres values are checked.
 * \param name tag name
 * \param layout layout to use
 * \param mwfact master width factor
 * \param nmaster number of master windows
 * \param ncol number of columns for slaves windows
 * \return a new tag with all these parameters
 */
Tag *
tag_new(const char *name, Layout *layout, double mwfact, int nmaster, int ncol)
{
    Tag *tag;

    tag = p_new(Tag, 1);
    tag->name = a_strdup(name);
    tag->layout = layout;

    tag->mwfact = mwfact;
    if(tag->mwfact <= 0 || tag->mwfact >= 1)
        tag->mwfact = 1.0 / 1.618; /* Reverse of the golden ratio */

    if((tag->nmaster = nmaster) < 0)
        tag->nmaster = 1;

    if((tag->ncol = ncol) < 1)
        tag->ncol = 1;

    return tag;
}

/** Append a tag to a screen.
 * \param tag the tag to append
 * \param screen the screen id
 */
void
tag_append_to_screen(Tag *tag, int screen)
{
    int phys_screen = screen_virttophys(screen);

    tag->screen = screen;
    tag_list_append(&globalconf.screens[screen].tags, tag);
    ewmh_update_net_numbers_of_desktop(phys_screen);
    ewmh_update_net_desktop_names(phys_screen);
    widget_invalidate_cache(screen, WIDGET_CACHE_TAGS);
}

/** Push a tag to a screen tag list.
 * \param tag the tag to push
 * \param screen the screen
 */
void
tag_push_to_screen(Tag *tag, int screen)
{
    tag->screen = screen;
    tag_list_push(&globalconf.screens[screen].tags, tag);
    widget_invalidate_cache(screen, WIDGET_CACHE_TAGS);
}

/** Tag a client with specified tag.
 * \param c the client to tag
 * \param t the tag to tag the client with
 */
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

/** Untag a client with specified tag.
 * \param c the client to tag
 * \param t the tag to tag the client with
 */
void
untag_client(Client *c, Tag *t)
{
    tag_client_node_t *tc;

    for(tc = globalconf.tclink; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
        {
            tag_client_node_list_detach(&globalconf.tclink, tc);
            p_delete(&tc);
            client_saveprops(c);
            widget_invalidate_cache(c->screen, WIDGET_CACHE_CLIENTS);
            globalconf.screens[c->screen].need_arrange = True;
            break;
        }
}

/** Check if a client is tagged with the specified tag.
 * \param c the client
 * \param t the tag
 * \return true if the client is tagged with the tag, false otherwise.
 */
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

/** Tag the client with the currently selected (visible) tags.
 * \param c the client
 */
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

/** Tag the client according to a rule.
 * \param c the client
 * \param r the rule
 */
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

/** Get the current tags for the specified screen.
 * Returned pointer must be p_delete'd after.
 * \param screen screen id
 * \return a double pointer of tag list finished with a NULL element
 */
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

/** Tag the focused client with the given tag.
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

/** Toggle a tag on the focused client.
 * \param screen virtual screen id
 * \param arg tag number
 * \ingroup ui_callback
 */
void
uicb_client_toggletag(int screen, char *arg)
{
    Client *sel = globalconf.focus->client;
    int i;
    Bool is_sticky = True;
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
    {
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            if(!is_client_tagged(sel, tag))
            {
                is_sticky = False;
                tag_client(sel, tag);
            }
        if(is_sticky)
            tag_client_with_current_selected(sel);
    }
}

/** Toggle the visibility of a tag.
 * \param screen Screen ID
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_tag_toggleview(int screen, char *arg)
{
    int i;
    Tag *tag, *target_tag,
        **backtag, **curtags = tags_get_current(screen);

    if(arg)
    {
        i = atoi(arg) - 1;
        for(target_tag = globalconf.screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);

        if(target_tag)
            tag_view(target_tag, !target_tag->selected);
    }
    else
        for(tag = globalconf.screens[screen].tags; tag; tag = tag->next)
            tag_view(tag, !tag->selected);

    /* check that there's at least one tag selected */
    for(tag = globalconf.screens[screen].tags; tag && !tag->selected; tag = tag->next);
    if(!tag)
        for(backtag = curtags; *backtag; backtag++)
            tag_view(*backtag, True);

    p_delete(&curtags);
}

/** Set a tag to be the only one viewed.
 * \param target the tag to see
 */
static void
tag_view_only(Tag *target)
{
    Tag *tag;

    if(!target) return;

    for(tag = globalconf.screens[target->screen].tags; tag; tag = tag->next)
        tag_view(tag, tag == target);
}

/** Use an index to set a tag viewable.
 * \param screen the screen id
 * \param dindex the index
 * \param view the view value
 */
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

/** View only a tag, selected by its index.
 * \param screen screen id
 * \param dindex the index
 */
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

/** View or unview a tag.
 * \param tag the tag
 * \param view set visible or not
 */
void
tag_view(Tag *tag, Bool view)
{
    tag->was_selected = tag->selected;
    tag->selected = view;
    ewmh_update_net_current_desktop(screen_virttophys(tag->screen));
    widget_invalidate_cache(tag->screen, WIDGET_CACHE_TAGS);
    globalconf.screens[tag->screen].need_arrange = True;
    client_focus(NULL, tag->screen, True);
}

/** View only this tag.
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

/** View the previously selected tags.
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

/** View the next tag.
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

/** View the previous tag.
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

/** Create a new tag. Argument must be the tag name.
 * \param screen the screen id
 * \param arg the tag name
 */
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
