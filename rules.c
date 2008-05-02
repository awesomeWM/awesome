/*
 * rules.c - rules management
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

#include "rules.h"
#include "common/xutil.h"

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

bool
tag_match_rule(tag_t *t, rule_t *r)
{
    regmatch_t tmp;

    if(r->tags_r && !regexec(r->tags_r, t->name, 1, &tmp, 0))
        return true;

    return false;
}

rule_t *
rule_matching_client(client_t *c)
{
    rule_t *r;
    char *prop = NULL, *buf = NULL;
    regmatch_t tmp;
    ssize_t len;
    class_hint_t *ch = NULL;
    bool ret = false;

    if(!(ch = xutil_get_class_hint(globalconf.connection, c->win)))
        return NULL;

    len = a_strlen(ch->res_class) + a_strlen(ch->res_name) + a_strlen(c->name) + 3;
    prop = p_new(char, len);

    snprintf(prop, len, "%s:%s:%s",
             ch->res_class ? ch->res_class : "", ch->res_name ? ch->res_name : "", c->name);

    p_delete(&ch->res_class);
    p_delete(&ch->res_name);
    p_delete(&ch);

    for(r = globalconf.rules; r; r = r->next)
    {
        if(r->prop_r && prop)
            ret = !regexec(r->prop_r, prop, 1, &tmp, 0);

        if(!ret
           && r->xprop
           && r->xpropval_r
           && xutil_gettextprop(globalconf.connection, c->win,
                                xutil_intern_atom(globalconf.connection, r->xprop),
                                &buf))
            ret = !regexec(r->xpropval_r, buf, 1, &tmp, 0);

        p_delete(&buf);

        if(ret)
        {
            p_delete(&prop);
            return r;
        }
    }

    p_delete(&prop);

    return NULL;
}

// vim: filetype=c:expandtab:shiftwidth=4:tabstop=8:softtabstop=4:encoding=utf-8:textwidth=80
