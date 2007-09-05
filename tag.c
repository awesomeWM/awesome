/* See LICENSE file for copyright and license details. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>

#include "util.h"
#include "layout.h"
#include "tag.h"

extern Client *sel;             /* global client list */
extern Bool *seltags, *prevtags;
extern Layout ** taglayouts;

static Regs *regs = NULL;
static char prop[512];

/** This function returns the index of
 * the tag given un argument in *tags
 * \param tag_to_find tag name
 * \return index of tag
 */
static int
idxoftag(const char *tag_to_find, const char **tags, int ntags)
{
    int i;

    if(!tag_to_find)
        return 0;

    for(i = 0; i < ntags; i++)
        if(!strcmp(tags[i], tag_to_find))
            return i;

    return 0;
}

void
applyrules(Client * c, jdwm_config *jdwmconf)
{
    int i, j;
    regmatch_t tmp;
    Bool matched = False;
    XClassHint ch = { 0, 0 };

    /* rule matching */
    XGetClassHint(c->display, c->win, &ch);
    snprintf(prop, sizeof(prop), "%s:%s:%s",
             ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "", c->name);
    for(i = 0; i < jdwmconf->nrules; i++)
        if(regs[i].propregex && !regexec(regs[i].propregex, prop, 1, &tmp, 0))
        {
            c->isfloating = jdwmconf->rules[i].isfloating;
            for(j = 0; regs[i].tagregex && j < jdwmconf->ntags; j++)
                if(!regexec(regs[i].tagregex, jdwmconf->tags[j], 1, &tmp, 0))
                {
                    matched = True;
                    c->tags[j] = True;
                }
        }
    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);
    if(!matched)
        for(i = 0; i < jdwmconf->ntags; i++)
            c->tags[i] = seltags[i];
}

void
compileregs(jdwm_config * jdwmconf)
{
    int i;
    regex_t *reg;

    if(regs)
        return;
    regs = p_new(Regs, jdwmconf->nrules);
    for(i = 0; i < jdwmconf->nrules; i++)
    {
        if(jdwmconf->rules[i].prop)
        {
            reg = p_new(regex_t, 1);
            if(regcomp(reg, jdwmconf->rules[i].prop, REG_EXTENDED))
                p_delete(&reg);
            else
                regs[i].propregex = reg;
        }
        if(jdwmconf->rules[i].tags)
        {
            reg = p_new(regex_t, 1);
            if(regcomp(reg, jdwmconf->rules[i].tags, REG_EXTENDED))
                p_delete(&reg);
            else
                regs[i].tagregex = reg;
        }
    }
}


/** Returns True if a client is visible on
 * one of the currently selected tag, false otherwise.
 * \param c Client
 * \return True or False
 */
Bool
isvisible(Client * c, int ntags)
{
    int i;

    for(i = 0; i < ntags; i++)
        if(c->tags[i] && seltags[i])
            return True;
    return False;
}


/** Tag selected window with tag
 * \param disp Display ref
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_tag(Display *disp, jdwm_config *jdwmconf, const char *arg)
{
    int i;

    if(!sel)
        return;
    for(i = 0; i < jdwmconf->ntags; i++)
        sel->tags[i] = arg == NULL;
    i = idxoftag(arg, jdwmconf->tags, jdwmconf->ntags);
    if(i >= 0 && i < jdwmconf->ntags)
        sel->tags[i] = True;
    saveprops(sel, jdwmconf->ntags);
    arrange(disp, jdwmconf);
}

/** Toggle floating state of a client
 * \param disp Display ref
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_togglefloating(Display *disp,
                    jdwm_config * jdwmconf,
                    const char *arg __attribute__ ((unused)))
{
    if(!sel)
        return;
    sel->isfloating = !sel->isfloating;
    if(sel->isfloating)
        /*restore last known float dimensions*/
        resize(sel, sel->rx, sel->ry, sel->rw, sel->rh, True);
    else
    {
        /*save last known float dimensions*/
        sel->rx = sel->x;
        sel->ry = sel->y;
        sel->rw = sel->w;
        sel->rh = sel->h;
    }
    saveprops(sel, jdwmconf->ntags);
    arrange(disp, jdwmconf);
}

