/* 
 * File:   DoublePinyinScheme.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * double pinyin scheme, query
 */

#ifndef _DOUBLEPINYINSCHEME_H
#define	_DOUBLEPINYINSCHEME_H

#include <string>
#include <vector>
#include <map>

#include "LuaBinding.h"

using std::string;
using std::vector;
using std::map;
using std::pair;

class DoublePinyinScheme {
public:
    DoublePinyinScheme();
    DoublePinyinScheme(const DoublePinyinScheme& orig);

    void bindKey(const char key, const string& consonant, const vector<string>& vowels);

    void clear();
    const int buildMap() const;
    const string query(const char firstChar, const char secondChar) const;
    const string query(const string& doublePinyinString) const;
    const bool isValidDoublePinyin(const string& doublePinyinString) const;
    const bool isKeyBinded(const char key) const;

    virtual ~DoublePinyinScheme();

    static void registerLuaFunctions();
    static const DoublePinyinScheme getDefaultDoublePinyinScheme();
    
private:
    mutable bool mapBuilt;
    map<char, pair<string, vector<string> > > bindedKeys;
    mutable map<pair<char, char>, string> queryMap;

    // Lua C Functions

    static int l_setDoublePinyinScheme(lua_State *L);
    static int l_isValidDoublePinyin(lua_State *L);
    static int l_doubleToFullPinyin(lua_State *L);
    static DoublePinyinScheme defaultDoublePinyinScheme;
};

#endif	/* _DOUBLEPINYINSCHEME_H */

