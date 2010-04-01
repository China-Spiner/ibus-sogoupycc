/* 
 * File:   defines.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * pkg path and debug macro defines
 */

#ifndef _DEFINES_H
#define	_DEFINES_H

#include <cstring>
#include <cstdio>

#ifndef UNUSED
#define UNUSED(x) ((void)(x));
#endif

#ifndef PKGDATADIR
#define PKGDATADIR "/usr/share/ibus-sogoupycc"
#endif

#define APP_ICON (PKGDATADIR "/icons/ibus-sogoupycc.png")

#ifndef VERSION
#define VERSION "0.2.5"
#endif

#include "XUtility.h"

// for debugging
extern int globalDebugLevel;

#define DEBUG_PRINT(level, ...) if (globalDebugLevel >= level) fprintf(stderr, "[DEBUG] (%.3lf) L%03d (thread 0x%x): ", (double)XUtility::getCurrentTime() / XUtility::MICROSECOND_PER_SECOND, __LINE__, (int)pthread_self()),fprintf(stderr,__VA_ARGS__),fflush(stderr);

#endif	/* _DEFINES_H */

