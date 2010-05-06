// common.c -- misc functions used in client and server

#ifdef SERVERONLY 
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#define NUM_SAFE_ARGVS	6

static char	*argvdummy = " ";

static char	*safeargvs[NUM_SAFE_ARGVS] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse"};

QCvar*	registered;

int		static_registered = 1;	// only for startup check, then set

qboolean		msg_suppress_1 = 0;

void COM_InitFilesystem (void);


// if a packfile directory differs from this, it is assumed to be hacked
// retail
#define PAK0_COUNT              797
#define PAK0_CRC                22780

#define PAK2_COUNT              183
#define PAK2_CRC                4807

#define PAK3_COUNT              245
#define PAK3_CRC                1478

#define PAK4_COUNT				102
#define PAK4_CRC				41062

qboolean		standard_quake = true, rogue, hipnotic;

char	gamedirfile[MAX_OSPATH];

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


All of Quake's data access is through a hierchal file system, but the contents of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all game directories.  The sys_* files pass this to host_init in quakeparms_t->basedir.  This can be overridden with the "-basedir" command line parm to allow code debugging in a different directory.  The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all generated files (savegames, screenshots, demos, config files) will be saved to.  This can be overridden with the "-game" command line parameter.  The game directory can never be changed while quake is executing.  This is a precacution against having a malicious server instruct clients to write files over areas they shouldn't.

The "cache directory" is only used during development to save network bandwidth, especially over ISDN / T1 lines.  If there is a cache directory
specified, when a file is found by the normal search path, it will be mirrored
into the cache directory, then opened there.
	
*/

//============================================================================


// ClearLink is used for new headnodes
void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}
void InsertLinkAfter (link_t *l, link_t *after)
{
	l->next = after->next;
	l->prev = after;
	l->prev->next = l;
	l->next->prev = l;
}

/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

//
// writing functions
//

void MSG_WriteUsercmd (QMsg *buf, usercmd_t *cmd, qboolean long_msg)
{
	int		bits;

//
// send the movement message
//
	bits = 0;
	if (cmd->angles[0])
		bits |= CM_ANGLE1;
	if (cmd->angles[2])
		bits |= CM_ANGLE3;
	if (cmd->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove)
		bits |= CM_UP;
	if (cmd->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse)
		bits |= CM_IMPULSE;
	if (cmd->msec)
		bits |= CM_MSEC;

    buf->WriteByte(bits);
	if (long_msg)
	{
		buf->WriteByte(cmd->light_level);
	}

	if (bits & CM_ANGLE1)
		buf->WriteAngle16(cmd->angles[0]);
	buf->WriteAngle16(cmd->angles[1]);
	if (bits & CM_ANGLE3)
		buf->WriteAngle16(cmd->angles[2]);
	
	if (bits & CM_FORWARD)
		buf->WriteChar((int)(cmd->forwardmove*0.25));
	if (bits & CM_SIDE)
	  	buf->WriteChar((int)(cmd->sidemove*0.25));
	if (bits & CM_UP)
		buf->WriteChar((int)(cmd->upmove*0.25));

 	if (bits & CM_BUTTONS)
	  	buf->WriteByte(cmd->buttons);
 	if (bits & CM_IMPULSE)
	    buf->WriteByte(cmd->impulse);
 	if (bits & CM_MSEC)
	    buf->WriteByte(cmd->msec);
}


//
// reading functions
//

void MSG_ReadUsercmd (usercmd_t *move, qboolean long_msg)
{
	int bits;

	Com_Memset(move, 0, sizeof(*move));

	bits = net_message.ReadByte ();
	if (long_msg)
	{
		move->light_level = net_message.ReadByte();
	}
	else
	{
		move->light_level = 0;
	}
		
// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = net_message.ReadAngle16();
	else
		move->angles[0] = 0;
	move->angles[1] = net_message.ReadAngle16();
	if (bits & CM_ANGLE3)
		move->angles[2] = net_message.ReadAngle16();
	else
		move->angles[2] = 0;
		
// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = net_message.ReadChar() * 4;
	if (bits & CM_SIDE)
		move->sidemove = net_message.ReadChar() * 4;
	if (bits & CM_UP)
		move->upmove = net_message.ReadChar() * 4;
	
// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = net_message.ReadByte ();
	else
		move->buttons = 0;

	if (bits & CM_IMPULSE)
		move->impulse = net_message.ReadByte ();
	else
		move->impulse = 0;

// read time to run command
	if (bits & CM_MSEC)
		move->msec = net_message.ReadByte ();
	else
		move->msec = 0;
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
	fileHandle_t	h;
	unsigned short	check[128];
	int			i;


	FS_FOpenFileRead("gfx/pop.lmp", &h, true);
	static_registered = 0;

	if (!h)
	{
		Con_Printf ("Playing demo version.\n");
		return;
	}

	FS_Read (check, sizeof(check), h);
	FS_FCloseFile (h);
	
	for (i=0 ; i<128 ; i++)
		if (pop[i] != (unsigned short)BigShort (check[i]))
			Sys_Error ("Corrupted data file.");
	
	Cvar_Set ("registered", "1");
	static_registered = 1;
	Con_Printf ("Playing registered version.\n");
}



