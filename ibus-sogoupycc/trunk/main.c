/* 
 * File:   main.c
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 * 
 * Created on November 3, 2009
 */

#define _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <ibus.h>
#include "engine.h"
#include "sogoupycc.h"
#include "doublepinyin_scheme.h"

#ifndef PKGDATADIR
#define PKGDATADIR "/usr/share/ibus-sogoupycc"
#endif

static IBusBus *bus = NULL;
static IBusFactory *factory = NULL;

static void
ibus_disconnected_cb(IBusBus *bus,
        gpointer user_data) {
    ibus_quit();
}

static void init(int run_stand_along) {
    IBusComponent *component;

    ibus_init();

    bus = ibus_bus_new();
    g_signal_connect(bus, "disconnected", G_CALLBACK(ibus_disconnected_cb), NULL);

    factory = ibus_factory_new(ibus_bus_get_connection(bus));
    ibus_factory_add_engine(factory, "Sgpycc", IBUS_TYPE_SGPYCC_ENGINE);

    ibus_bus_request_name(bus, "org.freedesktop.IBus.Sgpycc", 0);

    component = ibus_component_new("org.freedesktop.IBus.Sgpycc",
            "Sogou Pinyin Cloud",
            "0.0.1",
            "GPLv2",
            "Jun WU <quark@lihdd.net>",
            "http://lihdd.net",
            "",
            "ibus-sgpycc");
    ibus_component_add_engine(component,
            ibus_engine_desc_new("Sgpycc",
            "Sogou Pinyin Cloud1",
            "Sogou Pinyin Cloud1",
            "zh_CN",
            "GPLv2",
            "Jun WU <quark@lihdd.net>",
            PKGDATADIR "/ibus-sogoupycc.png",
            "zh"));
    if (run_stand_along) {
        ibus_bus_register_component(bus, component);
    } else {
        ibus_bus_request_name(bus, "org.freedesktop.IBus.Sgpycc", 0);
    }
    g_object_unref(component);
}

int main(int argc, char const *argv[]) {
    sogoupycc_init();
    init_default_doublepinyin_scheme();

    init(argc == 1);
    ibus_main();

    // never called:
    doublepinyin_scheme_free(0);
    sogoupycc_exit();

    return 0;
}
