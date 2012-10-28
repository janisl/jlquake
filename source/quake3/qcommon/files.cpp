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
/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena
 *
 * $Archive: /MissionPack/code/qcommon/files.c $
 *
 *****************************************************************************/


#include "../game/q_shared.h"
#include "qcommon.h"
#include "../../client/public.h"

static int fs_loadStack;					// total files in memory

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];

/*
=================
FS_LoadStack
return load stack
=================
*/
int FS_LoadStack()
{
	return fs_loadStack;
}

/*
================
FS_InitFilesystem

Called only at inital startup, not when the filesystem
is resetting due to a game change
================
*/
void FS_InitFilesystem(void)
{
	// allow command line parms to override our defaults
	// we have to specially handle this, because normal command
	// line variable sets don't happen until after the filesystem
	// has already been initialized
	fs_PrimaryBaseGame = BASEGAME;
	Com_StartupVariable("fs_basepath");
	Com_StartupVariable("fs_homepath");
	Com_StartupVariable("fs_game");

	// try to start up normally
	FS_Startup();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if (FS_ReadFile("default.cfg", NULL) <= 0)
	{
		common->FatalError("Couldn't load default.cfg");
		// bk001208 - SafeMode see below, FIXME?
	}

	String::NCpyZ(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	String::NCpyZ(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

	// bk001208 - SafeMode see below, FIXME?
}


/*
================
FS_Restart
================
*/
void FS_Restart(int checksumFeed)
{

	// free anything we currently have loaded
	FS_Shutdown();

	// set the checksum feed
	fs_checksumFeed = checksumFeed;

	// clear pak references
	FS_ClearPakReferences(0);

	// try to start up normally
	FS_Startup();

	// if we can't find default.cfg, assume that the paths are
	// busted and error out now, rather than getting an unreadable
	// graphics screen when the font fails to load
	if (FS_ReadFile("default.cfg", NULL) <= 0)
	{
		// this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		// (for instance a TA demo server)
		if (lastValidBase[0])
		{
			FS_PureServerSetLoadedPaks("", "");
			Cvar_Set("fs_basepath", lastValidBase);
			Cvar_Set("fs_gamedirvar", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart(checksumFeed);
			common->Error("Invalid game folder\n");
			return;
		}
		common->FatalError("Couldn't load default.cfg");
	}

	// bk010116 - new check before safeMode
	if (String::ICmp(fs_gamedirvar->string, lastValidGame))
	{
		// skip the q3config.cfg if "safe" is on the command line
		if (!Com_SafeMode())
		{
			Cbuf_AddText("exec q3config.cfg\n");
		}
	}

	String::NCpyZ(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	String::NCpyZ(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

}
