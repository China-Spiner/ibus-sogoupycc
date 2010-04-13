/*
 * File:   XUtility.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "XUtility.h"
#include <sys/timex.h>
#include <gtk/gtk.h>
#include <libnotify/notify.h>
#include <libnotify/notification.h>
#include "Configuration.h"
#include "defines.h"


namespace XUtility {
    static GtkClipboard *primaryClipboard;

    static long long updatedTime;
    static string currentSelection;

    static pthread_t gtkMainLoopThread;
    static pthread_rwlock_t selectionRwLock;

    static bool staticInited = false;
    static bool running = false;
    static bool notifyInited = false;

    static NotifyNotification *staticNotify = NULL;

    const long long MICROSECOND_PER_SECOND = 1000000;

    void* gtkMainLoop(void*) {
        gtk_main();
        DEBUG_PRINT(1, "[XUTIL] gtk main loop exited\n");
        return NULL;
    }

    void staticDestruct() {
        DEBUG_PRINT(1, "[XUTIL] staticDestruct\n");
        if (staticInited) {
            running = false;
            if (notifyInited) {
                g_object_unref(G_OBJECT(staticNotify));
                notify_uninit();
                notifyInited = false;
            }
            gtk_main_quit();
            pthread_rwlock_destroy(&selectionRwLock);
            // IMPROVE: other cleanning, how ever, seems this loop here never ends
        }
    }

    void* updateSelection() {
        DEBUG_PRINT(5, "[XUTIL] selection update callback\n");

        gchar *text = gtk_clipboard_wait_for_text(primaryClipboard);
        if (text) {
            pthread_rwlock_wrlock(&selectionRwLock);
            currentSelection = string((char*) text);
            updatedTime = getCurrentTime();
            DEBUG_PRINT(4, "[XUTIL] selection update to: '%s'\n", currentSelection.c_str());
            g_free(text);
            pthread_rwlock_unlock(&selectionRwLock);
        }
        return NULL;
    }

    void staticInit() {
        if (staticInited) return;
        // this probably won't be call that frequently -,- no lock here
        staticInited = true;
        DEBUG_PRINT(1, "[XUTIL] staticInit\n");

        primaryClipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
        g_signal_connect(primaryClipboard, "owner-change", G_CALLBACK(updateSelection), NULL);
        updatedTime = getCurrentTime();

        pthread_attr_t threadAttr;
        pthread_attr_init(&threadAttr);
        pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
        pthread_create(&gtkMainLoopThread, &threadAttr, &gtkMainLoop, NULL);
        pthread_attr_destroy(&threadAttr);

        pthread_rwlock_init(&selectionRwLock, NULL);
        running = true;

        // init libnotify
        notifyInited = notify_init("ibus-sogoupycc");
        if (notifyInited) {
            staticNotify = notify_notification_new("-", NULL, NULL, NULL);
        }

        staticInited = true;
    }

    bool showNotify(const char* summary, const char* body, const char* iconPath) {
        DEBUG_PRINT(2, "[XUTIL] showNotify(%s, %s, %s)\n", summary, body, iconPath);
        if (Configuration::staticNotification) return showStaticNotify(summary, body, iconPath);
        if (!notifyInited) return false;
        if (summary == NULL || summary[0] == '\0') return false;
        NotifyNotification *notify = notify_notification_new(summary, body, iconPath, NULL);
        int r = notify_notification_show(notify, NULL);
        g_object_unref(G_OBJECT(notify));
        return (r != FALSE);
    }

    bool showStaticNotify(const char* summary, const char* body, const char* iconPath) {
        DEBUG_PRINT(2, "[XUTIL] showStaticNotify(%s, %s, %s)\n", summary, body, iconPath);
        if (!notifyInited) return false;
        if (summary == NULL || summary[0] == '\0') return false;
        notify_notification_update(staticNotify, summary, body, iconPath);
        int r = notify_notification_show(staticNotify, NULL);
        return (r != FALSE);
    }

    const long long getSelectionUpdatedTime() {
        staticInit();
        DEBUG_PRINT(4, "[XUTIL] getSelectionUpdatedTime \n");
        pthread_rwlock_rdlock(&selectionRwLock);
        DEBUG_PRINT(5, "[XUTIL] getSelectionUpdatedTime -- get read lock\n");
        long long r = updatedTime;
        pthread_rwlock_unlock(&selectionRwLock);
        return r;
    }

    void setSelectionUpdatedTime(long long time) {
        staticInit();
        DEBUG_PRINT(4, "[XUTIL] setSelectionUpdatedTime \n");
        pthread_rwlock_rdlock(&selectionRwLock);
        updatedTime = time;
        pthread_rwlock_unlock(&selectionRwLock);
    }

    const string getSelection() {
        staticInit();
        DEBUG_PRINT(4, "[XUTIL] getSelection \n");
        pthread_rwlock_rdlock(&selectionRwLock);
        DEBUG_PRINT(5, "[XUTIL] getSelection: '%s'\n", currentSelection.c_str());
        string r = currentSelection;
        pthread_rwlock_unlock(&selectionRwLock);
        return r;
    }

    const long long getCurrentTime() {
        struct ntptimeval t;
        ntp_gettime(&t);
        return t.time.tv_usec + t.time.tv_sec * 1000000LL;
    }

    /**
     * show notify
     * in: string summary, string body(optional), string icon_path(optional)
     * out: -
     */
    static int l_notify(lua_State *L) {
        DEBUG_PRINT(2, "[LUA] l_notify\n");
        if (lua_type(L, 1) == LUA_TNONE) {
            luaL_error(L, "notify: require parameters.");
            lua_pushboolean(L, false);
            return 1;
        }
        const char* summary = lua_tostring(L, 1);
        const char* body = lua_type(L, 2) == LUA_TSTRING ? lua_tostring(L, 2) : NULL;
        const char* iconPath = lua_type(L, 3) == LUA_TSTRING ? lua_tostring(L, 3) : (PKGDATADIR "/icons/extensions.png");
        lua_checkstack(L, 1);
        lua_pushboolean(L, XUtility::showNotify(summary, body, iconPath));
        return 1;
    }

    /**
     * get selection
     * in: -
     * out: string
     */
    static int l_getSelection(lua_State * L) {
        DEBUG_PRINT(2, "[LUA] l_getSelection\n");
        lua_checkstack(L, 1);
        lua_pushstring(L, XUtility::getSelection().c_str());
        return 1;
    }

    void registerLuaFunctions() {
        LuaBinding::getStaticBinding().registerFunction(l_notify, "notify");
        LuaBinding::getStaticBinding().registerFunction(l_getSelection, "get_selection");
    }
}