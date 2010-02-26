/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * February 26, 2010
 *  created
 *  clipboard related code ported from xclip (http://xclip.sourceforge.net)
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

#define XCLIB_XCOUT_NONE	0	/* no context */
#define XCLIB_XCOUT_SENTCONVSEL	1	/* sent a request */
#define XCLIB_XCOUT_INCR	2	/* in an incr loop */
#define XCLIB_XCOUT_FALLBACK	3	/* UTF8_STRING failed, need fallback to XA_STRING */

using std::string;

class XUtility {
public:
    XUtility();
    virtual ~XUtility();
    string getSelection();

private:
    XUtility(const XUtility& orig);

    Display *dpy;
    Window win;
    Atom sseln;
    Atom target;
    int xcout(Display * dpy, Window win, XEvent evt, Atom sel, Atom target, unsigned char **txt, unsigned long *len, unsigned int *context);
};

#endif	/* _XUTILITY_H */

