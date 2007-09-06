/* See LICENSE file for copyright and license details. */

#ifndef JDWM_TAG_H
#define JDWM_TAG_H

#include <regex.h> 
#include "client.h"

void compileregs(jdwm_config *);         /* initialize regexps of rules defined in config.h */
Bool isvisible(Client *, Bool *, int);
void applyrules(Client * c, jdwm_config *);    /* applies rules to c */
void uicb_tag(Display *, jdwm_config *, const char *);         /* tags sel with arg's index */
void uicb_togglefloating(Display *, jdwm_config *, const char *);      /* toggles sel between floating/tiled state */
void uicb_toggletag(Display *, jdwm_config *, const char *);   /* toggles sel tags with arg's index */
void uicb_toggleview(Display *, jdwm_config *, const char *);  /* toggles the tag with arg's index (in)visible */
void uicb_view(Display *, jdwm_config *, const char *);        /* views the tag with arg's index */
void uicb_viewprevtags(Display *, jdwm_config *, const char *);
void uicb_tag_viewnext(Display *, jdwm_config *, const char *);        /* view only tag just after the first selected */
void uicb_tag_viewprev(Display *, jdwm_config *, const char *);        /* view only tag just before the first selected */

typedef struct 
{
    regex_t *propregex; 
    regex_t *tagregex; 
} Regs; 

#endif
