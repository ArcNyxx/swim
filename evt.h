/* swim - simple window manager
 * Copyright (C) 2021-2022 ArcNyxx
 * see LICENCE file for licensing information */

#ifndef EVT_H
#define EVT_H

#include <X11/Xlib.h>

void buttonpress     (XEvent *evt);
void clientmessage   (XEvent *evt);
void configurenotify (XEvent *evt);
void configurereques (XEvent *evt);
void destroynotify   (XEvent *evt);
void enternotify     (XEvent *evt);
void expose          (XEvent *evt);
void focusin         (XEvent *evt);
void keypress        (XEvent *evt);
void mappingnotify   (XEvent *evt);
void maprequest      (XEvent *evt);
void motionnotify    (XEvent *evt);
void propertynotify  (XEvent *evt);
void unmapnotify     (XEvent *evt);

#endif /* EVT_H */
