/*
 * rules.c - rules management
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

#include "rules.h"
#include "xutil.h"

extern AwesomeConf globalconf;

regex_t *
rules_compile_regex(char *val)
{
    regex_t *reg;

    if(val)
    {
        reg = p_new(regex_t, 1);
        if(regcomp(reg, val, REG_EXTENDED))
            p_delete(&reg);
        else
            return reg;
    }

    return NULL;
}

static Bool
client_match_rule(Client *c, Rule *r)
{
    char *prop, buf[512];
    regmatch_t tmp;
    ssize_t len;
    XClassHint ch = { 0, 0 };
    Bool ret = False;

    if(r->prop_r)
    {
        /* first try to match on name */
        XGetClassHint(globalconf.display, c->win, &ch);

        len = a_strlen(ch.res_class) + a_strlen(ch.res_name) + a_strlen(c->name);
        prop = p_new(char, len + 3);

        /* rule matching */
        snprintf(prop, len + 3, "%s:%s:%s",
                 ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "", c->name);

        ret = !regexec(r->prop_r, prop, 1, &tmp, 0);

        p_delete(&prop);

        if(ch.res_class)
            XFree(ch.res_class);
        if(ch.res_name)
            XFree(ch.res_name);

        if(ret)
            return True;
    }

    if(r->xprop
       && r->xpropval_r
       && xgettextprop(c->win,
                       XInternAtom(globalconf.display, r->xprop, False),
                       buf, ssizeof(buf)))
        ret = !regexec(r->xpropval_r, buf, 1, &tmp, 0);

    return ret;
}

Bool
tag_match_rule(Tag *t, Rule *r)
{
    regmatch_t tmp;

    if(r->tags_r && !regexec(r->tags_r, t->name, 1, &tmp, 0))
        return True;

    return False;
}

Rule *
rule_matching_client(Client *c)
{
    Rule *r;
    for(r = globalconf.rules; r; r = r->next)
        if(client_match_rule(c, r))
            return r;

    return NULL;
}

Fuzzy
rules_get_fuzzy_from_str(const char *str)
{
    if(!a_strcmp(str, "true") || !a_strcmp(str, "yes"))
        return Yes;
    else if(!a_strcmp(str, "false") || !a_strcmp(str, "no"))
        return No;

    return Auto;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
