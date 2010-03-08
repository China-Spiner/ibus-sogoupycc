/*
 * File:   main.c
 * Author: WU Jun <quark@lihdd.net>
 *
 *
 * November 3, 2009
 *  created.
 */

// use cc -pthread instead:
// #define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <ibus.h>
#include <signal.h>

#include "defines.h"
#include "engine.h"
#include "PinyinUtility.h"
#include "PinyinCloudClient.h"
#include "LuaBinding.h"
#include "XUtility.h"

int globalDebugLevel = 0;

static IBusBus *bus = NULL; // Connect with IBus daemon.
static IBusFactory *factory = NULL;

static void ibus_disconnected_cb(IBusBus *bus, gpointer user_data) {
    ibus_quit();
}

static void init(int mode) {
    IBusComponent *component;

    ibus_init();

    bus = ibus_bus_new();
    g_signal_connect(bus, "disconnected", G_CALLBACK(ibus_disconnected_cb), NULL);

    factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "sgpycc", IBUS_TYPE_SGPYCC_ENGINE);

    ibus_bus_request_name(bus, "org.freedesktop.IBus.sgpycc", 0);

    component = ibus_component_new("org.freedesktop.IBus.sgpycc",
            "Pinyin Cloud",
            VERSION,
            "GPLv2",
            "WU Jun <quark@lihdd.net>",
            "http://lihdd.net",
            "sgpycc", //exec
            "ibus-sgpycc");

    ibus_component_add_engine(component,
            ibus_engine_desc_new("sgpycc",
            "Sogou Cloud",
            "Sogou Pinyin Cloud Client",
            "zh_CN",
            "GPLv2",
            "WU Jun <quark@lihdd.net>",
            PKGDATADIR "/icons/ibus-sogoupycc.png",
            "en"));

    switch (mode) {
        case 1:
            // stand alone
            ibus_bus_register_component(bus, component);
            break;
        case 2:
            // called by ibus
            ibus_bus_request_name(bus, "org.freedesktop.IBus.sgycc", 0);
            break;
    }
    g_object_unref(component);
}

int main(int argc, char *argv[]) {
    // redirect output
    if (getenv("SGPYZCC_REDIRECT_OUTPUT")) {
        freopen("/tmp/.sgpycc.out", "w", stdout);
        freopen("/tmp/.sgpycc.err", "w", stderr);
    }

    // call dbus and glib, gdk multi thread init functions
    g_thread_init(NULL);
    gdk_threads_init();
    dbus_threads_init_default();

    // gdk and gtk init
    gdk_init(&argc, &argv);
    gtk_init(&argc, &argv);

    // selection clipboard monitor
    XUtility::staticInit();

    // simple argc parser
    if (argc > 1) {
        init(2);
    } else {
        init(1);
    }

    // load static data
    LuaBinding::staticInit();


    ibus_main();

    DEBUG_PRINT(1, "[MAIN] Exiting from ibus_main() ...\n");

    // clean up static vars
    XUtility::staticDestruct();
    LuaBinding::staticDestruct();

    return 0;
}

