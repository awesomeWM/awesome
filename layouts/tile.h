/* See LICENSE file for copyright and license details. */

#ifndef JDWM_TILE_H
#define JDWM_TILE_H

void uicb_incnmaster(Display *, jdwm_config *, const char *);        /* change number of master windows */
void uicb_setmwfact(Display *, jdwm_config *, const char *);        /* sets master width factor */
void tile(Display *, jdwm_config *);
void tileleft(Display *, jdwm_config *);
void bstack(Display *, jdwm_config *);
void bstackportrait(Display *, jdwm_config *);

#endif
