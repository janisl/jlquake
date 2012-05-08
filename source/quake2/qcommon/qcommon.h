/*
Copyright (C) 1997-2001 Id Software, Inc.

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

// qcommon.h -- definitions common between client and server, but not game.dll

#ifndef _qcommon_h
#define _qcommon_h

#include "q_shared.h"


#define VERSION     3.19

#define BASEDIRNAME "baseq2"

#ifdef WIN32

#ifdef NDEBUG
#define BUILDSTRING "Win32 RELEASE"
#else
#define BUILDSTRING "Win32 DEBUG"
#endif

#ifdef _M_IX86
#define CPUSTRING   "x86"
#elif defined _M_ALPHA
#define CPUSTRING   "AXP"
#elif defined _M_X64
#define CPUSTRING   "x64"
#endif

#elif defined __linux__

#define BUILDSTRING "Linux"

#ifdef __i386__
#define CPUSTRING "i386"
#elif defined __alpha__
#define CPUSTRING "axp"
#else
#define CPUSTRING "Unknown"
#endif

#elif defined __sun__

#define BUILDSTRING "Solaris"

#ifdef __i386__
#define CPUSTRING "i386"
#else
#define CPUSTRING "sparc"
#endif

#else	// !WIN32

#define BUILDSTRING "NON-WIN32"
#define CPUSTRING   "NON-WIN32"

#endif

void COM_Init(void);

char* CopyString(char* in);

/*
==============================================================

PROTOCOL

==============================================================
*/

// protocol.h -- communications protocols

#define PROTOCOL_VERSION    34

//=========================================

#define PORT_MASTER 27900
#define PORT_CLIENT 27901
#define PORT_SERVER 27910

//==============================================

// plyer_state_t communication

#define PS_M_TYPE           (1 << 0)
#define PS_M_ORIGIN         (1 << 1)
#define PS_M_VELOCITY       (1 << 2)
#define PS_M_TIME           (1 << 3)
#define PS_M_FLAGS          (1 << 4)
#define PS_M_GRAVITY        (1 << 5)
#define PS_M_DELTA_ANGLES   (1 << 6)

#define PS_VIEWOFFSET       (1 << 7)
#define PS_VIEWANGLES       (1 << 8)
#define PS_KICKANGLES       (1 << 9)
#define PS_BLEND            (1 << 10)
#define PS_FOV              (1 << 11)
#define PS_WEAPONINDEX      (1 << 12)
#define PS_WEAPONFRAME      (1 << 13)
#define PS_RDFLAGS          (1 << 14)

//==============================================

// a sound without an ent or pos will be a local only sound
#define SND_VOLUME      (1 << 0)		// a byte
#define SND_ATTENUATION (1 << 1)		// a byte
#define SND_POS         (1 << 2)		// three coordinates
#define SND_ENT         (1 << 3)		// a short 0-2: channel, 3-12: entity
#define SND_OFFSET      (1 << 4)		// a byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME 1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//===========================================================================

void    Cmd_Init(void);

void    Cmd_ForwardToServer(void);
// adds the current command line as a q2clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.


/*
==============================================================

CVAR

==============================================================
*/

void    Cvar_GetLatchedVars(void);
// any CVAR_LATCHED variables that have been set will now take effect

/*
==============================================================

NET

==============================================================
*/

// net.h -- quake's interface to the networking layer

#define PACKET_HEADER   10			// two ints and a short

void        NET_Init(void);
void        NET_Shutdown(void);

void        NET_Config(qboolean multiplayer);

qboolean    NET_GetPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
void        NET_SendPacket(netsrc_t sock, int length, void* data, netadr_t to);

void        NET_Sleep(int msec);

//============================================================================

#define OLD_AVG     0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define MAX_LATENT  32

extern netadr_t net_from;
extern QMsg net_message;
extern byte net_message_buffer[MAX_MSGLEN_Q2];


void Netchan_Init(void);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int qport);

qboolean Netchan_NeedReliable(netchan_t* chan);
void Netchan_Transmit(netchan_t* chan, int length, byte* data);
void Netchan_OutOfBand(int net_socket, netadr_t adr, int length, byte* data);
void Netchan_OutOfBandPrint(int net_socket, netadr_t adr, const char* format, ...);
qboolean Netchan_Process(netchan_t* chan, QMsg* msg);

qboolean Netchan_CanReliable(netchan_t* chan);


/*
==============================================================

PLAYER MOVEMENT CODE

Common between server and client so prediction matches

==============================================================
*/

extern float pm_airaccelerate;

void Pmove(pmove_t* pmove);

/*
==============================================================

FILESYSTEM

==============================================================
*/

void    FS_InitFilesystem(void);
void    FS_SetGamedir(char* dir);
char* FS_Gamedir(void);
void    FS_ExecAutoexec(void);


/*
==============================================================

MISC

==============================================================
*/


#define ERR_FATAL   0		// exit the entire game with a popup window
#define ERR_DROP    1		// print to console and disconnect from game
#define ERR_QUIT    2		// not an error, just a normal exit

#define EXEC_NOW    0		// don't return until completed
#define EXEC_INSERT 1		// insert at current position, but don't run yet
#define EXEC_APPEND 2		// add to end of the command buffer

void        Com_BeginRedirect(int target, char* buffer, int buffersize, void (*flush));
void        Com_EndRedirect(void);
void        Com_Printf(const char* fmt, ...);
void        Com_DPrintf(const char* fmt, ...);
void        Com_Error(int code, const char* fmt, ...);
void        Com_Quit(void);

int         Com_ServerState(void);		// this should have just been a cvar...
void        Com_SetServerState(int state);

byte        COM_BlockSequenceCRCByte(byte* base, int length, int sequence);

extern Cvar* host_speeds;
extern Cvar* log_stats;

extern fileHandle_t log_stats_file;

// host_speeds times
extern int time_before_game;
extern int time_after_game;
extern int time_before_ref;
extern int time_after_ref;

void Z_Free(void* ptr);
void* Z_Malloc(int size);			// returns 0 filled memory
void* Z_TagMalloc(int size, int tag);
void Z_FreeTags(int tag);

void Qcommon_Init(int argc, char** argv);
void Qcommon_Frame(int msec);
void Qcommon_Shutdown(void);

// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph(float value, int color);


/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

void    Sys_Init(void);

void    Sys_AppActivate(void);

void    Sys_Error(const char* error, ...);
void    Sys_Quit(void);

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

void CL_Init(void);
void CL_Drop(void);
void CL_Shutdown(void);
void CL_Frame(int msec);
void SCR_BeginLoadingPlaque(bool Clear = false);

void SV_Init(void);
void SV_Shutdown(const char* finalmsg, qboolean reconnect);
void SV_Frame(int msec);



#endif
