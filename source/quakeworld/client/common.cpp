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

#ifdef SERVERONLY
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif
#include "../../client/public.h"
#include "../../client/client.h"

#define NUM_SAFE_ARGVS  6

void Com_Quit_f()
{
#ifndef SERVERONLY
	CL_Disconnect(true);
#endif
	Sys_Quit();
}

static const char* safeargvs[NUM_SAFE_ARGVS] =
{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse"};

int static_registered = 1;		// only for startup check, then set

qboolean msg_suppress_1 = 0;

void COM_InitFilesystem(void);


// if a packfile directory differs from this, it is assumed to be hacked
#define PAK0_COUNT      339
#define PAK0_CRC        52883

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

*/

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
	COM_InitArgv(argc, const_cast<const char**>(argv));

	if (COM_CheckParm("-safe"))
	{
		// force all the safe-mode switches. Note that we reserved extra space in
		// case we need to add these, so we don't need an overflow check
		for (int i = 0; i < NUM_SAFE_ARGVS; i++)
		{
			COM_AddParm(safeargvs[i]);
		}
	}
}

/*
================
COM_Init
================
*/
void COM_Init(void)
{
	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);

	COM_InitFilesystem();
	COM_CheckRegistered();
}


/// just for debugging
int memsearch(byte* start, int count, int search)
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
	int i;
	char com_basedir[MAX_OSPATH];

	fs_PrimaryBaseGame = "id1";
	//
	// -basedir <path>
	// Overrides the system supplied base directory (under id1)
	//
	i = COM_CheckParm("-basedir");
	if (i && i < COM_Argc() - 1)
	{
		String::Cpy(com_basedir, COM_Argv(i + 1));
	}
	else
	{
		String::Cpy(com_basedir, host_parms.basedir);
	}
	Cvar_Set("fs_basepath", com_basedir);

	FS_SharedStartup();

	//
	// start up with id1 by default
	//
	FS_AddGameDirectory(fs_basepath->string, "id1", ADDPACKS_UntilMissing);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, "id1", ADDPACKS_UntilMissing);
	}
	FS_AddGameDirectory(fs_basepath->string, "qw", ADDPACKS_UntilMissing);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, "qw", ADDPACKS_UntilMissing);
	}

	// any set gamedirs will be freed up to here
	FS_SetSearchPathBase();
}

// char *date = "Oct 24 1996";
static const char* date = __DATE__;
static const char* mon[12] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char mond[12] =
{ 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

// returns days since Oct 24 1996
int build_number(void)
{
	int m = 0;
	int d = 0;
	int y = 0;
	static int b = 0;

	if (b != 0)
	{
		return b;
	}

	for (m = 0; m < 11; m++)
	{
		if (String::NICmp(&date[0], mon[m], 3) == 0)
		{
			break;
		}
		d += mond[m];
	}

	d += String::Atoi(&date[4]) - 1;

	y = String::Atoi(&date[7]) - 1900;

	b = d + (int)((y - 1) * 365.25);

	if (((y % 4) == 0) && m > 1)
	{
		b += 1;
	}

	b -= 35778;	// Dec 16 1998

	return b;
}

static qboolean con_debuglog;
fileHandle_t sv_logfile;

void Com_InitDebugLog()
{
	con_debuglog = COM_CheckParm("-condebug");
}

static void Con_DebugLog(const char* file, const char* msg)
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

	// add to redirected message
	if (rd_buffer)
	{
		if (String::Length(msg) + String::Length(rd_buffer) > rd_buffersize - 1)
		{
			rd_flush(rd_buffer);
		}
		String::Cat(rd_buffer, rd_buffersize, msg);
		return;
	}

	// also echo to debugging console
	Sys_Print(msg);

	// log all messages to file
	if (con_debuglog)
	{
		Con_DebugLog("qconsole.log", msg);
	}
	if (sv_logfile)
	{
		FS_Printf(sv_logfile, "%s", msg);
	}

	// write it to the scrollable buffer
	Con_ConsolePrint(msg);

#ifndef SERVERONLY
	// update the screen immediately if the console is displayed
	if (cls.state != CA_ACTIVE)
	{
		// protect against infinite loop if something in SCR_UpdateScreen calls
		// Con_Printd
		static bool inupdate;
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
