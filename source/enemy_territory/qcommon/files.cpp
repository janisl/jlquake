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


/*****************************************************************************
 * Name:		files.c
 *
 * desc:		handle based filesystem for Quake III Arena
 *
 *
 *****************************************************************************/

#define MP_LEGACY_PAK 0x7776DC09

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

qboolean FS_OS_FileExists(const char* file)
{
	FILE* f;
	f = fopen(file, "rb");
	if (f)
	{
		fclose(f);
		return true;
	}
	return false;
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
	// Arnout: we want the nice error message here as well
	if (FS_ReadFile("default.cfg", NULL) <= 0)
	{
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
		// TTimo - added some verbosity, 'couldn't load default.cfg' confuses the hell out of users
		common->FatalError("Couldn't load default.cfg - I am missing essential files - verify your installation?");
	}

	// bk010116 - new check before safeMode
	if (String::ICmp(fs_gamedirvar->string, lastValidGame))
	{
		// skip the wolfconfig.cfg if "safe" is on the command line
		if (!Com_SafeMode())
		{
			const char* cl_profileStr = Cvar_VariableString("cl_profile");

			if (comet_gameInfo.usesProfiles && cl_profileStr[0])
			{
				// bani - check existing pid file and make sure it's ok
				if (!Com_CheckProfile(va("profiles/%s/profile.pid", cl_profileStr)))
				{
#ifndef _DEBUG
					common->Printf("^3WARNING: profile.pid found for profile '%s' - system settings will revert to defaults\n", cl_profileStr);
					// ydnar: set crashed state
					Cbuf_AddText("set com_crashed 1\n");
#endif
				}

				// bani - write a new one
				if (!Com_WriteProfile(va("profiles/%s/profile.pid", cl_profileStr)))
				{
					common->Printf("^3WARNING: couldn't write profiles/%s/profile.pid\n", cl_profileStr);
				}

				// exec the config
				Cbuf_AddText(va("exec profiles/%s/%s\n", cl_profileStr, ETCONFIG_NAME));

			}
			else
			{
				Cbuf_AddText(va("exec %s\n", ETCONFIG_NAME));
			}
		}
	}

	String::NCpyZ(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	String::NCpyZ(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));

}

unsigned int FS_ChecksumOSPath(char* OSPath)
{
	FILE* f;
	int len;
	byte* buf;
	unsigned int checksum;

	f = fopen(OSPath, "rb");
	if (!f)
	{
		return (unsigned int)-1;
	}
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);

	buf = (byte*)malloc(len);
	if ((int)fread(buf, 1, len, f) != len)
	{
		common->FatalError("short read in FS_ChecksumOSPath\n");
	}
	fclose(f);

	// Com_BlockChecksum returns an indian-dependent value
	// (better fix would have to be doing the LittleLong inside that function..)
	checksum = LittleLong(Com_BlockChecksum(buf, len));

	free(buf);
	return checksum;
}
