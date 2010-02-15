/*
 * File:   engine.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * see .h file for changelog
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "engine.h"
#include "LuaIbusBinding.h"

typedef struct _IBusSgpyccEngine IBusSgpyccEngine;
typedef struct _IBusSgpyccEngineClass IBusSgpyccEngineClass;

struct _IBusSgpyccEngine {
    IBusEngine parent;
    IBusLookupTable *table;
    LuaIbusBinding* luaBinding;
    PinyinCloudClient* cloudClient;
    pthread_mutex_t updatePreeditMutex;
    int cursorX, cursorY, cursorWidth, cursorHeight;
    int debugLevel;
};

#define ENGINE_DEBUG_PRINT(level, ...) if (engine->debugLevel >= level) fprintf(stderr, "[DEBUG%d] L%d: ",level,__LINE__),fprintf(stderr,__VA_ARGS__),fflush(stderr);

struct _IBusSgpyccEngineClass {
    IBusEngineClass parent;
};

static IBusEngineClass *parentClass = NULL;

// init prototype
static void engineClassInit(IBusSgpyccEngineClass *klass);
static void engineInit(IBusSgpyccEngine *engine);
static void engineDestroy(IBusSgpyccEngine *engine);

// signals
static gboolean engineProcessKeyEvent(IBusSgpyccEngine *engine, guint32 keyval, guint32 keycode, guint32 state);
static void engineFocusIn(IBusSgpyccEngine *engine);
static void engineFocusOut(IBusSgpyccEngine *engine);
static void engineReset(IBusSgpyccEngine *engine);
static void engineEnable(IBusSgpyccEngine *engine);
static void engineDisable(IBusSgpyccEngine *engine);
static void engineSetCursorLocation(IBusSgpyccEngine *engine, gint x, gint y, gint w, gint h);
static void engineSetCapabilities(IBusSgpyccEngine *engine, guint caps);
static void enginePageUp(IBusSgpyccEngine *engine);
static void enginePageDown(IBusSgpyccEngine *engine);
static void engineCursorUp(IBusSgpyccEngine *engine);
static void engineCursorDown(IBusSgpyccEngine *engine);

static void engineUpdatePreedit(IBusSgpyccEngine *engine);

// lua state to engine, to lookup IBusSgpyccEngine from a lua state
static map<const lua_State*, IBusSgpyccEngine*> l2Engine;

// lua functions
static int l_commitText(lua_State* L);
static int l_sendRequest(lua_State* L);
static int l_isIdle(lua_State* L);
static int l_removeLastRequest(lua_State* L);

// entry function, indeed

GType ibusSgpyccEngineGetType(void) {
    static GType type = 0;
    static const GTypeInfo type_info = {
        sizeof (IBusSgpyccEngineClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) engineClassInit,
        NULL,
        NULL,
        sizeof (IBusSgpyccEngine),
        0,
        (GInstanceInitFunc) engineInit,
    };

    // register once
    if (type == 0) {
        type = g_type_register_static(IBUS_TYPE_ENGINE,
                "IBusSogouPinyinCloudClientEngine",
                &type_info,
                (GTypeFlags) 0);
    }

    return type;
}

static void engineClassInit(IBusSgpyccEngineClass *klass) {
    IBusObjectClass *ibusObjectClass = IBUS_OBJECT_CLASS(klass);
    IBusEngineClass *engineClass = IBUS_ENGINE_CLASS(klass);

    // parentClass is global static
    parentClass = (IBusEngineClass *) g_type_class_peek_parent(klass);

    ibusObjectClass->destroy = (IBusObjectDestroyFunc) engineDestroy;

    engineClass->process_key_event = (typeof (engineClass->process_key_event)) (typeof (engineClass->focus_in)) & engineProcessKeyEvent;
    engineClass->focus_in = (typeof (engineClass->focus_in)) & engineFocusIn;
    engineClass->focus_out = (typeof (engineClass->focus_out)) & engineFocusOut;
    engineClass->disable = (typeof (engineClass->disable)) & engineDisable;
    engineClass->enable = (typeof (engineClass->enable)) & engineEnable;
    engineClass->page_down = (typeof (engineClass->page_down)) & enginePageDown;
    engineClass->page_up = (typeof (engineClass->page_up)) & enginePageUp;
    engineClass->cursor_down = (typeof (engineClass->cursor_down)) & engineCursorDown;
    engineClass->cursor_up = (typeof (engineClass->cursor_up)) & engineCursorUp;
    engineClass->reset = (typeof (engineClass->reset)) & engineReset;
    engineClass->set_capabilities = (typeof (engineClass->set_capabilities)) & engineSetCapabilities;
    engineClass->set_cursor_location = (typeof (engineClass->set_cursor_location)) & engineSetCursorLocation;
}

static void engineInit(IBusSgpyccEngine *engine) {
    // init mutex
    if (pthread_mutex_init(&engine->updatePreeditMutex, NULL)) {
        fprintf(stderr, "can't create mutex.\nprogram aborted.");
        exit(EXIT_FAILURE);
    }

    // init lua binding
    engine->luaBinding = new LuaIbusBinding();
    l2Engine[engine->luaBinding->getLuaState()] = engine;

    // add variables to lua
    engine->luaBinding->doString("key={}");
#define add_key_const(var) engine->luaBinding->setValue(#var, IBUS_ ## var, "key");
    add_key_const(CONTROL_MASK);
    add_key_const(SHIFT_MASK);
    add_key_const(LOCK_MASK);
    add_key_const(MOD1_MASK);
    add_key_const(MOD2_MASK);
    add_key_const(MOD3_MASK);
    add_key_const(MOD4_MASK);
    add_key_const(BUTTON1_MASK);
    add_key_const(BUTTON2_MASK);
    add_key_const(BUTTON3_MASK);
    add_key_const(BUTTON4_MASK);
    add_key_const(HANDLED_MASK);
    add_key_const(SUPER_MASK);
    add_key_const(HYPER_MASK);
    add_key_const(META_MASK);
    add_key_const(RELEASE_MASK);
    add_key_const(MODIFIER_MASK);
    add_key_const(FORWARD_MASK);
    add_key_const(Shift_L);
    add_key_const(Shift_R);
    add_key_const(Control_L);
    add_key_const(Control_R);
    add_key_const(Alt_L);
    add_key_const(Alt_R);
    add_key_const(Tab);
    add_key_const(space);
    add_key_const(Return);
    add_key_const(BackSpace);
    add_key_const(Escape);
    add_key_const(Delete);
#undef add_key_const

    engine->luaBinding->setValue("preedit", "");

    // register functions
    engine->luaBinding->addFunction(l_commitText, "commit");
    engine->luaBinding->addFunction(l_sendRequest, "request");
    engine->luaBinding->addFunction(l_isIdle, "isIdle");
    engine->luaBinding->addFunction(l_removeLastRequest, "removeLastRequest");

    // load global config, per-user config will be loaded from there
    if (engine->luaBinding->doString("dofile('" PKGDATADIR "/config')")) {
        fprintf(stderr, "Corrupted configuration! please fix it !!\nAborted.");
        exit(EXIT_FAILURE);
    }

    // set debug level locally
    engine->debugLevel = engine->luaBinding->getValue("DEBUG", 0);

    ENGINE_DEBUG_PRINT(1, "Init\n");

    // init pinyin cloud client
    engine->cloudClient = new PinyinCloudClient();
}

static void engineDestroy(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(1, "Destroy\n");
    l2Engine.erase(engine->luaBinding->getLuaState());
    delete engine->luaBinding;
    delete engine->cloudClient;
    pthread_mutex_destroy(&engine->updatePreeditMutex);
    IBUS_OBJECT_CLASS(parentClass)->destroy((IBusObject *) engine);
}

static gboolean engineProcessKeyEvent(IBusSgpyccEngine *engine, guint32 keyval, guint32 keycode, guint32 state) {
    ENGINE_DEBUG_PRINT(1, "ProcessKeyEvent(%d, %d, 0x%x)\n", keyval, keycode, state);

    int res;
    switch (engine->luaBinding->callLuaFunction(false, "processkey", "ddd>b", keyval, keycode, state, &res)) {
        case 0: // success
            break;
        case -1:
            ENGINE_DEBUG_PRINT(1, "processkey(keyval, keycode, state) not found!\n");
        default:
            res = 0; // let other program handle this key
    }
    // update preedit
    if (res) engineUpdatePreedit(engine);
    return res;
}

static void engineFocusIn(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(1, "FocusIn\n");
    engine->luaBinding->callLuaFunction(false, "focus", "b", 1);
}

static void engineFocusOut(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(1, "FocusOut\n");
    engine->luaBinding->callLuaFunction(false, "focus", "b", 0);
}

static void engineReset(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(1, "Reset\n");
    engine->luaBinding->callLuaFunction(false, "reset", "");
}

static void engineEnable(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(1, "Enable\n");
    engine->luaBinding->callLuaFunction(false, "enable", "b", 1);
}

static void engineDisable(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(1, "Disable\n");
    engine->luaBinding->callLuaFunction(false, "enable", "b", 1);
}

static void engineSetCursorLocation(IBusSgpyccEngine *engine, gint x, gint y, gint w, gint h) {
    ENGINE_DEBUG_PRINT(2, "SetCursorLocation(%d, %d, %d, %d)\n", x, y, w, h);
    engine->cursorHeight = h;
    engine->cursorWidth = w;
    engine->cursorX = x;
    engine->cursorY = y;
}

static void engineSetCapabilities(IBusSgpyccEngine *engine, guint caps) {
    ENGINE_DEBUG_PRINT(2, "SetCapabilities(%u)\n", caps);
}

static void enginePageUp(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(2, "PageUp\n");
}

static void enginePageDown(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(2, "PageDown\n");
}

static void engineCursorUp(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(2, "CursorUp\n");
}

static void engineCursorDown(IBusSgpyccEngine *engine) {
    ENGINE_DEBUG_PRINT(2, "CursorDown\n");
}

// this is not executed by signal

static void engineUpdatePreedit(IBusSgpyccEngine *engine) {
    // this function need a mutex lock    
    ENGINE_DEBUG_PRINT(1, "Update Preedit\n");

    pthread_mutex_lock(&engine->updatePreeditMutex);
    ENGINE_DEBUG_PRINT(3, "engineUpdatePreedit mutex locked\n");

    engine->cloudClient->readLock();
    ENGINE_DEBUG_PRINT(3, "requests locked. start reading request\n");

    size_t requestCount = engine->cloudClient->getRequestCount();
    ENGINE_DEBUG_PRINT(3, "request count: %d\n", requestCount);

    // contains entire preedit text
    string preedit;

    // first few responsed requests can be commited, these two var count them
    bool canCommitToClient = true;
    size_t finishedCount = 0;

    // this loop will set preedit string and count finishedCount
    for (size_t i = 0; i < requestCount; ++i) {
        ENGINE_DEBUG_PRINT(4, "first loop, request # %d\n", i);
        const PinyinCloudRequest& request = engine->cloudClient->getRequest(i);
        if (!request.responsed) canCommitToClient = false;

        ENGINE_DEBUG_PRINT(5, "  id: %d\n", request.requestId);
        ENGINE_DEBUG_PRINT(5, "  request: %s\n", request.requestString.c_str());
        ENGINE_DEBUG_PRINT(5, "  responsed: %s\n", request.responsed ? request.responseString.c_str() : "(waiting)");

        if (canCommitToClient) {
            IBusText *commitText;
            commitText = ibus_text_new_from_string(request.responseString.c_str());
            ibus_engine_commit_text((IBusEngine *) engine, commitText);
            g_object_unref(commitText);
            finishedCount++;
        } else {
            if (request.responsed) preedit += request.responseString;
            else preedit += request.requestString;
        }
    }

    // append current preedit (not belong to a request, still editable, active)
    string activePreedit = engine->luaBinding->getValue("preedit", "");
    preedit += activePreedit;
    ENGINE_DEBUG_PRINT(3, "preedit: %s\n", preedit.c_str());

    // build IBusText-type preeditText
    IBusText *preeditText;
    preeditText = ibus_text_new_from_string(preedit.c_str());
    preeditText->attrs = ibus_attr_list_new();

    // second loop will set color to preeditText
    // note that '，' will be consided 1 char
    size_t preeditLen = 0;
    for (size_t i = finishedCount; i < requestCount; ++i) {
        ENGINE_DEBUG_PRINT(4, "second loop, request # %d\n", i);
        const PinyinCloudRequest& request = engine->cloudClient->getRequest(i);
        size_t currReqLen;
        if (request.responsed) {
            IBusText *text = ibus_text_new_from_string(request.responseString.c_str());
            currReqLen = ibus_text_get_length(text);
            g_object_unref(text);
        } else {
            IBusText *text = ibus_text_new_from_string(request.requestString.c_str());
            currReqLen = ibus_text_get_length(text);
            g_object_unref(text);
            // backgroundize requesting sections
            ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(0xB8CFE5, preeditLen, preeditLen + currReqLen));
            //ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(0xC2C2C2, preeditLen, preeditLen + currReqLen));
        }
        preeditLen += currReqLen;
    }

    // underlinize rightmost active preedit
    ibus_attr_list_append(preeditText->attrs, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, preeditLen, preedit.length()));
    ENGINE_DEBUG_PRINT(3, "prepare to call update preedit text\n");

    // finally, update preedit
    ibus_engine_update_preedit_text((IBusEngine *) engine, preeditText, preedit.length(), TRUE);
    ENGINE_DEBUG_PRINT(3, "called update preedit text\n");

    // remember to unref text
    g_object_unref(preeditText);
    ENGINE_DEBUG_PRINT(3, "preeditText unreffed\n");

    engine->cloudClient->readUnlock();
    ENGINE_DEBUG_PRINT(3, "end reading request (Unlocked)\n");

    // pop finishedCount from requeset queue, no lock here, it's safe.
    for (size_t i = 0; i < finishedCount; ++i) {
        engine->cloudClient->removeFirstRequest();
    }
    ENGINE_DEBUG_PRINT(3, "finished requests popped\n");

    pthread_mutex_unlock(&engine->updatePreeditMutex);
    ENGINE_DEBUG_PRINT(3, "engineUpdatePreedit mutex unlocked\n");
}

string fetchFunc(void* data, const string& requestString) {
    IBusSgpyccEngine* engine = (typeof (engine)) data;
    LuaIbusBinding* luaBinding = engine->luaBinding;
    ENGINE_DEBUG_PRINT(2, "fetchFunc(%s)\n", requestString.c_str());
    
    string res;
    if (luaBinding->callLuaFunction(true, "fetch", "s>s", requestString.c_str(), &res)) res = "";
    return res;
}

string directFunc(void* data, const string& requestString) {
    IBusSgpyccEngine* engine = (typeof (engine)) data;
    ENGINE_DEBUG_PRINT(2, "directFunc(%s)\n", requestString.c_str());

    fflush(stdout);
    return requestString;
}

// Lua Cfunctions

/**
 * commit string to ibus client
 * in: string; out: none
 */
