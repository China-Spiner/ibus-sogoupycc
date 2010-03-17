/* 
 * File:   engine.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * defines and exported lua C functions of engine.
 */

#ifndef _ENGINE_H
#define _ENGINE_H

#include <ibus.h>
#include "LuaBinding.h"

#define IBUS_TYPE_SGPYCC_ENGINE (ibusSgpyccEngineGetType())

GType ibusSgpyccEngineGetType(void);

namespace Engine {
    // lua C functions
    extern int l_commitText(lua_State* L);
    extern int l_sendRequest(lua_State* L);
}

#endif  /* _ENGINE_H */
