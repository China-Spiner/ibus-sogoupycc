/* 
 * File:   engine.h
 * Author: Jun WU <quark@lihdd.com>
 *
 * GNUv2 Licence
 *
 * Created on November 3, 2009
 */

#ifndef _ENGINE_H
#define	_ENGINE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <ibus.h>

#define IBUS_TYPE_SGPYCC_ENGINE	(ibus_sgpycc_engine_get_type ())

GType   ibus_sgpycc_engine_get_type    (void);


#ifdef	__cplusplus
}
#endif

#endif	/* _ENGINE_H */

