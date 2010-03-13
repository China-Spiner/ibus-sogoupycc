/* 
 * File:   LuaBinding.cpp
 * Author: WU Jun <quark@lihdd.net>
 */

#include "LuaBinding.h"
#include "Configuration.h"
#include "defines.h"
#include "XUtility.h"
#include <cstring>
#include <cstdlib>
#include <cassert>


// Lua C functions

int LuaBinding::l_applySettings(lua_State* L) {
    DEBUG_PRINT(3, "[LUABIND] l_applySettings\n");
    LuaBinding& lb = *luaStates[L];

    // database confs
    Configuration::dbResultLimit = lb.getValue("db_result_limit", Configuration::dbResultLimit);
    Configuration::dbLengthLimit = lb.getValue("db_length_limit", Configuration::dbLengthLimit);
    Configuration::dbLongPhraseAdjust = lb.getValue("db_phrase_adjust", Configuration::dbLongPhraseAdjust);
    Configuration::dbOrder = string(lb.getValue("db_query_order", Configuration::dbOrder.c_str()));

    // colors (-1: use default)
    Configuration::preeditForeColor = lb.getValue("preedit_fore_color", Configuration::preeditForeColor);
    Configuration::preeditBackColor = lb.getValue("preedit_back_color", Configuration::preeditBackColor);
    Configuration::requestingForeColor = lb.getValue("requesting_fore_color", Configuration::requestingForeColor);
    Configuration::requestingBackColor = lb.getValue("requesting_back_color", Configuration::requestingBackColor);
    Configuration::requestedForeColor = lb.getValue("responsed_fore_color", Configuration::requestedForeColor);
    Configuration::requestedBackColor = lb.getValue("responsed_back_color", Configuration::requestedBackColor);
    Configuration::correctingForeColor = lb.getValue("correcting_fore_color", Configuration::correctingForeColor);
    Configuration::correctingBackColor = lb.getValue("correcting_back_color", Configuration::correctingBackColor);

    // selection timeout set, user passed here is second
    Configuration::selectionTimout = (long long) lb.getValue("sel_timeout", (double) Configuration::selectionTimout / XUtility::MICROSECOND_PER_SECOND) * XUtility::MICROSECOND_PER_SECOND;

    // keys
    Configuration::engModeToggleKey = lb.getValue("eng_mode_toggle_key", Configuration::engModeToggleKey);
    Configuration::engModeKey = lb.getValue("eng_mode_key", Configuration::engModeKey);
    Configuration::chsModeKey = lb.getValue("chs_mode_key", Configuration::chsModeKey);
    Configuration::startCorrectionKey = lb.getValue("correction_mode_key", Configuration::startCorrectionKey);
    Configuration::pageDownKey = lb.getValue("page_down_key", Configuration::pageDownKey);
    Configuration::pageUpKey = lb.getValue("page_up_key", Configuration::pageUpKey);

    // bools
    Configuration::useDoublePinyin = lb.getValue("use_double_pinyin", Configuration::useDoublePinyin);
    Configuration::strictDoublePinyin = lb.getValue("strict_double_pinyin", Configuration::strictDoublePinyin);
    Configuration::startInEngMode = lb.getValue("start_in_eng_mode", Configuration::startInEngMode);
    Configuration::writeRequestCache = lb.getValue("cache_requests", Configuration::writeRequestCache);

    // labels used in lookup table, ibus has 16 chars limition.
    Configuration::tableLabelKeys = string(lb.getValue("label_keys", Configuration::tableLabelKeys.c_str()));
    if (Configuration::tableLabelKeys.length() > 16 || Configuration::tableLabelKeys.empty()) Configuration::tableLabelKeys = Configuration::tableLabelKeys.substr(0, 16);

    // external script path
    Configuration::fetcherPath = string(lb.getValue("fetcher_path", Configuration::fetcherPath.c_str()));
    Configuration::fetcherBufferSize = lb.getValue("fetcher_buffer_size", Configuration::fetcherBufferSize);

    // punc map
    if (lb.getValueType("punc_map") == LUA_TTABLE) {
        DEBUG_PRINT(4, "[LUABIND] read punc_map\n");
        Configuration::punctuationMap.clear();
        // IMPROVE: some lock here?
        int pushedCount = lb.reachValue("punc_map");
        for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
            // key is at index -2 and value is at index -1
            // key should be a string or a number

            int key = 0;
            switch (lua_type(L, -2)) {
                case LUA_TNUMBER: 
                    key = lua_tointeger(L, -2);
                    break;
                case LUA_TSTRING:
                    key = lua_tostring(L, -2)[0];
                    break;
                default:
                    luaL_checktype(L, -2, LUA_TSTRING);
            }

            Configuration::FullPunctuation fpunc;
            switch (lua_type(L, -1)) {
                case LUA_TSTRING:
                    // single punc
                    DEBUG_PRINT(4, "[LUABIND] single punc: %s\n", lua_tostring(L, -1));
                    fpunc.addPunctuation(lua_tostring(L, -1));
                    break;
                case LUA_TTABLE:
                    // multi puncs
                    DEBUG_PRINT(4, "[LUABIND] multi punc\n");
                    for (lua_pushnil(L); lua_next(L, -2) != 0; lua_pop(L, 1)) {
                        // value should be string
                        luaL_checktype(L, -1, LUA_TSTRING);
                        DEBUG_PRINT(5, "[LUABIND]   punc: %s\n", lua_tostring(L, -1));
                        fpunc.addPunctuation(lua_tostring(L, -1));
                    }
                    // after the loop, the key and value of inner table are all popped
                    break;
            }

            Configuration::punctuationMap.setPunctuationPair(key, fpunc);
        }
        lua_pop(L, pushedCount);
    }

    return 0;
}

