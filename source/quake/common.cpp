/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// common.c -- misc functions used in client and server

#include "quakedef.h"

#define NUM_SAFE_ARGVS  7

static const char* safeargvs[NUM_SAFE_ARGVS] =
{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"};

Cvar* cmdline;

int static_registered = 1;				// only for startup check, then set

qboolean msg_suppress_1 = 0;

void COM_InitFilesystem(void);

// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT              339
#define PAK0_CRC                32981

#define CMDLINE_LENGTH  256
char com_cmdline[CMDLINE_LENGTH];

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000,
	0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000,
	0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600,
	0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563,
	0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564,
	0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564,
	0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563,
	0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500,
	0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200,
	0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000,
	0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.



FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current command line arguments to allow different games to initialize startup parms differently.  This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because they are added at the end, they will not override an explicit setting on the original command line.

*/

//============================================================================

/*
================
COM_CheckRegistered

Looks for the pop.txt file and verifies it.
Sets the "registered" cvar.
Immediately exits out if an alternate game was attempted to be started without
being registered.
================
*/
void COM_CheckRegistered(void)
{
	fileHandle_t h;
	unsigned short check[128];
	int i;

	FS_FOpenFileRead("gfx/pop.lmp", &h, true);
	static_registered = 0;

	if (!h)
	{
#if WINDED
		common->FatalError("This dedicated server requires a full registered copy of Quake");
#endif
		common->Printf("Playing shareware version.\n");
		return;
	}

	FS_Read(check, sizeof(check), h);
	FS_FCloseFile(h);

	for (i = 0; i < 128; i++)
		if (pop[i] != (unsigned short)BigShort(check[i]))
		{
			common->FatalError("Corrupted data file.");
		}

	Cvar_Set("cmdline", com_cmdline);
	Cvar_Set("registered", "1");
	static_registered = 1;
	common->Printf("Playing registered version.\n");
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv2(int argc, char** argv)
{
	int i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j = 0; (j < argc); j++)
	{
		i = 0;

		while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (CMDLINE_LENGTH - 1))
		{
			com_cmdline[n++] = ' ';
		}
		else
		{
			break;
		}
	}

	com_cmdline[n] = 0;

	COM_InitArgv(argc, const_cast<const char**>(argv));

	if (COM_CheckParm("-safe"))
	{
		// force all the safe-mode switches. Note that we reserved extra space in
		// case we need to add these, so we don't need an overflow check
		for (i = 0; i < NUM_SAFE_ARGVS; i++)
		{
			COM_AddParm(safeargvs[i]);
		}
	}

	if (COM_CheckParm("-rogue"))
	{
		q1_rogue = true;
		q1_standard_quake = false;
	}

	if (COM_CheckParm("-hipnotic"))
	{
		q1_hipnotic = true;
		q1_standard_quake = false;
	}
}


/*
================
COM_Init
================
*/
void COM_Init(const char* basedir)
{
	Com_InitByteOrder();

	qh_registered = Cvar_Get("registered", "0", 0);
	cmdline = Cvar_Get("cmdline", "0", CVAR_SERVERINFO);

	COM_InitFilesystem();
	COM_CheckRegistered();
}

/// just for debugging
int     memsearch(byte* start, int count, int search)
{
	int i;

	for (i = 0; i < count; i++)
		if (start[i] == search)
		{
			return i;
		}
	return -1;
}

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem(void)
{
	char basedir[MAX_OSPATH];

	fs_PrimaryBaseGame = GAMENAME;
	//
	// -basedir <path>
	// Overrides the system supplied base directory (under GAMENAME)
	//
	int i = COM_CheckParm("-basedir");
	if (i && i < COM_Argc() - 1)
	{
		String::Cpy(basedir, COM_Argv(i + 1));
	}
	else
	{
		String::Cpy(basedir, host_parms.basedir);
	}
	int j = String::Length(basedir);
	if (j > 0)
	{
		if ((basedir[j - 1] == '\\') || (basedir[j - 1] == '/'))
		{
			basedir[j - 1] = 0;
		}
	}
	Cvar_Set("fs_basepath", basedir);

	FS_SharedStartup();

//
// start up with GAMENAME by default (id1)
//
	FS_AddGameDirectory(basedir, GAMENAME, ADDPACKS_UntilMissing);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, GAMENAME, ADDPACKS_UntilMissing);
	}

	if (COM_CheckParm("-rogue"))
	{
		FS_AddGameDirectory(basedir, "rogue", ADDPACKS_UntilMissing);
		if (fs_homepath->string[0])
		{
			FS_AddGameDirectory(fs_homepath->string, "rogue", ADDPACKS_UntilMissing);
		}
	}
	if (COM_CheckParm("-hipnotic"))
	{
		FS_AddGameDirectory(basedir, "hipnotic", ADDPACKS_UntilMissing);
		if (fs_homepath->string[0])
		{
			FS_AddGameDirectory(fs_homepath->string, "hipnotic", ADDPACKS_UntilMissing);
		}
	}

//
// -game <gamedir>
// Adds basedir/gamedir as an override game
//
	i = COM_CheckParm("-game");
	if (i && i < COM_Argc() - 1)
	{
		FS_AddGameDirectory(basedir, COM_Argv(i + 1), ADDPACKS_UntilMissing);
		if (fs_homepath->string[0])
		{
			FS_AddGameDirectory(fs_homepath->string, COM_Argv(i + 1), ADDPACKS_UntilMissing);
		}
	}
}

static qboolean con_debuglog;

void Com_InitDebugLog()
{
	const char* t2 = "qconsole.log";

	con_debuglog = COM_CheckParm("-condebug");

	if (con_debuglog)
	{
		FS_FCloseFile(FS_FOpenFileWrite(t2));
	}
}

/*
================
Con_DebugLog
================
*/
void Con_DebugLog(const char* file, const char* msg)
{
	fileHandle_t fd;

	FS_FOpenFileByMode(file, &fd, FS_APPEND);
	FS_Write(msg, String::Length(msg), fd);
	FS_FCloseFile(fd);
}


/*
================
Con_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Con_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

// also echo to debugging console
	Sys_Print(msg);	// also echo to debugging console

// log all messages to file
	if (con_debuglog)
	{
		Con_DebugLog("qconsole.log", msg);
	}

	if (!com_dedicated || com_dedicated->integer)
	{
		return;		// no graphics mode
	}

#ifndef DEDICATED
// write it to the scrollable buffer
	Con_ConsolePrint(msg);

// update the screen if the console is displayed
	if (clc.qh_signon != SIGNONS && !cls.disable_screen)
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls
		// Con_Printd
	static qboolean inupdate;
		if (!inupdate)
		{
			inupdate = true;
			SCR_UpdateScreen();
			inupdate = false;
		}
	}
#endif
}

/*
================
Con_DPrintf

A common->Printf that only shows up if the "developer" cvar is set
================
*/
void Con_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
	{
		return;			// don't confuse non-developers with techie stuff...

	}
	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	common->Printf("%s", msg);
}

void FS_Restart(int checksumFeed)
{
}

bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}
