//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _APPS_MAIN_H
#define _APPS_MAIN_H

//======================= WIN32 DEFINES =================================
#ifdef WIN32

// buildstring will be incorporated into the version string
#ifdef NDEBUG
#ifdef _M_IX86
#define CPUSTRING   "win-x86"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP"
#endif
#else
#ifdef _M_IX86
#define CPUSTRING   "win-x86-debug"
#elif defined _M_X64
#define CPUSTRING   "win-x86_64-debug"
#elif defined _M_ALPHA
#define CPUSTRING   "win-AXP-debug"
#endif
#endif

#endif

//======================= MAC OS X DEFINES =====================
#if defined(MACOS_X)

#ifdef __ppc__
#define CPUSTRING   "MacOSX-ppc"
#elif defined __i386__
#define CPUSTRING   "MacOSX-i386"
#elif defined __x86_64__
#define CPUSTRING   "MaxOSX-x86_64"
#else
#define CPUSTRING   "MacOSX-other"
#endif

#endif

//======================= LINUX DEFINES =================================
#ifdef __linux__

#ifdef __i386__
#define CPUSTRING   "linux-i386"
#elif defined __x86_64__
#define CPUSTRING   "linux-x86_64"
#elif defined __axp__
#define CPUSTRING   "linux-alpha"
#else
#define CPUSTRING   "linux-other"
#endif

#endif

//======================= FreeBSD DEFINES =====================
#ifdef __FreeBSD__	// rb010123

#ifdef __i386__
#define CPUSTRING       "freebsd-i386"
#elif defined __axp__
#define CPUSTRING       "freebsd-alpha"
#elif defined __x86_64__
#define CPUSTRING       "freebsd-x86_64"
#else
#define CPUSTRING       "freebsd-other"
#endif

#endif

void Com_Init(int argc, char* argv[], char* cmdline);
void Com_SharedInit(int argc, char* argv[], char* cmdline);
void Com_Frame();

extern Cvar* com_maxfps;
extern Cvar* com_fixedtime;
extern Cvar* comq3_cameraMode;
extern Cvar* com_watchdog;
extern Cvar* com_watchdog_cmd;
extern Cvar* com_showtrace;
extern int com_frameNumber;

#endif
