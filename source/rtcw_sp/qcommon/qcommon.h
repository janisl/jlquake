/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

//============================================================================

void MSG_Init(QMsg* buf, byte* data, int length);
void MSG_InitOOB(QMsg* buf, byte* data, int length);
void* MSG_GetSpace(QMsg* buf, int length);


struct wsentityState_t;
struct wsplayerState_t;

void MSG_WriteDeltaUsercmd(QMsg* msg, struct wsusercmd_t* from, struct wsusercmd_t* to);
void MSG_ReadDeltaUsercmd(QMsg* msg, struct wsusercmd_t* from, struct wsusercmd_t* to);

void MSG_WriteDeltaUsercmdKey(QMsg* msg, int key, wsusercmd_t* from, wsusercmd_t* to);
void MSG_ReadDeltaUsercmdKey(QMsg* msg, int key, wsusercmd_t* from, wsusercmd_t* to);

void MSG_WriteDeltaEntity(QMsg* msg, struct wsentityState_t* from, struct wsentityState_t* to,
	qboolean force);
void MSG_ReadDeltaEntity(QMsg* msg, wsentityState_t* from, wsentityState_t* to,
	int number);

void MSG_WriteDeltaPlayerstate(QMsg* msg, struct wsplayerState_t* from, struct wsplayerState_t* to);
void MSG_ReadDeltaPlayerstate(QMsg* msg, struct wsplayerState_t* from, struct wsplayerState_t* to);


void MSG_ReportChangeVectors_f(void);

/*
==============================================================

NET

==============================================================
*/

// TTimo: set to 1 to perform net encoding, 0 to drop
// single player game with no networking doesn't need encoding
// show_bug.cgi?id=404
#define DO_NET_ENCODE 0

#define MAX_PACKET_USERCMDS     32		// max number of wsusercmd_t in a packet

void        NET_Init(void);
void        NET_Shutdown(void);
void        NET_Restart(void);

void        NET_SendPacket(netsrc_t sock, int length, const void* data, netadr_t to);
void QDECL NET_OutOfBandPrint(netsrc_t net_socket, netadr_t adr, const char* format, ...);

qboolean    NET_GetLoopPacket(netsrc_t sock, netadr_t* net_from, QMsg* net_message);
void        NET_Sleep(int msec);

qboolean    Sys_GetPacket(netadr_t* net_from, QMsg* net_message);

// be fragmented into multiple packets
#define MAX_DOWNLOAD_WINDOW         8		// max of eight download frames
#define MAX_DOWNLOAD_BLKSIZE        2048	// 2048 byte block chunks


/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

void Netchan_Init(int qport);
void Netchan_Setup(netsrc_t sock, netchan_t* chan, netadr_t adr, int qport);

void Netchan_Transmit(netchan_t* chan, int length, const byte* data);
void Netchan_TransmitNextFragment(netchan_t* chan);

qboolean Netchan_Process(netchan_t* chan, QMsg* msg);




/*
==============================================================

PROTOCOL

==============================================================
*/

#define PROTOCOL_VERSION    49	// TA value
//#define	PROTOCOL_VERSION	43	// (SA) bump this up


//----(SA)	heh, whoops.  we've been talking to id servers since we got a connection...
//#define	UPDATE_SERVER_NAME	"update.quake3arena.com"
//#define MASTER_SERVER_NAME	"master.quake3arena.com"
//#define	AUTHORIZE_SERVER_NAME	"authorize.quake3arena.com"
//----(SA)	yes, these are bogus addresses.  I'm guessing these will be set to a machine at Activision or id eventually
#define UPDATE_SERVER_NAME      "update.gmistudios.com"
#define MASTER_SERVER_NAME      "master.gmistudios.com"
#define AUTHORIZE_SERVER_NAME   "authorize.gmistudios.com"

#define PORT_MASTER         27950
#define PORT_UPDATE         27951
#define PORT_AUTHORIZE      27952
#define PORT_SERVER         27960
#define NUM_SERVER_PORTS    4		// broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/

