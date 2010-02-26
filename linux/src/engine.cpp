/*
 * File:   engine.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * see .h file for changelog
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>

#include "defines.h"
#include "engine.h"
#include "LuaBinding.h"
#include "XUtility.h"

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

    // convertingPinyins are characters buffer in preedit and should be choiced manually
    string* convertingPinyins;
    IBusLookupTable *table;

    // full path of fetcher script
    string* fetcherPath;

    // buffer size receive return string from fetcher script
    int fetcherBufferSize;

    // keys used in select item in lookup table
    string* tableLabelKeys;

    // colors
    int requestingBackColor, requestingForeColor, requestedBackColor, requestedForeColor, preeditForeColor, preeditBackColor;

    // keys
    guint32 engModeToggleKey, startCorrectionKey, pageDownKey, pageUpKey;

    // boolean configs
    bool useDoublePinyin;
    bool writeRequestCache;

    // stored in lua state: puncMap

    // preedit: real key seq from user input, activePreedit: preedit on screen
    // they are not same if useDoublePinyin is true, the later shows full pinyin
    string *preedit, *activePreedit;

    // internal use
    int tablePageNumber;
    bool engMode;
    gboolean lastProcessKeyResult;
    guint32 lastKeyval;
    pthread_mutex_t engineMutex;
    LuaBinding* luaBinding;
    PinyinCloudClient* cloudClient;
    XUtility* xUtility;
#define INVALID_COLOR -1
};

struct _IBusSgpyccEngineClass {
    IBusEngineClass parent;
};

static IBusEngineClass *parentClass = NULL;
static bool engineFirstRun = true;

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

// lua state to engine, to lookup IBusSgpyccEngine from a lua state
static map<const lua_State*, IBusSgpyccEngine*> l2Engine;

// lua functions
static int l_commitText(lua_State* L);
static int l_sendRequest(lua_State* L);
static int l_isIdle(lua_State* L);
static int l_correct(lua_State* L);
static int l_removeLastRequest(lua_State* L);
static int l_getSelection(lua_State * L);

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

    // x utility
    engine->xUtility = new XUtility();

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

    // set values
    engine->luaBinding->setValue("preedit", "");
    engine->luaBinding->doString(
#define LIB_NAME "pycc"
            LIB_NAME ".puncMap = { \
                ['.'] = '。', [','] = '，', ['^'] = '……', ['@'] = '＠', ['!'] = '！', ['~'] = '～', \
                ['?'] = '？', ['#'] = '＃', ['$'] = '￥', ['&'] = '＆', ['('] = '（', [')'] = '）', \
                ['{'] = '｛', ['}'] = '｝', ['['] = '［', [']'] = '］', [';'] = '；', [':'] = '：', \
                ['<'] = '《', ['>'] = '》', \
                [\"'\"] = { 2, '‘', '’'}, ['\"'] = { 2, '“', '”'} }\n"
            LIB_NAME ".getPunc = function(keychr) \
                if " LIB_NAME ".puncMap[keychr] then \
                    local punc = " LIB_NAME ".puncMap[keychr] \
                    if type(punc) == 'string' then return punc else \
                        local id = punc[1] \
                        punc[1] = punc[1] + 1 if (punc[1] > #punc) then punc[1] = 2 end \
                        return punc[id] \
                    end \
                else return '' end\
            end");
#undef LIB_NAME
    engine->luaBinding->setValue("COLOR_NOCHANGE", INVALID_COLOR);
    engine->luaBinding->setValue("PKGDATADIR", PKGDATADIR);
    engine->luaBinding->setValue("firstRun", engineFirstRun);
    engineFirstRun = false;

    // register functions
    engine->luaBinding->addFunction(l_commitText, "commit");
    engine->luaBinding->addFunction(l_sendRequest, "request");
    engine->luaBinding->addFunction(l_isIdle, "isIdle");
    engine->luaBinding->addFunction(l_removeLastRequest, "removeLastRequest");
    engine->luaBinding->addFunction(l_correct, "correct");
    engine->luaBinding->addFunction(l_getSelection, "getSelection");

    // load global config, per-user config will be loaded from there
    if (engine->luaBinding->doString("dofile('" PKGDATADIR "/config')")) {
        fprintf(stderr, "Corrupted configuration! please fix it !!\nAborted.");
        exit(EXIT_FAILURE);
    }

    // read in external script path
    engine->fetcherPath = new string(engine->luaBinding->getValue("fetcher", PKGDATADIR "/fetcher"));
    engine->convertingPinyins = new string("");
    engine->fetcherBufferSize = engine->luaBinding->getValue("fetcherBufferSize", 1024);

    // init pinyin cloud client
    engine->cloudClient = new PinyinCloudClient();

    // keys
    engine->engModeToggleKey = engine->luaBinding->getValue("engModeToggleKey", IBUS_Shift_L);
    engine->startCorrectionKey = engine->luaBinding->getValue("correctionKey", IBUS_Tab);
    engine->pageDownKey = engine->luaBinding->getValue("pageDownKey", (int) 'h');
    engine->pageUpKey = engine->luaBinding->getValue("pageUpKey", (int) 'g');

    // config booleans
    engine->useDoublePinyin = engine->luaBinding->getValue("useDoublePinyin", true);
    engine->engMode = engine->luaBinding->getValue("engMode", false);
    engine->writeRequestCache = engine->luaBinding->getValue("writeRequestCache", true);

    // std::strings
    engine->preedit = new string("");
    engine->activePreedit = new string("");

    // internal vars
    engine->lastProcessKeyResult = FALSE;
    engine->lastKeyval = 0;

    // labels used in lookup table, ibus has 16 chars limition.
    engine->tableLabelKeys = new string(engine->luaBinding->getValue("labelKeys", "jkl;uiopasdf"));
    if (engine->tableLabelKeys->length() > 16 || engine->tableLabelKeys->length() == 0) *engine->tableLabelKeys = engine->tableLabelKeys->substr(0, 16);

    // lookup table
    engine->table = ibus_lookup_table_new(engine->tableLabelKeys->length(), 0, 0, 0);
    ibus_lookup_table_set_orientation(engine->table, IBUS_ORIENTATION_VERTICAL);
    for (size_t i = 0; i < engine->tableLabelKeys->length(); i++) {
        IBusText* text = ibus_text_new_from_printf("%c", engine->tableLabelKeys->data()[i]);
        ibus_lookup_table_append_label(engine->table, text);
        g_object_unref(text);
    }

    // for punctuation map, stored in lua state, do not store it here

    // read in colors (-1: use default)
    engine->preeditForeColor = engine->luaBinding->getValue("preeditForeColor", 0x0050FF);
    engine->preeditBackColor = engine->luaBinding->getValue("preeditBackColor", INVALID_COLOR);
    engine->requestingForeColor = engine->luaBinding->getValue("requestingForeColor", INVALID_COLOR);
    engine->requestingBackColor = engine->luaBinding->getValue("requestingBackColor", 0xB8CFE5);
    engine->requestedForeColor = engine->luaBinding->getValue("requestedForeColor", 0x00C97F);
    engine->requestedBackColor = engine->luaBinding->getValue("requestedBackColor", INVALID_COLOR);

    DEBUG_PRINT(1, "[ENGINE] Init completed\n");
}

static void engineDestroy(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(1, "[ENGINE] Destroy\n");
    l2Engine.erase(engine->luaBinding->getLuaState());
    pthread_mutex_destroy(&engine->engineMutex);
    delete engine->luaBinding;
    delete engine->cloudClient;
    delete engine->fetcherPath;
    delete engine->convertingPinyins;
    delete engine->tableLabelKeys;
    delete engine->preedit;
    delete engine->activePreedit;
    delete engine->xUtility;
    if (engine->table) {
        g_object_unref(engine->table);
        engine->table = NULL;
    }
    IBUS_OBJECT_CLASS(parentClass)->destroy((IBusObject *) engine);
}

static gboolean engineProcessKeyEvent(IBusSgpyccEngine *engine, guint32 keyval, guint32 keycode, guint32 state) {
    DEBUG_PRINT(1, "[ENGINE] ProcessKeyEvent(%d, %d, 0x%x)\n", keyval, keycode, state);

    ENGINE_MUTEX_LOCK;

    // return value
    gboolean res = FALSE;

engineProcessKeyEventStart:
    // commit all non-pinyin part of convertingPinyins
    while (engine->convertingPinyins->length() > 0) {
        // try parse that pinyin at left of convertingPinyins
        string s = *(engine->convertingPinyins) + " ";

        int spacePosition = s.find(' ');
        string pinyin = s.substr(0, spacePosition);

        if (PinyinUtility::isValidPinyin(pinyin)) {
            // valid pinyin, stop
            break;
        } else {
            // not found, submit 'pinyin' (it is indeed not a valid pinyin)
            engine->cloudClient->request(pinyin, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
            engine->convertingPinyins->erase(0, spacePosition + 1);
        }
    }

    if (engine->convertingPinyins->length() > 0) {
        // find a pinyin
        string s = *(engine->convertingPinyins) + " ";
        int spacePosition = s.find(' ');

        // try parse that pinyin (should parse successfully)
        string pinyin = s.substr(0, spacePosition);

        assert(PinyinUtility::isValidPinyin(pinyin));
        if ((state & IBUS_RELEASE_MASK) == IBUS_RELEASE_MASK) {
            res = engine->lastProcessKeyResult;
        } else if (keyval == engine->pageDownKey) {
            enginePageDown(engine);
            res = TRUE;
        } else if (keyval == engine->pageUpKey) {
            enginePageUp(engine);
            res = TRUE;
        } else if (keyval == IBUS_Delete) {
            engine->convertingPinyins->erase(0, (*(engine->convertingPinyins) + " ").find(' ') + 1);
            keyval = 0;
            res = TRUE;
            ibus_lookup_table_clear(engine->table);
            goto engineProcessKeyEventStart;
        } else if (keyval == IBUS_BackSpace) {
            keyval = 0;
            res = TRUE;
            ibus_lookup_table_clear(engine->table);
            goto engineProcessKeyEventStart;
        } else {
            // test if user press a key that in lookup table
            size_t pos = engine->tableLabelKeys->find((char) keyval);
            if (pos == string::npos) {
                // not found, user press invalid key, just ignore it
                // IMPROVE: beep here (system call?)
                res = TRUE;
            } else {
                IBusText* candidate = ibus_lookup_table_get_candidate(engine->table, pos + ibus_lookup_table_get_page_size(engine->table) * engine->tablePageNumber);
                if (candidate == NULL || ibus_text_get_length(candidate) == 0) {
                    // invalid option, ignore
                    // IMPROVE: beep
                    res = TRUE;
                } else if (ibus_text_get_length(candidate) == 1) {
                    // commit it
                    IBusText* text = ibus_text_new_from_string(candidate->text);
                    ibus_engine_commit_text((IBusEngine*) engine, text);
                    g_object_unref(text);
                    // remove pinyin section from commitingPinyins
                    engine->convertingPinyins->erase(0, (*(engine->convertingPinyins) + " ").find(' ') + 1);
                    // rescan, rebuild lookup table
                    ibus_lookup_table_clear(engine->table);
                    keyval = 0;
                    res = TRUE;
                    goto engineProcessKeyEventStart;
                } else {
                    // build new table according to selected
                    int l = ibus_text_get_length(candidate);

                    // backup candidate before clear table
                    IBusText *candidateCharacters = ibus_text_new_from_string(candidate->text);
                    const gchar *utf8char = candidateCharacters->text;
                    ibus_lookup_table_clear(engine->table);

                    for (int i = 0; i < l; ++i) {
                        IBusText *candidateCharacter = ibus_text_new_from_unichar(g_utf8_get_char(utf8char));
                        ibus_lookup_table_append_candidate(engine->table, candidateCharacter);
                        g_object_unref(candidateCharacter);
                        if (i < l - 1) utf8char = g_utf8_next_char(utf8char);
                    }
                    g_object_unref(candidateCharacters);
                    engine->tablePageNumber = 0;
                    res = TRUE;
                }
            }
        }
        if (ibus_lookup_table_get_number_of_candidates(engine->table) == 0) {
            // update and show lookup table
            ibus_lookup_table_clear(engine->table);
            for (int tone = 1; tone <= 5; ++tone) {
                IBusText* candidate = ibus_text_new_from_string((PinyinUtility::getCandidates(pinyin, tone)).c_str());
                ibus_lookup_table_append_candidate(engine->table, candidate);
                g_object_unref(candidate);
            }
            IBusText* text = ibus_text_new_from_string(pinyin.c_str());
            ibus_engine_update_auxiliary_text((IBusEngine*) engine, text, TRUE);
            g_object_unref(text);
            engine->tablePageNumber = 0;
            res = TRUE;
        }
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    } else { // (engine->convertingPinyins->length() == 0)
        ibus_engine_hide_lookup_table((IBusEngine*) engine);
        ibus_engine_hide_auxiliary_text((IBusEngine*) engine);
        if (res == 0) {
            switch (engine->luaBinding->callLuaFunction("processkey", "ddd>b", keyval, keycode, state, &res)) {
                case 0:
                    // lua function executed successfully, update engine->activePreedit ( = pycc.preedit in lua state )
                    *(engine->activePreedit) = engine->luaBinding->getValue("preedit", engine->activePreedit->c_str());
                    break;
                case -1:
                {
                    // fallback C key handler
                    engine->cloudClient->readLock();
                    int requestCount = engine->cloudClient->getRequestCount();
                    engine->cloudClient->readUnlock();

                    // normally do not handle release event, but watch for eng mode toggling
                    if ((state & IBUS_RELEASE_MASK) == IBUS_RELEASE_MASK && engine->lastKeyval == keyval) {
                        if (keyval == engine->engModeToggleKey) {
                            engine->engMode = !engine->engMode;
                            // IMPROVE: notify user
                            if (engine->engMode) {
                                *engine->preedit = "";
                                *engine->activePreedit = "";
                            } else {
                            }
                            res = TRUE;
                            break;
                        }
                        res = engine->lastProcessKeyResult;
                        break;
                    }
                    engine->lastKeyval = keyval;

                    // do not handle alt, ctrl ... event (such as Alt - F, Ctrl - T)
                    if (state != 0 && (state ^ IBUS_SHIFT_MASK) != 0) {
                        res = FALSE;
                        break;
                    }

                    bool handled = false;
                    char keychr = (keyval < 128) ? keyval : 0;
                    string keychrs = "";
                    if (keychr) keychrs += keychr;

                    // in chinese mode ?
                    if (!engine->engMode) {
                        // punctuation
                        string punctuation = "";
                        engine->luaBinding->callLuaFunction("getPunc", "s>s", keychrs.c_str(), &punctuation);
                        if (punctuation.length() > 0) {
                            // registered punctuation
                            if (engine->preedit->length() > 0) {
                                engine->cloudClient->request(*engine->activePreedit, fetchFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                                *engine->preedit = "";
                            }
                            engine->cloudClient->request(punctuation, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                            handled = true;
                        } else if (keyval == engine->startCorrectionKey) {
                            // switch to editing mode
                            if (engine->preedit->length() > 0) {
                                // edit active preedit
                                *engine->preedit = "";
                                *(engine->convertingPinyins) = *engine->activePreedit;
                                *engine->activePreedit = "";
                                // clear candidates, prepare for new candidates
                                ibus_lookup_table_clear(engine->table);
                                keyval = 0;
                                handled = true;
                                goto engineProcessKeyEventStart;
                            } else {
                                // TODO: try to edit selected text
                            }
                        } else if (keyval == IBUS_BackSpace) {
                            // backspace, remove last char from preedit or cancel last request
                            if (engine->preedit->length() > 0) {
                                engine->preedit->erase(engine->preedit->length() - 1, 1);
                                handled = true;
                            } else {
                                if (requestCount > 0) {
                                    engine->cloudClient->removeLastRequest();
                                    handled = true;
                                }
                            }
                        } else if (keyval == IBUS_Escape) {
                            if (engine->preedit->length() > 0) {
                                *engine->preedit = "";
                                handled = true;
                            }
                        } else if (keyval == IBUS_space) {
                            if (engine->preedit->length() > 0) {
                                // eat this space if we are going to commit preedit
                                keyval = 0;
                                keychrs = "";
                            }
                        } else if (keyval >= IBUS_a && keyval <= IBUS_z) {
                            // pinyin character
                            // IMPROVE: handle ';' in MSPY double pinyin scheme and "'" in full pinyin
                            if (engine->useDoublePinyin) {
                                if (LuaBinding::doublePinyinScheme.isValidDoublePinyin(*engine->preedit + keychr)) *engine->preedit += keychr;
                            } else {
                                *engine->preedit += keychr;
                            }
                            handled = true;
                        }

                        // update active preedit
                        if (engine->useDoublePinyin) {
                            *engine->activePreedit = LuaBinding::doublePinyinScheme.query(*engine->preedit);
                        } else {
                            *engine->activePreedit = *engine->preedit;
                        }
                    }

                    // eng mode or unhandled in chinese mode
                    if (handled || (requestCount == 0 && engine->preedit->length() == 0)) {
                        res = handled;
                        break;
                    } else {
                        if (engine->preedit->length() > 0) {
                            engine->cloudClient->request(*engine->activePreedit, fetchFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                            *engine->preedit = "";
                            *engine->activePreedit = "";
                        }
                        engine->cloudClient->request(keychrs, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                        res = TRUE;
                        break;
                    }
                    break;
                }
                default:
                    // let other program process this key
                    res = FALSE;
            }
        } // if (res == 0)
    }
    ENGINE_MUTEX_UNLOCK;

    // update preedit
    if (res) engineUpdatePreedit(engine);

    engine->lastProcessKeyResult = res;
    return res;
}

static void engineFocusIn(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] FocusIn\n");
    //ibus_engine_show_auxiliary_text((IBusEngine*) engine);
    //ibus_engine_show_lookup_table((IBusEngine*) engine);
}

static void engineFocusOut(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] FocusOut\n");
    //ibus_engine_hide_auxiliary_text((IBusEngine*) engine);
    //ibus_engine_hide_lookup_table((IBusEngine*) engine);
}

static void engineReset(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(1, "[ENGINE] Reset\n");
}

static void engineEnable(IBusSgpyccEngine * engine) {

    DEBUG_PRINT(1, "[ENGINE] Enable\n");
    engine->enabled = true;
}

static void engineDisable(IBusSgpyccEngine * engine) {

    DEBUG_PRINT(1, "[ENGINE] Disable\n");
    engine->enabled = false;
}

static void engineSetCursorLocation(IBusSgpyccEngine *engine, gint x, gint y, gint w, gint h) {
    DEBUG_PRINT(4, "[ENGINE] SetCursorLocation(%d, %d, %d, %d)\n", x, y, w, h);
    engine->cursor_area.height = h;
    engine->cursor_area.width = w;
    engine->cursor_area.x = x;
    engine->cursor_area.y = y;
}

static void engineSetCapabilities(IBusSgpyccEngine *engine, guint caps) {
    DEBUG_PRINT(2, "[ENGINE] SetCapabilities(%u)\n", caps);
    engine->client_capabilities = caps;
}

static void enginePageUp(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] PageUp\n");
    if (engine->convertingPinyins->length() > 0) {
        if (ibus_lookup_table_page_up(engine->table)) engine->tablePageNumber--;
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void enginePageDown(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] PageDown\n");
    if (engine->convertingPinyins->length() > 0) {
        if (ibus_lookup_table_page_down(engine->table)) engine->tablePageNumber++;
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineCursorUp(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorUp\n");
    if (engine->convertingPinyins->length() > 0) {
        ibus_lookup_table_cursor_up(engine->table);
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineCursorDown(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorDown\n");
    if (engine->convertingPinyins->length() > 0) {
        ibus_lookup_table_cursor_down(engine->table);
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineUpdatePreedit(IBusSgpyccEngine * engine) {
    // this function need a mutex lock, it will pop first several finished requests from cloudClient
    DEBUG_PRINT(1, "[ENGINE] Update Preedit\n");
    ENGINE_MUTEX_LOCK;

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
        IBusText *commitText = ibus_text_new_from_printf("%s", commitString.c_str());
        //void(*prevSigsegvHandler) (int sig);
        if (commitText) {
            // seems SEGMENTATION FAULT often happens here. I can not find the reason
            // ibus_engine_commit_text thread safe ? seems so without lua -.-
            ibus_engine_commit_text((IBusEngine *) engine, commitText);
            g_object_unref(G_OBJECT(commitText));
        } else {
            fprintf(stderr, "[ERROR] can not create commitText.\n");
        }
        DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] commited to client: %s\n", commitString.c_str());
    }

    // append current preedit (not belong to a request, still editable, active)

    string activePreedit = *engine->activePreedit;
    if (engine->convertingPinyins->length() > 0) {
        // only show convertingPinyins as activePreedit
        activePreedit = *engine->convertingPinyins;
    }
    preedit += activePreedit;
    DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] preedit: %s\n", preedit.c_str());

    // create IBusText-type preeditText
    IBusText *preeditText;
    preeditText = ibus_text_new_from_string(preedit.c_str());
    preeditText->attrs = ibus_attr_list_new();

    // second loop will set color to preeditText
    // note that '，' will be consided 1 char
    size_t preeditLen = 0;
    for (size_t i = finishedCount; i < requestCount; ++i) {
        const PinyinCloudRequest& request = engine->cloudClient->getRequest(i);
        size_t currReqLen;
        if (request.responsed) {
            // responsed
            IBusText *text = ibus_text_new_from_string(request.responseString.c_str());
            currReqLen = ibus_text_get_length(text);
            g_object_unref(text);
            // colors
            if (engine->requestedBackColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(engine->requestedBackColor, preeditLen, preeditLen + currReqLen));
            if (engine->requestedForeColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(engine->requestedForeColor, preeditLen, preeditLen + currReqLen));
        } else {
            // requesting
            IBusText *text = ibus_text_new_from_string(request.requestString.c_str());
            currReqLen = ibus_text_get_length(text);
            g_object_unref(text);
            // colors
            if (engine->requestingBackColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(engine->requestingBackColor, preeditLen, preeditLen + currReqLen));
            if (engine->requestingForeColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(engine->requestingForeColor, preeditLen, preeditLen + currReqLen));
        }
        preeditLen += currReqLen;
    }

    // colors, underline of (rightmost) active preedit
    // preeditLen now hasn't count rightmost active preedit
    if (engine->convertingPinyins->length() > 0) {
        // for convertingPinyins, use requesting color instead
        if (engine->requestingBackColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(engine->requestingBackColor, preeditLen, ibus_text_get_length(preeditText)));
        if (engine->requestingForeColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(engine->requestingForeColor, preeditLen, ibus_text_get_length(preeditText)));
    } else {
        if (engine->preeditBackColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(engine->preeditBackColor, preeditLen, ibus_text_get_length(preeditText)));
        if (engine->preeditForeColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(engine->preeditForeColor, preeditLen, ibus_text_get_length(preeditText)));
    }
    ibus_attr_list_append(preeditText->attrs, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, preeditLen, ibus_text_get_length(preeditText)));

    // finally, update preedit
    if (engine->convertingPinyins->length() > 0) {
        // cursor at left of active preedit
        ibus_engine_update_preedit_text((IBusEngine *) engine, preeditText, preeditLen, TRUE);
    } else {
        // cursor at rightmost
        ibus_engine_update_preedit_text((IBusEngine *) engine, preeditText, ibus_text_get_length(preeditText), TRUE);
    }
    // remember to unref text
    g_object_unref(preeditText);

    engine->cloudClient->readUnlock();

    // pop finishedCount from requeset queue, no lock here, it's safe.
    engine->cloudClient->removeFirstRequest(finishedCount);
    ENGINE_MUTEX_UNLOCK;
}

string fetchFunc(void* data, const string & requestString) {
    IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] fetchFunc(%s)\n", requestString.c_str());

    string res = engine->luaBinding->getValue(requestString.c_str(), "", "requestCache");

    if (res.length() == 0) {
        //if (luaBinding->callLuaFunction(true, "fetch", "s>s", requestString.c_str(), &res)) res = "";
        FILE* fresponse = popen((*(engine->fetcherPath) + " '" + requestString + "'") .c_str(), "r");

        char response[engine->fetcherBufferSize];
        fgets(response, sizeof (response), fresponse);
        res = response;
        pclose(fresponse);

        if (res.length() == 0) res = requestString;
        if (engine->writeRequestCache && requestString != res) {
            engine->luaBinding->setValue(requestString.c_str(), res.c_str(), "requestCache");
        }
    }
    return res;
}

string directFunc(void* data, const string & requestString) {
    //IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] directFunc(%s)\n", requestString.c_str());

    return requestString;
}

// Lua Cfunctions

/**
 * commit string to ibus client
 * in: string; out: none
 */
