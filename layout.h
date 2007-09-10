/* See LICENSE file for copyright and license details. */

#ifndef AWESOME_LAYOUT_H
#define AWESOME_LAYOUT_H

#include "client.h"

/** Check if current layout is arranged with a layout */
#define IS_ARRANGE(layout)          (layout == awesomeconf->current_layout->arrange)

#define AWESOMEPROPS_ATOM(disp)                    XInternAtom(disp, "_AWESOME_PROPERTIES", False)

void arrange(Display *, awesome_config *);             /* arranges all windows depending on the layout in use */
void restack(Display *, awesome_config *);        /* restores z layers of all clients */
void uicb_focusnext(Display *, awesome_config *, const char *);   /* focuses next visible client */
void uicb_focusprev(Display *, awesome_config *, const char *);   /* focuses prev visible client */
void uicb_setlayout(Display *, awesome_config *, const char *);   /* sets layout, NULL means next layout */
void uicb_togglebar(Display *, awesome_config *, const char *);   /* shows/hides the bar */
void uicb_togglemax(Display *, awesome_config *, const char *);   /* toggles maximization of floating client */
void uicb_toggleverticalmax(Display *, awesome_config *, const char *);
void uicb_togglehorizontalmax(Display *, awesome_config *, const char *);
void uicb_zoom(Display *, awesome_config *, const char *); /* set current window first in stack */
void loadawesomeprops(Display *, awesome_config *);
void saveawesomeprops(Display *disp, awesome_config *);

#endif
