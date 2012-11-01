// quakedef.h -- primary header for client

/*
 * $Header: /H2 Mission Pack/Quakedef.h 8     3/19/98 12:53p Jmonroe $
 */

#include "../common/qcommon.h"

#define HEXEN2_VERSION      1.12

//define	PARANOID			// speed sapping error checking

#if id386
#define UNALIGNED_OK    1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK    0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE  32		// used to align key data structures

#define UNUSED(x)   (x = x)	// for pesky compiler / lint warnings

#define SOUND_CHANNELS      8

#include "common.h"

//
// host
//
extern Cvar* sys_ticrate;

extern double host_frametime;
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void Host_Frame(float time);

extern qboolean check_bottom;

extern double host_time;
