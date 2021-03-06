/*
 * File:   main.c
 * Author: WU Jun <quark@lihdd.net>
 *
 * ibus-sogoupycc main file
 */

// use cc -pthread instead:
// #define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <ibus.h>
#include <signal.h>
#include <gtk/gtk.h>

#include "defines.h"
#include "engine.h"
#include "PinyinUtility.h"
#include "PinyinCloudClient.h"
#include "LuaBinding.h"
#include "XUtility.h"
#include "Configuration.h"
#include "PinyinDatabase.h"
#include "DoublePinyinScheme.h"

static IBusBus *bus = NULL; // Connect with IBus daemon.
static IBusFactory *factory = NULL;

static void ibus_disconnected_cb(IBusBus *bus, gpointer user_data) {
    ibus_quit();
}

static void ibusRegister(bool callByIbus) {
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
            APP_ICON,
            "en"));

    if (callByIbus) ibus_bus_request_name(bus, "org.freedesktop.IBus.sgycc", 0);
    else ibus_bus_register_component(bus, component);

    g_object_unref(component);
}

void* staticInitThreadFunc(void*) {
    // static selection clipboard monitor
    XUtility::staticInit();

    DEBUG_PRINT(1, "[MAIN] staticInitThreadFunc: start to load config.lua\n");
    // load global config (may contain dict loading and online update checking)
    LuaBinding::loadStaticConfigure();
    return NULL;
}

int main(int argc, char *argv[]) {
    // version
    if (argc > 1 && strstr(argv[1], "-v")) {
        printf("ibus-sogoupycc %s [", VERSION);
        printf("built for ibus %d.%d.%d]\n", IBUS_MAJOR_VERSION, IBUS_MINOR_VERSION, IBUS_MICRO_VERSION);
        exit(EXIT_SUCCESS);
    }

    // help
    if (argc > 1 && strstr(argv[1], "-h")) {
        printf("ibus-sogoupycc --version | --help | --debug | --ibus\n");
        printf("\t--version \tshow version\n");
        printf("\t--help    \tshow this information\n");
        printf("\t--debug   \tset debug level to full at startup\n");
        printf("\t--ibus    \tlaunched by ibus\n");
        exit(EXIT_SUCCESS);
    }

    if ((argc > 1 && strstr(argv[1], "-d")) || getenv("DEBUG")) globalDebugLevel = 10;

    // call dbus and glib, gdk multi thread init functions
    g_thread_init(NULL);
    gdk_threads_init();
    dbus_threads_init_default();

    // gdk and gtk init
    gdk_init(&argc, &argv);
    gtk_init(&argc, &argv);

    // global luaBinding construct, be first
    LuaBinding::staticInit();

    // other classes may add functions to LuaBinding::getStaticBinding()
    registerDebugLuaFunction();

    ImeEngine::registerLuaFunctions();
    Configuration::registerLuaFunctions();
    XUtility::registerLuaFunctions();
    DoublePinyinScheme::registerLuaFunctions();
    PinyinDatabase::registerLuaFunctions();
    PinyinUtility::registerLuaFunctions();


    // static inits
    Configuration::staticInit();
    PinyinCloudClient::staticInit();
    PinyinUtility::staticInit();
    PinyinDatabase::staticInit();
    
    // register ime
    ibusRegister(argc > 1 && strstr(argv[1], "-i"));

    // other init(ie, read settings) in background, prevent user from feeling delay
    pthread_t staticInitThread;
    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&staticInitThread, &threadAttr, &staticInitThreadFunc, NULL)) {
        fprintf(stderr, "Can not create init thread!\n");
        exit(EXIT_FAILURE);
    }
    pthread_attr_destroy(&threadAttr);

    DEBUG_PRINT(1, "[MAIN] Reaching ibus_main() ...\n");

    // wait static config be applied, 0.1 s should be ok.
    usleep(100000);
    ibus_main();

    DEBUG_PRINT(1, "[MAIN] Exiting from ibus_main() ...\n");

    // clean up static vars
    XUtility::staticDestruct();
    Configuration::staticDestruct();
    PinyinCloudClient::staticDestruct();
    PinyinDatabase::staticDestruct();
    PinyinUtility::staticDestruct();

    LuaBinding::staticDestruct();

    return 0;
}

