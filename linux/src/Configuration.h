/*
 * File:   LuaBinding.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 13, 2010
 *  add PunctuationMap, ImeKey
 * March 12, 2010
 *  created. aim to maintain global configuration.
 *  and move all non-status session config global
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
    // class defines, should be in front

    class FullPunctuation {
    public:
        FullPunctuation();
        FullPunctuation(const string punction);
        /**
         * Punctuation(2, "(", ")"), etc...
         */
        FullPunctuation(size_t count, ...);
        FullPunctuation(const FullPunctuation& punction);
        void addPunctuation(const string punction);
        const string getPunctuation();
    private:
        vector<string> punctions;
        size_t index;
    };

    class PunctuationMap {
    public:
        PunctuationMap();
        PunctuationMap(const PunctuationMap& orig);
        void setPunctuationPair(unsigned int key, FullPunctuation punctuation);
        const string getFullPunctuation(const unsigned int punctionKey) const;
        void clear();
    private:
        mutable map<const unsigned int, FullPunctuation> punctuationMap;
    };

    class ImeKey {
    public:
        ImeKey(const unsigned int keyval);
        /**
         * @return true if success
         */
        bool readFromLua(LuaBinding& luaBinding, const string& varName);
        const bool match(const unsigned int keyval) const;
        const string getLabel() const;
    private:
        std::set<unsigned int> keys;
        string label;
    };

    // full path of fetcher script
    extern string fetcherPath;

    // buffer size receive return string from fetcher script
    extern int fetcherBufferSize;

    // keys used in select item in lookup table
    extern vector<ImeKey> tableLabelKeys;

    // colors
    extern int requestingBackColor, requestingForeColor;
    extern int requestedBackColor, requestedForeColor;
    extern int preeditForeColor, preeditBackColor;
    extern int correctingForeColor, correctingBackColor;

    // keys
    extern ImeKey startCorrectionKey, engModeKey, chsModeKey, pageDownKey, pageUpKey;

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

    // half to full width punctuation map
    extern PunctuationMap punctuationMap;

    // multi tone limit
    extern size_t multiToneLimit;

    // functions (callby LuaBinding, main)
    void addConstantsToLua(LuaBinding& luaBinding);
    void staticInit();

};



#endif	/* _CONFIGURATION_H */

