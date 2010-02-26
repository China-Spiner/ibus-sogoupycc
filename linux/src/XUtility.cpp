/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * see .h file for changelog
 */

#include "XUtility.h"
#include <unistd.h>

Display *XUtility::dpy;
Window XUtility::win;
Atom XUtility::sseln;
Atom XUtility::target;
string XUtility::currentSelection = "";
pthread_t XUtility::selectionRequestThread;
pthread_rwlock_t XUtility::selectionRwLock;
bool XUtility::staticInited = false;
bool XUtility::running = false;

void* XUtility::selectionMonitor(void*) {
    // repeatly get selection

    bool requested = false;
    static Atom pty;
    static Atom inc;
    Atom pty_type;
    int pty_format;

    unsigned char *buffer;
    unsigned long pty_size, pty_items;

    if (!pty) pty = XInternAtom(dpy, "XCLIP_OUT", False);
    if (!inc) inc = XInternAtom(dpy, "INCR", False);

    XEvent evt;
    while (running) {
        sseln = XA_PRIMARY;
        target = XA_UTF8_STRING(dpy);

        if (XPending(dpy) > 0) {
            XNextEvent(dpy, &evt);
            if (evt.type == SelectionNotify) {
                // find the size and format of the data in property
                XGetWindowProperty(dpy, win, pty, 0, 0, False, AnyPropertyType, &pty_type, &pty_format, &pty_items, &pty_size, &buffer);
                XFree(buffer);

                // for simplity, no checking here, not using INCR mechanism, just read the property
                XGetWindowProperty(dpy, win, pty, 0, (long) pty_size, False, AnyPropertyType, &pty_type, &pty_format, &pty_items, &pty_size, &buffer);
                XDeleteProperty(dpy, win, pty);

                pthread_rwlock_wrlock(&selectionRwLock);
                currentSelection = string((char*) buffer, pty_items);
                pthread_rwlock_unlock(&selectionRwLock);
                XFree(buffer);
                requested = false;
            }
        } else {
            // send request
            if (!requested) {
                XConvertSelection(dpy, sseln, target, pty, win, CurrentTime);
                requested = true;
            }
        }

        usleep(interval);
    }
    return NULL;
}

void XUtility::staticDestruct() {
    if (staticInited) {
        running = false;
        pthread_rwlock_destroy(&selectionRwLock);
        if (win) XDestroyWindow(dpy, win);
        if (dpy) XCloseDisplay(dpy);
    }
}

void XUtility::staticInit() {
    if (staticInited) return;
    if (!(dpy = XOpenDisplay(NULL))) {
        fprintf(stderr, "Can not open display.\n");
        abort();
    }

    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);
    XSelectInput(dpy, win, PropertyChangeMask);

    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(&selectionRequestThread, &threadAttr, &selectionMonitor, NULL);
    pthread_attr_destroy(&threadAttr);

    pthread_rwlock_init(&selectionRwLock, NULL);
    running = true;
    staticInited = true;
}

XUtility::XUtility() {
}

XUtility::XUtility(const XUtility& orig) {
}

XUtility::~XUtility() {
}

string XUtility::getSelection() {
    staticInit();
    pthread_rwlock_rdlock(&selectionRwLock);
    string r = currentSelection;
    pthread_rwlock_unlock(&selectionRwLock);
    return r;
}

long long XUtility::getCurrentTime() {
    struct ntptimeval t;
    ntp_gettime(&t);
    return t.time.tv_usec + t.time.tv_sec * 1000000LL;
}
