/* 
 * File:   LuaIbusBinding.cpp
 * Author: WU Jun <quark@lihdd.net>
 * 
 * see .h file for changelog
 */

#include "LuaIbusBinding.h"
#include "defines.h"
#include <cstring>
#include <cstdlib>

// Lua C functions

/**
 * in: 2 int
 * out: 1 boolean
 */
int LuaIbusBinding::l_keymask(lua_State *L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_pushboolean(L, (lua_tointeger(L, 1) & lua_tointeger(L, 2)) == lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 2 int
 * out: 1 int
 */
int LuaIbusBinding::l_bitand(lua_State *L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_pushinteger(L, lua_tointeger(L, 1) & lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 2 int
 * out: 1 int
 */
int LuaIbusBinding::l_bitxor(lua_State *L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_pushinteger(L, lua_tointeger(L, 1) ^ lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 2 int
 * out: 1 int
 */
int LuaIbusBinding::l_bitor(lua_State *L) {
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_pushinteger(L, lua_tointeger(L, 1) | lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 1 string
 * out: 1 bool
 */
int LuaIbusBinding::l_isValidDoublePinyin(lua_State *L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    LuaIbusBinding* lib = luaStates[L];
    lua_pushboolean(L, lib->doublePinyinScheme.isValidDoublePinyin(lua_tostring(L, 1)));
    return 1;
}

/**
 * in: 1 string
 * out: 1 string
 */
int LuaIbusBinding::l_convertDoublePinyin(lua_State *L) {
    luaL_checktype(L, 1, LUA_TSTRING);
    LuaIbusBinding* lib = luaStates[L];
    lua_pushstring(L, lib->doublePinyinScheme.query(lua_tostring(L, 1)).c_str());
    return 1;
}

/**
 * in: 1 table
 * out: 1 int conflict count
 */
int LuaIbusBinding::l_setDoublePinyinScheme(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1); // only keep 1 value
    LuaIbusBinding* lib = luaStates[L];
    lib->doublePinyinScheme.clear();

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

        lib->doublePinyinScheme.bindKey(key, consonant, vowels);
    }
    // return conflict count
    lua_pushinteger(L, lib->doublePinyinScheme.buildMap());
    return 1;
}

const struct luaL_Reg LuaIbusBinding::pycclib[] = {
    {"setDoublePinyinScheme", LuaIbusBinding::l_setDoublePinyinScheme},
    {"convertDoublePinyin", LuaIbusBinding::l_convertDoublePinyin},
    {"isValidDoublePinyin", LuaIbusBinding::l_isValidDoublePinyin},
    {"bitand", LuaIbusBinding::l_bitand},
    {"bitxor", LuaIbusBinding::l_bitxor},
    {"bitor", LuaIbusBinding::l_bitor},
    {"keymask", LuaIbusBinding::l_keymask},
    {NULL, NULL}
};

// LuaIbusBinding class

map<const lua_State*, LuaIbusBinding*> LuaIbusBinding::luaStates;
DoublePinyinScheme LuaIbusBinding::doublePinyinScheme;
char LuaIbusBinding::LIB_NAME[] = "pycc";

LuaIbusBinding::LuaIbusBinding() {
    // init mutex
    if (pthread_mutex_init(&luaStateMutex, NULL)) {
        fprintf(stderr, "fail to create lua state mutex.\nAborted.\n");
        exit(EXIT_FAILURE);
    }
    // init lua state
    L = lua_open();
    luaL_openlibs(L);
    // add custom library functions
    luaL_register(L, LIB_NAME, pycclib);
    // set static information
    setValue("VERSION", VERSION);
    // insert into static map, let lib function be able to lookup this class
    // L - this one-to-one relation. (Now it takes O(logN) to lookup 'this'
    // from L. Any way to make it more smooth?)
    luaStates[L] = this;
}

const lua_State* LuaIbusBinding::getLuaState() const {
    return L;
}

/**
 * return 0 if no error
 */
int LuaIbusBinding::doString(const char* luaScript) {
    int r = luaL_dostring(L, luaScript);
    if (r) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    return r;
}

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
 */
int LuaIbusBinding::callLuaFunction(bool inNewThread, const char * const funcName, const char* sig, ...) {
    va_list vl;
    int narg, nres;

    va_start(vl, sig);

    lua_State * state = L;
    if (inNewThread) state = lua_newthread(L);
    else pthread_mutex_lock(&luaStateMutex);

    lua_checkstack(state, strlen(sig) + 2 + inNewThread ? 1 : 0);

    lua_getglobal(state, LIB_NAME);
    lua_getfield(state, -1, funcName);
    if (lua_isnil(state, -1)) {
        // function not found, here just ignore and skip.
        if (!inNewThread) pthread_mutex_unlock(&luaStateMutex);
        lua_settop(state, 0); // pop all
        return -1;
    }

    for (narg = 0; *sig; ++narg, ++sig) {
        luaL_checkstack(state, 1, "too many arguments");
        if (*sig == 'd') lua_pushinteger(state, va_arg(vl, int));
        else if (*sig == 'f') lua_pushnumber(state, va_arg(vl, double));
        else if (*sig == 's') lua_pushstring(state, va_arg(vl, char*));
        else if (*sig == 'b') lua_pushboolean(state, va_arg(vl, int));
        else if (*sig == '>' || *sig == '|' || *sig == ' ' || *sig == '=') {
            sig++;
            break;
        } else {
            fprintf(stderr, "invalid option (%c) in callLua\n", *sig);
        }
    }

    nres = strlen(sig);
    int ret;
    if ((ret = lua_pcall(state, narg, nres, 0)) != 0) {
        fprintf(stderr, "error calling lua function '%s.%s': %s.\nprogram is likely to crash soon\n", LIB_NAME, funcName, lua_tostring(state, -1));
        lua_pop(state, 2);
    }

    // clear stack and report error early.
    if (ret != 0) {
        if (!inNewThread) pthread_mutex_unlock(&luaStateMutex);
        lua_settop(state, 0);
        return ret;
    }

    for (int pres = -nres; *sig; ++pres, ++sig) {
        switch (*sig) {
            case 'd':
                if (!lua_isnumber(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect integer.\n", LIB_NAME, funcName);
                *va_arg(vl, int*) = lua_tointeger(state, pres);
                break;
            case 'f':
                if (!lua_isnumber(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect number.\n", LIB_NAME, funcName);
                *va_arg(vl, double*) = lua_tonumber(state, pres);
                break;
            case 's':
                if (!lua_isstring(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect string.\n", LIB_NAME, funcName);
                *va_arg(vl, string*) = string(lua_tostring(state, pres));
                break;
            case 'b':
                if (!lua_isboolean(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect boolean.\n", LIB_NAME, funcName);
                *va_arg(vl, int*) = lua_toboolean(state, pres);
                break;
            default:
                fprintf(stderr, "invalid option (%c) in callLua\n", *sig);
        }
    }

    if (!inNewThread) pthread_mutex_unlock(&luaStateMutex);
    lua_settop(state, 0); // pop all (may include thread)
    return ret;
}

void LuaIbusBinding::addFunction(const lua_CFunction func, const char * funcName) {
    lua_settop(L, 0);
    lua_getglobal(L, LIB_NAME);
    lua_pushstring(L, funcName);
    lua_pushcfunction(L, func);
    lua_settable(L, 1);
    lua_settop(L, 0);
}

bool LuaIbusBinding::getValue(const char* varName, const bool defaultValue, const char* libName) {
    lua_getglobal(L, libName);
    lua_getfield(L, -1, varName);
    if (lua_isboolean(L, -1)) {
        bool r = lua_toboolean(L, -1);
        lua_pop(L, 2);
        return r;
    } else {
        lua_pop(L, 2);
        return defaultValue;
    }
}

string LuaIbusBinding::getValue(const char* varName, const char* defaultValue, const char* libName) {
    lua_getglobal(L, libName);
    lua_getfield(L, -1, varName);
    if (lua_isstring(L, -1)) {
        string r = lua_tostring(L, -1);
        lua_pop(L, 2);
        return r;
    } else {
        lua_pop(L, 2);
        return defaultValue;
    }
}

int LuaIbusBinding::getValue(const char* varName, const int defaultValue, const char* libName) {
    lua_getglobal(L, libName);
    lua_getfield(L, -1, varName);
    if (lua_isnumber(L, -1)) {
        int r = lua_tointeger(L, -1);
        lua_pop(L, 2);
        return r;
    } else {
        lua_pop(L, 2);
        return defaultValue;
    }
}

void LuaIbusBinding::setValue(const char* varName, const int value, const char* libName) {
    lua_getglobal(L, libName);
    lua_pushinteger(L, value);
    lua_setfield(L, -2, varName); //it will pop value
    lua_pop(L, 1);
}

void LuaIbusBinding::setValue(const char* varName, const char value[], const char* libName) {
    lua_getglobal(L, libName);
    lua_pushstring(L, value);
    lua_setfield(L, -2, varName); //it will pop value
    lua_pop(L, 1);
}

void LuaIbusBinding::setValue(const char* varName, const bool value, const char* libName) {
    lua_getglobal(L, libName);
    lua_pushboolean(L, value);
    lua_setfield(L, -2, varName); //it will pop value
    lua_pop(L, 1);
}

/**
 * this is private and should not be used.
 */
LuaIbusBinding::LuaIbusBinding(const LuaIbusBinding& orig) {
}

LuaIbusBinding::~LuaIbusBinding() {
    luaStates.erase(L);
    lua_close(L);
    pthread_mutex_destroy(&luaStateMutex);
}
