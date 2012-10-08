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

static Cvar* fs_basegame;
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

bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}

/*
================
FS_Startup
================
*/
static void FS_Startup(const char* gameName)
{
	Cvar* fs;

	common->Printf("----- FS_Startup -----\n");

	FS_SharedStartup();

	fs_basegame = Cvar_Get("fs_basegame", "", CVAR_INIT);
	fs_gamedirvar = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);

	// add search path elements in reverse priority order
	if (fs_basepath->string[0])
	{
		FS_AddGameDirectory(fs_basepath->string, gameName, ADDPACKS_None);
	}
	// fs_homepath is somewhat particular to *nix systems, only add if relevant
	// NOTE: same filtering below for mods and basegame
	if (fs_basepath->string[0] && String::ICmp(fs_homepath->string,fs_basepath->string))
	{
		FS_AddGameDirectory(fs_homepath->string, gameName, ADDPACKS_None);
	}

	// check for additional base game so mods can be based upon other mods
	if (fs_basegame->string[0] && !String::ICmp(gameName, BASEGAME) && String::ICmp(fs_basegame->string, gameName))
	{
		if (fs_basepath->string[0])
		{
			FS_AddGameDirectory(fs_basepath->string, fs_basegame->string, ADDPACKS_None);
		}
		if (fs_homepath->string[0] && String::ICmp(fs_homepath->string,fs_basepath->string))
		{
			FS_AddGameDirectory(fs_homepath->string, fs_basegame->string, ADDPACKS_None);
		}
	}

	// check for additional game folder for mods
	if (fs_gamedirvar->string[0] && !String::ICmp(gameName, BASEGAME) && String::ICmp(fs_gamedirvar->string, gameName))
	{
		if (fs_basepath->string[0])
		{
			FS_AddGameDirectory(fs_basepath->string, fs_gamedirvar->string, ADDPACKS_None);
		}
		if (fs_homepath->string[0] && String::ICmp(fs_homepath->string,fs_basepath->string))
		{
			FS_AddGameDirectory(fs_homepath->string, fs_gamedirvar->string, ADDPACKS_None);
		}
	}

	CLT3_ReadCDKey("baseq3");
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (fs && fs->string[0] != 0)
	{
		CLT3_AppendCDKey(fs->string);
	}

	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=506
	// reorder the pure pk3 files according to server order
	FS_ReorderPurePaks();

	// print the current search paths
	FS_Path_f();

	fs_gamedirvar->modified = false;	// We just loaded, it's not modified

	common->Printf("----------------------\n");

	common->Printf("%d files in pk3 files\n", fs_packFiles);
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
	FS_Startup(BASEGAME);

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
	FS_Startup(BASEGAME);

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
