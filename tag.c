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

/** Returns True if a client is tagged
 * with one of the tags
 * \param c Client
 * \param tags tag to check
 * \param ntags number of tags in *tags
 * \return True or False
 */
Bool
isvisible(Client * c, int screen, Tag * tags, int ntags)
{
    int i;

    if(c->screen != screen)
        return False;

    for(i = 0; i < ntags; i++)
        if(c->tags[i] && tags[i].selected)
            return True;
    return False;
}

void
tag_client_with_current_selected(Client *c, VirtScreen *screen)
{
    int i;

    p_realloc(&c->tags, screen->ntags);
    for(i = 0; i < screen->ntags; i++)
        c->tags[i] = screen->tags[i].selected;
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
    int i, tag_id = -1;
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;

    if(!sel)
        return;

    if(arg)
    {
	tag_id = atoi(arg) - 1;
	if(tag_id < 0 || tag_id >= awesomeconf->screens[screen].ntags)
	    return;
    }

    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
        sel->tags[i] = arg == NULL;

    if(tag_id != -1)
        sel->tags[tag_id] = True;

    saveprops(sel, awesomeconf->screens[screen].ntags);
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
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;

    if(!sel)
        return;

    sel->isfloating = !sel->isfloating;

    if (arg == NULL)
        client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, awesomeconf, True, False);
    else
        client_resize(sel, sel->x, sel->y, sel->w, sel->h, awesomeconf, True, True);

    saveprops(sel, awesomeconf->screens[screen].ntags);
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
    Client *sel = get_current_tag(awesomeconf->screens[screen])->client_sel;
    int i, j;

    if(!sel)
        return;

    if(arg)
    {
        i = atoi(arg) - 1;

        if(i < 0 || i >= awesomeconf->screens[screen].ntags)
            return;

        sel->tags[i] = !sel->tags[i];

        /* check that there's at least one tag selected for this client*/
        for(j = 0; j < awesomeconf->screens[screen].ntags && !sel->tags[j]; j++);
            if(j == awesomeconf->screens[screen].ntags)
                sel->tags[i] = True;
    }
    else
        for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
            sel->tags[i] = True;

    saveprops(sel, awesomeconf->screens[screen].ntags);
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
    int i, j;

    i = arg ? atoi(arg) - 1: 0;

    if(i < 0 || i >= awesomeconf->screens[screen].ntags)
        return;

    awesomeconf->screens[screen].tags[i].selected = !awesomeconf->screens[screen].tags[i].selected;

    /* check that there's at least one tag selected */
    for(j = 0; j < awesomeconf->screens[screen].ntags && !awesomeconf->screens[screen].tags[j].selected; j++);
    if(j == awesomeconf->screens[screen].ntags)
        awesomeconf->screens[screen].tags[i].selected = True;

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
    int i, tag_id = -1;

    if(arg)
    {
	tag_id = atoi(arg) - 1;
	if(tag_id < 0 || tag_id >= awesomeconf->screens[screen].ntags)
	    return;
    }

    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
    {
        awesomeconf->screens[screen].tags[i].was_selected = awesomeconf->screens[screen].tags[i].selected;
        awesomeconf->screens[screen].tags[i].selected = arg == NULL;
    }

    if(tag_id != -1)
	awesomeconf->screens[screen].tags[tag_id].selected = True;

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
    int i;
    Bool t;

    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
    {
        t = awesomeconf->screens[screen].tags[i].selected;
        awesomeconf->screens[screen].tags[i].selected = awesomeconf->screens[screen].tags[i].was_selected;
        awesomeconf->screens[screen].tags[i].was_selected = t;
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
    int i;
    int firsttag = -1;

    for(i = 0; i < awesomeconf->screens[screen].ntags; i++)
    {
        if(firsttag < 0 && awesomeconf->screens[screen].tags[i].selected)
            firsttag = i;
        awesomeconf->screens[screen].tags[i].selected = False;
    }
    if(++firsttag >= awesomeconf->screens[screen].ntags)
        firsttag = 0;
    awesomeconf->screens[screen].tags[firsttag].selected = True;
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
    int i;
    int firsttag = -1;

    for(i = awesomeconf->screens[screen].ntags - 1; i >= 0; i--)
    {
        if(firsttag < 0 && awesomeconf->screens[screen].tags[i].selected)
            firsttag = i;
        awesomeconf->screens[screen].tags[i].selected = False;
    }
    if(--firsttag < 0)
        firsttag = awesomeconf->screens[screen].ntags - 1;
    awesomeconf->screens[screen].tags[firsttag].selected = True;
    saveawesomeprops(awesomeconf, screen);
    arrange(awesomeconf, screen);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
