/* 
 * File:   LuaBinding.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * communicate with lualib
 */

#ifndef _LUABINDING_H
#define	_LUABINDING_H

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <map>
#include <string>

using std::map;
using std::pair;
using std::string;

class LuaBinding {
public:
    static const char LIB_NAME[];
    static const string LibraryName;

    LuaBinding();

    /**
     * getValue usage:
     * ime.a["abc"]["1"][1][2]["bb"] : getValue("abc.1..1..2.bb", "", "ime")
     */
    string getValue(const char* varName, const char* defaultValue = "", const char* libName = LIB_NAME);
    int getValue(const char* varName, const int defaultValue = -1, const char* libName = LIB_NAME);
    size_t getValue(const char* varName, const size_t defaultValue, const char* libName = LIB_NAME);
    bool getValue(const char* varName, const bool defaultValue = false, const char* libName = LIB_NAME);
    double getValue(const char* varName, const double defaultValue, const char* libName = LIB_NAME);
    int getValueType(const char* varName, const char* libName = LIB_NAME);

    void setValue(const char* varName, const int value, const char* libName = LIB_NAME);
    void setValue(const char* varName, const char value[], const char* libName = LIB_NAME);
    void setValue(const char* varName, const bool value, const char* libName = LIB_NAME);

    virtual ~LuaBinding();
    int doString(const char* luaScript, bool locked = false);

    /**
     * allow .name (string key)
     * allow ..num (num key)
     * stop at non-table (may be not nil)
     * Intended to use privately
     * @return pushed count
     */
    int reachValue(const char* varName, const char* libName = LIB_NAME);


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
    void registerFunction(const lua_CFunction func, const char * funcName);

    const lua_State* getLuaState() const;

    /**
     * load global config
     */
    static void staticInit();
    static void loadStaticConfigure();
    
    /**
     * clean up static vars.
     * close dbs.
     */
    static void staticDestruct();
    static LuaBinding& getStaticBinding();
    static LuaBinding& getLuaBinding(lua_State *L);

    pthread_mutex_t* getAtomMutex();
private:
    LuaBinding(const LuaBinding& orig);
    pthread_mutex_t luaStateAtomMutex, luaStateFunctionMutex;

    static LuaBinding* staticLuaBinding;
    // Lua C functions related
    lua_State* L;
    static const struct luaL_Reg luaLibraryReg[];
    static map<const lua_State*, LuaBinding*> luaStates;

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
     * print lua stack, for debugging
     * in: -
     * out: -
     */
    static int l_printStack(lua_State *L);
    /**
     * do lua script
     * in: string
     * out: bool
     */
    static int l_executeScript(lua_State * L);
};

#endif	/* _LUAIBUSBINDING_H */

