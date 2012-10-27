// common.c -- misc functions used in client and server

#ifdef SERVERONLY
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include "../../client/public.h"
#include "../../server/public.h"

#define NUM_SAFE_ARGVS  6

static const char* safeargvs[NUM_SAFE_ARGVS] =
{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse"};

int static_registered = 1;		// only for startup check, then set

qboolean msg_suppress_1 = 0;

void COM_InitFilesystem(void);


// if a packfile directory differs from this, it is assumed to be hacked
// retail
#define PAK0_COUNT              797
#define PAK0_CRC                22780

#define PAK2_COUNT              183
#define PAK2_CRC                4807

#define PAK3_COUNT              245
#define PAK3_CRC                1478

#define PAK4_COUNT              102
#define PAK4_CRC                41062

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
		common->Printf("Playing demo version.\n");
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
void COM_Init(const char* basedir)
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

	fs_PrimaryBaseGame = "data1";

	FS_SharedStartup();

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

//
// start up with id1 by default
//
	FS_AddGameDirectory(fs_basepath->string, "data1", ADDPACKS_First10);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, "data1", ADDPACKS_First10);
	}
	FS_AddGameDirectory(fs_basepath->string, "portals", ADDPACKS_First10);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, "portals", ADDPACKS_First10);
	}
	FS_AddGameDirectory(fs_basepath->string, "hw", ADDPACKS_First10);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, "hw", ADDPACKS_First10);
	}

	i = COM_CheckParm("-game");
	if (i && i < COM_Argc() - 1)
	{
		FS_AddGameDirectory(fs_basepath->string, COM_Argv(i + 1), ADDPACKS_First10);
		if (fs_homepath->string[0])
		{
			FS_AddGameDirectory(fs_homepath->string, COM_Argv(i + 1), ADDPACKS_First10);
		}
	}

	// any set gamedirs will be freed up to here
	FS_SetSearchPathBase();
}

void Com_InitDebugLog()
{
}

/*
================
Com_Printf

Handles cursor positioning, line wrapping, etc
================
*/
void Com_Printf(const char* fmt, ...)
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
	Com_LogToFile(msg);

	// write it to the scrollable buffer
	Con_ConsolePrint(msg);
}

/*
================
Com_DPrintf

A common->Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf(const char* fmt, ...)
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
