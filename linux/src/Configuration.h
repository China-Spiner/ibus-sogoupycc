/*
 * File:   LuaBinding.cpp
 * Author: WU Jun <quark@lihdd.net>
 *
 * maintain global settings
 */

#ifndef _CONFIGURATION_H
#define	_CONFIGURATION_H

#include <set>
#include <string>
#include <ibus.h>

#include "LuaBinding.h"

#define WEAK_CACHE_PREFIX "._."

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
        ImeKey(const unsigned int keyval = IBUS_VoidSymbol);
        ImeKey(const ImeKey& orig);
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

    extern IBusPropList *extensionList;

    // struct defines

    class Extension {
    public:
        Extension(ImeKey key, unsigned int keymask, string label, string script);
        ~Extension();
        const string getLabel() const;
        void execute() const;
        bool matchKey(unsigned int keyval, unsigned keymask) const;
    private:
        Extension(const Extension& orig);
        ImeKey key;
        unsigned int keymask;
        string script, label;
        IBusProperty *prop;
    };
    // not part of setting, but save activeEngine
    extern void* activeEngine;

    // full path of fetcher script
    extern string fetcherPath;

    // buffer size receive return string from fetcher script
    extern const int fetcherBufferSize;
    extern int preRequestRetry;

    // keys used in select item in lookup table
    extern vector<ImeKey> tableLabelKeys;

    // colors
    extern int requestingBackColor, requestingForeColor;
    extern int requestedBackColor, requestedForeColor;
    extern int preeditForeColor, preeditBackColor;
    extern int correctingForeColor, correctingBackColor;
    extern int cloudCacheForeColor, cloudCacheBackColor;
    extern int localDbForeColor, localDbBackColor;
    extern int cloudCacheCandicateColor;
    extern int cloudWordsCandicateColor;
    extern int databaseCandicateColor;
    extern int internalCandicateColor;

    // keys
    extern ImeKey startCorrectionKey, engModeKey, chsModeKey, pageDownKey, pageUpKey, quickResponseKey;

    // boolean configs
    extern bool useDoublePinyin;
    extern bool strictDoublePinyin;
    extern bool startInEngMode;
    extern bool writeRequestCache;
    extern bool showNotification;
    extern bool preRequest;
    extern bool showCachedInPreedit;
    extern bool staticNotification;
    extern bool fallbackUsingDb;
    extern bool preRequestFallback;
    extern bool useAlternativePopen;

    // tolerances
    extern int fallbackEngTolerance;

    // pre request timeout
    extern double preRequestTimeout;
    extern double requestTimeout;

    // selection timeout tolerance
    extern long long selectionTimout;

    // database confs
    extern int dbResultLimit, dbLengthLimit;
    extern string dbOrder;
    extern double dbLongPhraseAdjust;
    extern double dbCompleteLongPhraseAdjust;

    // half to full width punctuation map
    extern PunctuationMap punctuationMap;
    extern string autoWidthPunctuations;
    
    // multi tone limit
    extern size_t multiToneLimit;

    // functions (callby LuaBinding, main, engine, database)
    void addConstantsToLua(LuaBinding& luaBinding);
    void staticInit();
    void staticDestruct();
    void activeExtension(string label);
    bool activeExtension(unsigned keyval, unsigned keymask);
    bool isPunctuationAutoWidth(char punctuation);
    const string getGlobalCache(const string& requestString, const bool includeWeak = false);
    const string getFullPinyinTailAdjusted(const string& fullPinyinString);
    void writeGlobalCache(const string& requsetSring, const string& content, const bool weak = false);


    // lua C functions
    /**
     * in: (key = I..., mod = key.SHIFT_MASK, label = "blabla", script = "functionName")
     */
    extern int l_registerCommand(lua_State *L);
};



#endif	/* _CONFIGURATION_H */

