/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "XUtility.h"

GtkClipboard *XUtility::primaryClipboard;

long long XUtility::updatedTime;
string XUtility::currentSelection;

pthread_t XUtility::gtkMainLoopThread;
pthread_rwlock_t XUtility::selectionRwLock;

bool XUtility::staticInited = false;
bool XUtility::running = false;

long long XUtility::MICROSECOND_PER_SECOND = 1000000;

void* XUtility::gtkMainLoop(void*) {
    gtk_main();
    DEBUG_PRINT(1, "[XUTIL] gtk main loop exited\n");
    return NULL;
}

void XUtility::staticDestruct() {
    DEBUG_PRINT(1, "[XUTIL] staticDestruct\n");
    if (staticInited) {
        running = false;
        gtk_main_quit();
        pthread_rwlock_destroy(&selectionRwLock);
        // TODO: other cleanning, how ever, seems this loop here never ends
    }
}

void* XUtility::updateSelection() {
    DEBUG_PRINT(5, "[XUTIL] selection update callback\n");

    if (gtk_clipboard_wait_is_text_available(primaryClipboard)) {
        gchar *text = gtk_clipboard_wait_for_text(primaryClipboard);
        if (text) {
            pthread_rwlock_wrlock(&selectionRwLock);
            currentSelection = string((char*) text);
            updatedTime = getCurrentTime();
            DEBUG_PRINT(4, "[XUTIL] selection update to: '%s'\n", currentSelection.c_str());
            g_free(text);
            pthread_rwlock_unlock(&selectionRwLock);
        }
    }
    return NULL;
}

void XUtility::staticInit() {
    if (staticInited) return;
    // this probably won't be call that frequently -,- no lock here
    staticInited = true;
    DEBUG_PRINT(1, "[XUTIL] staticInit\n");

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
    DEBUG_PRINT(4, "[XUTIL] getSelectionUpdatedTime \n");
    pthread_rwlock_rdlock(&selectionRwLock);
    DEBUG_PRINT(5, "[XUTIL] getSelectionUpdatedTime -- get read lock\n");
    long long r = updatedTime;
    pthread_rwlock_unlock(&selectionRwLock);
    return r;
}

void XUtility::setSelectionUpdatedTime(long long time) {
    staticInit();
    DEBUG_PRINT(4, "[XUTIL] setSelectionUpdatedTime \n");
    pthread_rwlock_rdlock(&selectionRwLock);
    updatedTime = time;
    pthread_rwlock_unlock(&selectionRwLock);
}

const string XUtility::getSelection() {
    staticInit();
    DEBUG_PRINT(4, "[XUTIL] getSelection \n");
    pthread_rwlock_rdlock(&selectionRwLock);
    DEBUG_PRINT(5, "[XUTIL] getSelection -- get read lock\n");
    string r = currentSelection;
    pthread_rwlock_unlock(&selectionRwLock);
    return r;
}

const long long XUtility::getCurrentTime() {
    struct ntptimeval t;
    ntp_gettime(&t);
    return t.time.tv_usec + t.time.tv_sec * 1000000LL;
}
