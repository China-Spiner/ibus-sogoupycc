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

#include "defines.h"

using std::string;

// all static

namespace XUtility {

    const string getSelection();
    const long long getSelectionUpdatedTime();
    void setSelectionUpdatedTime(long long time = 0);
    const long long getCurrentTime();
    bool showNotify(const char* summary, const char* body = "", const char* iconPath = APP_ICON);
    bool showStaticNotify(const char* summary, const char* body = "", const char* iconPath = APP_ICON);

    extern const long long MICROSECOND_PER_SECOND;

    void staticDestruct();
    void staticInit();
};

#endif	/* _XUTILITY_H */
