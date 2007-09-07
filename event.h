/* See LICENSE file for copyright and license details. */

#ifndef JDWM_EVENT_H
#define JDWM_EVENT_H

#include "config.h"

void grabkeys(Display *, jdwm_config *);            /* grab all keys defined in config */

void handle_event_buttonpress(XEvent *, jdwm_config *);
void handle_event_configurerequest(XEvent *, jdwm_config *);
void handle_event_configurenotify(XEvent *, jdwm_config *); 
void handle_event_destroynotify(XEvent *, jdwm_config *);
void handle_event_enternotify(XEvent *, jdwm_config *);
void handle_event_expose(XEvent *, jdwm_config *);
void handle_event_keypress(XEvent *, jdwm_config *);
void handle_event_leavenotify(XEvent *, jdwm_config *);
void handle_event_mappingnotify(XEvent *, jdwm_config *);
void handle_event_maprequest(XEvent *, jdwm_config *);
void handle_event_propertynotify(XEvent *, jdwm_config *);
void handle_event_unmapnotify(XEvent *, jdwm_config *);

#endif
