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

#include "util.h"
#include "rules.h"

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

Bool
client_match_rule(Client *c, Rule *r)
{
    char *prop;
    regmatch_t tmp;
    ssize_t len;
    XClassHint ch = { 0, 0 };
    Bool ret;

    XGetClassHint(c->display, c->win, &ch);

    len = a_strlen(ch.res_class) + a_strlen(ch.res_name) + a_strlen(c->name);
    prop = p_new(char, len + 3);

    /* rule matching */
    snprintf(prop, len + 3, "%s:%s:%s",
             ch.res_class ? ch.res_class : "", ch.res_name ? ch.res_name : "", c->name);

    ret = (r->propregex && !regexec(r->propregex, prop, 1, &tmp, 0));

    p_delete(&prop);

    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);

    return ret;
}

int
get_client_screen_from_rules(Client *c, Rule *rules)
{
    Rule *r;

    for(r = rules; r; r = r->next)
        if(client_match_rule(c, r))
            return r->screen;

    return RULE_NOSCREEN;
}

Bool
is_tag_match_rules(Tag *t, Rule *r)
{
    regmatch_t tmp;

    if(r->tagregex && !regexec(r->tagregex, t->name, 1, &tmp, 0))
        return True;

    return False;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=99