/*
================
COM_InitArgv
================
*/
void COM_InitArgv2(int argc, char **argv)
{
	COM_InitArgv(argc, const_cast<const char**>(argv));

	if (COM_CheckParm ("-safe"))
	{
	// force all the safe-mode switches. Note that we reserved extra space in
	// case we need to add these, so we don't need an overflow check
		for (int i =0 ; i<NUM_SAFE_ARGVS ; i++)
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
void COM_Init (char *basedir)
{
	Com_InitByteOrder();

	registered = Cvar_Get("registered", "0", 0);

	COM_InitFilesystem ();
	COM_CheckRegistered ();
}


/// just for debugging
int	memsearch (byte *start, int count, int search)
{
	int		i;
	
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

int	com_filesize;


/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Allways appends a 0 byte to the loaded data.
============
*/
cache_user_t *loadcache;
byte	*loadbuf;
int		loadsize;
byte *COM_LoadFile (char *path, int usehunk)
{
	fileHandle_t	h;
	byte	*buf;
	char	base[32];
	int		len;

	buf = NULL;	// quiet compiler warning

// look for it in the filesystem or pack files
	len = com_filesize = FS_FOpenFileRead (path, &h, true);
	if (!h)
		return NULL;
	
// extract the filename base name for hunk tag
	QStr::FileBase (path, base);
	
	if (usehunk == 1)
		buf = (byte*)Hunk_AllocName (len+1, base);
	else if (usehunk == 2)
		buf = (byte*)Hunk_TempAlloc (len+1);
	else if (usehunk == 0)
		buf = (byte*)Z_Malloc (len+1);
	else if (usehunk == 3)
		buf = (byte*)Cache_Alloc (loadcache, len+1, base);
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
#ifndef SERVERONLY
	Draw_BeginDisc ();
#endif
	FS_Read (buf, len, h);
	FS_FCloseFile (h);
#ifndef SERVERONLY
	Draw_EndDisc ();
#endif
	return buf;
}

byte *COM_LoadHunkFile (char *path)
{
	return COM_LoadFile (path, 1);
}

byte *COM_LoadTempFile (char *path)
{
	return COM_LoadFile (path, 2);
}

void COM_LoadCacheFile (char *path, struct cache_user_s *cu)
{
	loadcache = cu;
	COM_LoadFile (path, 3);
}

// uses temp hunk if larger than bufsize
byte *COM_LoadStackFile (char *path, void *buffer, int bufsize)
{
	byte	*buf;
	
	loadbuf = (byte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, 4);
	
	return buf;
}

/*
================
COM_Gamedir

Sets the gamedir and path to a different directory.
================
*/
void COM_Gamedir (char *dir)
{
	if (strstr(dir, "..") || strstr(dir, "/") ||
		strstr(dir, "\\") || strstr(dir, ":") )
	{
		Con_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}

	if (!QStr::Cmp(gamedirfile, dir))
		return;		// still the same
	QStr::Cpy(gamedirfile, dir);

	//
	// free up any current game dir info
	//
	FS_ResetSearchPathToBase();

	//
	// flush all data, so it will be forced to reload
	//
	Cache_Flush ();

	if (!QStr::Cmp(dir,"id1") || !QStr::Cmp(dir, "hw"))
		return;

	FS_AddGameDirectory(fs_basepath->string, dir, ADDPACKS_First10);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, dir, ADDPACKS_First10);
	}
}

/*
================
COM_InitFilesystem
================
*/
void COM_InitFilesystem (void)
{
	int		i;
	char	com_basedir[MAX_OSPATH];

	FS_SharedStartup();

//
// -basedir <path>
// Overrides the system supplied base directory (under id1)
//
	i = COM_CheckParm ("-basedir");
	if (i && i < COM_Argc()-1)
		QStr::Cpy(com_basedir, COM_Argv(i+1));
	else
		QStr::Cpy(com_basedir, host_parms.basedir);
	Cvar_Set("fs_basepath", com_basedir);

//
// start up with id1 by default
//
	FS_AddGameDirectory (fs_basepath->string, "data1", ADDPACKS_First10);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, "data1", ADDPACKS_First10);
	FS_AddGameDirectory (fs_basepath->string, "portals", ADDPACKS_First10);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, "portals", ADDPACKS_First10);
	FS_AddGameDirectory (fs_basepath->string, "hw", ADDPACKS_First10);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, "hw", ADDPACKS_First10);

	i = COM_CheckParm ("-game");
	if (i && i < COM_Argc()-1)
	{
		FS_AddGameDirectory (fs_basepath->string, COM_Argv(i+1), ADDPACKS_First10);
		if (fs_homepath->string[0])
			FS_AddGameDirectory(fs_homepath->string, COM_Argv(i + 1), ADDPACKS_First10);
	}

	// any set gamedirs will be freed up to here
	FS_SetSearchPathBase();
}

void FS_Restart(int checksumFeed)
{
}

int Com_Milliseconds()
{
	return Sys_Milliseconds();
}
