/* See LICENSE file for copyright and license details. */

#ifndef JDWM_TILE_H
#define JDWM_TILE_H

#include <config.h>

void uicb_incnmaster(Display *, awesome_config *, const char *);        /* change number of master windows */
void uicb_setmwfact(Display *, awesome_config *, const char *);        /* sets master width factor */
void tile(Display *, awesome_config *);
void tileleft(Display *, awesome_config *);
void bstack(Display *, awesome_config *);
void bstackportrait(Display *, awesome_config *);

#endif
