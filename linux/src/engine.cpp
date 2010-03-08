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
    gboolean hasFocus;

    // cursor location
    IBusRectangle cursorArea;
    guint clientCapabilities;

    // full path of fetcher script
    string* fetcherPath;

    // buffer size receive return string from fetcher script
    int fetcherBufferSize;

    // lookup table
    IBusLookupTable *table;
    int candicateCount;

    // in lookup table, how many pages user has turned
    int tablePageNumber;

    // keys used in select item in lookup table
    string* tableLabelKeys;

    // colors
    int requestingBackColor, requestingForeColor;
    int requestedBackColor, requestedForeColor;
    int preeditForeColor, preeditBackColor;
    int correctingForeColor, correctingBackColor;

    // keys
    guint32 engModeToggleKey, startCorrectionKey, pageDownKey, pageUpKey;

    // boolean configs
    bool useDoublePinyin;
    bool writeRequestCache;

    // preedit: real key seq from user input, activePreedit: preedit on screen
    // they are not same if useDoublePinyin is true, the later shows full pinyin
    string *preedit, *activePreedit;

    // convertingPinyins are pinyin string in preedit and should be choiced from left to right manually
    string* correctingPinyins;
    string* commitedConvertingPinyins, *commitedConvertingCharacters;

    // eng mode, requesting (used to update requestingProp)
    bool engMode, requesting;

    // mainly internally used by processKeyEvent
    gboolean lastProcessKeyResult;
    guint32 lastKeyval;
    pthread_mutex_t engineMutex;

    // lua binding
    // complicated data stored in lua state: puncMap
    LuaBinding* luaBinding;

    // multi-thread begins here
    PinyinCloudClient* cloudClient;

    // prop list
    IBusPropList *propList;
    IBusProperty *engModeProp;
    IBusProperty *requestingProp;

    // selection timeout tolerance
    long long selectionMaxTimout;

    // database confs
    int dbResultLimit, dbLengthLimit;
    string* dbOrder;
    double dbLongPhraseAdjust;
