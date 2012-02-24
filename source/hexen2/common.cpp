// common.c -- misc functions used in client and server

/*
 * $Header: /H2 Mission Pack/COMMON.C 15    3/19/98 4:28p Jmonroe $
 */

#include "quakedef.h"
#ifdef _WIN32
#include <windows.h>
#endif

#define NUM_SAFE_ARGVS  7

static const char     *safeargvs[NUM_SAFE_ARGVS] =
	{"-nomidi", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse", "-dibonly"};

Cvar*  registered;
Cvar*  sv_gamedir;
Cvar*  cmdline;

int             static_registered = 1;  // only for startup check, then set

qboolean		msg_suppress_1 = 0;

void COM_InitFilesystem (void);

// if a packfile directory differs from this, it is assumed to be hacked
// demo
//#define PAK0_COUNT              701
//#define PAK0_CRC                20870

// retail
#define PAK0_COUNT              697
#define PAK0_CRC                9787

#define PAK2_COUNT              183
#define PAK2_CRC                4807

#define CMDLINE_LENGTH	256
char	com_cmdline[CMDLINE_LENGTH];

// this graphic needs to be in the pak file to use registered features
unsigned short pop[] =
{
 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
,0x0000,0x0000,0x6600,0x0000,0x0000,0x0000,0x6600,0x0000
,0x0000,0x0066,0x0000,0x0000,0x0000,0x0000,0x0067,0x0000
,0x0000,0x6665,0x0000,0x0000,0x0000,0x0000,0x0065,0x6600
,0x0063,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6563
,0x0064,0x6561,0x0000,0x0000,0x0000,0x0000,0x0061,0x6564
,0x0064,0x6564,0x0000,0x6469,0x6969,0x6400,0x0064,0x6564
,0x0063,0x6568,0x6200,0x0064,0x6864,0x0000,0x6268,0x6563
,0x0000,0x6567,0x6963,0x0064,0x6764,0x0063,0x6967,0x6500
,0x0000,0x6266,0x6769,0x6a68,0x6768,0x6a69,0x6766,0x6200
,0x0000,0x0062,0x6566,0x6666,0x6666,0x6666,0x6562,0x0000
,0x0000,0x0000,0x0062,0x6364,0x6664,0x6362,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0062,0x6662,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0061,0x6661,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6500,0x0000,0x0000,0x0000
,0x0000,0x0000,0x0000,0x0000,0x6400,0x0000,0x0000,0x0000
};

/*
All of Quake's data access is through a hierchal file system, but the contents of the 
file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game 
directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This 
can be overridden with the "-basedir" command line parm to allow code debugging in a 
different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated 
files (savegames, screenshots, demos, config files) will be saved to.  This can be 
overridden with the "-game" command line parameter.  The game directory can never be 
changed while quake is executing.  This is a precacution against having a malicious 
server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, 
especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.


FIXME:
The file "parms.txt" will be read out of the game directory and appended to the current 
command line arguments to allow different games to initialize startup parms differently.  
This could be used to add a "-sspeed 22050" for the high quality sound edition.  Because 
they are added at the end, they will not override an explicit setting on the original 
command line.	
*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (qhlink_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (qhlink_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (qhlink_t *l, qhlink_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (qhlink_t *l, qhlink_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

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
void COM_CheckRegistered (void)
{
	fileHandle_t	f;
	unsigned short  check[128];
	int                     i;

	FS_FOpenFileRead("gfx/pop.lmp", &f, true);
	static_registered = 0;

//-basedir c:\h3\test -game data1 +exec rick.cfg
	if (!f)
	{
#if WINDED
	Sys_Error ("This dedicated server requires a full registered copy of Hexen II");
#endif
		Con_Printf ("Playing demo version.\n");
		return;
	}

	FS_Read(check, sizeof(check), f);
	FS_FCloseFile(f);

	for (i=0 ; i<128 ; i++)
		if (pop[i] != (unsigned short)BigShort (check[i]))
			Sys_Error ("Corrupted data file.");
	
	static_registered = 1;
	Cvar_Set ("cmdline", com_cmdline);

	Cvar_Set ("registered", "1");
	Con_Printf ("Playing retail version.\n");
}


/*
================
COM_InitArgv
================
*/
void COM_InitArgv2(int argc, char **argv)
{
	int             i, j, n;

// reconstitute the command line for the cmdline externally visible cvar
	n = 0;

	for (j=0 ; (j< argc) ; j++)
	{
		i = 0;

		while ((n < (CMDLINE_LENGTH - 1)) && argv[j][i])
		{
			com_cmdline[n++] = argv[j][i++];
		}

		if (n < (CMDLINE_LENGTH - 1))
			com_cmdline[n++] = ' ';
		else
			break;
	}

	com_cmdline[n] = 0;

	COM_InitArgv(argc, const_cast<const char**>(argv));

	if (COM_CheckParm ("-safe"))
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (i=0 ; i<NUM_SAFE_ARGVS ; i++)
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
void COM_Init (const char *basedir)
{
	Com_InitByteOrder();

	registered = Cvar_Get("registered", "0", 0);
	sv_gamedir = Cvar_Get("*gamedir", "portals", CVAR_SERVERINFO);
	cmdline = Cvar_Get("cmdline","0", CVAR_SERVERINFO);

	COM_InitFilesystem ();
	COM_CheckRegistered ();
}


/// just for debugging
int     memsearch (byte *start, int count, int search)
{
	int             i;
	
	for (i=0 ; i<count ; i++)
		if (start[i] == search)
			return i;
	return -1;
}

/*
=============================================================================

QUAKE FILESYSTEM

=============================================================================
*/

int     com_filesize;


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte.
============
*/
byte    *loadbuf;
int             loadsize;
byte *COM_LoadFile (const char *path, int usehunk, int *size)
{
	fileHandle_t	f;
	byte    *buf;
	char    base[32];
	int             len;

	buf = NULL;     // quiet compiler warning

// look for it in the filesystem or pack files
	com_filesize = FS_FOpenFileRead (path, &f, true);
	len = com_filesize;
	if (size) *size = len;
	if (!f)
		return NULL;
	
// extract the filename base name for hunk tag
	String::FileBase (path, base);
	
	if (usehunk == 1)
		buf = (byte*)Hunk_AllocName (len+1, path);
	else if (usehunk == 0)
		buf = (byte*)Z_Malloc (len+1);
	else if (usehunk == 4)
	{
		if (len+1 > loadsize)
			buf = (byte*)Hunk_TempAlloc (len+1);
		else
			buf = loadbuf;
	}
	else
		Sys_Error ("COM_LoadFile: bad usehunk");

	if (!buf)
		Sys_Error ("COM_LoadFile: not enough space for %s", path);
		
	((byte *)buf)[len] = 0;

	FS_Read(buf, len, f);
	FS_FCloseFile(f);

	return buf;
}

byte *COM_LoadHunkFile (const char *path)
{
	return COM_LoadFile (path, 1, NULL);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize)
{
	byte    *buf;
	
	loadbuf = (byte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, 4, NULL);
	
	return buf;
}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem (void)
{
	char    basedir[MAX_OSPATH];

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
	FS_AddGameDirectory (basedir, GAMENAME, ADDPACKS_First10);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, GAMENAME, ADDPACKS_First10);
#ifdef MISSIONPACK
	FS_AddGameDirectory (basedir, "portals", ADDPACKS_First10);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, "portals", ADDPACKS_First10);
#endif

/*	if (COM_CheckParm ("-rogue"))
		COM_AddGameDirectory (va("%s/rogue", basedir) );
*/

//
// -game <gamedir>
// Adds basedir/gamedir as an override game
//
	i = COM_CheckParm ("-game");
	if (i && i < COM_Argc()-1)
	{
		FS_AddGameDirectory (basedir, COM_Argv(i+1), ADDPACKS_First10);
		if (fs_homepath->string[0])
			FS_AddGameDirectory(fs_homepath->string, COM_Argv(i + 1), ADDPACKS_First10);
	}

//Mimic qw style cvar to help gamespy, do we really need a way to change it?
//	Cvar_Set ("*gamedir", com_gamedir);	//removed to prevent full path
}

void FS_Restart(int checksumFeed)
{
}

bool CL_WWWBadChecksum(const char* pakname)
{
	return false;
}

int Com_Milliseconds()
{
	return Sys_Milliseconds();
}
