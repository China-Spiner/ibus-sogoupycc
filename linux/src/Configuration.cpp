/* 
 * File:   Configuration.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "Configuration.h"
#include "defines.h"
#include "XUtility.h"
#include <ibus.h>

namespace Configuration {
    // full path of fetcher script
    string fetcherPath = PKGDATADIR "/fetcher";

    // buffer size receive return string from fetcher script
    int fetcherBufferSize = 1024;

    // keys used in select item in lookup table
    string tableLabelKeys = "jkl;uiopasdf";

    // colors
    int requestingBackColor = 0xB8CFE5, requestingForeColor = INVALID_COLOR;
    int requestedBackColor = INVALID_COLOR, requestedForeColor = 0x00C97F;
    int preeditBackColor = INVALID_COLOR, preeditForeColor = 0x0050FF;
    int correctingBackColor = 0xEDBE7D, correctingForeColor = INVALID_COLOR;

    // keys
    unsigned int engModeToggleKey = IBUS_Shift_L,
            startCorrectionKey = IBUS_Tab,
            engModeKey = IBUS_VoidSymbol,
            chsModeKey = IBUS_VoidSymbol,
            pageDownKey = 'h',
            pageUpKey = 'g';

    // boolean configs
    bool useDoublePinyin = false;
    bool strictDoublePinyin = true;
    bool startInEngMode = false;
    bool writeRequestCache = true;

    // selection timeout tolerance
    long long selectionTimout = 3LL * XUtility::MICROSECOND_PER_SECOND;

    // punctuations
    PunctuationMap punctuationMap;

    // database confs
    int dbResultLimit = 32, dbLengthLimit = 5;
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

    ImeKey::ImeKey(LuaBinding& L, const string& varName) {

        //lua_checkstack(L, 1);
        //lua_getglobal(L, "_G");
        //for (char* name = va_arg(vl, char*); name; name = va_arg(vl, char*)) {

    }

    ImeKey::ImeKey(const unsigned int keyval) {
        keys.insert(keyval);
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

        // create request_cache
        
    }

    void addConstantsToLua(LuaBinding& luaBinding) {
        // add variables to lua
        luaBinding.doString("key={}");
#define add_key_const(var) luaBinding.setValue(#var, IBUS_ ## var, "key");
        /*
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
         */
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
        luaBinding.setValue("None", IBUS_VoidSymbol, "key");

        // other constants
        luaBinding.setValue("COLOR_NOCHANGE", INVALID_COLOR);
    }
};