#define INVALID_COLOR -1
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
    engineClass->property_activate = (typeof (engineClass->property_activate)) & enginePropertyActive;
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

    // set values
    engine->luaBinding->setValue("preedit", "");
    engine->luaBinding->doString(
#define LIB_NAME "pycc"
            LIB_NAME ".puncMap = { \
                ['.'] = '。', [','] = '，', ['^'] = '……', ['@'] = '·', ['!'] = '！', ['~'] = '～', \
                ['?'] = '？', ['#'] = '＃', ['$'] = '￥', ['&'] = '＆', ['('] = '（', [')'] = '）', \
                ['{'] = '｛', ['}'] = '｝', ['['] = '［', [']'] = '］', [';'] = '；', [':'] = '：', \
                ['<'] = '《', ['>'] = '》', ['\\\\'] = '、', \
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
    engine->luaBinding->setValue("sessionInit", true);
    engine->luaBinding->setValue("firstRun", false);

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
    engine->fetcherBufferSize = engine->luaBinding->getValue("fetcherBufferSize", 1024);

    // init pinyin cloud client
    engine->cloudClient = new PinyinCloudClient();

    // keys
    engine->engModeToggleKey = engine->luaBinding->getValue("engModeToggleKey", IBUS_Shift_L);
    engine->startCorrectionKey = engine->luaBinding->getValue("correctionKey", IBUS_Tab);
    engine->pageDownKey = engine->luaBinding->getValue("pageDownKey", (int) 'h');
    engine->pageUpKey = engine->luaBinding->getValue("pageUpKey", (int) 'g');

    // config booleans
    engine->useDoublePinyin = engine->luaBinding->getValue("useDoublePinyin", false);
    engine->engMode = engine->luaBinding->getValue("engMode", false);
    engine->writeRequestCache = engine->luaBinding->getValue("writeRequestCache", true);
    engine->requesting = false;

    // std::strings
    engine->preedit = new string();
    engine->activePreedit = new string();
    engine->commitedConvertingCharacters = new string();
    engine->commitedConvertingPinyins = new string();
    engine->correctingPinyins = new string();

    // internal vars
    engine->lastProcessKeyResult = FALSE;
    engine->lastKeyval = 0;

    // labels used in lookup table, ibus has 16 chars limition.
    engine->tableLabelKeys = new string(engine->luaBinding->getValue("labelKeys", "jkl;uiopasdf"));
    if (engine->tableLabelKeys->length() > 16 || engine->tableLabelKeys->length() == 0) *engine->tableLabelKeys = engine->tableLabelKeys->substr(0, 16);

    // lookup table
    engine->table = ibus_lookup_table_new(engine->tableLabelKeys->length(), 0, 0, 0);

    // ibus_lookup_table_set_orientation() is not available in ibus-1.2.0.20090927, provided by ubuntu 9.10
    // and since ibus-1.2.0.20090927 and ibus-1.2.0.20100111 use same version defines, program can not tell
    // if ibus_lookup_table_set_orientation() is available.
    // updated: use IBUS_CAP_SURROUNDING_TEXT marco to tell -,-
#ifdef IBUS_CAP_SURROUNDING_TEXT
    ibus_lookup_table_set_orientation(engine->table, engine->luaBinding->getValue("tableOrientation", IBUS_ORIENTATION_VERTICAL));
#endif

    for (size_t i = 0; i < engine->tableLabelKeys->length(); i++) {
        IBusText* text = ibus_text_new_from_printf("%c", engine->tableLabelKeys->data()[i]);
        ibus_lookup_table_append_label(engine->table, text);
        g_object_unref(text);
    }

    // punctuation map is stored in lua state, since it is complicated, do not store it here

    // database confs
    engine->dbResultLimit = engine->luaBinding->getValue("dbLimit", 64);
    engine->dbLengthLimit = engine->luaBinding->getValue("dbLength", 6);
    engine->dbLongPhraseAdjust = engine->luaBinding->getValue("dbPhraseAdjust", 1.0);
    engine->dbOrder = new string(engine->luaBinding->getValue("dbOrder", ""));

    if (engine->dbOrder->length() == 0) {
        if (LuaBinding::pinyinDatabases.size() > 0) *engine->dbOrder = "d";

        else *engine->dbOrder = "2";
    }

    // read in colors (-1: use default)
    engine->preeditForeColor = engine->luaBinding->getValue("preeditForeColor", 0x0050FF);
    engine->preeditBackColor = engine->luaBinding->getValue("preeditBackColor", INVALID_COLOR);
    engine->requestingForeColor = engine->luaBinding->getValue("requestingForeColor", INVALID_COLOR);
    engine->requestingBackColor = engine->luaBinding->getValue("requestingBackColor", 0xB8CFE5);
    engine->requestedForeColor = engine->luaBinding->getValue("requestedForeColor", 0x00C97F);
    engine->requestedBackColor = engine->luaBinding->getValue("requestedBackColor", INVALID_COLOR);
    engine->correctingForeColor = engine->luaBinding->getValue("correctingForeColor", INVALID_COLOR);
    engine->correctingBackColor = engine->luaBinding->getValue("correctingBackColor", 0xEDBE7D);

    // selection timeout set, user passed here is second
    engine->selectionMaxTimout = (long long) engine->luaBinding->getValue("selectionTimeout", 5.0) * XUtility::MICROSECOND_PER_SECOND;

    // properties
    engine->propList = ibus_prop_list_new();
    engine->engModeProp = ibus_property_new("engModeIndicator", PROP_TYPE_NORMAL, NULL, NULL, NULL, TRUE, TRUE, PROP_STATE_INCONSISTENT, NULL);
    engine->requestingProp = ibus_property_new("requestingIndicator", PROP_TYPE_NORMAL, NULL, NULL, NULL, TRUE, TRUE, PROP_STATE_INCONSISTENT, NULL);

    ibus_prop_list_append(engine->propList, engine->engModeProp);
    ibus_prop_list_append(engine->propList, engine->requestingProp);

    DEBUG_PRINT(1, "[ENGINE] Init completed\n");
}