typedef enum {
	VMI_NATIVE,
	VMI_BYTECODE,
	VMI_COMPILED
} vmInterpret_t;

typedef enum {
	TRAP_MEMSET = 100,
	TRAP_MEMCPY,
	TRAP_STRNCPY,
	TRAP_SIN,
	TRAP_COS,
	TRAP_ATAN2,
	TRAP_SQRT,
	TRAP_MATRIXMULTIPLY,
	TRAP_ANGLEVECTORS,
	TRAP_PERPENDICULARVECTOR,
	TRAP_FLOOR,
	TRAP_CEIL,

	TRAP_TESTPRINTINT,
	TRAP_TESTPRINTFLOAT
} sharedTraps_t;

void    VM_Init(void);
vm_t* VM_Create(const char* module, qintptr (* systemCalls)(qintptr*),
	vmInterpret_t interpret);
// module should be bare: "cgame", not "cgame.dll" or "vm/cgame.qvm"

void    VM_Free(vm_t* vm);
void    VM_Clear(void);
vm_t* VM_Restart(vm_t* vm);

qintptr QDECL VM_Call(vm_t* vm, int callNum, ...);

/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

void    Cmd_Init(void);

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

#define BASEGAME "main"

void    FS_InitFilesystem(void);

qboolean    FS_ConditionalRestart(int checksumFeed);
void    FS_Restart(int checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

int     FS_LoadStack();

void    FS_CopyFileOS(char* from, char* to);	//DAJ

/*
==============================================================

MISC

==============================================================
*/

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0			// any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20			// Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21			// Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22			// Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23			// Intel Katmai

#define CPUID_AMD_3DNOW         0x30			// AMD K6 3DNOW!


char* CopyString(const char* in);

void Com_BeginRedirect(char* buffer, int buffersize, void (* flush)(char*));
void        Com_EndRedirect(void);
void QDECL Com_Printf(const char* fmt, ...);
void QDECL Com_DPrintf(const char* fmt, ...);
void QDECL Com_Error(int code, const char* fmt, ...);
void        Com_Quit_f(void);
int         Com_EventLoop(void);
int         Com_Milliseconds(void);		// will be journaled properly
int         Com_RealTime(qtime_t* qtime);
qboolean    Com_SafeMode(void);

void        Com_StartupVariable(const char* match);
void        Com_SetRecommended(qboolean vid_restart);
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.


extern Cvar* com_speeds;
extern Cvar* com_sv_running;
extern Cvar* com_cl_running;
extern Cvar* com_version;
extern Cvar* com_blood;
extern Cvar* com_buildScript;			// for building release pak files
extern Cvar* com_cameraMode;

// both client and server must agree to pause
extern Cvar* cl_paused;
extern Cvar* sv_paused;

// com_speeds times
extern int time_game;
extern int time_frontend;
extern int time_backend;			// renderer backend time

extern int com_frameMsec;

extern qboolean com_errorEntered;

typedef enum {
	TAG_FREE,
	TAG_GENERAL,
	TAG_BOTLIB,
	TAG_RENDERER,
	TAG_SMALL,
	TAG_STATIC
} memtag_t;

/*

--- low memory ----
server vm
server clipmap
---mark---
renderer initialization (shaders, etc)
UI vm
cgame vm
renderer map
renderer models

---free---

temp file loading
--- high memory ---

*/

#if defined(_DEBUG) && !defined(BSPC)
	#define ZONE_DEBUG
#endif

void* Z_TagMalloc(int size, int tag);	// NOT 0 filled memory
void* Z_Malloc(int size);			// returns 0 filled memory
void Z_Free(void* ptr);
void Z_FreeTags(int tag);

void Hunk_Clear(void);
void Hunk_ClearToMark(void);
void Hunk_SetMark(void);
qboolean Hunk_CheckMark(void);
//void *Hunk_Alloc( int size );
// void *Hunk_Alloc( int size, ha_pref preference );
void Hunk_ClearTempMemory(void);
void* Hunk_AllocateTempMemory(int size);
void Hunk_FreeTempMemory(void* buf);
int Hunk_MemoryRemaining(void);
void Hunk_SmallLog(void);
void Hunk_Log(void);

void Com_TouchMemory(void);

// commandLine should not include the executable name (argv[0])
void Com_Init(char* commandLine);
void Com_Frame(void);
void Com_Shutdown(void);


/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

//
// client interface
//
void CL_InitKeyCommands(void);
// the keyboard binding interface must be setup before execing
// config files, but the rest of client startup will happen later

void CL_Init(void);
void CL_Disconnect(qboolean showMainMenu);
void CL_Shutdown(void);
void CL_Frame(int msec);
qboolean CL_GameCommand(void);
void CL_KeyEvent(int key, qboolean down, unsigned time);

void CL_CharEvent(int key);
// char events are for field typing, not game control

void CL_MouseEvent(int dx, int dy, int time);

void CL_PacketEvent(netadr_t from, QMsg* msg);

void CL_ConsolePrint(char* text);

void CL_MapLoading(void);
// do a screen update before starting to load a map
// when the server is going to load a new map, the entire hunk
// will be cleared, so the client must shutdown cgame, ui, and
// the renderer

void    CL_ForwardCommandToServer(const char* string);
// adds the current command line as a q3clc_clientCommand to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.


void CL_CDDialog(void);
// bring up the "need a cd to play" dialog

//----(SA)	added
void CL_EndgameMenu(void);
// bring up the "need a cd to play" dialog
//----(SA)	end

void CL_ShutdownAll(void);
// shutdown all the client stuff

void CL_FlushMemory(void);
// dump all memory on an error

void CL_StartHunkUsers(void);
// start all the client stuff using the hunk

void Key_WriteBindings(fileHandle_t f);
// for writing the config files

void SCR_DebugGraph(float value, int color);	// FIXME: move logging to common?


//
// server interface
//
void SV_Init(void);
void SV_Shutdown(const char* finalmsg);
void SV_Frame(int msec);
void SV_PacketEvent(netadr_t from, QMsg* msg);
qboolean SV_GameCommand(void);


//
// UI interface
//
qboolean UI_GameCommand(void);
qboolean UI_usesUniqueCDKey();

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

sysEvent_t  Sys_GetEvent(void);

void    Sys_Init(void);

// general development dll loading for virtual machine testing
void* QDECL Sys_LoadDll(const char* name, qintptr(QDECL * *entryPoint) (int, ...),
	qintptr (QDECL* systemcalls)(int, ...));
void    Sys_UnloadDll(void* dllHandle);

void    Sys_UnloadGame(void);
void* Sys_GetGameAPI(void* parms);

void    Sys_UnloadCGame(void);
void* Sys_GetCGameAPI(void);

void    Sys_UnloadUI(void);
void* Sys_GetUIAPI(void);

//bot libraries
void    Sys_UnloadBotLib(void);
void* Sys_GetBotLibAPI(void* parms);

const char* Sys_GetCurrentUser(void);

void QDECL Sys_Error(const char* error, ...);
void    Sys_Quit(void);

void    Sys_SnapVector(float* v);

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole(qboolean show);

int     Sys_GetProcessorId(void);

void    Sys_SetErrorText(const char* text);

qboolean    Sys_CheckCD(void);

void    Sys_BeginProfiling(void);
void    Sys_EndProfiling(void);

unsigned int Sys_ProcessorCount();

void Sys_StartProcess(const char* exeName, qboolean doexit);				// NERVE - SMF
// TTimo
// show_bug.cgi?id=447
//int Sys_ShellExecute(char *op, char *file, qboolean doexit, char *params, char *dir);	//----(SA) added
void Sys_OpenURL(char* url, qboolean doexit);						// NERVE - SMF
int Sys_GetHighQualityCPU();

extern huffman_t clientHuffTables;

#define SV_ENCODE_START     4
#define SV_DECODE_START     12
#define CL_ENCODE_START     12
#define CL_DECODE_START     4

#endif	// _QCOMMON_H_
