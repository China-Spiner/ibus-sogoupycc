/* 
 * File:   LuaBinding.cpp
 * Author: WU Jun <quark@lihdd.net>
 * 
 * see .h file for changelog
 */

#include "LuaBinding.h"
#include "defines.h"
#include <cstring>
#include <cstdlib>
#include <cassert>
// Lua C functions

/**
 * in: 0
 * out: 0
 */
int LuaBinding::l_printStack(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_printStack\n");
    int c;
    printf("LuaStack element count: %d\n", c = lua_gettop(L));
    for (int i = 1; i <= c; i++) {
        printf("  Emement[-%2d]: ", i);
        switch (lua_type(L, -i)) {
            case LUA_TNUMBER:
                printf("%.2f\n", lua_tonumber(L, -i));
                break;
            case LUA_TBOOLEAN:
                printf("%s\n", lua_toboolean(L, -i) ? "true" : "false");
                break;
            case LUA_TSTRING:
                printf("%s\n", lua_tostring(L, -i));
                break;
            case LUA_TNIL:
                printf("nil\n");
                break;
            case LUA_TTABLE:
                printf("table\n");
                break;
            default:
                printf("(%s)\n", lua_typename(L, lua_type(L, -i)));
        }
        fflush(stdout);
    }
    return 0;
}

/**
 * in: 2 int
 * out: 1 boolean
 */