int LuaBinding::l_getPhraseDatabaseLoadedCount(lua_State* L) {
    DEBUG_PRINT(3, "[LUABIND] l_getPhraseDatabaseLoadedCount\n");
    lua_checkstack(L, 1);
    lua_pushinteger(L, pinyinDatabases.size());
    return 1;
}

int LuaBinding::l_loadPhraseDatabase(lua_State* L) {
    DEBUG_PRINT(3, "[LUABIND] l_loadPhraseDatabase\n");
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);

    string dbPath = lua_tostring(L, 1);
    double weight = 1;
    int r = 0;

    if (lua_type(L, 2) == LUA_TNUMBER) weight = lua_tonumber(L, 2);
    if (pinyinDatabases.find(dbPath) == pinyinDatabases.end()) {
        PinyinDatabase *pydb = new PinyinDatabase(dbPath, weight);
        if (pydb->isDatabaseOpened()) {
            r = 1;
            pinyinDatabases.insert(pair<string, PinyinDatabase*>(dbPath, pydb));
        } else delete pydb;
    }

    lua_pushinteger(L, r);
    return 1;
}

int LuaBinding::l_setDebugLevel(lua_State *L) {
    DEBUG_PRINT(3, "[LUABIND] l_setDebugLevel\n");
    luaL_checktype(L, 1, LUA_TNUMBER);
    globalDebugLevel = lua_tointeger(L, 1);
    return 0;
}

int LuaBinding::l_printStack(lua_State *L) {
    DEBUG_PRINT(3, "[LUABIND] l_printStack\n");
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

int LuaBinding::l_keymask(lua_State *L) {
    DEBUG_PRINT(3, "[LUABIND] l_keymask\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushboolean(L, (lua_tointeger(L, 1) & lua_tointeger(L, 2)) == lua_tointeger(L, 2));
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

int LuaBinding::l_bitand(lua_State *L) {
    DEBUG_PRINT(3, "[LUABIND] l_bitand\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushinteger(L, lua_tointeger(L, 1) & lua_tointeger(L, 2));
    pthread_mutex_unlock(&lib->luaStateMutex);

    return 1;
}

int LuaBinding::l_bitxor(lua_State *L) {
    DEBUG_PRINT(3, "[LUABIND] l_bitxor\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushinteger(L, lua_tointeger(L, 1) ^ lua_tointeger(L, 2));
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

int LuaBinding::l_bitor(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_bitor\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TNUMBER);
    luaL_checktype(L, 2, LUA_TNUMBER);
    lua_checkstack(L, 1);
    lua_pushinteger(L, lua_tointeger(L, 1) | lua_tointeger(L, 2));
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

int LuaBinding::l_isValidPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_isValidPinyin\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushboolean(L, PinyinUtility::isValidPinyin(lua_tostring(L, 1)));
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

int LuaBinding::l_charsToPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_charsToPinyin\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushstring(L, PinyinUtility::charactersToPinyins(lua_tostring(L, 1)).c_str());
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

/**
 * in: 1 string
 * out: 1 bool
 */
int LuaBinding::l_isValidDoublePinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_isValidDoublePinyin\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushboolean(L, lib->doublePinyinScheme.isValidDoublePinyin(lua_tostring(L, 1)));
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

int LuaBinding::l_doubleToFullPinyin(lua_State *L) {
    DEBUG_PRINT(2, "[LUABIND] l_doubleToFullPinyin\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TSTRING);
    lua_checkstack(L, 1);
    lua_pushstring(L, lib->doublePinyinScheme.query(lua_tostring(L, 1)).c_str());
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

int LuaBinding::l_setDoublePinyinScheme(lua_State *L) {
    DEBUG_PRINT(1, "[LUABIND] setDoublePinyinScheme\n");
    LuaBinding* lib = luaStates[L];
    pthread_mutex_lock(&lib->luaStateMutex);
    luaL_checktype(L, 1, LUA_TTABLE);

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
    pthread_mutex_unlock(&lib->luaStateMutex);
    return 1;
}

const struct luaL_Reg LuaBinding::luaLibraryReg[] = {
    {"set_double_pinyin_scheme", LuaBinding::l_setDoublePinyinScheme},
    {"double_to_full_pinyin", LuaBinding::l_doubleToFullPinyin},
    {"is_valid_double_pinyin", LuaBinding::l_isValidDoublePinyin},
    //{"bitand", LuaBinding::l_bitand},
    //{"bitxor", LuaBinding::l_bitxor},
    //{"bitor", LuaBinding::l_bitor},
    //{"keymask", LuaBinding::l_keymask},
    //{"printStack", LuaBinding::l_printStack},
    {"chars_to_pinyin", LuaBinding::l_charsToPinyin},
    {"set_debug_level", LuaBinding::l_setDebugLevel},
    {"load_database", LuaBinding::l_loadPhraseDatabase},
    {"database_count", LuaBinding::l_getPhraseDatabaseLoadedCount},
    {"apply_settings", LuaBinding::l_applySettings},
    {NULL, NULL}
};

// LuaBinding class

map<const lua_State*, LuaBinding*> LuaBinding::luaStates;
DoublePinyinScheme LuaBinding::doublePinyinScheme;

const char LuaBinding::LIB_NAME[] = "ime";
const string LuaBinding::LibraryName = LuaBinding::LIB_NAME;

map<string, PinyinDatabase*> LuaBinding::pinyinDatabases;

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
    luaL_register(L, LIB_NAME, luaLibraryReg);
    lua_pop(L, 1);

    // set constants
    setValue("VERSION", VERSION);
    setValue("PKGDATADIR", PKGDATADIR);
    setValue("USERCONFIGDIR", ((string) (g_get_user_config_dir()) + G_DIR_SEPARATOR_S "ibus" G_DIR_SEPARATOR_S "sogoupycc").c_str());
    setValue("USERCACHEDIR", ((string) (g_get_user_cache_dir()) + G_DIR_SEPARATOR_S "ibus" G_DIR_SEPARATOR_S "sogoupycc").c_str());
    setValue("USERDATADIR", ((string) (g_get_user_data_dir()) + G_DIR_SEPARATOR_S "ibus" G_DIR_SEPARATOR_S "sogoupycc").c_str());
    setValue("DIRSEPARATOR", G_DIR_SEPARATOR_S);

    Configuration::addConstantsToLua(*this);

    // insert into static map, let lib function be able to lookup this class
    // L - this one-to-one relation. (Now it takes O(logN) to lookup 'this'
    // from L. Any way to make it more smooth?)
    luaStates[L] = this;
}

void LuaBinding::staticInit() {
    // load config, modify static vars:
    // doublePinyinScheme, pinyinDatabases, etc
    staticLuaBinding = new LuaBinding();
    staticLuaBinding->doString("dofile('" PKGDATADIR G_DIR_SEPARATOR_S "config.lua')");

    l_applySettings(staticLuaBinding->L);
}

void LuaBinding::staticDestruct() {
    DEBUG_PRINT(1, "[LUABIND] Static Destroy\n");
    // close dbs
    for (map<string, PinyinDatabase*>::iterator it = pinyinDatabases.begin(); it != pinyinDatabases.end(); ++it) {
        if (it->second) {
            delete it->second;
            it->second = NULL;
        }
    }
    delete staticLuaBinding;
}

const lua_State* LuaBinding::getLuaState() const {

    return L;
}

/**
 * return 0 if no error
 */
int LuaBinding::doString(const char* luaScript) {

    DEBUG_PRINT(3, "[LUABIND] doString(%s)\n", luaScript);
    pthread_mutex_lock(&luaStateMutex);
    int r = luaL_dostring(L, luaScript);
    if (r) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

int LuaBinding::callLuaFunction(const char * const funcName, const char* sig, ...) {
    DEBUG_PRINT(2, "[LUABIND] callLua: %s (%s)\n", funcName, sig);

    int narg, nres, ret = 0;

    va_list vl;
    va_start(vl, sig);

    assert(lua_gettop(L) == 0);
    lua_State * state = L;

    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(state, strlen(sig) + 2);
    int pushedCount = reachValue(funcName, "_G");
    printf("count:%d\n", pushedCount);


    if (lua_isnil(state, -1)) {
        // function not found, here just ignore and skip.
        ret = -1;
        goto aborting;
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

    // lua_pcall will remove paras and function
    pushedCount--;
    nres = strlen(sig);
    // since we use recursive mutex here, no unlock needed
    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Params pushed\n");
    if ((ret = lua_pcall(state, narg, nres, 0)) != 0) {
        fprintf(stderr, "error calling lua function '%s.%s': %s.\nprogram is likely to crash soon\n", LIB_NAME, funcName, lua_tostring(state, -1));
        pushedCount++;
        // report error early.
        goto aborting;
    }
    DEBUG_PRINT(3, "[LUABIND.CALLLUA] Pcall finished\n");

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

aborting:
    // pop values before enter here
    lua_pop(state, pushedCount);
    pthread_mutex_unlock(&luaStateMutex);
    va_end(vl);

    //assert(lua_gettop(L) == 0);
    return ret;
}

void LuaBinding::addFunction(const lua_CFunction func, const char * funcName) {
    DEBUG_PRINT(1, "[LUABIND] addFunction: %s\n", funcName);

    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 3);
    lua_getglobal(L, LIB_NAME); // should be a table
    lua_pushstring(L, funcName); // key
    lua_pushcfunction(L, func); // value
    lua_settable(L, -3); // will pop key and value
    lua_pop(L, 1); // stack should be empty.
    pthread_mutex_unlock(&luaStateMutex);
}

int LuaBinding::reachValue(const char* varName, const char* libName) {
    DEBUG_PRINT(5, "[LUABIND] reachValue: %s.%s\n", libName, varName);
    // internal use, lock before call this
    vector<string> names = splitString(varName, '.');

    int pushedCount = 1;
    lua_checkstack(L, 1 + names.size());
    lua_getglobal(L, libName);

    bool nextIsNumber = false;
    for (size_t i = 0; i < names.size(); i++) {
        string& name = names[i];

        if (name.empty()) {
            nextIsNumber = true;
            continue;
        }
        if (!lua_istable(L, -1)) {
            DEBUG_PRINT(6, "[LUABIND] Not a table, stop\n");
            // stop here
            break;
        }
        if (nextIsNumber) {
            int id;
            DEBUG_PRINT(6, "[LUABIND] number: %s\n", name.c_str());
            sscanf(name.c_str(), "%d", &id);
            lua_pushnumber(L, id);
        } else {
            DEBUG_PRINT(6, "[LUABIND] string: %s\n", name.c_str());
            lua_pushstring(L, name.c_str());
        }
        pushedCount++;
        lua_gettable(L, -2);
    }
    return pushedCount;
}

bool LuaBinding::getValue(const char* varName, const bool defaultValue, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] getValue(boolean): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    bool r = defaultValue;
    int pushedCount = reachValue(varName, libName);
    if (lua_isboolean(L, -1)) r = lua_toboolean(L, -1);
    lua_pop(L, pushedCount);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

string LuaBinding::getValue(const char* varName, const char* defaultValue, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] getValue(string): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    string r = defaultValue;
    int pushedCount = reachValue(varName, libName);
    if (lua_isstring(L, -1)) r = lua_tostring(L, -1);
    lua_pop(L, pushedCount);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

int LuaBinding::getValue(const char* varName, const int defaultValue, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] getValue(int): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    int r = defaultValue;
    int pushedCount = reachValue(varName, libName);
    if (lua_isnumber(L, -1)) r = lua_tointeger(L, -1);
    lua_pop(L, pushedCount);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

unsigned int LuaBinding::getValue(const char* varName, const unsigned int defaultValue, const char* libName) {
    DEBUG_PRINT(5, "[LUABIND] getValue(unsigned int): %s.%s\n", libName, varName);
    return getValue(varName, (int) defaultValue, libName);
}

double LuaBinding::getValue(const char* varName, const double defaultValue, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] getValue(double): %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    int r = defaultValue;
    int pushedCount = reachValue(varName, libName);
    if (lua_isnumber(L, -1)) r = lua_tonumber(L, -1);
    lua_pop(L, pushedCount);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

int LuaBinding::getValueType(const char* varName, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] getValueType: %s.%s\n", libName, varName);
    pthread_mutex_lock(&luaStateMutex);
    int pushedCount = reachValue(varName, libName);
    int r = lua_type(L, -1);
    lua_pop(L, pushedCount);
    pthread_mutex_unlock(&luaStateMutex);
    return r;
}

void LuaBinding::setValue(const char* varName, const int value, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] setValue(int): %s.%s = %d\n", libName, varName, value);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    if (lua_istable(L, -1)) {
        lua_pushinteger(L, value);
        lua_setfield(L, -2, varName); //it will pop value
    }
    lua_pop(L, 1);
    pthread_mutex_unlock(&luaStateMutex);
}

void LuaBinding::setValue(const char* varName, const char value[], const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] setValue(string): %s.%s = '%s'\n", libName, varName, value);
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    if (lua_istable(L, -1)) {
        lua_pushstring(L, value);
        lua_setfield(L, -2, varName); //it will pop value
    }
    lua_pop(L, 1);
    pthread_mutex_unlock(&luaStateMutex);
}

void LuaBinding::setValue(const char* varName, const bool value, const char* libName) {
    DEBUG_PRINT(4, "[LUABIND] setValue(boolean): %s.%s = %s\n", libName, varName, value ? "true" : "false");
    pthread_mutex_lock(&luaStateMutex);
    lua_checkstack(L, 2);
    lua_getglobal(L, libName);
    if (lua_istable(L, -1)) {
        lua_pushboolean(L, value);
        lua_setfield(L, -2, varName); //it will pop value
    }
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

LuaBinding* LuaBinding::staticLuaBinding = NULL;

LuaBinding& LuaBinding::getStaticBinding() {
    return *staticLuaBinding;
}
