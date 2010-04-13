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

namespace ImeEngine {
    // lua C functions
    extern void registerLuaFunctions();
}

#endif  /* _ENGINE_H */
