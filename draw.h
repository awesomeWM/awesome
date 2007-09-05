/* See LICENSE file for copyright and license details. */

#ifndef JDWM_DRAW_H
#define JDWM_DRAW_H

#include "config.h"

void drawstatus(Display *, jdwm_config *);          /* draw the bar */
inline unsigned int textw(const char *text);   /* return the width of text in px */

#endif
