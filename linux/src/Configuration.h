/*
 * File:   LuaBinding.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 12, 2010
 *  created. aim to maintain global configuration.
 *  and move non-status session config global
 */

#ifndef _CONFIGURATION_H
#define	_CONFIGURATION_H

#include "LuaBinding.h"
#include <set>
#include <string>

using std::string;
using std::set;

#define INVALID_COLOR -1

namespace Configuration {
    // full path of fetcher script
    extern string fetcherPath;

    // buffer size receive return string from fetcher script
    extern int fetcherBufferSize;

    // keys used in select item in lookup table
    extern string tableLabelKeys;

    // colors
    extern int requestingBackColor, requestingForeColor;
    extern int requestedBackColor, requestedForeColor;
    extern int preeditForeColor, preeditBackColor;
    extern int correctingForeColor, correctingBackColor;

    // keys
    extern unsigned int engModeToggleKey, startCorrectionKey, engModeKey, chsModeKey, pageDownKey, pageUpKey;

    // boolean configs
    extern bool useDoublePinyin;
    extern bool strictDoublePinyin;
    extern bool startInEngMode;
    extern bool writeRequestCache;

    // selection timeout tolerance
    extern long long selectionTimout;

    // database confs
    extern int dbResultLimit, dbLengthLimit;
    extern string dbOrder;
    extern double dbLongPhraseAdjust;

    void addConstants(LuaBinding& luaBinding);
};

class ImeKey {
private:
    std::set<unsigned int> keys;
};

#endif	/* _CONFIGURATION_H */

