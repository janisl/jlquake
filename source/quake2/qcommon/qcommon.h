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

/*
==============================================================

FILESYSTEM

==============================================================
*/

void    FS_InitFilesystem(void);


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

void        Com_Printf(const char* fmt, ...);
void        Com_DPrintf(const char* fmt, ...);
void        Com_Error(int code, const char* fmt, ...);

void Qcommon_Init(int argc, char** argv);
void Qcommon_Frame(int msec);

#endif