static int l_commitText(lua_State* L) {
    luaL_checkstring(L, 1);

#if(0)
    IBusText *commitText;
    commitText = ibus_text_new_from_string(lua_tostring(L, 1));
    ibus_engine_commit_text((IBusEngine *) (l2Engine[L]), commitText);
    g_object_unref(commitText);
#else
    IBusSgpyccEngine* engine = l2Engine[L];
    ENGINE_DEBUG_PRINT(1, "Commit: %s\n", lua_tostring(L, 1));
    engine->cloudClient->request(string(lua_tostring(L, 1)), directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
#endif

    return 0; // return 0 value to lua code
}

/**
 * call request func (multi-threaded)
 * in: string; out: none
 */
static int l_sendRequest(lua_State* L) {
    luaL_checkstring(L, 1);

    IBusSgpyccEngine* engine = l2Engine[L];
    ENGINE_DEBUG_PRINT(1, "Request: %s\n", lua_tostring(L, 1));

    engine->cloudClient->request(string(lua_tostring(L, 1)), fetchFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
    return 0; // return 0 value to lua code
}

/**
 * is idle now (no requset and no active preedit)
 * in: none; out: boolean
 */
static int l_isIdle(lua_State* L) {
    IBusSgpyccEngine* engine = l2Engine[L];
    ENGINE_DEBUG_PRINT(2, "IsIdle\n");

    engine->cloudClient->readLock();
    lua_pushboolean(L, engine->cloudClient->getRequestCount() == 0 && engine->luaBinding->getValue("preedit", "").length() == 0);
    engine->cloudClient->readUnlock();

    return 1;
}

/**
 * remove last request
 * in: none; out: none
 */
static int l_removeLastRequest(lua_State* L) {
    IBusSgpyccEngine* engine = l2Engine[L];
    ENGINE_DEBUG_PRINT(1, "RemoveLastRequest\n");

    engine->cloudClient->removeLastRequest();
    return 0;
}
