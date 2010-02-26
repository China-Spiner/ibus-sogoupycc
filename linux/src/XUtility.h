/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
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