int LuaBinding::l_keymask(lua_State *L) {
    
    DEBUG_PRINT(2, "[LUABIND] l_keymask (L: %x)\n", (int) L);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushboolean(L, (lua_tointeger(L, 1) & lua_tointeger(L, 2)) == lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 2 int
 * out: 1 int
 */
int LuaBinding::l_bitand(lua_State *L) {
    
    DEBUG_PRINT(2, "[LUABIND] l_bitand\n");
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushinteger(L, lua_tointeger(L, 1) & lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 2 int
 * out: 1 int
 */
int LuaBinding::l_bitxor(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_bitxor\n");
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushinteger(L, lua_tointeger(L, 1) ^ lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 2 int
 * out: 1 int
 */
int LuaBinding::l_bitor(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_bitor\n");
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushinteger(L, lua_tointeger(L, 1) | lua_tointeger(L, 2));
    return 1;
}

/**
 * in: 1 string
 * out: 1 bool
 */
int LuaBinding::l_isValidPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_isValidPinyin\n");
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushboolean(L, PinyinUtility::isValidPinyin(lua_tostring(L, 1)));
    return 1;
}

/**
 * in: 1 string
 * out: 1 string
 */
int LuaBinding::l_charsToPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_charsToPinyin\n");
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushstring(L, PinyinUtility::charactersToPinyins(lua_tostring(L, 1)).c_str());
    return 1;
}

/**
 * in: 1 string
 * out: 1 bool
 */
int LuaBinding::l_isValidDoublePinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_isValidDoublePinyin\n");
    luaL_checktype(L, 1, LUA_TSTRING);
    LuaBinding* lib = luaStates[L];
    lua_checkstack(L, 1);
    lua_pushboolean(L, lib->doublePinyinScheme.isValidDoublePinyin(lua_tostring(L, 1)));
    return 1;
}

/**
 * in: 1 string
 * out: 1 string
 */
int LuaBinding::l_doubleToFullPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_doubleToFullPinyin\n");
    luaL_checktype(L, 1, LUA_TSTRING);
    LuaBinding* lib = luaStates[L];
    lua_checkstack(L, 1);
    lua_pushstring(L, lib->doublePinyinScheme.query(lua_tostring(L, 1)).c_str());
    return 1;
}

/**
 * in: 1 table
 * out: 1 int conflict count
 */
int LuaBinding::l_setDoublePinyinScheme(lua_State *L) {
    
    DEBUG_PRINT(1, "[LUABIND] setDoublePinyinScheme\n");
    luaL_checktype(L, 1, LUA_TTABLE);

    LuaBinding* lib = luaStates[L];
    lib->doublePinyinScheme.clear();

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

        lib->doublePinyinScheme.bindKey(key, consonant, vowels);
    }
    // return conflict count
    lua_pushinteger(L, lib->doublePinyinScheme.buildMap());
    return 1;
}

const struct luaL_Reg LuaBinding::pycclib[] = {
    {"setDoublePinyinScheme", LuaBinding::l_setDoublePinyinScheme},
    {"doubleToFullPinyin", LuaBinding::l_doubleToFullPinyin},
    {"isValidDoublePinyin", LuaBinding::l_isValidDoublePinyin},
    {"bitand", LuaBinding::l_bitand},
    {"bitxor", LuaBinding::l_bitxor},
    {"bitor", LuaBinding::l_bitor},
    {"keymask", LuaBinding::l_keymask},
    // {"printStack", LuaBinding::l_printStack},
    {"charsToPinyin", LuaBinding::l_charsToPinyin},
    {NULL, NULL}
};

// LuaIbusBinding class

map<const lua_State*, LuaBinding*> LuaBinding::luaStates;
DoublePinyinScheme LuaBinding::doublePinyinScheme;
char LuaBinding::LIB_NAME[] = "pycc";

LuaBinding::LuaBinding() {
    
    DEBUG_PRINT(1, "[LUABIND] Init\n");
    // init mutex
    pthread_mutexattr_t luaStateMutexAttribute;
    pthread_mutexattr_init(&luaStateMutexAttribute);
    pthread_mutexattr_settype(&luaStateMutexAttribute, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&luaStateMutex, &luaStateMutexAttribute)) {
        fprintf(stderr, "fail to create lua state mutex.\nAborted.\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutexattr_destroy(&luaStateMutexAttribute);
    // init lua state
    L = lua_open();
    luaL_openlibs(L);
    // add custom library functions. after this, stack has 1 element.
    luaL_register(L, LIB_NAME, pycclib);
    lua_pop(L, 1);
    // set static information
    setValue("VERSION", VERSION);
    // insert into static map, let lib function be able to lookup this class
    // L - this one-to-one relation. (Now it takes O(logN) to lookup 'this'
    // from L. Any way to make it more smooth?)
    luaStates[L] = this;
    
}

const lua_State* LuaBinding::getLuaState() const {
    
    return L;
}

/**
 * return 0 if no error
 */
int LuaBinding::doString(const char* luaScript) {
    
    DEBUG_PRINT(3, "[LUABIND] doString(%s)\n", luaScript);
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
 * do not use in multi-threads
 */
int LuaBinding::callLuaFunction(const char * const funcName, const char* sig, ...) {
    DEBUG_PRINT(2, "[LUABIND] callLua: %s (%s)\n", funcName, sig);
    
    int narg, nres, ret = 0;
    
    va_list vl;
    va_start(vl, sig);

    assert(lua_gettop(L) == 0);
    lua_State * state = L;
    pthread_mutex_lock(&luaStateMutex);

    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Stack size at the beginning: %d\n", lua_gettop(state));
    
    lua_checkstack(state, strlen(sig) + 2);

    lua_getglobal(state, LIB_NAME);
    lua_getfield(state, -1, funcName); // stack: 2 elements

    
    if (lua_isnil(state, -1)) {
        // function not found, here just ignore and skip.
        ret = -1;
        lua_pop(state, 2); // pop nil and libname
        goto callLuaFunction_exit;
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
    // prevent dead lock
    pthread_mutex_unlock(&luaStateMutex);

    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Params pushed\n");
    if ((ret = lua_pcall(state, narg, nres, 0)) != 0) {
        fprintf(stderr, "error calling lua function '%s.%s': %s.\nprogram is likely to crash soon\n", LIB_NAME, funcName, lua_tostring(state, -1));
        lua_pop(state, 2); // pop err string and libname
    }

    pthread_mutex_lock(&luaStateMutex);
    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Pcall finished\n");

    // report error early.
    if (ret != 0) goto callLuaFunction_exit;

    for (int pres = -nres; *sig; ++pres, ++sig) {
        switch (*sig) {
            case 'd':
                if (!lua_isnumber(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect integer.\n", LIB_NAME, funcName);
                else *va_arg(vl, int*) = lua_tointeger(state, pres);
                break;
            case 'f':
                if (!lua_isnumber(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect number.\n", LIB_NAME, funcName);
                else *va_arg(vl, double*) = lua_tonumber(state, pres);
                break;
            case 's':
                if (!lua_isstring(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect string.\n", LIB_NAME, funcName);
                else *va_arg(vl, string*) = string(lua_tostring(state, pres));
                break;
            case 'b':
                if (!lua_isboolean(state, pres)) fprintf(stderr, "Lua function '%s.%s' result type error. expect boolean.\n", LIB_NAME, funcName);
                else *va_arg(vl, int*) = lua_toboolean(state, pres);
                break;
            default:
                fprintf(stderr, "invalid option (%c) in callLua\n", *sig);
        }
        
    }
    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Cleaning\n");
    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Stack size: %d, result count: %d\n", lua_gettop(state), nres);
    lua_pop(state, nres + 1); // pop ret values and lib name

callLuaFunction_exit:    
    // pop values before enter here
    pthread_mutex_unlock(&luaStateMutex);

    //assert(lua_gettop(L) == 0);
    return ret;
}

void LuaBinding::addFunction(const lua_CFunction func, const char * funcName) {
    
    DEBUG_PRINT(0, "[LUABIND] addFunction: %s\n", funcName);

    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 3);
    lua_getglobal(L, LIB_NAME); // should be a table
    lua_pushstring(L, funcName); // key
    lua_pushcfunction(L, func); // value
    lua_settable(L, -3); // will pop key and value
    lua_pop(L, 1); // stack should be empty.
    pthread_mutex_unlock(&luaStateMutex);
}

bool LuaBinding::getValue(const char* varName, const bool defaultValue, const char* libName) {
    
    DEBUG_PRINT(4, "[LUABIND] getValue(boolean): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    lua_getfield(L, -1, varName);
    bool r = defaultValue;
    if (lua_isboolean(L, -1)) r = lua_toboolean(L, -1);
    lua_pop(L, 2);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

string LuaBinding::getValue(const char* varName, const char* defaultValue, const char* libName) {
    
    DEBUG_PRINT(4, "[LUABIND] getValue(string): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    lua_getfield(L, -1, varName);
    string r = defaultValue;
    if (lua_isstring(L, -1)) r = lua_tostring(L, -1);
    lua_pop(L, 2);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

int LuaBinding::getValue(const char* varName, const int defaultValue, const char* libName) {
    
    DEBUG_PRINT(4, "[LUABIND] getValue(int): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    lua_getfield(L, -1, varName);
    int r = defaultValue;
    if (lua_isnumber(L, -1)) r = lua_tointeger(L, -1);
    lua_pop(L, 2);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

void LuaBinding::setValue(const char* varName, const int value, const char* libName) {
    
    DEBUG_PRINT(4, "[LUABIND] setValue(int): %s.%s = %d\n", libName, varName, value);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    lua_pushinteger(L, value);
    lua_setfield(L, -2, varName); //it will pop value
    lua_pop(L, 1);
    pthread_mutex_unlock(&luaStateMutex);
}

void LuaBinding::setValue(const char* varName, const char value[], const char* libName) {
    
    DEBUG_PRINT(4, "[LUABIND] setValue(string): %s.%s = '%s'\n", libName, varName, value);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    lua_pushstring(L, value);
    lua_setfield(L, -2, varName); //it will pop value
    lua_pop(L, 1);
    pthread_mutex_unlock(&luaStateMutex);
}

void LuaBinding::setValue(const char* varName, const bool value, const char* libName) {
    
    DEBUG_PRINT(4, "[LUABIND] setValue(boolean): %s.%s = %s\n", libName, varName, value ? "true" : "false");
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    lua_pushboolean(L, value);
    lua_setfield(L, -2, varName); //it will pop value
    lua_pop(L, 1);
    pthread_mutex_unlock(&luaStateMutex);
}

/**
 * this is private and should not be used.
 */
LuaBinding::LuaBinding(const LuaBinding& orig) {
    
}

LuaBinding::~LuaBinding() {
    
    DEBUG_PRINT(1, "[LUABIND] Destroy\n");
    luaStates.erase(L);
    lua_close(L);
    pthread_mutex_destroy(&luaStateMutex);
}
