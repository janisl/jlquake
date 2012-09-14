// quakedef.h -- primary header for client

/*
 * $Header: /H2 Mission Pack/Quakedef.h 8     3/19/98 12:53p Jmonroe $
 */

#include "../common/qcommon.h"
#ifndef DEDICATED
#include "../client/client.h"
#include "../client/game/hexen2/local.h"
#include "../client/game/quake_hexen2/network_channel.h"
#include "../client/game/quake_hexen2/menu.h"
#else
#include "../client/public.h"
#endif
#include "../server/server.h"
#include "../server/quake_hexen/local.h"
#include "../server/hexen2/local.h"
#include "../server/progsvm/progsvm.h"
#include "../common/hexen2strings.h"

//#define MISSIONPACK

#define HEXEN2_VERSION      1.12

//define	PARANOID			// speed sapping error checking

#define GAMENAME    "data1"		// directory to look in by default

#include <setjmp.h>

#if id386
#define UNALIGNED_OK    1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK    0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE  32		// used to align key data structures

#define UNUSED(x)   (x = x)	// for pesky compiler / lint warnings

#ifdef MISSIONPACK
#define NUM_CLASSES                 NUM_CLASSES_H2MP
#else
#define NUM_CLASSES                 NUM_CLASSES_H2
#endif
#define ABILITIES_STR_INDEX         400

#define SOUND_CHANNELS      8

#include "common.h"
#include "vid.h"
#include "sys.h"

#ifndef DEDICATED
#include "draw.h"
#include "screen.h"
#endif
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#ifndef DEDICATED
#include "render.h"
#endif
#include "client.h"
#ifndef DEDICATED
#include "keys.h"
#endif
#include "console.h"
#ifndef DEDICATED
#include "view.h"
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	const char* basedir;
	int argc;
	char** argv;
} quakeparms_t;


//=============================================================================



//
// host
//
extern quakeparms_t host_parms;

extern Cvar* sys_ticrate;

extern qboolean host_initialized;			// true if into command execution
extern double host_frametime;
extern int host_framecount;				// incremented every frame, never reset
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void Host_InitCommands(void);
void Host_Init(quakeparms_t* parms);
void Host_Shutdown(void);
void Host_Error(const char* error, ...);
void Host_EndGame(const char* message, ...);
void Host_Frame(float time);
void Com_Quit_f();

extern qboolean msg_suppress_1;			// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss

extern qboolean isDedicated;

extern int num_intro_msg;
extern qboolean check_bottom;

extern double host_time;
