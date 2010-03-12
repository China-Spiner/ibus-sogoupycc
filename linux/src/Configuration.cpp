/* 
 * File:   Configuration.cpp
 * Author: WU Jun <quark@lihdd.net>
 * 
 * see .h file for changelog
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

    // database confs
    int dbResultLimit = 32, dbLengthLimit = 5;
    string dbOrder = "";
    double dbLongPhraseAdjust = 1.2;
};

void Configuration::addConstants(LuaBinding& luaBinding) {
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

    // punc maps
    luaBinding.doString(
            (string(LuaBinding::LIB_NAME) + ".punc_map = { \
                ['.'] = '。', [','] = '，', ['^'] = '……', ['@'] = '·', ['!'] = '！', ['~'] = '～', \
                ['?'] = '？', ['#'] = '＃', ['$'] = '￥', ['&'] = '＆', ['('] = '（', [')'] = '）', \
                ['{'] = '｛', ['}'] = '｝', ['['] = '［', [']'] = '］', [';'] = '；', [':'] = '：', \
                ['<'] = '《', ['>'] = '》', ['\\\\'] = '、', \
                [\"'\"] = { 2, '‘', '’'}, ['\"'] = { 2, '“', '”'} }\n"
            + LuaBinding::LIB_NAME + "._get_punc = function(keychr) \
                if " + LuaBinding::LIB_NAME + ".punc_map[keychr] then \
                    local punc = " + LuaBinding::LIB_NAME + ".punc_map[keychr] \
                    if type(punc) == 'string' then return punc else \
                        local id = punc[1] \
                        punc[1] = punc[1] + 1 if (punc[1] > #punc) then punc[1] = 2 end \
                        return punc[id] \
                    end \
                else return '' end\
            end\n" \
            + "request_cache = {}").c_str());

    // other constants
    luaBinding.setValue("COLOR_NOCHANGE", INVALID_COLOR);
}
