// quakedef.h -- primary header for client

#include "../../common/qcommon.h"

//define	PARANOID			// speed sapping error checking

#include "bothdefs.h"

#include "common.h"

//
// host
//
extern double host_frametime;
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset
