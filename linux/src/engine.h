/* 
 * File:   engine.h
 * Author: WU Jun <quark@lihdd.net>
 *
 * November 3, 2009
 *  created
 */

#ifndef _ENGINE_H
#define _ENGINE_H

extern "C" {

#include <ibus.h>

#define IBUS_TYPE_SGPYCC_ENGINE (ibusSgpyccEngineGetType())

GType ibusSgpyccEngineGetType(void);

}

#endif  /* _ENGINE_H */
