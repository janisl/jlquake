/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// quakedef.h -- primary header for client

#include "../common/qcommon.h"
#ifndef DEDICATED
#include "../client/client.h"
#include "../client/game/quake/local.h"
#include "../client/game/quake_hexen2/network_channel.h"
#else
#include "../client/public.h"
#endif
#include "../server/server.h"
#include "../server/quake_hexen/local.h"
#include "../server/quake/local.h"
#include "../server/progsvm/progsvm.h"

#define VERSION             1.09
#define GLQUAKE_VERSION     1.00
#define D3DQUAKE_VERSION    0.01
#define WINQUAKE_VERSION    0.996
#define LINUX_VERSION       1.30
#define X11_VERSION         1.10

//define	PARANOID			// speed sapping error checking

#define GAMENAME    "id1"		// directory to look in by default

#include <setjmp.h>

#if id386
#define UNALIGNED_OK    1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK    0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE  32		// used to align key data structures

#define UNUSED(x)   (x = x)	// for pesky compiler / lint warnings

//===========================================

#define SOUND_CHANNELS      8

#include "common.h"
#include "vid.h"
#include "sys.h"
#ifndef DEDICATED
#include "draw.h"
#include "screen.h"
#endif
#include "net.h"
#include "cmd.h"
#ifndef DEDICATED
#include "render.h"
#include "client.h"
#include "keys.h"
#endif
#include "console.h"
#ifndef DEDICATED
#include "view.h"
#include "menu.h"
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
void Com_Quit_f(void);

extern qboolean msg_suppress_1;			// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss

extern qboolean isDedicated;

extern double host_time;
