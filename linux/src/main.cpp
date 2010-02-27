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

char debugCurrentSourceFile[2048];
int debugCurrentSourceLine;
int debugCurrentThread;
int mainThread;

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
    ibus_quit();
    g_object_unref(component);
}

void signalHandler(int signal) {
    switch (signal) {
        case SIGSEGV:
#ifdef DEBUG
            fprintf(stderr, "segmentation fault in thread 0x%x (main thread: 0x%x)\nprogram possibly stopped at: %s Line %d, thread 0x%x.\n", (int) pthread_self(), mainThread, debugCurrentSourceFile, debugCurrentSourceLine, debugCurrentThread);
#else
            fprintf(stderr, "segmentation fault\nno program possibly stop information.\nrecompile with -DDEBUG to get some useful information.");
#endif
            fprintf(stderr, "  -- SIGSEGV handler\n");
            fflush(stderr);
            ibus_quit();
            exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    // call dbus and glib, gdk multi thread init functions
    g_thread_init(NULL);
    gdk_threads_init();
    dbus_threads_init_default();

    // gdk and gtk init
    gdk_init(&argc, &argv);
    gtk_init(&argc, &argv);

    mainThread = pthread_self();

    // selection clipboard monitor
    XUtility::staticInit();
    signal(SIGSEGV, signalHandler);

    // simple argc parser
    if (argc > 1) {
        init(2);
    } else {
        init(1);
    }

    ibus_main();
    DEBUG_PRINT(1, "[MAIN] Exiting...\n");
    XUtility::staticDestruct();

    return 0;
}

