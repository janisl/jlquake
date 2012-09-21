/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

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

// NERVE - SMF - wolf multiplayer master servers
#define UPDATE_SERVER_NAME      "wolfmotd.idsoftware.com"			// 192.246.40.65

#define PORT_UPDATE         27951
#define Q3PORT_AUTHORIZE      27952

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

/*
==============================================================

MISC

==============================================================
*/

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

void QDECL Com_Printf(const char* fmt, ...);
void QDECL Com_DPrintf(const char* fmt, ...);
void QDECL Com_Error(int code, const char* fmt, ...);
void        Com_Quit_f(void);
int         Com_EventLoop(void);
int         Com_Milliseconds(void);		// will be journaled properly
qboolean    Com_SafeMode(void);

void        Com_StartupVariable(const char* match);
void        Com_SetRecommended();
// checks for and removes command line "+set var arg" constructs
// if match is NULL, all set commands will be executed, otherwise
// only a set with the exact name.  Only used during startup.


extern Cvar* com_version;
extern Cvar* com_blood;
extern Cvar* com_buildScript;			// for building release pak files
extern Cvar* com_cameraMode;

// com_speeds times
extern int time_frontend;
extern int time_backend;			// renderer backend time

extern int com_frameMsec;

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

void CL_Init(void);
void CL_Disconnect(qboolean showMainMenu);
void CL_Shutdown(void);
void CL_Frame(int msec);
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

void CL_ShutdownAll(void);
// shutdown all the client stuff

void CL_FlushMemory(void);
// dump all memory on an error

void CL_StartHunkUsers(void);
// start all the client stuff using the hunk

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

sysEvent_t  Sys_GetEvent(void);

void    Sys_Init(void);

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

#ifdef __linux__
// TTimo only on linux .. maybe on Mac too?
// will OR with the existing mode (chmod ..+..)
void Sys_Chmod(char* file, int mode);
#endif

extern huffman_t clientHuffTables;

#endif	// _QCOMMON_H_
