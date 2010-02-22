/*
 * File:   engine.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * see .h file for changelog
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include "defines.h"
#include "engine.h"
#include "LuaBinding.h"

typedef struct _IBusSgpyccEngine IBusSgpyccEngine;
typedef struct _IBusSgpyccEngineClass IBusSgpyccEngineClass;

using std::string;

struct _IBusSgpyccEngine {
    IBusEngine parent;
    gboolean enabled;
    gboolean has_focus;

    // cursor location
    IBusRectangle cursor_area;
    guint client_capabilities;

    string* commitingCharacters;
    IBusLookupTable *table;
    LuaBinding* luaBinding;
    PinyinCloudClient* cloudClient;

    pthread_mutex_t engineMutex;
    int cursorX, cursorY, cursorWidth, cursorHeight;
    int debugLevel;

    // full path of fetcher script
    string* fetcherPath;
    int fetcherBufferSize;
    bool needUpdatePreedit;
};

struct _IBusSgpyccEngineClass {
    IBusEngineClass parent;
};

static IBusEngineClass *parentClass = NULL;

// lock defines
#define ENGINE_MUTEX_LOCK if (pthread_mutex_lock(&engine->engineMutex)) fprintf(stderr, "[FATAL] mutex lock fail.\n"), exit(EXIT_FAILURE);
#define ENGINE_MUTEX_UNLOCK if (pthread_mutex_unlock(&engine->engineMutex)) fprintf(stderr, "[FATAL] mutex unlock fail.\n"), exit(EXIT_FAILURE);

// init funcs
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
static void enginePendingUpdatePreedit(IBusSgpyccEngine *engine);
static void engineProcessPendingActions(IBusSgpyccEngine *engine);

// lua state to engine, to lookup IBusSgpyccEngine from a lua state
static map<const lua_State*, IBusSgpyccEngine*> l2Engine;

// lua functions
static int l_commitText(lua_State* L);
static int l_sendRequest(lua_State* L);
static int l_isIdle(lua_State* L);
static int l_removeLastRequest(lua_State* L);

// fetch functions
string directFunc(void* data, const string& requestString);
string fetchFunc(void* data, const string& requestString);

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
    pthread_mutexattr_t engineMutexAttr;
    pthread_mutexattr_init(&engineMutexAttr);
    pthread_mutexattr_settype(&engineMutexAttr, PTHREAD_MUTEX_ERRORCHECK);

    if (pthread_mutex_init(&engine->engineMutex, &engineMutexAttr)) {
        fprintf(stderr, "can't create mutex.\nprogram aborted.");
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_destroy(&engineMutexAttr);

    // init lua binding
    engine->luaBinding = new LuaBinding();
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

    // read in debug level globally
    globalDebugLevel = engine->luaBinding->getValue("DEBUG", 0);
    DEBUG_PRINT(1, "[ENGINE] Debug Level: %d\n", globalDebugLevel);

    // read in external script path
    engine->fetcherPath = new string(engine->luaBinding->getValue("fetcher", PKGDATADIR "/fetcher"));
    engine->commitingCharacters = new string("wo men");
    engine->fetcherBufferSize = engine->luaBinding->getValue("fetcherBufferSize", 1024);
    engine->needUpdatePreedit = false;

    // init pinyin cloud client
    engine->cloudClient = new PinyinCloudClient();
    DEBUG_PRINT(1, "[ENGINE] Init completed\n");
}

static void engineDestroy(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(1, "[ENGINE] Destroy\n");
    l2Engine.erase(engine->luaBinding->getLuaState());
    delete engine->luaBinding;
    delete engine->cloudClient;
    delete engine->fetcherPath;
    delete engine->commitingCharacters;
    pthread_mutex_destroy(&engine->engineMutex);
    IBUS_OBJECT_CLASS(parentClass)->destroy((IBusObject *) engine);
}

static gboolean engineProcessKeyEvent(IBusSgpyccEngine *engine, guint32 keyval, guint32 keycode, guint32 state) {
    DEBUG_PRINT(1, "[ENGINE] ProcessKeyEvent(%d, %d, 0x%x)\n", keyval, keycode, state);

    ENGINE_MUTEX_LOCK;
    // if commitingCharacters is not empty, use internal handle process, otherwise call lua function
    int res;
    if (engine->commitingCharacters->length() > 0) {
        // find a pinyin
        string s = *(engine->commitingCharacters) + " ";
        int spacePosition = s.find(' ');
        // try parse that pinyin
        string pinyin = s.substr(0, spacePosition);
        if (PinyinUtility::isValidPinyin(pinyin)) {
            // convert valid pinyin to lookup table

        } else {
            // not found, allow submit all
            engine->cloudClient->request(*(engine->commitingCharacters), directFunc, (void*) engine, (ResponseCallbackFunc) enginePendingUpdatePreedit, (void*) engine);
        }
    } else {
        switch (engine->luaBinding->callLuaFunction("processkey", "ddd>b", keyval, keycode, state, &res)) {
            case 0: // success
                break;
            case -1:
                DEBUG_PRINT(1, "[ENGINE] processkey(keyval, keycode, state) not found!\n");
            default:
                res = 0; // let other program handle this key
        }
    }
    ENGINE_MUTEX_UNLOCK;

    // update preedit
    engine->needUpdatePreedit = true;
    engineUpdatePreedit(engine);
    return res;
}

static void engineFocusIn(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(2, "[ENGINE] FocusIn\n");
    engineUpdatePreedit(engine);
    ibus_engine_show_auxiliary_text((IBusEngine*) engine);
    ibus_engine_show_lookup_table((IBusEngine*) engine);
}

static void engineFocusOut(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(2, "[ENGINE] FocusOut\n");
    engineUpdatePreedit(engine);
    ibus_engine_hide_auxiliary_text((IBusEngine*) engine);
    ibus_engine_hide_lookup_table((IBusEngine*) engine);
}

static void engineReset(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(1, "[ENGINE] Reset\n");
}

static void engineEnable(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(1, "[ENGINE] Enable\n");
    engine->enabled = true;
    engineUpdatePreedit(engine);
}

static void engineDisable(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(1, "[ENGINE] Disable\n");
    engine->enabled = false;
    engineUpdatePreedit(engine);
}

static void engineSetCursorLocation(IBusSgpyccEngine *engine, gint x, gint y, gint w, gint h) {
    DEBUG_PRINT(4, "[ENGINE] SetCursorLocation(%d, %d, %d, %d)\n", x, y, w, h);
    engine->cursor_area.height = h;
    engine->cursor_area.width = w;
    engine->cursor_area.x = x;
    engine->cursor_area.y = y;
    engineUpdatePreedit(engine);
}

static void engineSetCapabilities(IBusSgpyccEngine *engine, guint caps) {
    DEBUG_PRINT(2, "[ENGINE] SetCapabilities(%u)\n", caps);
    ENGINE_MUTEX_LOCK;

    ENGINE_MUTEX_UNLOCK;
}

static void enginePageUp(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(2, "[ENGINE] PageUp\n");
    ENGINE_MUTEX_LOCK;

    ENGINE_MUTEX_UNLOCK;
}

static void enginePageDown(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(2, "[ENGINE] PageDown\n");
    ENGINE_MUTEX_LOCK;

    ENGINE_MUTEX_UNLOCK;
}

static void engineCursorUp(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorUp\n");
    ENGINE_MUTEX_LOCK;

    ENGINE_MUTEX_UNLOCK;
}

static void engineCursorDown(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorDown\n");
    ENGINE_MUTEX_LOCK;

    ENGINE_MUTEX_UNLOCK;
}

static jmp_buf sigBuffer;

void sigsegvResumer(int sig) {
    UNUSED(sig);
    longjmp(sigBuffer, 1);
}

// this is not executed by signal

static void enginePendingUpdatePreedit(IBusSgpyccEngine *engine) {
    engine->needUpdatePreedit = true;
}

static void engineUpdatePreedit(IBusSgpyccEngine *engine) {
    // this function need a mutex lock
    if (!engine->needUpdatePreedit) return;
    DEBUG_PRINT(1, "[ENGINE] Update Preedit\n");
    ENGINE_MUTEX_LOCK;
    engine->needUpdatePreedit = false;
    engine->cloudClient->readLock();
    size_t requestCount = engine->cloudClient->getRequestCount();

    // contains entire preedit text
    string preedit;
    // first few responsed requests can be commited, these two var count them
    bool canCommitToClient = true;
    size_t finishedCount = 0;

    // this loop will commit front responed string, set preedit string and count finishedCount
    string commitString = "";
    for (size_t i = 0; i < requestCount; ++i) {
        const PinyinCloudRequest& request = engine->cloudClient->getRequest(i);
        if (!request.responsed) canCommitToClient = false;


        if (canCommitToClient) {
            if (request.responseString.length() > 0) commitString += request.responseString;
            finishedCount++;
        } else {
            if (request.responsed) preedit += request.responseString;
            else preedit += request.requestString;
        }
    }


    if (commitString.length() > 0) {
        {
            IBusText *commitText;
            commitText = ibus_text_new_from_printf("%s", commitString.c_str());
            //void(*prevSigsegvHandler) (int sig);
            if (commitText) {
                // seems SEGMENTATION FAULT often happens here. I can not find the reason
                // ibus_engine_commit_text thread safe ? seems so without lua -.-
                ibus_engine_commit_text((IBusEngine *) engine, commitText);
                g_object_unref(G_OBJECT(commitText));
            } else {
                fprintf(stderr, "[ERROR] can not create commitText.\n");
            }
        }
        DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] commited to client: %s\n", commitString.c_str());
    }

    // append current preedit (not belong to a request, still editable, active)
    string activePreedit = engine->luaBinding->getValue("preedit", "");
    preedit += activePreedit;
    DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] preedit: %s\n", preedit.c_str());

    // build IBusText-type preeditText
#if(1)
    IBusText *preeditText;
    preeditText = ibus_text_new_from_string(preedit.c_str());
    preeditText->attrs = ibus_attr_list_new();
#endif
    // second loop will set color to preeditText
    // note that '，' will be consided 1 char
#if(1)
    size_t preeditLen = 0;
    for (size_t i = finishedCount; i < requestCount; ++i) {
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

    // finally, update preedit
    ibus_engine_update_preedit_text((IBusEngine *) engine, preeditText, preedit.length(), TRUE);

    // remember to unref text
    g_object_unref(preeditText);
#endif
    engine->cloudClient->readUnlock();

    // pop finishedCount from requeset queue, no lock here, it's safe.
    engine->cloudClient->removeFirstRequest(finishedCount);

    ENGINE_MUTEX_UNLOCK;
}

string fetchFunc(void* data, const string& requestString) {
    // do not use same luaState in multi-thread env. lua will just be messed up

    IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] fetchFunc(%s)\n", requestString.c_str());

    string res;

    //if (luaBinding->callLuaFunction(true, "fetch", "s>s", requestString.c_str(), &res)) res = "";
    FILE* fresponse = popen((*(engine->fetcherPath) + " '" + requestString + "'") .c_str(), "r");

    char response[engine->fetcherBufferSize];
    fgets(response, sizeof (response), fresponse);
    res = response;
    pclose(fresponse);

    return res;
}

string directFunc(void* data, const string& requestString) {

    //IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] directFunc(%s)\n", requestString.c_str());


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
    // direct commit test
    IBusText *commitText;
    commitText = ibus_text_new_from_string(lua_tostring(L, 1));
    ibus_engine_commit_text((IBusEngine *) (l2Engine[L]), commitText);
    g_object_unref(commitText);
#else
    IBusSgpyccEngine* engine = l2Engine[L];
    DEBUG_PRINT(1, "[ENGINE] l_commitText: %s\n", lua_tostring(L, 1));
    //engine->cloudClient->request(string(lua_tostring(L, 1)), directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
    engine->cloudClient->request(string(lua_tostring(L, 1)), directFunc, (void*) engine, (ResponseCallbackFunc) enginePendingUpdatePreedit, (void*) engine);
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
    DEBUG_PRINT(1, "[ENGINE] l_sendRequest: %s\n", lua_tostring(L, 1));

    //engine->cloudClient->request(string(lua_tostring(L, 1)), fetchFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
    engine->cloudClient->request(string(lua_tostring(L, 1)), fetchFunc, (void*) engine, (ResponseCallbackFunc) enginePendingUpdatePreedit, (void*) engine);
    return 0; // return 0 value to lua code
}

/**
 * is idle now (no requset and no active preedit)
 * in: none; out: boolean
 */
static int l_isIdle(lua_State* L) {
    IBusSgpyccEngine* engine = l2Engine[L];
    DEBUG_PRINT(2, "[ENGINE] l_isIdle\n");

    lua_checkstack(L, 1);


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
    DEBUG_PRINT(2, "[ENGINE] l_removeLastRequest\n");

    engine->cloudClient->removeLastRequest();
    return 0;
}
