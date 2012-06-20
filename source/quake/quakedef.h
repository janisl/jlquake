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

#include "../client/client.h"
#include "../server/server.h"
#include "../client/game/quake/local.h"
#include "../server/quake_hexen/local.h"
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

#define MINIMUM_MEMORY          0x550000
#define MINIMUM_MEMORY_LEVELPAK (MINIMUM_MEMORY + 0x100000)


#define ON_EPSILON      0.1			// point on plane side epsilon

#define SAVEGAME_COMMENT_LENGTH 39

// stock defines

#define IT_SHOTGUN              1
#define IT_SUPER_SHOTGUN        2
#define IT_NAILGUN              4
#define IT_SUPER_NAILGUN        8
#define IT_GRENADE_LAUNCHER     16
#define IT_ROCKET_LAUNCHER      32
#define IT_LIGHTNING            64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define IT_INVISIBILITY         524288
#define IT_INVULNERABILITY      1048576
#define IT_SUIT                 2097152
#define IT_QUAD                 4194304
#define IT_SIGIL1               (1 << 28)
#define IT_SIGIL2               (1 << 29)
#define IT_SIGIL3               (1 << 30)
#define IT_SIGIL4               (1 << 31)

//===========================================
//rogue changed and added defines

#define RIT_SHELLS              128
#define RIT_NAILS               256
#define RIT_ROCKETS             512
#define RIT_CELLS               1024
#define RIT_AXE                 2048
#define RIT_LAVA_NAILGUN        4096
#define RIT_LAVA_SUPER_NAILGUN  8192
#define RIT_MULTI_GRENADE       16384
#define RIT_MULTI_ROCKET        32768
#define RIT_PLASMA_GUN          65536
#define RIT_ARMOR1              8388608
#define RIT_ARMOR2              16777216
#define RIT_ARMOR3              33554432
#define RIT_LAVA_NAILS          67108864
#define RIT_PLASMA_AMMO         134217728
#define RIT_MULTI_ROCKETS       268435456
#define RIT_SHIELD              536870912
#define RIT_ANTIGRAV            1073741824
#define RIT_SUPERHEALTH         2147483648

//MED 01/04/97 added hipnotic defines
//===========================================
//hipnotic added defines
#define HIT_PROXIMITY_GUN_BIT 16
#define HIT_MJOLNIR_BIT       7
#define HIT_LASER_CANNON_BIT  23
#define HIT_PROXIMITY_GUN   (1 << HIT_PROXIMITY_GUN_BIT)
#define HIT_MJOLNIR         (1 << HIT_MJOLNIR_BIT)
#define HIT_LASER_CANNON    (1 << HIT_LASER_CANNON_BIT)
#define HIT_WETSUIT         (1 << (23 + 2))
#define HIT_EMPATHY_SHIELDS (1 << (23 + 3))

//===========================================

#define SOUND_CHANNELS      8

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#include "common.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "draw.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "render.h"
#include "client.h"
#include "progs.h"
#include "server.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	const char* basedir;
	int argc;
	char** argv;
	void* membase;
	int memsize;
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

void Host_ClearMemory(void);
void Host_ServerFrame(void);
void Host_InitCommands(void);
void Host_Init(quakeparms_t* parms);
void Host_Shutdown(void);
void Host_Error(const char* error, ...);
void Host_EndGame(const char* message, ...);
void Host_Frame(float time);
void Host_Quit_f(void);
void Host_ClientCommands(const char* fmt, ...);
void Host_ShutdownServer(qboolean crash);

extern qboolean msg_suppress_1;			// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss

extern qboolean isDedicated;

extern int minimum_memory;
