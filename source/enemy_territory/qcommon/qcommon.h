/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

/*
==============================================================

PROTOCOL

==============================================================
*/

// long version used by the client in diagnostic window
#define PROTOCOL_MISMATCH_ERROR_LONG "ERROR: Protocol Mismatch Between Client and Server.\n\n\
The server you attempted to join is running an incompatible version of the game.\n\
You or the server may be running older versions of the game. Press the auto-update\
 button if it appears on the Main Menu screen."

#define GAMENAME_STRING "et"

// NERVE - SMF - wolf multiplayer master servers
#ifndef MASTER_SERVER_NAME
	#define MASTER_SERVER_NAME      "etmaster.idsoftware.com"
#endif
#define MOTD_SERVER_NAME        "etmaster.idsoftware.com"	//"etmotd.idsoftware.com"			// ?.?.?.?

// TTimo: override autoupdate server for testing
#ifndef AUTOUPDATE_SERVER_NAME
//	#define AUTOUPDATE_SERVER_NAME "127.0.0.1"
	#define AUTOUPDATE_SERVER_NAME "au2rtcw2.activision.com"
#endif

// TTimo: allow override for easy dev/testing..
// FIXME: not planning to support more than 1 auto update server
// see cons -- update_server=myhost
#if !defined(AUTOUPDATE_SERVER_NAME)
  #define AUTOUPDATE_SERVER1_NAME   "au2rtcw1.activision.com"			// DHM - Nerve
  #define AUTOUPDATE_SERVER2_NAME   "au2rtcw2.activision.com"			// DHM - Nerve
  #define AUTOUPDATE_SERVER3_NAME   "au2rtcw3.activision.com"			// DHM - Nerve
  #define AUTOUPDATE_SERVER4_NAME   "au2rtcw4.activision.com"			// DHM - Nerve
  #define AUTOUPDATE_SERVER5_NAME   "au2rtcw5.activision.com"			// DHM - Nerve
#else
  #define AUTOUPDATE_SERVER1_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER2_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER3_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER4_NAME   AUTOUPDATE_SERVER_NAME
  #define AUTOUPDATE_SERVER5_NAME   AUTOUPDATE_SERVER_NAME
#endif

#define PORT_MASTER         27950
#define PORT_MOTD           27951
#define NUM_SERVER_PORTS    4		// broadcast scan this many ports after
									// Q3PORT_SERVER so a single machine can
									// run multiple servers

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

#define BASEGAME "etmain"

void    FS_InitFilesystem(void);

