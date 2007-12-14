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


static void
detach_tagclientlink(TagClientLink **head, TagClientLink *tc)
{
    TagClientLink *tmp;

    if(*head == tc)
        *head = tc->next;
    else
    {
        for(tmp = *head; tmp && tmp->next != tc; tmp = tmp->next);
        tmp->next = tc->next;
    }
    
    p_delete(&tc);
}

void
tag_client(TagClientLink **head, Client *c, Tag *t)
{
    TagClientLink *tc, *new_tc;

    /* don't tag twice */
    if(is_client_tagged(*head, c, t))
        return;

    new_tc = p_new(TagClientLink, 1);

    if(!*head)
        *head = new_tc;
    else
    {
        for(tc = *head; tc->next; tc = tc->next);
        tc->next = new_tc;
    }

    new_tc->client = c;
    new_tc->tag = t;
}

void
untag_client(TagClientLink **head, Client *c, Tag *t)
{
    TagClientLink *tc;

    for(tc = *head; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
            detach_tagclientlink(head, tc);
}

Bool
is_client_tagged(TagClientLink *head, Client *c, Tag *t)
{
    TagClientLink *tc;

    for(tc = head; tc; tc = tc->next)
        if(tc->client == c && tc->tag == t)
            return True;
    return False;
}

void
tag_client_with_current_selected(Client *c, VirtScreen *screen)
{
    Tag *tag;

    for(tag = screen->tags; tag; tag = tag->next)
        if(tag->selected)
            tag_client(&screen->tclink, c, tag);
        else
            untag_client(&screen->tclink, c, tag);
}

void
tag_client_with_rules(Client *c, awesome_config *awesomeconf)
{
    Rule *r;
    Tag *tag;
    Bool matched = False;

    for(r = awesomeconf->rules; r; r = r->next)
        if(client_match_rule(c, r))
        {
            c->isfloating = r->isfloating;

            if(r->screen != RULE_NOSCREEN && r->screen != c->screen)
                move_client_to_screen(c, awesomeconf, r->screen, True);

            for(tag = awesomeconf->screens[c->screen].tags; tag; tag = tag->next)
                if(is_tag_match_rules(tag, r))
                {
                    matched = True;
                    tag_client(&awesomeconf->screens[c->screen].tclink, c, tag);
                }

            if(!matched)
                tag_client_with_current_selected(c, &awesomeconf->screens[c->screen]);
            break;
        }
}

/** Tag selected window with tag
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_client_tag(awesome_config *awesomeconf,
                int screen,
                const char *arg)
{
    int tag_id = -1;
    Tag *tag, *target_tag;
    Client *sel = awesomeconf->focus->client;

    if(!sel)
        return;

    if(arg)
    {
	tag_id = atoi(arg) - 1;
        if(tag_id != -1)
        {
            for(target_tag = awesomeconf->screens[screen].tags; target_tag && tag_id > 0;
                target_tag = target_tag->next, tag_id--);
            if(target_tag)
            {
                for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
                    untag_client(&awesomeconf->screens[screen].tclink, sel, tag);
                tag_client(&awesomeconf->screens[screen].tclink, sel, target_tag);
            }
        }
    }
    else
        for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
            tag_client(&awesomeconf->screens[screen].tclink, sel, tag);

    client_saveprops(sel, &awesomeconf->screens[screen]);
    arrange(awesomeconf, screen);
}

/** Toggle floating state of a client
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_client_togglefloating(awesome_config * awesomeconf,
                           int screen,
                    const char *arg __attribute__ ((unused)))
{
    Client *sel = awesomeconf->focus->client;
    
    if(!sel)
        return;

    sel->isfloating = !sel->isfloating;

    if (arg == NULL)
        client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, awesomeconf, True, False);
    else
        client_resize(sel, sel->x, sel->y, sel->w, sel->h, awesomeconf, True, True);

    client_saveprops(sel, &awesomeconf->screens[screen]);
    arrange(awesomeconf, screen);
}

/** Toggle a tag on client
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_client_toggletag(awesome_config *awesomeconf,
                      int screen,
                      const char *arg)
{
    Client *sel = awesomeconf->focus->client;
    int i;
    Tag *tag, *target_tag;

    if(!sel)
        return;

    if(arg)
    {
        i = atoi(arg) - 1;
        for(target_tag = awesomeconf->screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);
        if(target_tag)
        {
            if(is_client_tagged(awesomeconf->screens[screen].tclink, sel, target_tag))
                untag_client(&awesomeconf->screens[screen].tclink, sel, target_tag);
            else
                tag_client(&awesomeconf->screens[screen].tclink, sel, target_tag);
        }

        /* check that there's at least one tag selected for this client*/
        for(tag = awesomeconf->screens[screen].tags; tag
            && !is_client_tagged(awesomeconf->screens[screen].tclink, sel, tag); tag = tag->next)

        if(!tag)
            tag_client(&awesomeconf->screens[screen].tclink, sel, target_tag);
    }
    else
        for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
            if(is_client_tagged(awesomeconf->screens[screen].tclink, sel, tag))
                tag_client(&awesomeconf->screens[screen].tclink, sel, tag);
            else
                untag_client(&awesomeconf->screens[screen].tclink, sel, tag);

    client_saveprops(sel, &awesomeconf->screens[screen]);
    arrange(awesomeconf, screen);
}

