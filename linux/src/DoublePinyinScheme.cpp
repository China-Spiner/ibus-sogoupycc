/* 
 * File:   DoublePinyinScheme.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include <cstdio>
#include "DoublePinyinScheme.h"
#include "PinyinUtility.h"
#include "LuaBinding.h"
#include "defines.h"

DoublePinyinScheme DoublePinyinScheme::defaultDoublePinyinScheme;

DoublePinyinScheme::DoublePinyinScheme() {
    mapBuilt = false;
}

DoublePinyinScheme::DoublePinyinScheme(const DoublePinyinScheme& orig) {
    bindedKeys = orig.bindedKeys;
    buildMap();
}

DoublePinyinScheme::~DoublePinyinScheme() {

}

void DoublePinyinScheme::bindKey(const char key, const string& consonant, const vector<string>& vowels) {
    bindedKeys[key] = pair<string, vector<string> >(consonant, vowels);
    mapBuilt = false;
}

void DoublePinyinScheme::clear() {
    bindedKeys.clear();
    mapBuilt = false;
}

const string DoublePinyinScheme::query(const char firstKey, const char secondKey) const {
    if (!mapBuilt) buildMap();
    map<pair<char, char>, string>::const_iterator it =
            queryMap.find(pair<char, char>(firstKey, secondKey));

    if (it != queryMap.end()) return it->second;
    else return "";
}

const string DoublePinyinScheme::query(const string& doublePinyinString) const {
    string result;
    char keys[2];
    int p = 0;
    for (size_t i = 0; i < doublePinyinString.length(); ++i) {
        char key = doublePinyinString[i];
        if (bindedKeys.count(key) > 0) {
            keys[p] = key;
            p ^= 1;
            if (p == 0) {
                if (result.empty())
                    result += query(keys[0], keys[1]);
                else
                    result += " " + query(keys[0], keys[1]);
            }
        } else {
            result += key;
        }
    }
    if (p == 1) {
        if (result.length() > 0) result += " ";
        string consonant = bindedKeys.find(keys[0])->second.first;
        if (consonant.empty())
            result += "'";
        else result += consonant;
    }
    return result;
}

const bool DoublePinyinScheme::isValidDoublePinyin(const string& doublePinyinString) const {
    char keys[2];
    int p = 0;
    for (size_t i = 0; i < doublePinyinString.length(); ++i) {
        keys[p++] = doublePinyinString[i];
        if (p == 2) {
            if (query(keys[0], keys[1]).empty()) return false;
            p = 0;
        }
    }
    if (p == 1) {
        return bindedKeys.find(keys[0])->second.first != "-";
    }
    return true;
}

const int DoublePinyinScheme::buildMap() const {
    queryMap.clear();
    int conflictCount = 0;
    for (map<char, pair<string, vector<string> > >::const_iterator firstKey = bindedKeys.begin(); firstKey != bindedKeys.end(); ++firstKey) {
        for (map<char, pair<string, vector<string> > >::const_iterator secondKey = bindedKeys.begin(); secondKey != bindedKeys.end(); ++secondKey) {
            const string& consonant = firstKey->second.first;
            for (vector<string>::const_iterator vowel = secondKey->second.second.begin(); vowel != secondKey->second.second.end(); ++vowel) {
                string pinyin = consonant + *vowel;
                if (PinyinUtility::isValidPinyin(pinyin)) {
                    if (queryMap.count(pair<char, char>(firstKey->first, secondKey->first)) == 0) {
                        // ok, let's set it.
                        queryMap[pair<char, char>(firstKey->first, secondKey->first)] = pinyin;
                    } else {
                        fprintf(stderr, "DoublePinyinScheme::buildMap: conflict found: %s %s\n", queryMap[pair<char, char>(firstKey->first, secondKey->first)].c_str(), pinyin.c_str());
                        ++conflictCount;
                    }
                }
            }
        }
    }
    mapBuilt = true;
    return conflictCount;
}

const bool DoublePinyinScheme::isKeyBinded(const char key) const {
    return bindedKeys.find(key) != bindedKeys.end();
}

/**
 * in: table
 * out: int
 */
int DoublePinyinScheme::l_setDoublePinyinScheme(lua_State *L) {
    DEBUG_PRINT(1, "[LUA] setDoublePinyinScheme\n");

    LuaBinding& lib = LuaBinding::getLuaBinding(L);

    pthread_mutex_lock(lib.getAtomMutex());
    luaL_checktype(L, 1, LUA_TTABLE);

    DoublePinyinScheme::defaultDoublePinyinScheme.clear();

    lua_checkstack(L, 5);
    // traverse that table (at pos 1)
    for (lua_pushnil(L); lua_next(L, 1) != 0; lua_pop(L, 1)) {
        // key is at index -2 and value is at index -1
        luaL_checktype(L, -1, LUA_TTABLE); // value is a table
        luaL_checktype(L, -2, LUA_TSTRING); //key is a string

        // key
        char key = lua_tostring(L, -2)[0];

        // expand value table, consonant
        lua_pushnumber(L, 1); // push k
        lua_gettable(L, -2); // t[k], t is at -2, k is at top
        luaL_checktype(L, -1, LUA_TSTRING);

        string consonant = lua_tostring(L, -1);
        lua_pop(L, 1); // consonant not used, pop it

        // vowels table
        lua_pushnumber(L, 2); // push k
        lua_gettable(L, -2); // t[k]
        luaL_checktype(L, -1, LUA_TTABLE);
        vector<string> vowels;
        for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
            luaL_checktype(L, -1, LUA_TSTRING);
            vowels.push_back(lua_tostring(L, -1));
        }
        lua_pop(L, 1); // pop vowels table

        DoublePinyinScheme::defaultDoublePinyinScheme.bindKey(key, consonant, vowels);
    }
    // return conflict count
    lua_pushinteger(L, DoublePinyinScheme::defaultDoublePinyinScheme.buildMap());
    pthread_mutex_unlock(lib.getAtomMutex());
    return 1;
}

/**
 * call this after setDoublePinyinScheme
 * in: string
 * out: boolean
 */
int DoublePinyinScheme::l_isValidDoublePinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUA] l_isValidDoublePinyin\n");
    LuaBinding& lib = LuaBinding::getLuaBinding(L);
    pthread_mutex_lock(lib.getAtomMutex());
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushboolean(L, DoublePinyinScheme::defaultDoublePinyinScheme.isValidDoublePinyin(lua_tostring(L, 1)));
    pthread_mutex_unlock(lib.getAtomMutex());
    return 1;
}

/**
 * in: string
 * out: string
 */
int DoublePinyinScheme::l_doubleToFullPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUA] l_doubleToFullPinyin\n");
    LuaBinding& lib = LuaBinding::getLuaBinding(L);
    pthread_mutex_lock(lib.getAtomMutex());
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushstring(L, DoublePinyinScheme::defaultDoublePinyinScheme.query(lua_tostring(L, 1)).c_str());
    pthread_mutex_unlock(lib.getAtomMutex());
    return 1;
}

void DoublePinyinScheme::registerLuaFunctions() {
    LuaBinding::getStaticBinding().registerFunction(l_setDoublePinyinScheme, "set_double_pinyin_scheme");
    LuaBinding::getStaticBinding().registerFunction(l_doubleToFullPinyin, "double_to_full_pinyin");
    LuaBinding::getStaticBinding().registerFunction(l_isValidDoublePinyin, "is_valid_double_pinyin");
}

const DoublePinyinScheme DoublePinyinScheme::getDefaultDoublePinyinScheme() {
    return defaultDoublePinyinScheme;
}