static int l_commitText(lua_State * L) {
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
    engine->cloudClient->request(string(lua_tostring(L, 1)), directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
#endif

    return 0; // return 0 value to lua code
}

/**
 * call request func (multi-threaded)
 * in: string; out: none
 */
static int l_sendRequest(lua_State * L) {
    luaL_checkstring(L, 1);
    IBusSgpyccEngine* engine = l2Engine[L];
    DEBUG_PRINT(1, "[ENGINE] l_sendRequest: %s\n", lua_tostring(L, 1));

    engine->cloudClient->request(string(lua_tostring(L, 1)), fetchFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);

    return 0; // return 0 value to lua code
}

/**
 * in: string; out: none
 */
static int l_correct(lua_State * L) {
    luaL_checkstring(L, 1);
    IBusSgpyccEngine* engine = l2Engine[L];
    DEBUG_PRINT(1, "[ENGINE] l_correct: %s\n", lua_tostring(L, 1));

    *engine->convertingPinyins = string(lua_tostring(L, 1));
    *engine->preedit = "";
    *engine->activePreedit = "";

    return 0; // return 0 value to lua code
}

/**
 * is idle now (no requset and no active preedit)
 * in: none; out: boolean
 */
static int l_isIdle(lua_State * L) {
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
static int l_removeLastRequest(lua_State * L) {
    IBusSgpyccEngine* engine = l2Engine[L];
    DEBUG_PRINT(2, "[ENGINE] l_removeLastRequest\n");

    engine->cloudClient->removeLastRequest();
    return 0;
}

/**
 * get X selection
 * in: none; out: string
 */
static int l_getSelection(lua_State * L) {
    IBusSgpyccEngine* engine = l2Engine[L];
    DEBUG_PRINT(2, "[ENGINE] l_removeLastRequest\n");
    lua_checkstack(L, 1);
    lua_pushstring(L, engine->xUtility->getSelection().c_str());
    return 1;
}
