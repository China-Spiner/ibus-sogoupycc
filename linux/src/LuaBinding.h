/* 
 * File:   LuaBinding.h
 * Author: WU Jun <quark@lihdd.net>
 * 
 * February 27, 2010
 *  0.1.0 first release
 * February 9, 2010
 *  created.
 *  i move much code to external lua configure script to gain flexibility.
 *  as designed, it should be instantiated once by each ibus engine.
 */

#ifndef _LUAIBUSBINDING_H
#define	_LUAIBUSBINDING_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <map>
#include "PinyinUtility.h"
#include "PinyinCloudClient.h"
#include "PinyinDatabase.h"

using std::map;
using std::pair;

class LuaBinding {
public:
    static char LIB_NAME[];

    LuaBinding();

    string getValue(const char* varName, const char* defaultValue = "", const char* libName = LIB_NAME);
    int getValue(const char* varName, const int defaultValue = -1, const char* libName = LIB_NAME);
    bool getValue(const char* varName, const bool defaultValue = false, const char* libName = LIB_NAME);
    double getValue(const char* varName, const double defaultValue, const char* libName = LIB_NAME);
    void setValue(const char* varName, const int value, const char* libName = LIB_NAME);
    void setValue(const char* varName, const char value[], const char* libName = LIB_NAME);
    void setValue(const char* varName, const bool value, const char* libName = LIB_NAME);

    virtual ~LuaBinding();
    int doString(const char* luaScript);

    int callLuaFunction(const char*const func, const char* sig, ...);
    void addFunction(const lua_CFunction func, const char * funcName);

    const lua_State* getLuaState() const;
    
    static DoublePinyinScheme doublePinyinScheme;
    static map<string, PinyinDatabase*> pinyinDatabases;
private:
    LuaBinding(const LuaBinding& orig);
    pthread_mutex_t luaStateMutex;

    // Lua C functions related
    lua_State* L;
    static const struct luaL_Reg pycclib[];
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
};

#endif	/* _LUAIBUSBINDING_H */

