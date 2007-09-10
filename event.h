/* See LICENSE file for copyright and license details. */

#ifndef JDWM_EVENT_H
#define JDWM_EVENT_H

#include "config.h"

void grabkeys(Display *, awesome_config *);            /* grab all keys defined in config */

void handle_event_buttonpress(XEvent *, awesome_config *);
void handle_event_configurerequest(XEvent *, awesome_config *);
void handle_event_configurenotify(XEvent *, awesome_config *); 
void handle_event_destroynotify(XEvent *, awesome_config *);
void handle_event_enternotify(XEvent *, awesome_config *);
void handle_event_expose(XEvent *, awesome_config *);
void handle_event_keypress(XEvent *, awesome_config *);
void handle_event_leavenotify(XEvent *, awesome_config *);
void handle_event_mappingnotify(XEvent *, awesome_config *);
void handle_event_maprequest(XEvent *, awesome_config *);
void handle_event_propertynotify(XEvent *, awesome_config *);
void handle_event_unmapnotify(XEvent *, awesome_config *);

#endif
