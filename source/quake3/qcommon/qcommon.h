/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// qcommon.h -- definitions common between client and server, but not game.or ref modules
#ifndef _QCOMMON_H_
#define _QCOMMON_H_

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

#define BASEGAME "baseq3"

void FS_InitFilesystem();

void    FS_Restart(int checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

int     FS_LoadStack();

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

void        Com_Printf(const char* fmt, ...);
void        Com_DPrintf(const char* fmt, ...);
void        Com_Error(int code, const char* fmt, ...);
void        Com_Quit_f(void);

extern Cvar* com_blood;
extern Cvar* com_buildScript;			// for building release pak files
extern Cvar* com_cameraMode;

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

void CL_Frame(int msec);

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/

void    Sys_Init(void);

const char* Sys_GetCurrentUser(void);

void    Sys_Error(const char* error, ...);
void    Sys_Quit(void);

// the system console is shown when a dedicated server is running
void    Sys_DisplaySystemConsole(qboolean show);

int     Sys_GetProcessorId(void);

unsigned int Sys_ProcessorCount();

extern huffman_t clientHuffTables;

#endif	// _QCOMMON_H_