/** Add a tag to viewed tags
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_tag_toggleview(awesome_config *awesomeconf,
                    int screen,
                    const char *arg)
{
    int i;
    Tag *tag, *target_tag;

    if(arg)
    {
        i = atoi(arg) - 1;
        for(target_tag = awesomeconf->screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);

        if(target_tag)
            target_tag->selected = !target_tag->selected;

        /* check that there's at least one tag selected */
        for(tag = awesomeconf->screens[screen].tags; tag && !tag->selected; tag = tag->next);
        if(!tag)
            target_tag->selected = True;
    }
    else
        for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
            tag->selected = !tag->selected;

    saveawesomeprops(awesomeconf, screen);
    arrange(awesomeconf, screen);
}

/** View tag
 * \param awesomeconf awesome config ref
 * \param arg tag to view
 * \ingroup ui_callback
 */
void
uicb_tag_view(awesome_config *awesomeconf,
              int screen,
              const char *arg)
{
    int i;
    Tag *tag, *target_tag;

    if(arg)
    {
	i = atoi(arg) - 1;
        for(target_tag = awesomeconf->screens[screen].tags; target_tag && i > 0;
            target_tag = target_tag->next, i--);
        if(target_tag)
        {
            for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
                tag->selected = False;
            target_tag->selected = True;
        }
    }
    else
        for(tag = awesomeconf->screens[screen].tags; tag; tag = tag->next)
            tag->selected = True;

    saveawesomeprops(awesomeconf, screen);
    arrange(awesomeconf, screen);
}

/** View previously selected tags
 * \param awesomeconf awesome config ref
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_prev_selected(awesome_config *awesomeconf,
                       int screen,
                       const char *arg __attribute__ ((unused)))
{
    Tag *tag;
    Bool t;

    for(tag =  awesomeconf->screens[screen].tags; tag; tag = tag->next)
    {
        t = tag->selected;
        tag->selected = tag->was_selected;
        tag->was_selected = t;
    }
    arrange(awesomeconf, screen);
}

/** View next tag
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewnext(awesome_config *awesomeconf,
                  int screen,
                  const char *arg __attribute__ ((unused)))
{
    Tag *curtag = get_current_tag(awesomeconf->screens[screen]);

    curtag->selected = False;
    curtag->next->selected = True;

    saveawesomeprops(awesomeconf, screen);
    arrange(awesomeconf, screen);
}

/** View previous tag
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewprev(awesome_config *awesomeconf,
                  int screen,
                  const char *arg __attribute__ ((unused)))
{
    Tag *tag, *curtag = get_current_tag(awesomeconf->screens[screen]);

    for(tag = awesomeconf->screens[screen].tags - 1; tag && tag->next != curtag; tag = tag->next);
    if(tag)
    {
        tag->selected = True;
        curtag->selected = False;
    }

    saveawesomeprops(awesomeconf, screen);
    arrange(awesomeconf, screen);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
