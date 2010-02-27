/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * February 28, 2010
 *  add debug output
 * February 27, 2010
 *  0.1.0 first release
 * February 26, 2010
 *  created
 *  X selection related code uses part of xclip (http://xclip.sourceforge.net)
 *  xclip copyright:
 *   Copyright (C) 2001 Kim Saunders
 *   Copyright (C) 2007-2008 Peter Åstrand
 */


#ifndef _XUTILITY_H
#define	_XUTILITY_H

#include <cstdio>
#include <cstdlib>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xmu/Atoms.h>
#include <string>
#include <sys/timex.h>
#include <pthread.h>
#include "defines.h"

#define XCLIB_XCOUT_NONE	0	/* no context */
#define XCLIB_XCOUT_SENTCONVSEL	1	/* sent a request */
#define XCLIB_XCOUT_INCR	2	/* in an incr loop */
#define XCLIB_XCOUT_FALLBACK	3	/* UTF8_STRING failed, need fallback to XA_STRING */

using std::string;

// send selection request every 100ms
const long interval = 100000;

// all static

class XUtility {
public:
    /**
     * get X selection
     * @param timeout timeout, unit us (400000us = 400ms)
     * @return false when timeout, otherwise true
     */
    //bool static getSelection(string& result, long long timeout = 400000);
    static string getSelection();
    static void staticDestruct();
    static void staticInit();

private:
    XUtility();
    virtual ~XUtility();
    XUtility(const XUtility& orig);

    static Display *dpy;
    static Window win;
    static Atom sseln;
    static Atom target;

    /**
     * this must be manually calls!
     */
    static bool running;
    static pthread_t selectionRequestThread;
    static pthread_rwlock_t selectionRwLock;
    static string currentSelection;
    static void* selectionMonitor(void*);
    static bool staticInited;

    //int xcout(Display * dpy, Window win, XEvent evt, Atom sel, Atom target, unsigned char **txt, unsigned long *len, unsigned int *context);
    long long getCurrentTime();
};

#endif	/* _XUTILITY_H */

/*
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __CLIPMAN_COLLECTOR_H__
#define __CLIPMAN_COLLECTOR_H__

#include <glib-object.h>

#define CLIPMAN_TYPE_COLLECTOR                  (clipman_collector_get_type())

#define CLIPMAN_COLLECTOR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLIPMAN_TYPE_COLLECTOR, ClipmanCollector))
#define CLIPMAN_COLLECTOR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), CLIPMAN_TYPE_COLLECTOR, ClipmanCollectorClass))

#define CLIPMAN_IS_COLLECTOR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLIPMAN_TYPE_COLLECTOR))
#define CLIPMAN_IS_COLLECTOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), CLIPMAN_TYPE_COLLECTOR))

#define CLIPMAN_COLLECTOR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), CLIPMAN_TYPE_COLLECTOR, ClipmanCollectorClass))

typedef struct _ClipmanCollectorClass           ClipmanCollectorClass;
typedef struct _ClipmanCollector                ClipmanCollector;
typedef struct _ClipmanCollectorPrivate         ClipmanCollectorPrivate;

struct _ClipmanCollectorClass
{
  GObjectClass              parent_class;
};

struct _ClipmanCollector
{
  GObject                   parent;

  /* Private */
  ClipmanCollectorPrivate  *priv;
};

GType                   clipman_collector_get_type              ();

ClipmanCollector *      clipman_collector_get                   ();
void                    clipman_collector_set_is_restoring      (ClipmanCollector *collector);
//////////////////////////////////////////////////////////

/* History */
#define DEFAULT_MAX_TEXTS_IN_HISTORY                    10
#define DEFAULT_MAX_IMAGES_IN_HISTORY                   1
#define DEFAULT_SAVE_ON_QUIT                            TRUE

/* Collector */
#define DEFAULT_ADD_PRIMARY_CLIPBOARD                   FALSE
#define DEFAULT_HISTORY_IGNORE_PRIMARY_CLIPBOARD        TRUE
#define DEFAULT_ENABLE_ACTIONS                          FALSE

/*
 * Selection for the popup command
 */

#define XFCE_CLIPMAN_SELECTION    "XFCE_CLIPMAN_SELECTION"
#define XFCE_CLIPMAN_MESSAGE      "MENU"

/*
 * Action Groups
 */

#define ACTION_GROUP_SELECTION  0
#define ACTION_GROUP_MANUAL     1

#endif /* !__CLIPMAN_COLLECTOR_H__ */


