/*  
 * tag.h - tag management header
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

#ifndef AWESOME_TAG_H
#define AWESOME_TAG_H

#include <regex.h> 
#include "client.h"

/** Check if a client is tiled */ 
#define IS_TILED(client, screen, tags, ntags)            (client && !client->isfloating && isvisible(client, tags, ntags) && client->screen == screen)

void compileregs(Rule *, int);         /* initialize regexps of rules defined in config.h */
Bool isvisible(Client *, Bool *, int);
void applyrules(Client * c, awesome_config *);    /* applies rules to c */
void uicb_tag(Display *, int, DC *, awesome_config *, const char *);         /* tags sel with arg's index */
void uicb_togglefloating(Display *, int, DC *, awesome_config *, const char *);      /* toggles sel between floating/tiled state */
void uicb_toggletag(Display *, int, DC *, awesome_config *, const char *);   /* toggles sel tags with arg's index */
void uicb_toggleview(Display *, int, DC *, awesome_config *, const char *);  /* toggles the tag with arg's index (in)visible */
void uicb_view(Display *, int, DC *, awesome_config *, const char *);        /* views the tag with arg's index */
void uicb_viewprevtags(Display *, int, DC *, awesome_config *, const char *);
void uicb_tag_viewnext(Display *, int, DC *, awesome_config *, const char *);        /* view only tag just after the first selected */
void uicb_tag_viewprev(Display *, int, DC *, awesome_config *, const char *);        /* view only tag just before the first selected */

typedef struct 
{
    regex_t *propregex; 
    regex_t *tagregex; 
} Regs; 

#endif
