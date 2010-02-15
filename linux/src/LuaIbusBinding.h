/* 
 * File:   LuaIbusBinding.h
 * Author: WU Jun <quark@lihdd.net>
 * 
 * February 9, 2010
 *  created, new.
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
#include "DoublePinyinScheme.h"
#include "PinyinCloudClient.h"

using std::map;

class LuaIbusBinding {
public:
    static char LIB_NAME[];
    
    LuaIbusBinding();

    string getValue(const char* varName, const char* defaultValue = "", const char* libName = LIB_NAME);
    int getValue(const char* varName, const int defaultValue = -1, const char* libName = LIB_NAME);
    bool getValue(const char* varName, const bool defaultValue = false, const char* libName = LIB_NAME);
    void setValue(const char* varName, const int value, const char* libName = LIB_NAME);
    void setValue(const char* varName, const char value[], const char* libName = LIB_NAME);
    void setValue(const char* varName, const bool value, const char* libName = LIB_NAME);

    virtual ~LuaIbusBinding();
    int doString(const char* luaScript);

    int callLuaFunction(bool inNewThread, const char*const func, const char* sig, ...);
    void addFunction(const lua_CFunction func, const char * funcName);

    const lua_State* getLuaState() const;
private:
    LuaIbusBinding(const LuaIbusBinding& orig);
    static DoublePinyinScheme doublePinyinScheme;
    pthread_mutex_t luaStateMutex;

    // Lua C functions related
    lua_State* L;
    static const struct luaL_Reg pycclib[];
    static map<const lua_State*, LuaIbusBinding*> luaStates;

    static int l_setDoublePinyinScheme(lua_State *L);
    static int l_isValidDoublePinyin(lua_State *L);
    static int l_convertDoublePinyin(lua_State *L);
    static int l_bitand(lua_State *L);
    static int l_bitor(lua_State *L);
    static int l_bitxor(lua_State *L);
    static int l_keymask(lua_State *L);
};

#endif	/* _LUAIBUSBINDING_H */

