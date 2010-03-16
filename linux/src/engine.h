/* 
 * File:   engine.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * March 12, 2010
 *  use global static luaBinding. All settings go global
 * March 9, 2010
 *  0.1.3 separate lua global / session init
 * March 2, 2010
 *  0.1.2
 * February 28, 2010
 *  0.1.1 major bugs fixed
 * February 27, 2010
 *  0.1.0 first release
 * November 3, 2009
 *  created
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
