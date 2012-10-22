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

#include "qcommon.h"

/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem(void)
{
	fs_PrimaryBaseGame = BASEDIRNAME;
	FS_SharedStartup();

	//
	// basedir <path>
	// allows the game to run from outside the data tree
	//

	//
	// start up with baseq2 by default
	//
	FS_AddGameDirectory(fs_basepath->string, BASEDIRNAME, ADDPACKS_First10);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, BASEDIRNAME, ADDPACKS_First10);
	}

	// any set gamedirs will be freed up to here
	FS_SetSearchPathBase();

	// check for game override
	Cvar_Get("gamedir", "", CVAR_SERVERINFO | CVAR_INIT);
	fs_gamedirvar = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO);
	if (fs_gamedirvar->string[0])
	{
		FS_SetGamedir(fs_gamedirvar->string);
	}
}

void FS_Restart(int checksumFeed)
{
}
