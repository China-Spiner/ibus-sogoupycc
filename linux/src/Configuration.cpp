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
    int fetcherBufferSize = 1024;

    // keys used in select item in lookup table
    vector<ImeKey> tableLabelKeys;

    // colors
    int requestingBackColor = 0xB8CFE5, requestingForeColor = INVALID_COLOR;
    int requestedBackColor = INVALID_COLOR, requestedForeColor = 0x00C97F;
    int preeditBackColor = INVALID_COLOR, preeditForeColor = 0x0050FF;
    int correctingBackColor = 0xEDBE7D, correctingForeColor = INVALID_COLOR;

    // keys
    ImeKey startCorrectionKey = IBUS_Tab,
            engModeKey = IBUS_Shift_L,
            chsModeKey = IBUS_Shift_L,
            pageDownKey = 'h',
            pageUpKey = 'g';

    // boolean configs
    bool useDoublePinyin = false;
    bool strictDoublePinyin = true;
    bool startInEngMode = false;
    bool writeRequestCache = true;
    bool showNotification = true;
    bool preRequest = true;
    bool showCachedInPreedit = true;
    bool staticNotification = false;

    // pre request timeout
    double preRequestTimeout = 1.;

    // selection timeout tolerance
    long long selectionTimout = 3LL * XUtility::MICROSECOND_PER_SECOND;

    // punctuations
    PunctuationMap punctuationMap;

    // multi tone chineses
    size_t multiToneLimit = 2;

    // database confs
    int dbResultLimit = 128, dbLengthLimit = 10;
    string dbOrder = "";
    double dbLongPhraseAdjust = 1.2;

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
                unsigned int key = (unsigned int)luaBinding.getValue(varName.c_str(), (int) IBUS_VoidSymbol);
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
        ibus_prop_list_append(extensionList, this->prop);
    }

    Extension::~Extension() {
        if (this->prop) g_object_unref(this->prop), this->prop = NULL;
    }

    const string Extension::getLabel() const {
        return label;
    }

    void Extension::execute() const {
        LuaBinding::getStaticBinding().doString(script.c_str());
    }

    bool Extension::matchKey(unsigned int keyval, unsigned keymask) const {
        return (keymask == this->keymask) && this->key.match(keyval);
    }

    // functions

    void staticInit() {
        DEBUG_PRINT(3, "[CONF] staticInit\n");
        // init complex structs, such as punctuations
        Configuration::punctuationMap.setPunctuationPair('.', FullPunctuation("。"));
        Configuration::punctuationMap.setPunctuationPair(',', FullPunctuation("，"));
        Configuration::punctuationMap.setPunctuationPair('^', FullPunctuation("……"));
        Configuration::punctuationMap.setPunctuationPair('@', FullPunctuation("·"));
        Configuration::punctuationMap.setPunctuationPair('!', FullPunctuation("！"));
        Configuration::punctuationMap.setPunctuationPair('~', FullPunctuation("～"));
        Configuration::punctuationMap.setPunctuationPair('?', FullPunctuation("？"));
        Configuration::punctuationMap.setPunctuationPair('#', FullPunctuation("＃"));
        Configuration::punctuationMap.setPunctuationPair('$', FullPunctuation("￥"));
        Configuration::punctuationMap.setPunctuationPair('&', FullPunctuation("＆"));
        Configuration::punctuationMap.setPunctuationPair('(', FullPunctuation("（"));
        Configuration::punctuationMap.setPunctuationPair(')', FullPunctuation("）"));
        Configuration::punctuationMap.setPunctuationPair('{', FullPunctuation("｛"));
        Configuration::punctuationMap.setPunctuationPair('}', FullPunctuation("｝"));
        Configuration::punctuationMap.setPunctuationPair(']', FullPunctuation("［"));
        Configuration::punctuationMap.setPunctuationPair(']', FullPunctuation("］"));
        Configuration::punctuationMap.setPunctuationPair(';', FullPunctuation("；"));
        Configuration::punctuationMap.setPunctuationPair(':', FullPunctuation("："));
        Configuration::punctuationMap.setPunctuationPair('<', FullPunctuation("《"));
        Configuration::punctuationMap.setPunctuationPair('>', FullPunctuation("》"));
        Configuration::punctuationMap.setPunctuationPair('\\', FullPunctuation("、"));
        Configuration::punctuationMap.setPunctuationPair('\'', FullPunctuation(2, "‘", "’"));
        Configuration::punctuationMap.setPunctuationPair('\"', FullPunctuation(2, "“", "”"));

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

        // moved to LuaBinding static C function register array
        // luaBinding.addFunction(l_registerCommand, "register_command");
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

    int l_registerCommand(lua_State *L) {
        DEBUG_PRINT(3, "[CONF] l_registerCommand\n");
        luaL_checktype(L, 1, LUA_TNUMBER);
        luaL_checktype(L, 2, LUA_TNUMBER);
        luaL_checktype(L, 3, LUA_TSTRING);
        luaL_checktype(L, 4, LUA_TSTRING);

        // Extension::Extension(ImeKey key, unsigned int keymask, string script, string label)
        Configuration::ImeKey key = lua_tointeger(L, 1);
        unsigned int modifiers = lua_tointeger(L, 2);
        string label = lua_tostring(L, 3);
        string script = lua_tostring(L, 4);
        // find existed same name Extension and delete it first
        for (vector<Configuration::Extension*>::iterator it = Configuration::extensions.begin(); it != Configuration::extensions.end(); ++it) {
            if ((*it)->getLabel() == label) {
                // modify label
                label = label + " (改)";
                break;
            }
        }
        Configuration::extensions.push_back(new Configuration::Extension(key, modifiers, label, script));
        return 0;
    }

};

