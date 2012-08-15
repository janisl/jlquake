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


/*****************************************************************************
 * name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena
 *
 *
 *****************************************************************************/


#include "../game/q_shared.h"
#include "qcommon.h"

static Cvar* fs_basegame;
static Cvar* fs_gamedirvar;
static int fs_loadStack;					// total files in memory

// last valid game folder used
char lastValidBase[MAX_OSPATH];
char lastValidGame[MAX_OSPATH];

bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}

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
==========
FS_ShiftStr
perform simple string shifting to avoid scanning from the exe
==========
*/
char* FS_ShiftStr(const char* string, int shift)
{
	static char buf[MAX_STRING_CHARS];
	int i,l;

	l = String::Length(string);
	for (i = 0; i < l; i++)
	{
		buf[i] = string[i] + shift;
	}
	buf[i] = '\0';
	return buf;
}

//============================================================================

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

	Com_ReadCDKey(BASEGAME);
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (fs && fs->string[0] != 0)
	{
		Com_AppendCDKey(fs->string);
	}

	// show_bug.cgi?id=506
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
		// TTimo - added some verbosity, 'couldn't load default.cfg' confuses the hell out of users
		common->FatalError("Couldn't load default.cfg - I am missing essential files - verify your installation?");
	}

	String::NCpyZ(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	String::NCpyZ(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
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
		// skip the wolfconfig.cfg if "safe" is on the command line
		if (!Com_SafeMode())
		{
			Cbuf_AddText("exec wolfconfig_mp.cfg\n");
		}
	}

	String::NCpyZ(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	String::NCpyZ(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

}

/*
=================
FS_ConditionalRestart
restart if necessary

 FIXME TTimo
this doesn't catch all cases where an FS_Restart is necessary
see show_bug.cgi?id=478
=================
*/
qboolean FS_ConditionalRestart(int checksumFeed)
{
	if (fs_gamedirvar->modified || checksumFeed != fs_checksumFeed)
	{
		FS_Restart(checksumFeed);
		return true;
	}
	return false;
}
