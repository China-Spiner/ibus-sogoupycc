/*
 * File:   engine.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <vector>

#include "defines.h"
#include "engine.h"
#include "LuaBinding.h"
#include "XUtility.h"
#include "Configuration.h"


typedef struct _IBusSgpyccEngine IBusSgpyccEngine;
typedef struct _IBusSgpyccEngineClass IBusSgpyccEngineClass;

using std::vector;
using std::string;
using Configuration::PunctuationMap;
using Configuration::ImeKey;

struct _IBusSgpyccEngine {
    IBusEngine parent;
    gboolean enabled;
    gboolean hasFocus;

    // cursor location
    IBusRectangle cursorArea;
    guint clientCapabilities;

    // lookup table
    IBusLookupTable *table;
    int candicateCount;

    // in lookup table, how many pages user has turned
    int tablePageNumber;
    size_t lookupTableLabelCount;

    // preedit: real key seq from user input, activePreedit: preedit on screen
    // they are not same if useDoublePinyin is true, the later shows full pinyin
    string *preedit, *activePreedit;

    // convertingPinyins are pinyin string in preedit and should be choiced from left to right manually
    PinyinSequence* correctings;
    //string* correctingPinyins;
    string* commitedConvertingPinyins, *commitedConvertingCharacters, *lastActivePreedit;

    // eng mode, requesting (used to update requestingProp)
    bool engMode, requesting;

    // mainly internally used by processKeyEvent
    gboolean lastProcessKeyResult;
    guint32 lastKeyval;
    pthread_mutex_t engineMutex;

    // lua binding
    // now it is indeed global
    LuaBinding* luaBinding;

    // cloud client, handle multi-thread requests
    PinyinCloudClient* cloudClient;

    // prop list
    IBusPropList *propList;
    IBusProperty *engModeProp;
    IBusProperty *requestingProp;
    IBusProperty *extensionMenuProp;

    // punc map (have states, should store here)
    // edit: use global, for punc_map may be updated
    PunctuationMap *punctuationMap;
};

struct _IBusSgpyccEngineClass {
    IBusEngineClass parent;
};

static IBusEngineClass *parentClass = NULL;

// statistics (static vars, otherwise data may not enough)
static int totalRequestCount = 0;
static int totalFailedRequestCount = 0;
static double totalResponseTime = .0;
static double maximumResponseTime = .0;

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
static void enginePropertyActive(IBusSgpyccEngine *engine, const gchar *prop_name, guint prop_state);

static void engineUpdatePreedit(IBusSgpyccEngine *engine);
static void engineUpdateProperties(IBusSgpyccEngine * engine);

// inline procedures
inline static void engineClearLookupTable(IBusSgpyccEngine *engine);
inline static void engineAppendLookupTable(IBusSgpyccEngine *engine, IBusText *candidate);

// fetcher functions (callback by cloudClient)
static string directFetcher(void* data, const string& requestString);
static string externalFetcher(void* data, const string& requestString);
static string luaFetcher(void* voidData, const string & requestString);
static string preFetcher(void* voidData, const string& requestString);

// request cache
static const string getRequestCache(IBusSgpyccEngine* engine, const string& requestString);
static void writeRequestCache(IBusSgpyccEngine* engine, const string& requsetSring, const string& content);

// paritical convert (using cache)
static const string particalConvert(IBusSgpyccEngine* engine, const string& pinyins);

inline void ibus_object_unref(gpointer object) {
#if !IBUS_CHECK_VERSION(1, 3, 0)
    g_object_unref(G_OBJECT(text));
#endif
}

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
    engineClass->property_activate = (typeof (engineClass->property_activate)) & enginePropertyActive;
    engineClass->set_capabilities = (typeof (engineClass->set_capabilities)) & engineSetCapabilities;
    engineClass->set_cursor_location = (typeof (engineClass->set_cursor_location)) & engineSetCursorLocation;

    // register Lua functions (lua state may be locked, so move them to luaBinding)
    // LuaBinding::getStaticBinding().addFunction(l_commitText, "commit");
    // LuaBinding::getStaticBinding().addFunction(l_sendRequest, "request");
}

static void engineInit(IBusSgpyccEngine *engine) {
    DEBUG_PRINT(1, "[ENGINE] engineInit\n");

    // init mutex
    pthread_mutexattr_t engineMutexAttr;
    pthread_mutexattr_init(&engineMutexAttr);
    pthread_mutexattr_settype(&engineMutexAttr, PTHREAD_MUTEX_ERRORCHECK);

    if (pthread_mutex_init(&engine->engineMutex, &engineMutexAttr)) {
        fprintf(stderr, "can't create mutex.\nprogram aborted.");
        exit(EXIT_FAILURE);
    }

    pthread_mutexattr_destroy(&engineMutexAttr);

    // init pinyin cloud client, lua binding
    engine->cloudClient = new PinyinCloudClient();
    engine->luaBinding = &LuaBinding::getStaticBinding();

    // booleans
    engine->engMode = Configuration::startInEngMode;
    engine->requesting = false;

    // std::strings
    engine->preedit = new string();
    engine->activePreedit = new string();
    engine->commitedConvertingCharacters = new string();
    engine->commitedConvertingPinyins = new string();
    engine->lastActivePreedit = new string();

    engine->correctings = new PinyinSequence();

    // internal vars
    engine->lastProcessKeyResult = FALSE;
    engine->lastKeyval = 0;

    // lookup table
    engine->lookupTableLabelCount = Configuration::tableLabelKeys.size();
    engine->table = ibus_lookup_table_new(engine->lookupTableLabelCount, 0, 0, 0);
#if IBUS_CHECK_VERSION(1, 3, 0)
    g_object_ref_sink(engine->table);
#endif

    // ibus_lookup_table_set_orientation() is not available in ibus-1.2.0.20090927, provided by ubuntu 9.10
    // and since ibus-1.2.0.20090927 and ibus-1.2.0.20100111 use same version defines, program can not tell
    // if ibus_lookup_table_set_orientation() is available.

    for (size_t i = 0; i < engine->lookupTableLabelCount; i++) {
        IBusText* text = ibus_text_new_from_printf("%s", Configuration::tableLabelKeys[i].getLabel().c_str());
        ibus_lookup_table_append_label(engine->table, text);
        ibus_object_unref(text);
    }

    if (engine->lookupTableLabelCount <= 0) {
        // fatal error
        fprintf(stderr, "FATAL Error: ime.label_keys invalid!\n");
        exit(EXIT_FAILURE);
    }

    // punctuation map
    // engine->punctuationMap = new PunctuationMap(Configuration::punctuationMap);
    engine->punctuationMap = &Configuration::punctuationMap;

    // properties
    engine->propList = ibus_prop_list_new();

    engine->engModeProp = ibus_property_new("engModeIndicator", PROP_TYPE_NORMAL, NULL, NULL, ibus_text_new_from_static_string("中英文模式切换"), TRUE, TRUE, PROP_STATE_INCONSISTENT, NULL);
    engine->requestingProp = ibus_property_new("requestingIndicator", PROP_TYPE_NORMAL, NULL, NULL, ibus_text_new_from_static_string("云服务器请求状态"), TRUE, TRUE, PROP_STATE_INCONSISTENT, NULL);
    engine->extensionMenuProp = ibus_property_new("extensionMenu", PROP_TYPE_MENU, NULL, PKGDATADIR "/icons/extensions.png", ibus_text_new_from_static_string("用户扩展"), TRUE, TRUE, PROP_STATE_INCONSISTENT, Configuration::extensionList);

#if IBUS_CHECK_VERSION(1, 3, 0)
    g_object_ref_sink(engine->propList);
    g_object_ref_sink(engine->engModeProp);
    g_object_ref_sink(engine->requestingProp);
    g_object_ref_sink(engine->extensionMenuProp);
#endif

    // extension sub props
    ibus_prop_list_append(engine->propList, engine->engModeProp);
    ibus_prop_list_append(engine->propList, engine->requestingProp);
    ibus_prop_list_append(engine->propList, engine->extensionMenuProp);

    DEBUG_PRINT(1, "[ENGINE] Init completed\n");
}

static void engineDestroy(IBusSgpyccEngine *engine) {
#define DELETE_G_OBJECT(x) if(x != NULL) g_object_unref(x), x = NULL;
    DEBUG_PRINT(1, "[ENGINE] Destroy\n");
    pthread_mutex_destroy(&engine->engineMutex);

    // delete strings
    delete engine->cloudClient;
    delete engine->preedit;
    delete engine->activePreedit;
    delete engine->commitedConvertingCharacters;
    delete engine->commitedConvertingPinyins;
    delete engine->correctings;
    delete engine->lastActivePreedit;

    // delete other things
    // delete engine->punctuationMap; // now global

    // unref g objects
    DELETE_G_OBJECT(engine->table);
    DELETE_G_OBJECT(engine->propList);
    DELETE_G_OBJECT(engine->engModeProp);
    DELETE_G_OBJECT(engine->requestingProp);
    DELETE_G_OBJECT(engine->extensionMenuProp);
#undef DELETE_G_OBJECT
    IBUS_OBJECT_CLASS(parentClass)->destroy((IBusObject *) engine);
}

// ibus_lookup_table_get_number_of_candidates() is not available in ibus-1.2.0.20090927, provided by ubuntu 9.10
// use these inline functions as alternative

inline static void engineAppendLookupTable(IBusSgpyccEngine *engine, IBusText *candidate) {
    ibus_lookup_table_append_candidate(engine->table, candidate);
    engine->candicateCount++;
}

inline static void engineClearLookupTable(IBusSgpyccEngine *engine) {
    ibus_lookup_table_clear(engine->table);
    engine->candicateCount = 0;
}

static gboolean engineProcessKeyEvent(IBusSgpyccEngine *engine, guint32 keyval, guint32 keycode, guint32 state) {
    DEBUG_PRINT(1, "[ENGINE] ProcessKeyEvent(%d, %d, 0x%x)\n", keyval, keycode, state);

#define CONSIDER_MASKS (IBUS_SHIFT_MASK | IBUS_LOCK_MASK | IBUS_CONTROL_MASK | IBUS_MOD1_MASK | IBUS_MOD4_MASK | IBUS_MOD5_MASK | IBUS_SUPER_MASK | IBUS_HYPER_MASK | IBUS_RELEASE_MASK | IBUS_META_MASK)
    // check extension hot key first
    if (((state & IBUS_RELEASE_MASK) == 0) && Configuration::activeExtension(keyval, state & CONSIDER_MASKS)) {
        engine->lastKeyval = IBUS_VoidSymbol, engine->lastProcessKeyResult = TRUE;
        return TRUE;
    }

    ENGINE_MUTEX_LOCK;
    gboolean res = FALSE;
engineProcessKeyEventStart:

    // try parse that pinyin at left of convertingPinyins
    while (!engine->correctings->empty()) {
        string pinyin = (*engine->correctings)[0];
        bool isPinyinParsable, isChineseCharacter;

        if (Configuration::useDoublePinyin) isPinyinParsable = PinyinUtility::isValidPinyin(pinyin);
        else isPinyinParsable = PinyinUtility::isValidPartialPinyin(pinyin);

        // try to parse it as a Chinsese character
        if (!isPinyinParsable) isChineseCharacter = isPinyinParsable = PinyinUtility::isRecognisedCharacter(pinyin);

        if (isPinyinParsable) {
            // parsable pinyin, stop
            break;
        } else {
            // not found, submit 'pinyin' (it is indeed not a valid pinyin)
            engine->correctings->removeAt(0);
            if (!isChineseCharacter && !engine->correctings->empty()) pinyin += " ";
            engine->cloudClient->request(pinyin, directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
        }
    }

    // still in corretion mode?
    if (!engine->correctings->empty()) {
        if ((state & IBUS_RELEASE_MASK) == IBUS_RELEASE_MASK) {
            res = engine->lastProcessKeyResult;
        } else if (Configuration::pageDownKey.match(keyval)) {
            enginePageDown(engine);
            res = TRUE;
        } else if (Configuration::pageUpKey.match(keyval)) {
            enginePageUp(engine);
            res = TRUE;
        } else if (keyval == IBUS_Delete) {
            engine->correctings->removeAt(0);
            keyval = 0;
            res = TRUE;
            engineClearLookupTable(engine);
            goto engineProcessKeyEventStart;
        } else if (keyval == IBUS_Escape) {
            // cancel correcting, submit all remaining
            engine->cloudClient->request(engine->correctings->toString(), directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
            engine->correctings->clear();
            engine->commitedConvertingCharacters->clear();
            engine->commitedConvertingPinyins->clear();
            XUtility::setSelectionUpdatedTime();
            keyval = 0;
            res = TRUE;
            goto engineProcessKeyEventStart;
        } else if (keyval == IBUS_BackSpace) {
            keyval = 0;
            res = TRUE;
            engineClearLookupTable(engine);
            goto engineProcessKeyEventStart;
        } else {
            // test if user press a key that in lookup table
            size_t pos = string::npos;
            for (size_t i = 0; i < engine->lookupTableLabelCount; ++i) {
                if (Configuration::tableLabelKeys[i].match(keyval)) {
                    pos = i;
                    break;
                }
            }
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
                } else if (candidate->attrs->attributes->len > 0) {
                    // underlined, need 2nd select
                    // build new table according to selected
                    int l = ibus_text_get_length(candidate);
                    // backup candidate before clear table
                    IBusText *candidateCharacters = ibus_text_new_from_string(candidate->text);
                    const gchar *utf8char = candidateCharacters->text;
                    engineClearLookupTable(engine);

                    for (int i = 0; i < l; ++i) {
                        IBusText *candidateCharacter = ibus_text_new_from_unichar(g_utf8_get_char(utf8char));
                        engineAppendLookupTable(engine, candidateCharacter);
                        ibus_object_unref(candidateCharacter);
                        if (i < l - 1) utf8char = g_utf8_next_char(utf8char);
                    }
                    ibus_object_unref(candidateCharacters);
                    engine->tablePageNumber = 0;
                    res = TRUE;
                } else {
                    // user select a phrase, commit it
                    int length = g_utf8_strlen(candidate->text, -1);
                    // use cloud client commit, do not direct commit !
                    engine->cloudClient->request(candidate->text, directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                    // remove pinyin section from commitingPinyins
                    for (int i = 0; i < length; ++i) {
                        if (!engine->commitedConvertingPinyins->empty()) *engine->commitedConvertingPinyins += ' ';
                        *engine->commitedConvertingPinyins += (*engine->correctings)[0].c_str();
                        engine->correctings->removeAt(0);
                    }
                    *engine->commitedConvertingCharacters += candidate->text;
                    // rescan, rebuild lookup table
                    engineClearLookupTable(engine);
                    keyval = 0;
                    res = TRUE;
                    goto engineProcessKeyEventStart;
                }
            }
        }

        // ibus_lookup_table_get_number_of_candidates() is not availble in ibus-1.2.0.20090927, provided by ubuntu 9.10
        // if (ibus_lookup_table_get_number_of_candidates(engine->table) == 0) {
        if (engine->candicateCount == 0) {
            // update and show lookup table
            // according to dbOrder
            engineClearLookupTable(engine);

            // default: use external dict if possible
            string dbOrder = Configuration::dbOrder;
            if (dbOrder.empty()) {
                if (LuaBinding::pinyinDatabases.size() > 0) dbOrder = "d";
                else dbOrder = "2";
            }

            for (const char *dbo = dbOrder.c_str(); *dbo; ++dbo)
                switch (*dbo) {
                    case '2':
                    {
                        // gb2312 internal db
                        string character = (*engine->correctings)[0];
                        // correct character
                        if (PinyinUtility::isRecognisedCharacter(character)) {
                            for (size_t i = 0;; i++) {
                                string pinyin = PinyinUtility::charactersToPinyins(character, i, false);
                                if (pinyin.empty()) break;
                                for (int tone = 1; tone <= 5; ++tone) {
                                    // IMPROVE: partical pinyin won't work here
                                    IBusText* candidate = ibus_text_new_from_string((PinyinUtility::getCandidates(pinyin, tone)).c_str());
                                    if (ibus_text_get_length(candidate) > 1) {
                                        candidate->attrs = ibus_attr_list_new();
                                        ibus_attr_list_append(candidate->attrs, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, 0, ibus_text_get_length(candidate)));
                                    }
                                    engineAppendLookupTable(engine, candidate);
                                    ibus_object_unref(candidate);
                                }
                            }
                        }
                        // pinyin ( user selecting )
                        if (PinyinUtility::isValidPinyin(character)) {
                            string pinyin = character;
                            for (int tone = 1; tone <= 5; ++tone) {
                                // IMPROVE: partical pinyin won't work here
                                IBusText* candidate = ibus_text_new_from_string((PinyinUtility::getCandidates(pinyin, tone)).c_str());
                                if (ibus_text_get_length(candidate) > 1) {
                                    candidate->attrs = ibus_attr_list_new();
                                    ibus_attr_list_append(candidate->attrs, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, 0, ibus_text_get_length(candidate)));
                                }
                                engineAppendLookupTable(engine, candidate);
                                ibus_object_unref(candidate);
                            }
                        }
                        break;
                    }
                    case 'd':
                    {
                        // external db
                        CandidateList cl;
                        set<string> cInserted;
                        string characters = engine->correctings->toString();
                        for (map<string, PinyinDatabase*>::iterator it = LuaBinding::pinyinDatabases.begin(); it != LuaBinding::pinyinDatabases.end(); ++it) {
                            for (size_t i = 0;; i++) {
                                string pinyins = PinyinUtility::charactersToPinyins(characters, i, false);
                                if (pinyins.empty()) break;
                                it->second->query(pinyins, cl, Configuration::dbResultLimit, Configuration::dbLongPhraseAdjust, Configuration::dbLengthLimit);
                            }
                        }
                        for (CandidateList::iterator it = cl.begin(); it != cl.end(); ++it) {
                            const string & candidate = it->second;
                            if (cInserted.find(candidate) == cInserted.end()) {
                                IBusText* candidateText = ibus_text_new_from_string(it->second.c_str());
                                engineAppendLookupTable(engine, candidateText);
                                ibus_object_unref(candidateText);
                                cInserted.insert(it->second);
                            }
                        }
                        if (cInserted.size() == 0) {
                            // put a dummy one
                            IBusText* dummyCandidate = ibus_text_new_from_string("?");
                            engineAppendLookupTable(engine, dummyCandidate);
                            ibus_object_unref(dummyCandidate);
                        }
                        break;
                    }
                } // for dborder
            // still zero result? put a dummy one
            if (engine->candicateCount == 0) {
                IBusText* dummyCandidate = ibus_text_new_from_string("-");
                engineAppendLookupTable(engine, dummyCandidate);
                ibus_object_unref(dummyCandidate);
            }
            IBusText* text = ibus_text_new_from_string((*engine->correctings)[0].c_str());
            ibus_engine_update_auxiliary_text((IBusEngine*) engine, text, TRUE);
            ibus_object_unref(text);
            engine->tablePageNumber = 0;
            res = TRUE;
        }
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);

    } else {
        // not in correction mode

        // check *engine->commitedConverting
        if (!engine->commitedConvertingCharacters->empty()) {
            // rtrim, assuming string::npos + 1 == 0
            engine->commitedConvertingCharacters->erase(engine->commitedConvertingCharacters->find_last_not_of(" \n\r\t") + 1);
            if (!engine->commitedConvertingCharacters->empty() && Configuration::writeRequestCache) {
                writeRequestCache(engine, *engine->commitedConvertingPinyins, *engine->commitedConvertingCharacters);
            }
            XUtility::setSelectionUpdatedTime();
            engine->commitedConvertingCharacters->clear();
            engine->commitedConvertingPinyins->clear();
        }

        // hide lookup table
        ibus_engine_hide_lookup_table((IBusEngine*) engine);
        ibus_engine_hide_auxiliary_text((IBusEngine*) engine);

        while (res == 0) { // use while here to make 'break' available
            // ignore some masks (Issue 8, Comment #11)
            state = state & CONSIDER_MASKS;

            // fallback C key handler
            engine->cloudClient->readLock();
            int requestCount = engine->cloudClient->getRequestCount();
            engine->cloudClient->readUnlock();

            // normally do not handle release event, but watch for eng mode toggling
            if ((state & IBUS_RELEASE_MASK) == IBUS_RELEASE_MASK && engine->lastKeyval == keyval) {
                bool engModeChanged = false;
                if ((engine->engMode && Configuration::chsModeKey.match(keyval)) || (!engine->engMode && Configuration::engModeKey.match(keyval))) {
                    engine->engMode = !engine->engMode;
                    engModeChanged = true;
                }
                if (engModeChanged) {
                    if (engine->engMode) {
                        *engine->preedit = "";
                        *engine->activePreedit = "";
                        if (Configuration::showNotification) XUtility::showStaticNotify("已切换到英文模式", NULL, PKGDATADIR "/icons/engmode-on.png");
                    } else {
                        if (Configuration::showNotification) XUtility::showStaticNotify("已切换到中文模式", NULL, PKGDATADIR "/icons/engmode-off.png");
                    }
                    engineUpdateProperties(engine);
                    res = TRUE;
                    break;
                }
                res = engine->lastProcessKeyResult;
                break; // break the while
            }
            engine->lastKeyval = keyval;

            // do not handle alt, ctrl ... event (such as Alt - F, Ctrl - T)
            // accept SHIFT + a 'A' event.
            if (state != 0 && (state ^ IBUS_SHIFT_MASK) != 0) {
                res = FALSE;
                break;
            }

            bool handled = false;
            char keychr = (keyval < 128) ? keyval : 0;
            string keychrs = "";
            if (keychr) keychrs += keychr;

            // since IBUS_Return != '\n', IBUS_Tab != '\t', handle this.
            if (keyval == IBUS_Return) {
                keychrs = "\n";
            } else if (keyval == IBUS_Tab) {
                keychrs = "\t";
            }

            // in chinese mode ?
            if (!engine->engMode) {
                // used by not strict double pinyin mode
                bool fallbackToFullPinyin = false;
                if (Configuration::startCorrectionKey.match(keyval)) {
                    // switch to editing mode
                    if (!engine->preedit->empty()) {
                        // edit active preedit
                        *engine->preedit = "";
                        *engine->correctings = *engine->activePreedit;
                        *engine->activePreedit = "";
                        // clear candidates, prepare for new candidates
                        engineClearLookupTable(engine);
                        keyval = 0;
                        handled = true;
                        goto engineProcessKeyEventStart;
                    } else if (requestCount == 0) {
                        // no preedit, no pending requests, we are clear.
                        // try to edit selected text
                        // check selected text do not contain '\n' and is not empty

                        // since ibus doesn't provide a method to get selected text,
                        // use this currently, it does add some dependencies and may has some issues ...
                        string selection = XUtility::getSelection();
                        long long selectionTimeout = XUtility::getCurrentTime() - XUtility::getSelectionUpdatedTime();
                        if (selectionTimeout > Configuration::selectionTimout) selection = "";

                        // selection may be up to date, then check
                        if (selection.length() > 0 && selection.find('\n') == string::npos) {
                            IBusText *emptyText = ibus_text_new_from_static_string("");
                            // delete selection
                            ibus_engine_commit_text((IBusEngine*) engine, emptyText);
                            *engine->correctings = selection;
                            keyval = 0;
                            handled = true;
                            goto engineProcessKeyEventStart;
                        }
                    } else {
                        // user may want to edit last commited pinyin string due to slow network
                        // due to lock issues, this is not implemented now
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
                    if (!engine->preedit->empty()) {
                        *engine->preedit = "";
                        handled = true;
                    }
                } else if (keyval == IBUS_space) {
                    if (!engine->preedit->empty()) {
                        // eat this space if we are going to commit preedit
                        keyval = 0;
                        keychrs = "";
                    }
                } else if (keyval < 128) {
                    // assuming that this is a pinyin char
                    // double pinyin may use ';' while full pinyin may use '\''
                    if (Configuration::useDoublePinyin) {
                        if (LuaBinding::doublePinyinScheme.isKeyBinded(keychr)) {
                            if (LuaBinding::doublePinyinScheme.isValidDoublePinyin(*engine->preedit + keychr)) {
                                *engine->preedit += keychr;
                                handled = true;
                            } else {
                                if (!Configuration::strictDoublePinyin) fallbackToFullPinyin = true;
                            }
                        }
                        if (keyval >= IBUS_a && keyval <= IBUS_z) handled = true;
                    }
                    if ((fallbackToFullPinyin || !Configuration::useDoublePinyin) && ((keyval >= IBUS_a && keyval <= IBUS_z) || (keyval == '\'' && engine->preedit->length() > 0))) {
                        if (keyval == '\'') keychr = ' ';
                        *engine->preedit += keychr;
                        handled = true;
                    }
                }

                if (!handled) {
                    // check punctuation
                    // todo: move to configuration
                    string punctuation = engine->punctuationMap->getFullPunctuation(keyval);
                    if (!punctuation.empty()) {
                        // found mapped punctuation
                        if (engine->preedit->length() > 0) {
                            engine->requesting = true;
                            engineUpdateProperties(engine);
                            engine->cloudClient->request(*engine->activePreedit, externalFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                            *engine->preedit = "";
                        }
                        engine->cloudClient->request(punctuation, directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                        handled = true;
                    }
                }

                // update active preedit
                if (!LuaBinding::doublePinyinScheme.isValidDoublePinyin(*engine->preedit)) fallbackToFullPinyin = true;
                if (Configuration::useDoublePinyin && fallbackToFullPinyin == false) {
                    *engine->activePreedit = LuaBinding::doublePinyinScheme.query(*engine->preedit);
                } else {
                    *engine->activePreedit = PinyinUtility::separatePinyins(*engine->preedit);
                }

                if (*engine->lastActivePreedit != *engine->activePreedit && !engine->activePreedit->empty()) {
                    // send prerequest
                    // handle case: wo ', if last pinyin is partial, do not perform prerequest
                    PinyinSequence ps = *engine->activePreedit;
                    string preRequestString;
                    if (PinyinUtility::isValidPinyin(ps[ps.size() - 1])) preRequestString = ps.toString();
                    else preRequestString = ps.toString(0, ps.size() - 1);
                    engine->cloudClient->preRequest(preRequestString, preFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                    *engine->lastActivePreedit = *engine->activePreedit;
                }
            }

            // eng mode or unhandled in chinese mode
            if (handled || (requestCount == 0 && engine->preedit->length() == 0)) {
                res = handled;
                break;
            } else {
                if (engine->preedit->length() > 0) {
                    engine->requesting = true;
                    engineUpdateProperties(engine);
                    engine->cloudClient->request(*engine->activePreedit, externalFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                    *engine->preedit = "";
                    *engine->activePreedit = "";
                }
                engine->cloudClient->request(keychrs, directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                res = TRUE;
                break;
            }
            break;
        } // while (res == 0)
    }
    ENGINE_MUTEX_UNLOCK;

    // update preedit
    if (res) engineUpdatePreedit(engine);

    engine->lastProcessKeyResult = res;

    return res;
}

static void enginePropertyActive(IBusSgpyccEngine *engine, const gchar *propName, guint propState) {
    if (strcmp(propName, "engModeIndicator") == 0) {
        engine->engMode = !engine->engMode;
        engineUpdateProperties(engine);
    } else if (strcmp(propName, "requestingIndicator") == 0) {
        // show avg response time ... info
        char statisticsBuffer[1024];
        if (totalRequestCount == 0) {
            snprintf(statisticsBuffer, sizeof (statisticsBuffer), "现在还没有数据\n");
        } else {
            snprintf(statisticsBuffer, sizeof (statisticsBuffer), "已发送请求: %d 个\n失败的请求: %d 个\n平均响应时间: %.3lf 秒\n最慢响应时间: %.3lf 秒\n", totalRequestCount, totalFailedRequestCount, totalResponseTime / totalRequestCount, maximumResponseTime);
        }
        if (Configuration::showNotification) {
            XUtility::showNotify("统计数据", statisticsBuffer);
        } else {
            engine->cloudClient->request("\n==== 统计数据 ====\n", directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
            engine->cloudClient->request(statisticsBuffer, directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
        }
    } else if (propName[0] == '.') {
        // extension action
        Configuration::activeExtension(propName + 1);
    }
}

static void engineUpdateProperties(IBusSgpyccEngine * engine) {
    if (engine->engMode) {
        ibus_property_set_icon(engine->engModeProp, PKGDATADIR "/icons/engmode-on.png");
    } else {
        ibus_property_set_icon(engine->engModeProp, PKGDATADIR "/icons/engmode-off.png");
    }
    if (engine->requesting) {
        ibus_property_set_icon(engine->requestingProp, PKGDATADIR "/icons/requesting.png");
    } else {
        ibus_property_set_icon(engine->requestingProp, PKGDATADIR "/icons/idle.png");
    }

    ibus_property_set_sensitive(engine->engModeProp, engine->enabled);
    ibus_property_set_sensitive(engine->requestingProp, engine->enabled);
    ibus_property_set_sensitive(engine->extensionMenuProp, engine->enabled && (Configuration::activeEngine != NULL));

    ibus_engine_register_properties((IBusEngine*) engine, engine->propList);
}

static void engineFocusIn(IBusSgpyccEngine* engine) {
    DEBUG_PRINT(2, "[ENGINE] FocusIn\n");
    Configuration::activeEngine = (typeof (Configuration::activeEngine))engine;
    engineUpdateProperties(engine);
}

static void engineFocusOut(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] FocusOut\n");
    Configuration::activeEngine = NULL;
    engineUpdateProperties(engine);
}

static void engineReset(IBusSgpyccEngine * engine) {

    DEBUG_PRINT(1, "[ENGINE] Reset\n");
}

static void engineEnable(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(1, "[ENGINE] Enable\n");
    engine->enabled = true;
    engineUpdateProperties(engine);
}

static void engineDisable(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(1, "[ENGINE] Disable\n");
    engine->enabled = false;
    engineUpdateProperties(engine);
    // cancel all requests
    engine->cloudClient->removeFirstRequest(INT_MAX);
    engineUpdatePreedit(engine);
}

static void engineSetCursorLocation(IBusSgpyccEngine *engine, gint x, gint y, gint w, gint h) {

    DEBUG_PRINT(7, "[ENGINE] SetCursorLocation(%d, %d, %d, %d)\n", x, y, w, h);
    engine->cursorArea.height = h;
    engine->cursorArea.width = w;
    engine->cursorArea.x = x;
    engine->cursorArea.y = y;
}

static void engineSetCapabilities(IBusSgpyccEngine *engine, guint caps) {
    DEBUG_PRINT(2, "[ENGINE] SetCapabilities(%u)\n", caps);
    engine->clientCapabilities = caps;
#ifdef IBUS_CAP_SURROUNDING_TEXT
    if (engine->clientCapabilities & IBUS_CAP_SURROUNDING_TEXT) {
        DEBUG_PRINT(3, "[ENGINE] SetCapabilities: IBUS_CAP_SURROUNDING_TEXT\n");
    }
#endif
}

static void enginePageUp(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] PageUp\n");
    if (!engine->correctings->empty()) {
        if (ibus_lookup_table_page_up(engine->table)) engine->tablePageNumber--;
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void enginePageDown(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] PageDown\n");
    if (!engine->correctings->empty()) {
        if (ibus_lookup_table_page_down(engine->table)) engine->tablePageNumber++;
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineCursorUp(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorUp\n");
    if (!engine->correctings->empty()) {
        ibus_lookup_table_cursor_up(engine->table);
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineCursorDown(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorDown\n");
    if (!engine->correctings->empty()) {
        ibus_lookup_table_cursor_down(engine->table);
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static const string particalConvert(IBusSgpyccEngine* engine, const string& pinyins) {
    if (Configuration::showCachedInPreedit == false) return pinyins;
    // check pre request result
    PinyinSequence ps = pinyins;
    for (size_t i = ps.size(); i > 0; i--) {
        if (i < ps.size() || PinyinUtility::isValidPinyin(ps[i - 1])) {
            string cache = getRequestCache(engine, ps.toString(0, i));
            DEBUG_PRINT(5, "[ENGINE] partical convert find cache: %s\n", cache.c_str());
            if (!cache.empty()) {
                string remainingPinyins = ps.toString(i, 0);
                DEBUG_PRINT(5, "[ENGINE] partical convert, remaining: %s\n", remainingPinyins.c_str());
                if (remainingPinyins.empty()) return cache;
                else return cache + " " + remainingPinyins;
            }
        }
    }
    return pinyins;
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

    // commit front responed string, set preedit string and count finishedCount
    // and, set up color list
    string commitString;
    size_t preeditLen = 0;
    IBusAttrList *textAttrList = ibus_attr_list_new();

#if IBUS_CHECK_VERSION(1, 3, 0)
    g_object_ref_sink(textAttrList);
#endif
    for (size_t i = 0; i < requestCount; ++i) {
        size_t currReqLen;
        const PinyinCloudRequest& request = engine->cloudClient->getRequest(i);
        if (!request.responsed) canCommitToClient = false;

        if (canCommitToClient) {
            if (request.responseString.length() > 0) commitString += request.responseString;
            finishedCount++;
        } else {
            if (request.responsed) {
                preedit += request.responseString;
                currReqLen = g_utf8_strlen(request.responseString.c_str(), -1);
                // colors
                if (Configuration::requestedBackColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_background_new(Configuration::requestedBackColor, preeditLen, preeditLen + currReqLen));
                if (Configuration::requestedForeColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_foreground_new(Configuration::requestedForeColor, preeditLen, preeditLen + currReqLen));
            } else {
                string requestString = particalConvert(engine, request.requestString);
                preedit += requestString;
                currReqLen = g_utf8_strlen(requestString.c_str(), -1);
                // colors
                if (Configuration::requestingBackColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_background_new(Configuration::requestingBackColor, preeditLen, preeditLen + currReqLen));
                if (Configuration::requestingForeColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_foreground_new(Configuration::requestingForeColor, preeditLen, preeditLen + currReqLen));
            }
            preeditLen += currReqLen;
        }
    }

    if (commitString.length() > 0) {
        IBusText *commitText = ibus_text_new_from_printf("%s", commitString.c_str());
        if (commitText) {
            ibus_engine_commit_text((IBusEngine *) engine, commitText);
            ibus_object_unref(commitText);
        } else {
            fprintf(stderr, "[ERROR] can not create commitText.\n");
        }
        DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] commited to client: %s\n", commitString.c_str());
    }

    // append current preedit (not belong to a request, still editable, active)
    string activePreedit = *engine->activePreedit;
    if (!engine->correctings->empty()) {
        // only show convertingPinyins as activePreedit
        activePreedit = engine->correctings->toString();
    } else if (Configuration::showCachedInPreedit) {
        activePreedit = particalConvert(engine, activePreedit);
    }
    preedit += activePreedit;

    // create IBusText-type preeditText
    size_t preeditFullLen = g_utf8_strlen(preedit.c_str(), -1);

    DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] preedit: %s, full len: %d\n", preedit.c_str(), preeditFullLen);

    // colors, underline of (rightmost) active preedit
    // preeditLen now hasn't count rightmost active preedit
    if (!engine->correctings->empty()) {
        // for convertingPinyins, use requesting color instead
        if (Configuration::correctingBackColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_background_new(Configuration::correctingBackColor, preeditLen, preeditFullLen));
        if (Configuration::correctingForeColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_foreground_new(Configuration::correctingForeColor, preeditLen, preeditFullLen));
    } else {
        if (Configuration::preeditBackColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_background_new(Configuration::preeditBackColor, preeditLen, preeditFullLen));
        if (Configuration::preeditForeColor != INVALID_COLOR) ibus_attr_list_append(textAttrList, ibus_attr_foreground_new(Configuration::preeditForeColor, preeditLen, preeditFullLen));
    }
    ibus_attr_list_append(textAttrList, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, preeditLen, preeditFullLen));

    DEBUG_PRINT(5, "[ENGINE.UpdatePreedit] attr length: %d\n", textAttrList->attributes->len);
    IBusText *preeditText;
    preeditText = ibus_text_new_from_string(preedit.c_str());
    preeditText->attrs = textAttrList;

    // finally, update preedit
    if (engine->correctings->empty()) {
        // cursor at rightmost
        ibus_engine_update_preedit_text((IBusEngine *) engine, preeditText, preeditFullLen, TRUE);
    } else {
        // cursor at left of active preedit
        ibus_engine_update_preedit_text((IBusEngine *) engine, preeditText, preeditLen, TRUE);
    }
    // remember to unref text
    ibus_object_unref(preeditText);

    engine->cloudClient->readUnlock();

    // pop finishedCount from requeset queue, no lock here, it's safe.
    engine->cloudClient->removeFirstRequest(finishedCount);

    // update properties
    if (requestCount - finishedCount <= 0 && engine->requesting) {
        engine->requesting = false;
        engineUpdateProperties(engine);
    }

    ENGINE_MUTEX_UNLOCK;
}

// callback by PinyinCloudClient

string externalFetcher(void* data, const string & requestString) {
    IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] fetchFunc(%s)\n", requestString.c_str());

    string res = getRequestCache(engine, requestString);

    if (res.empty()) {
        char response[Configuration::fetcherBufferSize];

        // for statistics
        long long startMicrosecond = XUtility::getCurrentTime();

        FILE* fresponse = popen(string((Configuration::fetcherPath) + " '" + requestString + "'").c_str(), "r");
        // IMPROVE: pipe may be empty and closed during this read (say, a empty fetcher script)
        // this will cause program to stop.

        // fgets may read '\n' in, remove it.
        fgets(response, sizeof (response), fresponse);
        pclose(fresponse);
        for (int p = strlen(response) - 1; p >= 0; p--) {
            if (response[p] == '\n') response[p] = 0;
            else break;
        }

        // update statistics
        totalRequestCount++;
        double requestTime = (XUtility::getCurrentTime() - startMicrosecond) / (double) XUtility::MICROSECOND_PER_SECOND;
        totalResponseTime += requestTime;
        if (requestTime > maximumResponseTime) maximumResponseTime = requestTime;

        // try read cache first if fails
        if ((res = response).empty()) res = getRequestCache(engine, requestString);

        if ((res = response).empty()) {
            // empty, means fails
            totalFailedRequestCount++;
            // try partial convert
            res = particalConvert(engine, requestString);
        } else {
            if (Configuration::writeRequestCache && requestString != res) {
                writeRequestCache(engine, requestString, res);
            }
        }
    }
    return res;
}

static const string getRequestCache(IBusSgpyccEngine* engine, const string& requestString) {
    return engine->luaBinding->getValue(requestString.c_str(), "", "request_cache");
}

static void writeRequestCache(IBusSgpyccEngine* engine, const string& requsetSring, const string& content) {
    engine->luaBinding->setValue(requsetSring.c_str(), content.c_str(), "request_cache");
}

static string directFetcher(void* data, const string & requestString) {
    DEBUG_PRINT(2, "[ENGINE] directFunc(%s)\n", requestString.c_str());

    return requestString;
}

static string preFetcher(void* data, const string& requestString) {
    IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] preFetcher(%s)\n", requestString.c_str());

    string res = getRequestCache(engine, requestString);

    if (res.empty()) {
        char timeLimitBuffer[64];
        char response[Configuration::fetcherBufferSize];

        // for statistics
        long long startMicrosecond = XUtility::getCurrentTime();

        snprintf(timeLimitBuffer, sizeof (timeLimitBuffer), " '%.2lf'", Configuration::preRequestTimeout);
        FILE* fresponse = popen(string((Configuration::fetcherPath) + " '" + requestString + "'" + timeLimitBuffer).c_str(), "r");

        // fgets may read '\n' in, remove it.
        fgets(response, sizeof (response), fresponse);
        pclose(fresponse);

        for (int p = strlen(response) - 1; p >= 0; p--) {
            if (response[p] == '\n') response[p] = 0;
            else break;
        }

        if ((res = response).empty()) {
            // empty, means fails
            res = requestString;
        } else {
            // update statistics
            totalRequestCount++;
            double requestTime = (XUtility::getCurrentTime() - startMicrosecond) / (double) XUtility::MICROSECOND_PER_SECOND;
            totalResponseTime += requestTime;
            if (requestTime > maximumResponseTime) maximumResponseTime = requestTime;
            if (Configuration::writeRequestCache && requestString != res) writeRequestCache(engine, requestString, res);
        }
    }
    return res;
}

struct LuaFuncData {
    IBusSgpyccEngine *engine;
    string luaFuncName;
};

static string luaFetcher(void* voidData, const string & requestString) {
    DEBUG_PRINT(2, "[ENGINE] luaFunc(%s)\n", requestString.c_str());
    LuaFuncData *data = (LuaFuncData*) voidData;
    string response;
    data->engine->luaBinding->callLuaFunction(data->luaFuncName.c_str(), "s>s", requestString.c_str(), &response);
    delete (LuaFuncData*) data;
    // fails?
    if (response.empty()) response = requestString;
    return response;
}

int Engine::l_commitText(lua_State * L) {
    lua_tostring(L, 1);
    DEBUG_PRINT(1, "[ENGINE] l_commitText: %s\n", lua_tostring(L, 1));
    luaL_checkstring(L, 1);
    IBusSgpyccEngine* engine = (IBusSgpyccEngine*) Configuration::activeEngine;
    if (engine)
        engine->cloudClient->request(string(lua_tostring(L, 1)), directFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
    return 0; // return 0 value to lua code
}

int Engine::l_sendRequest(lua_State * L) {
    luaL_checkstring(L, 1);
    DEBUG_PRINT(1, "[ENGINE] l_sendRequest: %s\n", lua_tostring(L, 1));
    IBusSgpyccEngine* engine = (IBusSgpyccEngine*) Configuration::activeEngine;
    if (!engine) return 0;

    engine->requesting = true;
    if (lua_type(L, 2) == LUA_TSTRING) {
        LuaFuncData *data = new LuaFuncData();
        data->luaFuncName = lua_tostring(L, 2);
        data->engine = engine;
        engine->cloudClient->request(string(lua_tostring(L, 1)), luaFetcher, (void*) data, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
    } else {
        engine->cloudClient->request(string(lua_tostring(L, 1)), externalFetcher, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
    }
    // delete selection
    IBusText *emptyText = ibus_text_new_from_static_string("");
    ibus_engine_commit_text((IBusEngine*) engine, emptyText);
    ibus_object_unref(emptyText);
    engineUpdatePreedit(engine);
    return 0; // return 0 value to lua code
}
