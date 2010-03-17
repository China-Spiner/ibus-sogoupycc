/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * access to X selection clipboard and maintain notifies
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
