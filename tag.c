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

int
applyrules(Client *c, awesome_config *awesomeconf)
{
    int i, screen = RULE_NOSCREEN, len = 0;
    regmatch_t tmp;
    Bool matched = False;
    XClassHint ch = { 0, 0 };
    char *prop;
    Rule *r;

    XGetClassHint(c->display, c->win, &ch);

    len = a_strlen(ch.res_class) + a_strlen(ch.res_name) + a_strlen(c->name);

    prop = p_new(char, len + 3);

    /* rule matching */
    snprintf(prop, len + 3, "%s:%s:%s",
             ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "", c->name);
    for(r = awesomeconf->rules; r; r = r->next)
        if(r->propregex && !regexec(r->propregex, prop, 1, &tmp, 0))
        {
            c->isfloating = r->isfloating;
            if(r->screen != RULE_NOSCREEN && r->screen != awesomeconf->screen)
            {
                screen = r->screen;
                move_client_to_screen(c, &awesomeconf[r->screen - awesomeconf->screen], True);
            }
            /* we need to recompute awesomeconf index because we might have changed screen */
            for(i = 0; r->tagregex && i < awesomeconf[c->screen - awesomeconf->screen].ntags; i++)
                if(!regexec(r->tagregex, awesomeconf[c->screen - awesomeconf->screen].tags[i].name, 1, &tmp, 0))
                {
                    matched = True;
                    c->tags[i] = True;
                }
                else
                    c->tags[i] = False;
        }
    p_delete(&prop);
    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);
    if(!matched)
        for(i = 0; i < awesomeconf->ntags; i++)
            c->tags[i] = awesomeconf->tags[i].selected;

    return screen;
}

void
compileregs(Rule *rules)
{
    Rule *r;
    regex_t *reg;

    for(r = rules; r; r = r->next)
    {
        if(r->prop)
        {
            reg = p_new(regex_t, 1);
            if(regcomp(reg, r->prop, REG_EXTENDED))
                p_delete(&reg);
            else
                r->propregex = reg;
        }
        if(r->tags)
        {
            reg = p_new(regex_t, 1);
            if(regcomp(reg, r->tags, REG_EXTENDED))
                p_delete(&reg);
            else
                r->tagregex = reg;
        }
    }
}


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


/** Tag selected window with tag
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_tag(awesome_config *awesomeconf,
         const char *arg)
{
    int i;
    Client *sel = get_current_tag(awesomeconf->tags, awesomeconf->ntags)->client_sel;

    if(!sel)
        return;

    for(i = 0; i < awesomeconf->ntags; i++)
        sel->tags[i] = arg == NULL;

    i =  arg ? atoi(arg) - 1 : 0;

    if(i >= 0 && i < awesomeconf->ntags)
        sel->tags[i] = True;

    saveprops(sel, awesomeconf->ntags);
    arrange(awesomeconf);
}

/** Toggle floating state of a client
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_togglefloating(awesome_config * awesomeconf,
                    const char *arg __attribute__ ((unused)))
{
    Client *sel = get_current_tag(awesomeconf->tags, awesomeconf->ntags)->client_sel;

    if(!sel)
        return;

    sel->isfloating = !sel->isfloating;

    if (arg == NULL)
        client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, awesomeconf, True, False);
    else
        client_resize(sel, sel->x, sel->y, sel->w, sel->h, awesomeconf, True, True);

    saveprops(sel, awesomeconf->ntags);
    arrange(awesomeconf);
}

/** Toggle tag view
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_toggletag(awesome_config *awesomeconf,
               const char *arg)
{
    Client *sel = get_current_tag(awesomeconf->tags, awesomeconf->ntags)->client_sel;
    unsigned int i;
    int j;

    if(!sel)
        return;
    i = arg ? atoi(arg) - 1 : 0;
    sel->tags[i] = !sel->tags[i];
    for(j = 0; j < awesomeconf->ntags && !sel->tags[j]; j++);
    if(j == awesomeconf->ntags)
        sel->tags[i] = True;
    saveprops(sel, awesomeconf->ntags);
    arrange(awesomeconf);
}

/** Add a tag to viewed tags
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_toggleview(awesome_config *awesomeconf,
                const char *arg)
{
    unsigned int i;
    int j;

    i = arg ? atoi(arg) - 1: 0;
    awesomeconf->tags[i].selected = !awesomeconf->tags[i].selected;
    for(j = 0; j < awesomeconf->ntags && !awesomeconf->tags[j].selected; j++);
    if(j == awesomeconf->ntags)
        awesomeconf->tags[i].selected = True;
    saveawesomeprops(awesomeconf);
    arrange(awesomeconf);
}

/** View tag
 * \param awesomeconf awesome config ref
 * \param arg tag to view
 * \ingroup ui_callback
 */
void
uicb_view(awesome_config *awesomeconf,
          const char *arg)
{
    int i;

    for(i = 0; i < awesomeconf->ntags; i++)
    {
        awesomeconf->tags[i].was_selected = awesomeconf->tags[i].selected;
        awesomeconf->tags[i].selected = arg == NULL;
    }

    if(arg)
    {
        i = atoi(arg) - 1;
        if(i >= 0 && i < awesomeconf->ntags)
            awesomeconf->tags[i].selected = True;
    }

    saveawesomeprops(awesomeconf);
    arrange(awesomeconf);
}

/** View previously selected tags
 * \param awesomeconf awesome config ref
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_prev_selected(awesome_config *awesomeconf,
                       const char *arg __attribute__ ((unused)))
{
    int i;
    Bool t;

    for(i = 0; i < awesomeconf->ntags; i++)
    {
        t = awesomeconf->tags[i].selected;
        awesomeconf->tags[i].selected = awesomeconf->tags[i].was_selected;
        awesomeconf->tags[i].was_selected = t;
    }
    arrange(awesomeconf);
}

/** View next tag
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewnext(awesome_config *awesomeconf,
                  const char *arg __attribute__ ((unused)))
{
    int i;
    int firsttag = -1;

    for(i = 0; i < awesomeconf->ntags; i++)
    {
        if(firsttag < 0 && awesomeconf->tags[i].selected)
            firsttag = i;
        awesomeconf->tags[i].selected = False;
    }
    if(++firsttag >= awesomeconf->ntags)
        firsttag = 0;
    awesomeconf->tags[firsttag].selected = True;
    saveawesomeprops(awesomeconf);
    arrange(awesomeconf);
}

/** View previous tag
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewprev(awesome_config *awesomeconf,
                  const char *arg __attribute__ ((unused)))
{
    int i;
    int firsttag = -1;

    for(i = awesomeconf->ntags - 1; i >= 0; i--)
    {
        if(firsttag < 0 && awesomeconf->tags[i].selected)
            firsttag = i;
        awesomeconf->tags[i].selected = False;
    }
    if(--firsttag < 0)
        firsttag = awesomeconf->ntags - 1;
    awesomeconf->tags[firsttag].selected = True;
    saveawesomeprops(awesomeconf);
    arrange(awesomeconf);
}
// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
