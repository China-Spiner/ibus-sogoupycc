/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 9, 2010
 *  0.1.3+ add setSelectionUpdatedTime()
 * February 28, 2010
 *  0.1.1 major bugs fixed
 *  add debug output, use gtk callback update instead of X loop checking
 *  now no relation with xclip
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

#include <string>
#include <pthread.h>
#include <gtk/gtk.h>

#include "defines.h"

using std::string;

// all static

class XUtility {
public:
    static const string getSelection();
    static const long long getSelectionUpdatedTime();
    static void setSelectionUpdatedTime(long long time = 0);
    static const long long getCurrentTime();
    static bool showNotify(const char* summary, const char* body = "", const char* iconPath = APP_ICON);

    static long long MICROSECOND_PER_SECOND;

    static void staticDestruct();
    static void staticInit();
private:
    XUtility();
    virtual ~XUtility();
    XUtility(const XUtility& orig);

    static GtkClipboard *primaryClipboard;
    static pthread_t gtkMainLoopThread;
    static pthread_rwlock_t selectionRwLock;
    static string currentSelection;
    static long long updatedTime;
    static bool staticInited;
    static bool running;
    static bool notifyInited;
    static void* gtkMainLoop(void*);
    static void* updateSelection();
    static void* updateSelectionThread(void*);
};

#endif	/* _XUTILITY_H */