static void engineDestroy(IBusSgpyccEngine *engine) {

    DEBUG_PRINT(1, "[ENGINE] Destroy\n");
    l2Engine.erase(engine->luaBinding->getLuaState());
    pthread_mutex_destroy(&engine->engineMutex);

    // delete strings
    delete engine->luaBinding;
    delete engine->cloudClient;
    delete engine->fetcherPath;
    delete engine->correctingPinyins;
    delete engine->tableLabelKeys;
    delete engine->preedit;
    delete engine->activePreedit;
    delete engine->dbOrder;
    delete engine->commitedConvertingCharacters;
    delete engine->commitedConvertingPinyins;

    // unref g objects
#define DELETE_G_OBJECT(x) if(x != NULL) g_object_unref(x), x = NULL;
    DELETE_G_OBJECT(engine->table);
    DELETE_G_OBJECT(engine->propList);
    DELETE_G_OBJECT(engine->engModeProp);
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

    ENGINE_MUTEX_LOCK;

    gboolean res = FALSE;

engineProcessKeyEventStart:
    // commit all non-pinyin part of convertingPinyins
    while (engine->correctingPinyins->length() > 0) {
        // try parse that pinyin at left of convertingPinyins
        string s = *(engine->correctingPinyins) + " ";

        int spacePosition = s.find(' ');
        string pinyin = s.substr(0, spacePosition);
        bool isPinyinParsable;

        // for double pinyin, parse full pinyin
        // for normal pinyin, parse partial pinyin

        if (engine->useDoublePinyin) isPinyinParsable = PinyinUtility::isValidPinyin(pinyin);
        else isPinyinParsable = PinyinUtility::isValidPartialPinyin(pinyin);

        if (isPinyinParsable) {
            // parsable pinyin, stop
            break;
        } else {
            // not found, submit 'pinyin' (it is indeed not a valid pinyin)
            if (pinyin.empty()) pinyin += " ";
            engine->cloudClient->request(pinyin, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
            engine->correctingPinyins->erase(0, spacePosition + 1);
        }
    }

    if (engine->correctingPinyins->length() > 0) {
        // cut off pinyins, get leftmost pinyin
        string s = *(engine->correctingPinyins) + " ";
        int spacePosition = s.find(' ');

        // try parse that pinyin (should parse successfully)
        string pinyin = s.substr(0, spacePosition);

        assert(PinyinUtility::isValidPartialPinyin(pinyin));
        if ((state & IBUS_RELEASE_MASK) == IBUS_RELEASE_MASK) {
            res = engine->lastProcessKeyResult;
        } else if (keyval == engine->pageDownKey) {
            enginePageDown(engine);
            res = TRUE;
        } else if (keyval == engine->pageUpKey) {
            enginePageUp(engine);
            res = TRUE;
        } else if (keyval == IBUS_Delete) {
            engine->correctingPinyins->erase(0, (*(engine->correctingPinyins) + " ").find(' ') + 1);
            keyval = 0;
            res = TRUE;
            engineClearLookupTable(engine);
            goto engineProcessKeyEventStart;
        } else if (keyval == IBUS_Escape) {
            // cancel correcting
            engine->correctingPinyins->clear();
            engine->commitedConvertingCharacters->clear();
            engine->commitedConvertingPinyins->clear();
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
                        g_object_unref(candidateCharacter);
                        if (i < l - 1) utf8char = g_utf8_next_char(utf8char);
                    }
                    g_object_unref(candidateCharacters);
                    engine->tablePageNumber = 0;
                    res = TRUE;
                } else {
                    // user select a phrase, commit it
                    int length = g_utf8_strlen(candidate->text, -1);
                    // use cloud client commit, do not direct commit !
                    engine->cloudClient->request(candidate->text, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                    // remove pinyin section from commitingPinyins
                    for (int i = 0; i < length; ++i) {
                        *engine->commitedConvertingPinyins += engine->correctingPinyins->substr(0, (*(engine->correctingPinyins) + " ").find(' ') + 1);
                        engine->correctingPinyins->erase(0, (*(engine->correctingPinyins) + " ").find(' ') + 1);
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
            for (const char *dbo = engine->dbOrder->c_str(); *dbo; ++dbo)
                switch (*dbo) {
                    case '2':
                    {
                        // gb2312 internal db
                        for (int tone = 1; tone <= 5; ++tone) {
                            IBusText* candidate = ibus_text_new_from_string((PinyinUtility::getCandidates(pinyin, tone)).c_str());
                            if (ibus_text_get_length(candidate) > 1) {
                                candidate->attrs = ibus_attr_list_new();
                                ibus_attr_list_append(candidate->attrs, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, 0, ibus_text_get_length(candidate)));
                            }
                            engineAppendLookupTable(engine, candidate);
                            g_object_unref(candidate);
                        }
                        break;
                    }
                    case 'd':
                    {
                        // external db
                        CandidateList cl;
                        set<string> cInserted;
                        for (map<string, PinyinDatabase*>::iterator it = LuaBinding::pinyinDatabases.begin(); it != LuaBinding::pinyinDatabases.end(); ++it) {
                            it->second->query(*(engine->correctingPinyins), cl, engine->dbResultLimit, engine->dbLongPhraseAdjust, engine->dbLengthLimit);
                        }
                        for (CandidateList::iterator it = cl.begin(); it != cl.end(); ++it) {
                            const string & candidate = it->second;
                            if (cInserted.find(candidate) == cInserted.end()) {
                                IBusText* candidateText = ibus_text_new_from_string(it->second.c_str());
                                engineAppendLookupTable(engine, candidateText);
                                g_object_unref(candidateText);
                                cInserted.insert(it->second);
                            }
                        }
                        if (cInserted.size() == 0) {
                            // put a dummy one
                            IBusText* dummyCandidate = ibus_text_new_from_string("?");
                            engineAppendLookupTable(engine, dummyCandidate);
                            g_object_unref(dummyCandidate);
                        }
                        break;
                    }
                } // for dborder
            // still zero result? put a dummy one
            if (engine->candicateCount == 0) {
                IBusText* dummyCandidate = ibus_text_new_from_string("-");
                engineAppendLookupTable(engine, dummyCandidate);
                g_object_unref(dummyCandidate);
            }
            IBusText* text = ibus_text_new_from_string(pinyin.c_str());
            ibus_engine_update_auxiliary_text((IBusEngine*) engine, text, TRUE);
            g_object_unref(text);
            engine->tablePageNumber = 0;
            res = TRUE;
        }
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);

    } else { // (engine->convertingPinyins->length() == 0)
        // check *engine->commitedConverting
        if (!engine->commitedConvertingCharacters->empty()) {
            // rtrim, assuming string::npos + 1 == 0
            engine->commitedConvertingCharacters->erase(engine->commitedConvertingCharacters->find_last_not_of(" \n\r\t") + 1);
            if (!engine->commitedConvertingCharacters->empty() && engine->writeRequestCache) {
                engine->luaBinding->setValue(engine->commitedConvertingPinyins->c_str(), engine->commitedConvertingCharacters->c_str(), "requestCache");
            }
            engine->commitedConvertingCharacters->clear();
            engine->commitedConvertingPinyins->clear();
        }

        // hide lookup table
        ibus_engine_hide_lookup_table((IBusEngine*) engine);
        ibus_engine_hide_auxiliary_text((IBusEngine*) engine);

        if (res == 0) {
            // try to use config logic to process key event
            switch (engine->luaBinding->callLuaFunction("processkey", "ddd>b", keyval, keycode, state, &res)) {
                case 0:
                    // lua function executed successfully, update engine->activePreedit ( = pycc.preedit in lua state ) and engmode
                    *(engine->activePreedit) = engine->luaBinding->getValue("preedit", engine->activePreedit->c_str());
                    engine->engMode = engine->luaBinding->getValue("engMode", engine->engMode);
                    engineUpdateProperties(engine);
                    break;
                case -1:
                {
                    // ignore some masks (Issue 8, Comment #11)
                    state = state & (IBUS_SHIFT_MASK | IBUS_LOCK_MASK | IBUS_CONTROL_MASK | IBUS_MOD1_MASK | IBUS_MOD4_MASK | IBUS_MOD5_MASK | IBUS_SUPER_MASK | IBUS_HYPER_MASK | IBUS_RELEASE_MASK | IBUS_META_MASK);

                    // fallback C key handler
                    engine->cloudClient->readLock();
                    int requestCount = engine->cloudClient->getRequestCount();
                    engine->cloudClient->readUnlock();

                    // normally do not handle release event, but watch for eng mode toggling
                    if ((state & IBUS_RELEASE_MASK) == IBUS_RELEASE_MASK && engine->lastKeyval == keyval) {
                        if (keyval == engine->engModeToggleKey) {
                            engine->engMode = !engine->engMode;
                            if (engine->engMode) {
                                *engine->preedit = "";
                                *engine->activePreedit = "";
                            }
                            engineUpdateProperties(engine);
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
                        if (keyval == engine->startCorrectionKey) {
                            // switch to editing mode
                            if (!engine->preedit->empty()) {
                                // edit active preedit
                                *engine->preedit = "";
                                *engine->correctingPinyins = *engine->activePreedit;
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
                                if (selectionTimeout > engine->selectionMaxTimout) selection = "";

                                // selection may be up to date, then check
                                if (selection.length() > 0 && selection.find('\n') == string::npos) {
                                    IBusText *emptyText = ibus_text_new_from_static_string("");
                                    // delete selection
                                    ibus_engine_commit_text((IBusEngine*) engine, emptyText);
                                    *engine->correctingPinyins = PinyinUtility::charactersToPinyins(selection);
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
                            if (engine->preedit->length() > 0) {
                                *engine->preedit = "";
                                handled = true;
                            }
                        } else if (keyval == IBUS_Return) {
                            // since IBUS_Return != '\n', handle this.
                            keychrs = "\n";
                        } else if (keyval == IBUS_space) {
                            if (engine->preedit->length() > 0) {
                                // eat this space if we are going to commit preedit
                                keyval = 0;
                                keychrs = "";
                            }
                        } else if (keyval < 128) {
                            // assuming that this is a pinyin char
                            // double pinyin may use ';' while full pinyin may use '\''
                            if (engine->useDoublePinyin) {
                                if (LuaBinding::doublePinyinScheme.isKeyBinded(keychr)) {
                                    if (LuaBinding::doublePinyinScheme.isValidDoublePinyin(*engine->preedit + keychr)) {
                                        *engine->preedit += keychr;
                                        handled = true;
                                    }
                                }
                                if (keyval >= IBUS_a && keyval <= IBUS_z) handled = true;
                            } else if ((keyval >= IBUS_a && keyval <= IBUS_z) || (keyval == '\'' && engine->preedit->length() > 0)) {
                                if (keyval == '\'') keychr = ' ';
                                *engine->preedit += keychr;
                                handled = true;
                            }
                        }

                        if (!handled) {
                            // check punctuation
                            string punctuation = "";
                            engine->luaBinding->callLuaFunction("getPunc", "s>s", keychrs.c_str(), &punctuation);
                            if (punctuation.length() > 0) {
                                // registered punctuation
                                if (engine->preedit->length() > 0) {
                                    engine->requesting = true;
                                    engineUpdateProperties(engine);
                                    engine->cloudClient->request(*engine->activePreedit, fetchFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                                    *engine->preedit = "";
                                }
                                engine->cloudClient->request(punctuation, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
                                handled = true;
                            }
                        }

                        // update active preedit
                        if (engine->useDoublePinyin) {
                            *engine->activePreedit = LuaBinding::doublePinyinScheme.query(*engine->preedit);
                        } else {
                            *engine->activePreedit = PinyinUtility::separatePinyins(*engine->preedit);
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

static void enginePropertyActive(IBusSgpyccEngine *engine, const gchar *propName, guint propState) {
    if (strcmp(propName, "engModeIndicator") == 0) {
        engine->engMode = !engine->engMode;
        engineUpdateProperties(engine);
    } else if (strcmp(propName, "requestingIndicator") == 0) {
        // do smth here, such as show statistics ?
        // show avg response time

        char statisticsBuffer[1024];
        snprintf(statisticsBuffer, sizeof (statisticsBuffer), "\n==== 统计数据 ====\n已发送请求: %d 个\n失败的请求: %d 个\n云服务器平均响应时间: %.3lf 秒\n云服务器最慢响应时间: %.3lf 秒\n", totalRequestCount, totalFailedRequestCount, totalResponseTime / totalRequestCount, maximumResponseTime);
        engine->cloudClient->request(statisticsBuffer, directFunc, (void*) engine, (ResponseCallbackFunc) engineUpdatePreedit, (void*) engine);
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
    ibus_engine_register_properties((IBusEngine*) engine, engine->propList);
}

static void engineFocusIn(IBusSgpyccEngine * engine) {

    DEBUG_PRINT(2, "[ENGINE] FocusIn\n");
    engineUpdateProperties(engine);
}

static void engineFocusOut(IBusSgpyccEngine * engine) {

    DEBUG_PRINT(2, "[ENGINE] FocusOut\n");
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
    if (engine->correctingPinyins->length() > 0) {

        if (ibus_lookup_table_page_up(engine->table)) engine->tablePageNumber--;
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void enginePageDown(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] PageDown\n");
    if (engine->correctingPinyins->length() > 0) {

        if (ibus_lookup_table_page_down(engine->table)) engine->tablePageNumber++;
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineCursorUp(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorUp\n");
    if (engine->correctingPinyins->length() > 0) {

        ibus_lookup_table_cursor_up(engine->table);
        ibus_engine_update_lookup_table((IBusEngine*) engine, engine->table, TRUE);
    }
}

static void engineCursorDown(IBusSgpyccEngine * engine) {
    DEBUG_PRINT(2, "[ENGINE] CursorDown\n");
    if (engine->correctingPinyins->length() > 0) {

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
    string commitString;
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
        if (commitText) {
            ibus_engine_commit_text((IBusEngine *) engine, commitText);
            g_object_unref(G_OBJECT(commitText));
        } else {
            fprintf(stderr, "[ERROR] can not create commitText.\n");
        }
        DEBUG_PRINT(4, "[ENGINE.UpdatePreedit] commited to client: %s\n", commitString.c_str());
    }

    // append current preedit (not belong to a request, still editable, active)

    string activePreedit = *engine->activePreedit;
    if (engine->correctingPinyins->length() > 0) {
        // only show convertingPinyins as activePreedit
        activePreedit = *engine->correctingPinyins;
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
    if (engine->correctingPinyins->length() > 0) {
        // for convertingPinyins, use requesting color instead
        if (engine->correctingBackColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(engine->correctingBackColor, preeditLen, ibus_text_get_length(preeditText)));
        if (engine->correctingForeColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(engine->correctingForeColor, preeditLen, ibus_text_get_length(preeditText)));
    } else {
        if (engine->preeditBackColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_background_new(engine->preeditBackColor, preeditLen, ibus_text_get_length(preeditText)));
        if (engine->preeditForeColor != INVALID_COLOR) ibus_attr_list_append(preeditText->attrs, ibus_attr_foreground_new(engine->preeditForeColor, preeditLen, ibus_text_get_length(preeditText)));
    }
    ibus_attr_list_append(preeditText->attrs, ibus_attr_underline_new(IBUS_ATTR_UNDERLINE_SINGLE, preeditLen, ibus_text_get_length(preeditText)));

    // finally, update preedit
    if (engine->correctingPinyins->length() > 0) {
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

    // update properties
    if (requestCount - finishedCount <= 0 && engine->requesting) {

        engine->requesting = false;
        engineUpdateProperties(engine);
    }

    ENGINE_MUTEX_UNLOCK;
}

string fetchFunc(void* data, const string & requestString) {
    IBusSgpyccEngine* engine = (typeof (engine)) data;
    DEBUG_PRINT(2, "[ENGINE] fetchFunc(%s)\n", requestString.c_str());

    string res = engine->luaBinding->getValue(requestString.c_str(), "", "requestCache");

    if (res.empty()) {
        char response[engine->fetcherBufferSize];

        // for statistics
        long long startMicrosecond = XUtility::getCurrentTime();

        FILE* fresponse = popen((*(engine->fetcherPath) + " '" + requestString + "'") .c_str(), "r");
        // IMPROVE: pipe may be empty and closed during this read (say, a empty fetcher script)
        // this will cause program to stop.

        // fgets may read '\n' in, remove it.
        fgets(response, sizeof (response), fresponse);
        pclose(fresponse);

        // update statistics
        totalRequestCount++;
        double requestTime = (XUtility::getCurrentTime() - startMicrosecond) / (double) XUtility::MICROSECOND_PER_SECOND;
        totalResponseTime += requestTime;
        if (requestTime > maximumResponseTime) maximumResponseTime = requestTime;

        for (int p = strlen(response) - 1; p >= 0; p--) {
            if (response[p] == '\n') response[p] = 0;
            else break;
        }

        res = response;

        if (res.empty()) {
            // empty, means fails
            totalFailedRequestCount++;
            res = requestString;
        } else {
            if (engine->writeRequestCache && requestString != res) {
                engine->luaBinding->setValue(requestString.c_str(), res.c_str(), "requestCache");
            }
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
    engine->requesting = true;
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

    *engine->correctingPinyins = string(lua_tostring(L, 1));
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
    lua_pushboolean(L, engine->cloudClient->getRequestCount() == 0 && engine->luaBinding->getValue("preedit", "").empty());
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
    DEBUG_PRINT(2, "[ENGINE] l_removeLastRequest\n");
    lua_checkstack(L, 1);
    string selection = XUtility::getSelection();
    lua_pushstring(L, selection.c_str());
    return 1;
}
