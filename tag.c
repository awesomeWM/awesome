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

#include "layout.h"
#include "tag.h"
#include "tab.h"
#include "util.h"

/** This function returns the index of
 * the tag given un argument in *tags
 * \param tag_to_find tag name
 * \param tags tag list
 * \param ntags number of tags in tag list
 * \return index of tag
 */
static int
idxoftag(const char *tag_to_find, Tag *tags, int ntags)
{
    int i;

    if(!tag_to_find)
        return 0;

    for(i = 0; i < ntags; i++)
        if(!a_strcmp(tags[i].name, tag_to_find))
            return i;

    return 0;
}

void
applyrules(Client * c, awesome_config *awesomeconf)
{
    int i, j, len = 0;
    regmatch_t tmp;
    Bool matched = False;
    XClassHint ch = { 0, 0 };
    char *prop;

    XGetClassHint(c->display, c->win, &ch);

    len = a_strlen(ch.res_class) + a_strlen(ch.res_name) + a_strlen(c->name);

    prop = p_new(char, len + 3);

    /* rule matching */
    snprintf(prop, len + 3, "%s:%s:%s",
             ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "", c->name);
    for(i = 0; i < awesomeconf->nrules; i++)
        if(awesomeconf->rules[i].propregex && !regexec(awesomeconf->rules[i].propregex, prop, 1, &tmp, 0))
        {
            c->isfloating = awesomeconf->rules[i].isfloating;
            for(j = 0; awesomeconf->rules[i].tagregex && j < awesomeconf->ntags; j++)
                if(!regexec(awesomeconf->rules[i].tagregex, awesomeconf->tags[j].name, 1, &tmp, 0))
                {
                    matched = True;
                    c->tags[j] = True;
                }
                else
                    c->tags[j] = False;
        }
    p_delete(&prop);
    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);
    if(!matched)
        for(i = 0; i < awesomeconf->ntags; i++)
            c->tags[i] = awesomeconf->tags[i].selected;
}

void
compileregs(Rule * rules, int nrules)
{
    int i;
    regex_t *reg;

    for(i = 0; i < nrules; i++)
    {
        if(rules[i].prop)
        {
            reg = p_new(regex_t, 1);
            if(regcomp(reg, rules[i].prop, REG_EXTENDED))
                p_delete(&reg);
            else
                rules[i].propregex = reg;
        }
        if(rules[i].tags)
        {
            reg = p_new(regex_t, 1);
            if(regcomp(reg, rules[i].tags, REG_EXTENDED))
                p_delete(&reg);
            else
                rules[i].tagregex = reg;
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

    if(c->screen != screen || !c->tab.isvisible)
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
    i = idxoftag(arg, awesomeconf->tags, awesomeconf->ntags);
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

    client_resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, awesomeconf, True);

    client_untab(sel);
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
    i = idxoftag(arg, awesomeconf->tags, awesomeconf->ntags);
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

    i = idxoftag(arg, awesomeconf->tags, awesomeconf->ntags);
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

    i = idxoftag(arg, awesomeconf->tags, awesomeconf->ntags);

    if(i >= 0 && i < awesomeconf->ntags)
        awesomeconf->tags[i].selected = True;

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
