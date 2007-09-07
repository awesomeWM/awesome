/* See LICENSE file for copyright and license details. */

#ifndef JDWM_LAYOUT_H
#define JDWM_LAYOUT_H

#include "client.h"

/** Check if current layout is arranged with a layout */
#define IS_ARRANGE(layout)          (layout == jdwmconf->current_layout->arrange)

#define JDWMPROPS_ATOM(disp)                    XInternAtom(disp, "_JDWM_PROPERTIES", False)

void arrange(Display *, jdwm_config *);             /* arranges all windows depending on the layout in use */
void restack(Display *, jdwm_config *);        /* restores z layers of all clients */
void uicb_focusnext(Display *, jdwm_config *, const char *);   /* focuses next visible client */
void uicb_focusprev(Display *, jdwm_config *, const char *);   /* focuses prev visible client */
void uicb_setlayout(Display *, jdwm_config *, const char *);   /* sets layout, NULL means next layout */
void uicb_togglebar(Display *, jdwm_config *, const char *);   /* shows/hides the bar */
void uicb_togglemax(Display *, jdwm_config *, const char *);   /* toggles maximization of floating client */
void uicb_toggleverticalmax(Display *, jdwm_config *, const char *);
void uicb_togglehorizontalmax(Display *, jdwm_config *, const char *);
void uicb_zoom(Display *, jdwm_config *, const char *); /* set current window first in stack */
void loadjdwmprops(Display *, jdwm_config *);
void savejdwmprops(Display *disp, jdwm_config *);

#endif
