/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * see .h file for changelog
 */

#include "XUtility.h"

GtkClipboard *XUtility::primaryClipboard;

long long XUtility::updatedTime;
string XUtility::currentSelection = "";

pthread_t XUtility::gtkMainLoopThread;
pthread_rwlock_t XUtility::selectionRwLock;

bool XUtility::staticInited = false;
bool XUtility::running = false;

void* XUtility::gtkMainLoop(void*) {
    gtk_main();
    return NULL;
}

void XUtility::staticDestruct() {
    if (staticInited) {
        running = false;
        pthread_rwlock_destroy(&selectionRwLock);
        // TODO: other cleanning, how ever, seems this loop here never ends
    }
}

void* XUtility::updateSelection() {
    if (gtk_clipboard_wait_is_text_available(primaryClipboard)) {
        pthread_rwlock_wrlock(&selectionRwLock);
        gchar *text = gtk_clipboard_wait_for_text(primaryClipboard);
        if (text) {
            currentSelection = string((char*) text);
            updatedTime = getCurrentTime();
            DEBUG_PRINT(4, "[XUTIL] selection update to: '%s'\n", currentSelection.c_str());
            g_free(text);
        }
        pthread_rwlock_unlock(&selectionRwLock);
    }
    return NULL;
}

void XUtility::staticInit() {
    if (staticInited) return;
    // this probably won't be call that frequently -,- no lock here
    staticInited = true;

    XUtility::primaryClipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    g_signal_connect(primaryClipboard, "owner-change", G_CALLBACK(XUtility::updateSelection), NULL);
    updatedTime = getCurrentTime();

    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    pthread_create(&gtkMainLoopThread, &threadAttr, &gtkMainLoop, NULL);
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

const long long XUtility::getSelectionUpdatedTime() {
    staticInit();
    pthread_rwlock_rdlock(&selectionRwLock);
    long long r = updatedTime;
    pthread_rwlock_unlock(&selectionRwLock);
    return r;
}

const string XUtility::getSelection() {
    staticInit();
    pthread_rwlock_rdlock(&selectionRwLock);
    string r = currentSelection;
    pthread_rwlock_unlock(&selectionRwLock);
    return r;
}

const long long XUtility::getCurrentTime() {
    struct ntptimeval t;
    ntp_gettime(&t);
    return t.time.tv_usec + t.time.tv_sec * 1000000LL;
}