qboolean    FS_ConditionalRestart(int checksumFeed);
void    FS_Restart(int checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

qboolean FS_OS_FileExists(const char* file);	// TTimo - test file existence given OS path

int     FS_LoadStack();

#if defined(DO_LIGHT_DEDICATED)
int FS_RandChecksumFeed();
#endif

char* FS_ShiftStr(const char* string, int shift);

unsigned int FS_ChecksumOSPath(char* OSPath);

/*
==============================================================

DOWNLOAD

==============================================================
*/

#include "dl_public.h"

/*
==============================================================

MISC

==============================================================
*/

// centralizing the declarations for cl_cdkey
// (old code causing buffer overflows)
extern char cl_cdkey[34];
void Com_AppendCDKey(const char* filename);
void Com_ReadCDKey(const char* filename);

// returnbed by Sys_GetProcessorId
#define CPUID_GENERIC           0			// any unrecognized processor

#define CPUID_AXP               0x10

#define CPUID_INTEL_UNSUPPORTED 0x20			// Intel 386/486
#define CPUID_INTEL_PENTIUM     0x21			// Intel Pentium or PPro
#define CPUID_INTEL_MMX         0x22			// Intel Pentium/MMX or P2/MMX
#define CPUID_INTEL_KATMAI      0x23			// Intel Katmai

#define CPUID_AMD_3DNOW         0x30			// AMD K6 3DNOW!

char* CopyString(const char* in);

int QDECL Com_VPrintf(const char* fmt, va_list argptr) id_attribute((format(printf,1,0)));			// conforms to vprintf prototype for print callback passing
void QDECL Com_Printf(const char* fmt, ...) id_attribute((format(printf,1,2)));			// this one calls to Com_VPrintf now
void QDECL Com_DPrintf(const char* fmt, ...) id_attribute((format(printf,1,2)));
void QDECL Com_Error(int code, const char* fmt, ...) id_attribute((format(printf,2,3)));
void        Com_Quit_f(void);
int         Com_EventLoop(void);
int         Com_Milliseconds(void);		// will be journaled properly
qboolean    Com_SafeMode(void);

void        Com_StartupVariable(const char* match);
void        Com_SetRecommended();
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.

//bani - profile functions
void Com_TrackProfile(char* profile_path);
qboolean Com_CheckProfile(char* profile_path);
qboolean Com_WriteProfile(char* profile_path);

extern Cvar* com_ignorecrash;		//bani

extern Cvar* com_pid;		//bani

extern Cvar* com_speeds;
extern Cvar* com_version;
//extern	Cvar	*com_blood;
extern Cvar* com_buildScript;			// for building release pak files
extern Cvar* com_cameraMode;
extern Cvar* com_logosPlaying;

// watchdog
extern Cvar* com_watchdog;
extern Cvar* com_watchdog_cmd;

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

void* Z_TagMalloc(int size, int tag);	// NOT 0 filled memory
void* Z_Malloc(int size);			// returns 0 filled memory
void* S_Malloc(int size);			// NOT 0 filled memory only for small allocations
void Z_Free(void* ptr);
void Z_LogHeap(void);

void Hunk_Clear(void);
void Hunk_ClearToMark(void);
void Hunk_SetMark(void);
int Hunk_MemoryRemaining(void);

void Com_TouchMemory(void);

// commandLine should not include the executable name (argv[0])
void Com_Init(char* commandLine);
void Com_Frame(void);
void Com_Shutdown(qboolean badProfile);


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
void CL_ClearStaticDownload(void);
void CL_Disconnect(qboolean showMainMenu);
void CL_Shutdown(void);
void CL_Frame(int msec);
qboolean CL_GameCommand(void);
void CL_KeyEvent(int key, qboolean down, unsigned time);

void CL_CharEvent(int key);
// char events are for field typing, not game control

void CL_MouseEvent(int dx, int dy, int time);

void CL_PacketEvent(netadr_t from, QMsg* msg);

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

void CL_ShutdownAll(void);
// shutdown all the client stuff

void CL_FlushMemory(void);
// dump all memory on an error

void CL_StartHunkUsers(void);
// start all the client stuff using the hunk

void CL_CheckAutoUpdate(void);
qboolean CL_NextUpdateServer(void);
void CL_GetAutoUpdate(void);

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
qboolean Sys_IsNumLockDown(void);

const char* Sys_GetCurrentUser(void);

void QDECL Sys_Error(const char* error, ...);
void    Sys_Quit(void);

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole(qboolean show);

int     Sys_GetProcessorId(void);

void    Sys_SetErrorText(const char* text);

void    Sys_BeginProfiling(void);
void    Sys_EndProfiling(void);

unsigned int Sys_ProcessorCount();

// NOTE TTimo - on win32 the cwd is prepended .. non portable behaviour
void Sys_StartProcess(const char* exeName, qboolean doexit);				// NERVE - SMF
void Sys_OpenURL(const char* url, qboolean doexit);							// NERVE - SMF
int Sys_GetHighQualityCPU();
float Sys_GetCPUSpeed(void);

#ifdef __linux__
// TTimo only on linux .. maybe on Mac too?
// will OR with the existing mode (chmod ..+..)
void Sys_Chmod(char* file, int mode);
#endif

extern huffman_t clientHuffTables;

void Com_GetHunkInfo(int* hunkused, int* hunkexpected);

#endif	// _QCOMMON_H_
