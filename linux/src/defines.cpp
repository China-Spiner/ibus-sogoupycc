/*
 * File:   defines.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "defines.h"
#include "LuaBinding.h"

int globalDebugLevel;

/**
 * set global debug level
 * in: int
 * out: -
 */
int l_setDebugLevel(lua_State *L) {
    DEBUG_PRINT(3, "[LUABIND] l_setDebugLevel\n");
    luaL_checktype(L, 1, LUA_TNUMBER);
    globalDebugLevel = lua_tointeger(L, 1);
    return 0;
}

void registerDebugLuaFunction() {
    LuaBinding::getStaticBinding().registerFunction(l_setDebugLevel, "set_debug_level");
}
