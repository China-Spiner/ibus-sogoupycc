/* 
 * File:   defines.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 2, 2010
 *  0.1.2
 * February 28, 2010
 *  0.1.1 major bugs fixed
 * February 27, 2010
 *  0.1.0 first release
 * February 14, 2010
 *  created
 */

#ifndef _DEFINES_H
#define	_DEFINES_H
#include <cstring>

extern "C" {

#ifndef UNUSED
#define UNUSED(x) ((void)(x));
#endif

#ifndef PKGDATADIR
#define PKGDATADIR "/usr/share/ibus-sogoupycc"
#endif

#ifndef VERSION
#define VERSION "0.1.2"
#endif

// for debugging
extern int globalDebugLevel;

#define DEBUG_PRINT(level, ...) if (globalDebugLevel >= level) fprintf(stderr, "[DEBUG] L%03d (thread 0x%x): ", __LINE__, (int)pthread_self()),fprintf(stderr,__VA_ARGS__),fflush(stderr);

// for internal debug use, especially for SIGSEGV

}

#endif	/* _DEFINES_H */