/** Toggle tag view
 * \param disp Display ref
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_toggletag(Display *disp,
               jdwm_config *jdwmconf,
               const char *arg)
{
    unsigned int i;
    int j;

    if(!sel)
        return;
    i = idxoftag(arg, jdwmconf->tags, jdwmconf->ntags);
    sel->tags[i] = !sel->tags[i];
    for(j = 0; j < jdwmconf->ntags && !sel->tags[j]; j++);
    if(j == jdwmconf->ntags)
        sel->tags[i] = True;
    saveprops(sel, jdwmconf->ntags);
    arrange(disp, jdwmconf);
}

/** Add a tag to viewed tags
 * \param disp Display ref
 * \param arg Tag name
 * \ingroup ui_callback
 */
void
uicb_toggleview(Display *disp,
                jdwm_config *jdwmconf,
                const char *arg)
{
    unsigned int i;
    int j;

    i = idxoftag(arg, jdwmconf->tags, jdwmconf->ntags);
    seltags[i] = !seltags[i];
    for(j = 0; j < jdwmconf->ntags && !seltags[j]; j++);
    if(j == jdwmconf->ntags)
        seltags[i] = True;      /* cannot toggle last view */
    savejdwmprops(disp, jdwmconf);
    arrange(disp, jdwmconf);
}

/**
 * \param disp Display ref
 * \param arg
 * \ingroup ui_callback
 */
void
uicb_view(Display *disp,
          jdwm_config *jdwmconf,
          const char *arg)
{
    int i;

    for(i = 0; i < jdwmconf->ntags; i++)
    {
        prevtags[i] = seltags[i];
        seltags[i] = arg == NULL;
    }
    i = idxoftag(arg, jdwmconf->tags, jdwmconf->ntags);
    if(i >= 0 && i < jdwmconf->ntags)
    {
        seltags[i] = True;
        jdwmconf->current_layout = taglayouts[i];
    }
    savejdwmprops(disp, jdwmconf);
    arrange(disp, jdwmconf);
}

/** View previously selected tags
 * \param disp Display ref
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_viewprevtags(Display * disp,
                  jdwm_config *jdwmconf,
                  const char *arg __attribute__ ((unused)))
{
    int i;
    Bool t;

    for(i = 0; i < jdwmconf->ntags; i++)
    {
        t = seltags[i];
        seltags[i] = prevtags[i];
        prevtags[i] = t;
    }
    arrange(disp, jdwmconf);
}

/** View next tag
 * \param disp Display ref
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewnext(Display *disp,
                  jdwm_config *jdwmconf,
                  const char *arg __attribute__ ((unused)))
{
    int i;
    int firsttag = -1;

    for(i = 0; i < jdwmconf->ntags; i++)
    {
        if(firsttag < 0 && seltags[i])
            firsttag = i;
        seltags[i] = False;
    }
    if(++firsttag >= jdwmconf->ntags)
        firsttag = 0;
    seltags[firsttag] = True;
    savejdwmprops(disp, jdwmconf);
    arrange(disp, jdwmconf);
}

/** View previous tag
 * \param disp Display ref
 * \param arg unused
 * \ingroup ui_callback
 */
void
uicb_tag_viewprev(Display *disp,
                  jdwm_config *jdwmconf,
                  const char *arg __attribute__ ((unused)))
{
    int i;
    int firsttag = -1;

    for(i = jdwmconf->ntags - 1; i >= 0; i--)
    {
        if(firsttag < 0 && seltags[i])
            firsttag = i;
        seltags[i] = False;
    }
    if(--firsttag < 0)
        firsttag = jdwmconf->ntags - 1;
    seltags[firsttag] = True;
    savejdwmprops(disp, jdwmconf);
    arrange(disp, jdwmconf);
}
