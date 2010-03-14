/* 
 * File:   defines.h
 * Author: WU Jun <quark@lihdd.net>
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

#ifndef VERSION
#define VERSION "0.2.0"
#endif

// for debugging
extern int globalDebugLevel;

#define DEBUG_PRINT(level, ...) if (globalDebugLevel >= level) fprintf(stderr, "[DEBUG] L%03d (thread 0x%x): ", __LINE__, (int)pthread_self()),fprintf(stderr,__VA_ARGS__),fflush(stderr);

#endif	/* _DEFINES_H */

