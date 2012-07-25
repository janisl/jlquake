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

// if a packfile directory differs from this, it is assumed to be hacked
// Full version
#define PAK0_CHECKSUM   0x40e614e0
// Demo
//#define	PAK0_CHECKSUM	0xb2c6d7ea
// OEM
//#define	PAK0_CHECKSUM	0x78e135c

Cvar* fs_gamedirvar;

/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec(void)
{
	const char* dir;
	char name [MAX_QPATH];

	dir = Cvar_VariableString("gamedir");
	if (*dir)
	{
		String::Sprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basepath->string, dir);
	}
	else
	{
		String::Sprintf(name, sizeof(name), "%s/%s/autoexec.cfg", fs_basepath->string, BASEDIRNAME);
	}
	if (Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
	{
		Cbuf_AddText("exec autoexec.cfg\n");
	}
	Sys_FindClose();
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
void FS_SetGamedir(char* dir)
{
	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":"))
	{
		common->Printf("Gamedir should be a single filename, not a path\n");
		return;
	}

	//
	// free up any current game dir info
	//
	FS_ResetSearchPathToBase();

	//
	// flush all data, so it will be forced to reload
	//
	if (com_dedicated && !com_dedicated->value)
	{
		Cbuf_AddText("vid_restart\nsnd_restart\n");
	}

	String::Sprintf(fs_gamedir, sizeof(fs_gamedir), "%s", dir);

	if (!String::Cmp(dir,BASEDIRNAME) || (*dir == 0))
	{
		Cvar_Set("gamedir", "");
		Cvar_Set("game", "");
	}
	else
	{
		Cvar_Set("gamedir", dir);
		FS_AddGameDirectory(fs_basepath->string, dir, ADDPACKS_First10);
		if (fs_homepath->string[0])
		{
			FS_AddGameDirectory(fs_homepath->string, dir, ADDPACKS_First10);
		}
	}
}

/*
** FS_ListFiles
*/
char** FS_ListFiles(char* findname, int* numfiles, unsigned musthave, unsigned canthave)
{
	char* s;
	int nfiles = 0;
	char** list = 0;

	s = Sys_FindFirst(findname, musthave, canthave);
	while (s)
	{
		if (s[String::Length(s) - 1] != '.')
		{
			nfiles++;
		}
		s = Sys_FindNext(musthave, canthave);
	}
	Sys_FindClose();

	if (!nfiles)
	{
		return NULL;
	}

	nfiles++;	// add space for a guard
	*numfiles = nfiles;

	list = (char**)malloc(sizeof(char*) * nfiles);
	Com_Memset(list, 0, sizeof(char*) * nfiles);

	s = Sys_FindFirst(findname, musthave, canthave);
	nfiles = 0;
	while (s)
	{
		if (s[String::Length(s) - 1] != '.')
		{
			list[nfiles] = strdup(s);
#ifdef _WIN32
			String::ToLower(list[nfiles]);
#endif
			nfiles++;
		}
		s = Sys_FindNext(musthave, canthave);
	}
	Sys_FindClose();

	return list;
}


/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem(void)
{
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




bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}

void FS_Restart(int checksumFeed)
{
}
