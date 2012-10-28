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

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator char
issues.
==============================================================
*/

#define BASEGAME "etmain"

void    FS_InitFilesystem(void);

void    FS_Restart(int checksumFeed);
// shutdown and restart the filesystem so changes to fs_gamedir can take effect

qboolean FS_OS_FileExists(const char* file);	// TTimo - test file existence given OS path

int     FS_LoadStack();

unsigned int FS_ChecksumOSPath(char* OSPath);

/*
==============================================================

MISC

==============================================================
*/

//bani - profile functions
void Com_TrackProfile(char* profile_path);
qboolean Com_CheckProfile(char* profile_path);
qboolean Com_WriteProfile(char* profile_path);

extern Cvar* com_ignorecrash;		//bani

extern Cvar* com_pid;		//bani

//extern	Cvar	*com_blood;
extern Cvar* com_cameraMode;
extern Cvar* com_logosPlaying;

// watchdog
extern Cvar* com_watchdog;
extern Cvar* com_watchdog_cmd;

extern int com_frameMsec;

// commandLine should not include the executable name (argv[0])
void Com_Init(char* commandLine);
void Com_Frame(void);

#endif	// _QCOMMON_H_
