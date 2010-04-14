/* 
 * File:   Configuration.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "Configuration.h"
#include "defines.h"
#include "XUtility.h"
#include <ibus.h>

namespace Configuration {
    void* activeEngine = NULL;

    // engine global extensions
    // IMPROVE: use some lock here?
    // note: currently single thread.
    static vector<Extension*> extensions;
    IBusPropList *extensionList = NULL;

    // full path of fetcher script
    string fetcherPath = PKGDATADIR "/fetcher";

    // buffer size receive return string from fetcher script
    const int fetcherBufferSize = 1024;

    // keys used in select item in lookup table
    vector<ImeKey> tableLabelKeys;

    // colors
    int requestingBackColor = 0xB8CFE5, requestingForeColor = INVALID_COLOR;
    int requestedBackColor = INVALID_COLOR, requestedForeColor = 0x0024B2;
    int preeditBackColor = INVALID_COLOR, preeditForeColor = 0x0050FF;
    int correctingBackColor = 0xFFB442, correctingForeColor = INVALID_COLOR;
    int cloudCacheBackColor = INVALID_COLOR, cloudCacheForeColor = 0x0050FF;
    int localDbBackColor = INVALID_COLOR, localDbForeColor = 0x8C8C8C;
    int cloudCacheCandicateColor = 0x0050FF;
    int cloudWordsCandicateColor = 0x00A811;
    int databaseCandicateColor = INVALID_COLOR;
    int internalCandicateColor = 0x8C8C8C;

    // keys
    ImeKey startCorrectionKey = IBUS_Tab,
            engModeKey = IBUS_Shift_L,
            chsModeKey = IBUS_Shift_L,
            pageDownKey = 'h',
            pageUpKey = 'g',
            quickResponseKey = IBUS_Alt_R,
            commitRawPreeditKey = IBUS_Shift_R;

    // boolean configs
    bool useDoublePinyin = false;
    bool strictDoublePinyin = false;
    bool startInEngMode = false;
    bool writeRequestCache = true;
    bool showNotification = true;
    bool preRequest = true;
    bool showCachedInPreedit = true;
    bool staticNotification = false;
    bool fallbackUsingDb = true;
    bool preRequestFallback = true;
    bool useAlternativePopen = true;

    // int
    int fallbackEngTolerance = 5;
    int preRequestRetry = 4;
    int preeditReservedPinyinCount = 0;

    // pre request timeout
    double preRequestTimeout = 0.6;
    double requestTimeout = 12.;

    // selection timeout tolerance
    long long selectionTimout = 3LL * XUtility::MICROSECOND_PER_SECOND;

    // punctuations
    PunctuationMap punctuationMap;
    string autoWidthPunctuations = ".,:";

    // multi tone chineses
    size_t multiToneLimit = 4;

    // database confs
    int dbResultLimit = 128, dbLengthLimit = 10;
    string dbOrder = "cwd2";
    double dbLongPhraseAdjust = 1.2;
    double dbCompleteLongPhraseAdjust = 4;

    // class FullPunctuation

    FullPunctuation::FullPunctuation() {
        index = 0;
    }

    FullPunctuation::FullPunctuation(const string punction) {
        addPunctuation(punction);
        index = 0;
    }

    FullPunctuation::FullPunctuation(size_t count, ...) {
        va_list vl;
        va_start(vl, count);
        for (size_t i = 0; i < count; i++) {
            string punction = va_arg(vl, const char*);
            addPunctuation(punction);
        }
        index = 0;
    }

    void FullPunctuation::addPunctuation(const string punction) {
        punctions.push_back(punction);
    }

    FullPunctuation::FullPunctuation(const FullPunctuation& orig) {
        punctions = orig.punctions;
        index = 0;
    }

    const string FullPunctuation::getPunctuation() {
        DEBUG_PRINT(6, "[CONF] getPunctuation, index = %d\n", index);
        string r;
        if (punctions.size() > index) r = punctions[index];
        if (++index >= punctions.size()) index = 0;
        return r;
    }

    // class PunctuationMap

    PunctuationMap::PunctuationMap() {
    }

    PunctuationMap::PunctuationMap(const PunctuationMap& orig) {
        punctuationMap = orig.punctuationMap;
    }

    void PunctuationMap::setPunctuationPair(unsigned int key, FullPunctuation punctuation) {
        punctuationMap[key] = punctuation;
    }

    const string PunctuationMap::getFullPunctuation(const unsigned int punctionKey) const {
        DEBUG_PRINT(4, "[CONF] getFullPunctuation(0x%x)\n", punctionKey);
        if (punctuationMap.count(punctionKey)) {
            return punctuationMap.find(punctionKey)->second.getPunctuation();
        } else return "";
    }

    void PunctuationMap::clear() {
        punctuationMap.clear();
    }

    const string ImeKey::getLabel() const {
        return label;
    }

    bool ImeKey::readFromLua(LuaBinding& luaBinding, const string& varName) {
        switch (luaBinding.getValueType(varName.c_str())) {
            case LUA_TTABLE:
            {
                keys.clear();
                // multi keys
                char buffer[128];
                label = luaBinding.getValue((varName + ".label").c_str(), "");
                DEBUG_PRINT(5, "[CONF] Multi hot key - label: '%s'\n", label.c_str());
                for (size_t index = 1;; index++) {
                    snprintf(buffer, sizeof (buffer), "%s..%u", varName.c_str(), index);
                    unsigned int key = IBUS_VoidSymbol;
                    switch (luaBinding.getValueType(buffer)) {
                        case LUA_TSTRING:
                            key = luaBinding.getValue(buffer, " ")[0];
                            break;
                        case LUA_TNUMBER:
                            key = luaBinding.getValue(buffer, IBUS_VoidSymbol);
                    }
                    DEBUG_PRINT(5, "[CONF] Multi hot key - got: 0x%x\n", key);
                    if (key != IBUS_VoidSymbol) {
                        keys.insert(key);
                        if (keys.size() == 1 && label.empty()) label = string("") + (char) key;
                    } else break;
                }
                DEBUG_PRINT(4, "[CONF] Multi hot key %s - size: %d\n", label.c_str(), keys.size());
                return true;
            }
            case LUA_TNUMBER:
            {
                keys.clear();
                unsigned int key = (unsigned int) luaBinding.getValue(varName.c_str(), (int) IBUS_VoidSymbol);
                keys.insert(key);
                label = string("") + (char) key;
                return true;
            }
            case LUA_TSTRING:
            {
                keys.clear();
                unsigned key = luaBinding.getValue(varName.c_str(), "\f")[0];
                keys.insert(key);
                label = string("") + (char) key;
                return true;
            }
            default:
                // do nothing, keep current value
                return false;
        }
    }

    // ImeKey

    ImeKey::ImeKey(const unsigned int keyval) {
        keys.insert(keyval);
        label = string("") + (char) keyval;
    }

    ImeKey::ImeKey(const ImeKey& orig) {
        label = orig.label;
        keys = orig.keys;
    }

    const bool ImeKey::match(const unsigned int keyval) const {
        if (keyval == 0 || keyval == IBUS_VoidSymbol) return false;
        return (keys.count(keyval) > 0);
    }

    // Extension

    Extension::Extension(ImeKey key, unsigned int keymask, string label, string script) {
        this->key = key;
        this->keymask = keymask;
        this->script = script;
        this->label = label;
        this->prop = ibus_property_new((string(".") + label).c_str(), PROP_TYPE_NORMAL, ibus_text_new_from_string(label.c_str()), NULL, NULL, TRUE, TRUE, PROP_STATE_INCONSISTENT, NULL);
#if IBUS_CHECK_VERSION(1, 2, 98)
        g_object_ref_sink(this->prop);
#endif
        ibus_prop_list_append(extensionList, this->prop);
    }

    Extension::~Extension() {
        if (this->prop) g_object_unref(this->prop), this->prop = NULL;
    }

    const string Extension::getLabel() const {
        return label;
    }

    void Extension::execute() const {
        LuaBinding::getStaticBinding().doString(script.c_str(), true);
    }

    bool Extension::matchKey(unsigned int keyval, unsigned keymask) const {
        return (keymask == this->keymask) && this->key.match(keyval);
    }

    // functions

    void staticInit() {
        DEBUG_PRINT(3, "[CONF] staticInit\n");
        // init complex structs, such as punctuations
        punctuationMap.setPunctuationPair('.', FullPunctuation("。"));
        punctuationMap.setPunctuationPair(',', FullPunctuation("，"));
        punctuationMap.setPunctuationPair('^', FullPunctuation("……"));
        punctuationMap.setPunctuationPair('@', FullPunctuation("·"));
        punctuationMap.setPunctuationPair('!', FullPunctuation("！"));
        punctuationMap.setPunctuationPair('~', FullPunctuation("～"));
        punctuationMap.setPunctuationPair('?', FullPunctuation("？"));
        punctuationMap.setPunctuationPair('#', FullPunctuation("＃"));
        punctuationMap.setPunctuationPair('$', FullPunctuation("￥"));
        punctuationMap.setPunctuationPair('&', FullPunctuation("＆"));
        punctuationMap.setPunctuationPair('(', FullPunctuation("（"));
        punctuationMap.setPunctuationPair(')', FullPunctuation("）"));
        punctuationMap.setPunctuationPair('{', FullPunctuation("｛"));
        punctuationMap.setPunctuationPair('}', FullPunctuation("｝"));
        punctuationMap.setPunctuationPair(']', FullPunctuation("［"));
        punctuationMap.setPunctuationPair(']', FullPunctuation("］"));
        punctuationMap.setPunctuationPair(';', FullPunctuation("；"));
        punctuationMap.setPunctuationPair(':', FullPunctuation("："));
        punctuationMap.setPunctuationPair('<', FullPunctuation("《"));
        punctuationMap.setPunctuationPair('>', FullPunctuation("》"));
        punctuationMap.setPunctuationPair('\\', FullPunctuation("、"));
        punctuationMap.setPunctuationPair('\'', FullPunctuation(2, "‘", "’"));
        punctuationMap.setPunctuationPair('\"', FullPunctuation(2, "“", "”"));

        // tableLabelKeys = "jkl;uiopasdf";
        tableLabelKeys.push_back('j');
        tableLabelKeys.push_back('k');
        tableLabelKeys.push_back('l');
        tableLabelKeys.push_back(';');
        tableLabelKeys.push_back('u');
        tableLabelKeys.push_back('i');
        tableLabelKeys.push_back('o');
        tableLabelKeys.push_back('p');
        tableLabelKeys.push_back('a');
        tableLabelKeys.push_back('s');
        tableLabelKeys.push_back('d');
        tableLabelKeys.push_back('f');

        extensionList = ibus_prop_list_new();
#if IBUS_CHECK_VERSION(1, 2, 98)
        g_object_ref_sink(extensionList);
#endif
    }

    void addConstantsToLua(LuaBinding& luaBinding) {
        // add variables to lua
        luaBinding.doString("key={} request_cache={}");
#define add_key_const(var) luaBinding.setValue(#var, IBUS_ ## var, "key");

        add_key_const(CONTROL_MASK);
        add_key_const(SHIFT_MASK);
        add_key_const(LOCK_MASK);
        add_key_const(MOD1_MASK);
        //add_key_const(MOD2_MASK);
        //add_key_const(MOD3_MASK);
        add_key_const(MOD4_MASK);
        add_key_const(SUPER_MASK);
        add_key_const(META_MASK);
        //add_key_const(MODIFIER_MASK);

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
        add_key_const(Page_Down);
        add_key_const(Page_Up);
#undef add_key_const
        luaBinding.setValue("None", IBUS_VoidSymbol, "key");
        // other constants
        luaBinding.setValue("COLOR_NOCHANGE", INVALID_COLOR);
    }

    void staticDestruct() {
        for (size_t i = 0; i < extensions.size(); ++i) {
            delete extensions.at(i);
        }
        extensions.clear();
        g_object_unref(extensionList);
    }

    void activeExtension(string label) {
        for (size_t i = 0; i < extensions.size(); ++i) {
            if (extensions.at(i)->getLabel() == label)
                extensions.at(i)->execute();
        }
    }

    bool activeExtension(unsigned keyval, unsigned keymask) {
        for (size_t i = 0; i < extensions.size(); ++i) {
            if (extensions.at(i)->matchKey(keyval, keymask)) {
                extensions.at(i)->execute();
                return true;
            }
        }
        return false;
    }

    bool isPunctuationAutoWidth(char punc) {
        return autoWidthPunctuations.find(punc) != string::npos;
    }

    const string getFullPinyinTailAdjusted(const string& fullPinyinString) {
        for (size_t i = 0; i < fullPinyinString.length(); i++) {
            string s = LuaBinding::getStaticBinding().getValue((string("full_pinyin_adjustments.") + fullPinyinString.substr(i)).c_str(), "");
            if (!s.empty())
                return (fullPinyinString.substr(0, i) + s);
        }
        return fullPinyinString;
    }

    const string getGlobalCache(const string& requestString, const bool includeWeak) {
        string content = LuaBinding::getStaticBinding().getValue(requestString.c_str(), "", "request_cache");
        if (content.substr(0, sizeof (WEAK_CACHE_PREFIX) - 1) == string(WEAK_CACHE_PREFIX)) {
            if (includeWeak) return content.substr(sizeof (WEAK_CACHE_PREFIX) - 1);
            else return "";
        } else {

            return content;
        }
    }

    void writeGlobalCache(const string& requsetSring, const string& content, const bool weak) {
        if (weak) {
            LuaBinding::getStaticBinding().setValue(requsetSring.c_str(), (string(WEAK_CACHE_PREFIX) + content).c_str(), "request_cache");
        } else {
            LuaBinding::getStaticBinding().setValue(requsetSring.c_str(), content.c_str(), "request_cache");
        }
    }

    /**
     * register extension functions
     * in: int, int, string, string (key, modifiers, label, script)
     * out: -
     */
    int l_registerCommand(lua_State *L) {
        DEBUG_PRINT(3, "[CONF] l_registerCommand\n");
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        luaL_checktype(L, 3, LUA_TSTRING);
        luaL_checktype(L, 4, LUA_TSTRING);

        // Extension::Extension(ImeKey key, unsigned int keymask, string script, string label)
        ImeKey key = lua_tointeger(L, 1);
        unsigned int modifiers = lua_tointeger(L, 2);
        string label = lua_tostring(L, 3);
        string script = lua_tostring(L, 4);
        // find existed same name Extension and delete it first
        for (vector<Extension*>::iterator it = extensions.begin(); it != extensions.end(); ++it) {
            if ((*it)->getLabel() == label) {
                // modify label
                label = label + " (改)";
                break;
            }
        }
        extensions.push_back(new Extension(key, modifiers, label, script));
        return 0;
    }

    /**
     * apply global settings immediately
     * in: -
     * out: -
     */
    static int l_applySettings(lua_State* L) {
        DEBUG_PRINT(3, "[CONF] l_applySettings\n");
        LuaBinding& lb = LuaBinding::getLuaBinding(L);

        // multi-tone chinese lookup result limit
        multiToneLimit = lb.getValue("multi_tone_limit", multiToneLimit);

        // database confs
        dbResultLimit = lb.getValue("db_result_limit", dbResultLimit);
        dbLengthLimit = lb.getValue("db_length_limit", dbLengthLimit);
        dbLongPhraseAdjust = lb.getValue("db_phrase_adjust", dbLongPhraseAdjust);
        dbCompleteLongPhraseAdjust = lb.getValue("db_completion_adjust", dbCompleteLongPhraseAdjust);
        dbOrder = string(lb.getValue("db_query_order", dbOrder.c_str()));

        // colors (-1: use default)
        preeditForeColor = lb.getValue("preedit_fore_color", preeditForeColor);
        preeditBackColor = lb.getValue("preedit_back_color", preeditBackColor);
        requestingForeColor = lb.getValue("requesting_fore_color", requestingForeColor);
        requestingBackColor = lb.getValue("requesting_back_color", requestingBackColor);
        requestedForeColor = lb.getValue("responsed_fore_color", requestedForeColor);
        requestedBackColor = lb.getValue("responsed_back_color", requestedBackColor);
        correctingForeColor = lb.getValue("correcting_fore_color", correctingForeColor);
        correctingBackColor = lb.getValue("correcting_back_color", correctingBackColor);
        cloudCacheBackColor = lb.getValue("cloud_cache_back_color", cloudCacheBackColor);
        cloudCacheForeColor = lb.getValue("cloud_cache_fore_color", cloudCacheForeColor);
        localDbBackColor = lb.getValue("local_cache_back_color", localDbBackColor);
        localDbForeColor = lb.getValue("local_cache_fore_color", localDbForeColor);
        cloudCacheCandicateColor = lb.getValue("cloud_cache_candicate_color", cloudCacheCandicateColor);
        cloudWordsCandicateColor = lb.getValue("cloud_word_candicate_color", cloudWordsCandicateColor);
        databaseCandicateColor = lb.getValue("database_candicate_color", databaseCandicateColor);
        internalCandicateColor = lb.getValue("internal_candicate_color", internalCandicateColor);

        // timeouts
        selectionTimout = (long long) lb.getValue("sel_timeout", (double) selectionTimout / XUtility::MICROSECOND_PER_SECOND) * XUtility::MICROSECOND_PER_SECOND;
        preRequestTimeout = lb.getValue("pre_request_timeout", (double) preRequestTimeout);
        requestTimeout = lb.getValue("request_timeout", (double) requestTimeout);

        // keys
        engModeKey.readFromLua(lb, "eng_mode_key");
        chsModeKey.readFromLua(lb, "chs_mode_key");
        startCorrectionKey.readFromLua(lb, "correction_mode_key");
        pageDownKey.readFromLua(lb, "page_down_key");
        pageUpKey.readFromLua(lb, "page_up_key");
        quickResponseKey.readFromLua(lb, "quick_response_key");
        commitRawPreeditKey.readFromLua(lb, "raw_preedit_key");

        // bools
        staticNotification = lb.getValue("static_notification", staticNotification);
        useDoublePinyin = lb.getValue("use_double_pinyin", useDoublePinyin);
        strictDoublePinyin = lb.getValue("strict_double_pinyin", strictDoublePinyin);
        startInEngMode = lb.getValue("start_in_eng_mode", startInEngMode);
        writeRequestCache = lb.getValue("cache_requests", writeRequestCache);
        showNotification = lb.getValue("show_notificaion", showNotification);
        preRequest = lb.getValue("pre_request", preRequest);
        showCachedInPreedit = lb.getValue("show_cache_preedit", showCachedInPreedit);
        fallbackUsingDb = lb.getValue("fallback_use_db", fallbackUsingDb);
        useAlternativePopen = lb.getValue("strict_timeout", useAlternativePopen);
        preRequestFallback = lb.getValue("fallback_pre_request", preRequestFallback);
        if (preRequestFallback || preRequest) writeRequestCache = true;

        // int, tolerances
        fallbackEngTolerance = lb.getValue("auto_eng_tolerance", fallbackEngTolerance);
        preRequestRetry = lb.getValue("pre_request_retry", preRequestRetry);
        preeditReservedPinyinCount = lb.getValue("preedit_reserved_pinyin", preeditReservedPinyinCount);

        // labels used in lookup table, ibus has 16 chars limition.
        {
            switch (lb.getValueType("label_keys")) {
                case LUA_TSTRING:
                {
                    tableLabelKeys.clear();
                    string tableLabelKeys = lb.getValue("label_keys", "");
                    for (size_t i = 0; i < tableLabelKeys.length(); i++) {
                        tableLabelKeys.push_back(tableLabelKeys.c_str()[i]);
                    }
                    break;
                }
                case LUA_TTABLE:
                {
                    tableLabelKeys.clear();
                    char buffer[128];
                    for (size_t index = 1;; index++) {
                        snprintf(buffer, sizeof (buffer), "%s..%u", "label_keys", index);
                        // key is at index -2 and value is at index -1
                        // key should be a string or a number
                        ImeKey key = IBUS_VoidSymbol;
                        if (!key.readFromLua(lb, buffer)) break;
                        else tableLabelKeys.push_back(key);
                    }
                    break;
                }
            }
        }

        // external script path
        fetcherPath = string(lb.getValue("fetcher_path", fetcherPath.c_str()));

        // auto width punc and punc map
        autoWidthPunctuations = string(lb.getValue("punc_after_chinese", ".,?:"));

        if (lb.getValueType("punc_map") == LUA_TTABLE) {
            DEBUG_PRINT(4, "[LUA] read punc_map\n");
            punctuationMap.clear();
            // IMPROVE: some lock here?
            int pushedCount = lb.reachValue("punc_map");
            for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
                // key is at index -2 and value is at index -1
                // key should be a string or a number

                int key = 0;
                switch (lua_type(L, -2)) {
                    case LUA_TNUMBER:
                        key = lua_tointeger(L, -2);
                        break;
                    case LUA_TSTRING:
                        key = lua_tostring(L, -2)[0];
                        break;
                    default:
                        luaL_checktype(L, -2, LUA_TSTRING);
                }

                FullPunctuation fpunc;
                switch (lua_type(L, -1)) {
                    case LUA_TSTRING:
                        // single punc
                        DEBUG_PRINT(4, "[LUA] single punc: %s\n", lua_tostring(L, -1));
                        fpunc.addPunctuation(lua_tostring(L, -1));
                        break;
                    case LUA_TTABLE:
                        // multi puncs
                        DEBUG_PRINT(4, "[LUA] multi punc\n");
                        for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
                            // value should be string
                            luaL_checktype(L, -1, LUA_TSTRING);
                            DEBUG_PRINT(5, "[LUA]   punc: %s\n", lua_tostring(L, -1));
                            fpunc.addPunctuation(lua_tostring(L, -1));
                        }
                        // after the loop, the key and value of inner table are all popped
                        break;
                }

                punctuationMap.setPunctuationPair(key, fpunc);
            }
            lua_pop(L, pushedCount);
        }

        return 0;
    }

    void registerLuaFunctions() {
        // register lua C function
        LuaBinding::getStaticBinding().registerFunction(l_registerCommand, "register_command");
        LuaBinding::getStaticBinding().registerFunction(l_applySettings, "apply_settings");
    }

};

