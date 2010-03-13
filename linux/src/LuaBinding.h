/* 
 * File:   LuaBinding.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 13, 2010
 *  read ime.punc_map table struct natively
 * March 12, 2010
 *  global conf moved from engine.cpp
 *  many per-session settings go global
 * March 9, 2010
 *  0.1.3 global static init run first
 * March 2, 2010
 *  0.1.2
 * February 28, 2010
 *  0.1.1 major bugs fixed
 *  do not unload dbs in ~LuaBinding
 * February 27, 2010
 *  0.1.0 first release
 * February 9, 2010
 *  created.
 *  i move much code to external lua configure script to gain flexibility.
 *  as designed, it should be instantiated once by each ibus engine.
 */

#ifndef _LUABINDING_H
#define	_LUABINDING_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <map>
#include "PinyinUtility.h"
#include "PinyinCloudClient.h"
#include "PinyinDatabase.h"
#include "DoublePinyinScheme.h"
#include "PinyinSequence.h"

using std::map;
using std::pair;

class LuaBinding {
public:
    static const char LIB_NAME[];
    static const string LibraryName;
    
    LuaBinding();

    /**
     * ime.a["abc"]["1"][1][2]["bb"] : getValue("abc.1..1..2.bb", "", "ime")
     */
    string getValue(const char* varName, const char* defaultValue = "", const char* libName = LIB_NAME);
    int getValue(const char* varName, const int defaultValue = -1, const char* libName = LIB_NAME);
    unsigned int getValue(const char* varName, const unsigned int defaultValue, const char* libName = LIB_NAME);
    bool getValue(const char* varName, const bool defaultValue = false, const char* libName = LIB_NAME);
    double getValue(const char* varName, const double defaultValue, const char* libName = LIB_NAME);
    int getValueType(const char* varName, const char* libName = LIB_NAME);

    void setValue(const char* varName, const int value, const char* libName = LIB_NAME);
    void setValue(const char* varName, const char value[], const char* libName = LIB_NAME);
    void setValue(const char* varName, const bool value, const char* libName = LIB_NAME);

    virtual ~LuaBinding();
    int doString(const char* luaScript);
    
    /**
     * call a lua function in LIB_NAME table
     * ... : in_1, in_2, &out_1, &out_2, ...
     * note here, std::string in output, but const char* in input
     * sig format examples:
     * "dd>s"
     * take in 2 ints, fills a std::string
     * "s"
     * take in 1 string(char[]), no return values
     * d: int, f: double, s: string, b: boolean(int)
     * return -1 if function is nil, 0 if successful, otherwise same as lua_pcall
     * do not use in multi-threads
     */
    int callLuaFunction(const char*const func, const char* sig, ...);
    void addFunction(const lua_CFunction func, const char * funcName);

    const lua_State* getLuaState() const;

    static DoublePinyinScheme doublePinyinScheme;
    static map<string, PinyinDatabase*> pinyinDatabases;

    /**
     * load global config
     */
    static void staticInit();

    /**
     * clean up static vars.
     * close dbs.
     */
    static void staticDestruct();
    static LuaBinding& getStaticBinding();
private:
    LuaBinding(const LuaBinding& orig);
    pthread_mutex_t luaStateMutex;

    /**
     * allow .name (string key)
     * allow ..num (num key)
     * stop at non-table (may be not nil)
     * @return pushed count
     */
    int reachValue(const char* varName, const char* libName = LIB_NAME);

    static LuaBinding* staticLuaBinding;
    // Lua C functions related
    lua_State* L;
    static const struct luaL_Reg luaLibraryReg[];
    static map<const lua_State*, LuaBinding*> luaStates;

    /**
     * in: table
     * out: int
     */
    static int l_setDoublePinyinScheme(lua_State *L);
    /**
     * call this after setDoublePinyinScheme
     * in: string
     * out: boolean
     */
    static int l_isValidDoublePinyin(lua_State *L);
    /**
     * in: string
     * out: string
     */
    static int l_doubleToFullPinyin(lua_State *L);

    /**
     * in: string
     * out: string
     */
    static int l_charsToPinyin(lua_State *L);
    /**
     * in: string
     * out: boolean
     */
    static int l_isValidPinyin(lua_State *L);
    /**
     * set global debug level
     * in: int
     * out: -
     */
    static int l_setDebugLevel(lua_State *L);
    /**
     * in: int, int
     * out: int
     */
    static int l_bitand(lua_State *L);
    /**
     * in: int, int
     * out: int
     */
    static int l_bitor(lua_State *L);
    /**
     * in: int, int
     * out: int
     */
    static int l_bitxor(lua_State *L);
    /**
     * in: int(state), int(mask)
     * out: boolean
     */
    static int l_keymask(lua_State *L);
    /**
     * print lua stack, for debugging
     * in: -
     * out: -
     */
    static int l_printStack(lua_State *L);
    /**
     * load pinyin database (ibus-pinyin 1.2.99 compatible)
     * in: string, double
     * out: int
     */
    static int l_loadPhraseDatabase(lua_State *L);
    /**
     * get database loaded count
     * in: -
     * out: int
     */
    static int l_getPhraseDatabaseLoadedCount(lua_State* L);
    /**
     * apply global settings immediately
     * in: -
     * out: -
     */
    static int l_applySettings(lua_State* L);
};

#endif	/* _LUAIBUSBINDING_H */

