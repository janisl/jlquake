/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
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

#define NUM_SAFE_ARGVS	6

usercmd_t nullcmd; // guarenteed to be zero

static char	*argvdummy = " ";

static char	*safeargvs[NUM_SAFE_ARGVS] =
	{"-stdvid", "-nolan", "-nosound", "-nocdaudio", "-nojoy", "-nomouse"};

QCvar*	registered;

int		static_registered = 1;	// only for startup check, then set

qboolean		msg_suppress_1 = 0;

void COM_InitFilesystem (void);


// if a packfile directory differs from this, it is assumed to be hacked
#define	PAK0_COUNT		339
#define	PAK0_CRC		52883

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

void MSG_WriteDeltaUsercmd (QMsg *buf, usercmd_t *from, usercmd_t *cmd)
{
	int		bits;

//
// send the movement message
//
	bits = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= CM_UP;
	if (cmd->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= CM_IMPULSE;

    buf->WriteByte(bits);

	if (bits & CM_ANGLE1)
		buf->WriteAngle16(cmd->angles[0]);
	if (bits & CM_ANGLE2)
		buf->WriteAngle16(cmd->angles[1]);
	if (bits & CM_ANGLE3)
		buf->WriteAngle16(cmd->angles[2]);
	
	if (bits & CM_FORWARD)
		buf->WriteShort(cmd->forwardmove);
	if (bits & CM_SIDE)
	  	buf->WriteShort(cmd->sidemove);
	if (bits & CM_UP)
		buf->WriteShort(cmd->upmove);

 	if (bits & CM_BUTTONS)
	  	buf->WriteByte(cmd->buttons);
 	if (bits & CM_IMPULSE)
	    buf->WriteByte(cmd->impulse);
	buf->WriteByte(cmd->msec);
}


//
// reading functions
//

void MSG_ReadDeltaUsercmd (usercmd_t *from, usercmd_t *move)
{
	int bits;

	Com_Memcpy(move, from, sizeof(*move));

	bits = net_message.ReadByte ();
		
// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = net_message.ReadAngle16();
	if (bits & CM_ANGLE2)
		move->angles[1] = net_message.ReadAngle16();
	if (bits & CM_ANGLE3)
		move->angles[2] = net_message.ReadAngle16();
		
// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = net_message.ReadShort ();
	if (bits & CM_SIDE)
		move->sidemove = net_message.ReadShort ();
	if (bits & CM_UP)
		move->upmove = net_message.ReadShort ();
	
// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = net_message.ReadByte ();

	if (bits & CM_IMPULSE)
		move->impulse = net_message.ReadByte ();

// read time to run command
	move->msec = net_message.ReadByte ();
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
		Con_Printf ("Playing shareware version.\n");
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
		for (int i=0 ; i<NUM_SAFE_ARGVS ; i++)
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
void COM_Init (void)
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

	if (!QStr::Cmp(dir,"id1") || !QStr::Cmp(dir, "qw"))
		return;

	FS_AddGameDirectory(fs_basepath->string, dir, ADDPACKS_UntilMissing);
	if (fs_homepath->string[0])
	{
		FS_AddGameDirectory(fs_homepath->string, dir, ADDPACKS_UntilMissing);
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

	FS_SharedStartup();

	//
	// start up with id1 by default
	//
	FS_AddGameDirectory (fs_basepath->string, "id1", ADDPACKS_UntilMissing);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, "id1", ADDPACKS_UntilMissing);
	FS_AddGameDirectory (fs_basepath->string, "qw", ADDPACKS_UntilMissing);
	if (fs_homepath->string[0])
		FS_AddGameDirectory(fs_homepath->string, "qw", ADDPACKS_UntilMissing);

	// any set gamedirs will be freed up to here
	FS_SetSearchPathBase();
}



static byte chktbl[1024 + 4] = {
0x78,0xd2,0x94,0xe3,0x41,0xec,0xd6,0xd5,0xcb,0xfc,0xdb,0x8a,0x4b,0xcc,0x85,0x01,
0x23,0xd2,0xe5,0xf2,0x29,0xa7,0x45,0x94,0x4a,0x62,0xe3,0xa5,0x6f,0x3f,0xe1,0x7a,
0x64,0xed,0x5c,0x99,0x29,0x87,0xa8,0x78,0x59,0x0d,0xaa,0x0f,0x25,0x0a,0x5c,0x58,
0xfb,0x00,0xa7,0xa8,0x8a,0x1d,0x86,0x80,0xc5,0x1f,0xd2,0x28,0x69,0x71,0x58,0xc3,
0x51,0x90,0xe1,0xf8,0x6a,0xf3,0x8f,0xb0,0x68,0xdf,0x95,0x40,0x5c,0xe4,0x24,0x6b,
0x29,0x19,0x71,0x3f,0x42,0x63,0x6c,0x48,0xe7,0xad,0xa8,0x4b,0x91,0x8f,0x42,0x36,
0x34,0xe7,0x32,0x55,0x59,0x2d,0x36,0x38,0x38,0x59,0x9b,0x08,0x16,0x4d,0x8d,0xf8,
0x0a,0xa4,0x52,0x01,0xbb,0x52,0xa9,0xfd,0x40,0x18,0x97,0x37,0xff,0xc9,0x82,0x27,
0xb2,0x64,0x60,0xce,0x00,0xd9,0x04,0xf0,0x9e,0x99,0xbd,0xce,0x8f,0x90,0x4a,0xdd,
0xe1,0xec,0x19,0x14,0xb1,0xfb,0xca,0x1e,0x98,0x0f,0xd4,0xcb,0x80,0xd6,0x05,0x63,
0xfd,0xa0,0x74,0xa6,0x86,0xf6,0x19,0x98,0x76,0x27,0x68,0xf7,0xe9,0x09,0x9a,0xf2,
0x2e,0x42,0xe1,0xbe,0x64,0x48,0x2a,0x74,0x30,0xbb,0x07,0xcc,0x1f,0xd4,0x91,0x9d,
0xac,0x55,0x53,0x25,0xb9,0x64,0xf7,0x58,0x4c,0x34,0x16,0xbc,0xf6,0x12,0x2b,0x65,
0x68,0x25,0x2e,0x29,0x1f,0xbb,0xb9,0xee,0x6d,0x0c,0x8e,0xbb,0xd2,0x5f,0x1d,0x8f,
0xc1,0x39,0xf9,0x8d,0xc0,0x39,0x75,0xcf,0x25,0x17,0xbe,0x96,0xaf,0x98,0x9f,0x5f,
0x65,0x15,0xc4,0x62,0xf8,0x55,0xfc,0xab,0x54,0xcf,0xdc,0x14,0x06,0xc8,0xfc,0x42,
0xd3,0xf0,0xad,0x10,0x08,0xcd,0xd4,0x11,0xbb,0xca,0x67,0xc6,0x48,0x5f,0x9d,0x59,
0xe3,0xe8,0x53,0x67,0x27,0x2d,0x34,0x9e,0x9e,0x24,0x29,0xdb,0x69,0x99,0x86,0xf9,
0x20,0xb5,0xbb,0x5b,0xb0,0xf9,0xc3,0x67,0xad,0x1c,0x9c,0xf7,0xcc,0xef,0xce,0x69,
0xe0,0x26,0x8f,0x79,0xbd,0xca,0x10,0x17,0xda,0xa9,0x88,0x57,0x9b,0x15,0x24,0xba,
0x84,0xd0,0xeb,0x4d,0x14,0xf5,0xfc,0xe6,0x51,0x6c,0x6f,0x64,0x6b,0x73,0xec,0x85,
0xf1,0x6f,0xe1,0x67,0x25,0x10,0x77,0x32,0x9e,0x85,0x6e,0x69,0xb1,0x83,0x00,0xe4,
0x13,0xa4,0x45,0x34,0x3b,0x40,0xff,0x41,0x82,0x89,0x79,0x57,0xfd,0xd2,0x8e,0xe8,
0xfc,0x1d,0x19,0x21,0x12,0x00,0xd7,0x66,0xe5,0xc7,0x10,0x1d,0xcb,0x75,0xe8,0xfa,
0xb6,0xee,0x7b,0x2f,0x1a,0x25,0x24,0xb9,0x9f,0x1d,0x78,0xfb,0x84,0xd0,0x17,0x05,
0x71,0xb3,0xc8,0x18,0xff,0x62,0xee,0xed,0x53,0xab,0x78,0xd3,0x65,0x2d,0xbb,0xc7,
0xc1,0xe7,0x70,0xa2,0x43,0x2c,0x7c,0xc7,0x16,0x04,0xd2,0x45,0xd5,0x6b,0x6c,0x7a,
0x5e,0xa1,0x50,0x2e,0x31,0x5b,0xcc,0xe8,0x65,0x8b,0x16,0x85,0xbf,0x82,0x83,0xfb,
0xde,0x9f,0x36,0x48,0x32,0x79,0xd6,0x9b,0xfb,0x52,0x45,0xbf,0x43,0xf7,0x0b,0x0b,
0x19,0x19,0x31,0xc3,0x85,0xec,0x1d,0x8c,0x20,0xf0,0x3a,0xfa,0x80,0x4d,0x2c,0x7d,
0xac,0x60,0x09,0xc0,0x40,0xee,0xb9,0xeb,0x13,0x5b,0xe8,0x2b,0xb1,0x20,0xf0,0xce,
0x4c,0xbd,0xc6,0x04,0x86,0x70,0xc6,0x33,0xc3,0x15,0x0f,0x65,0x19,0xfd,0xc2,0xd3,

// map checksum goes here
0x00,0x00,0x00,0x00
};

static byte chkbuf[16 + 60 + 4];

static unsigned last_mapchecksum = 0;

/*
====================
COM_BlockSequenceCRCByte

For proxy protecting
====================
*/
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence)
{
	unsigned short crc;
	byte	*p;
	byte chkb[60 + 4];

	p = chktbl + (sequence % (sizeof(chktbl) - 8));

	if (length > 60)
		length = 60;
	Com_Memcpy(chkb, base, length);

	chkb[length] = (sequence & 0xff) ^ p[0];
	chkb[length+1] = p[1];
	chkb[length+2] = ((sequence>>8) & 0xff) ^ p[2];
	chkb[length+3] = p[3];

	length += 4;

	crc = CRC_Block(chkb, length);

	crc &= 0xff;

	return crc;
}

// char *date = "Oct 24 1996";
static char *date = __DATE__ ;
static char *mon[12] = 
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char mond[12] = 
{ 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

// returns days since Oct 24 1996
int build_number( void )
{
	int m = 0; 
	int d = 0;
	int y = 0;
	static int b = 0;

	if (b != 0)
		return b;

	for (m = 0; m < 11; m++)
	{
		if (QStr::NICmp( &date[0], mon[m], 3 ) == 0)
			break;
		d += mond[m];
	}

	d += QStr::Atoi( &date[4] ) - 1;

	y = QStr::Atoi( &date[7] ) - 1900;

	b = d + (int)((y - 1) * 365.25);

	if (((y % 4) == 0) && m > 1)
	{
		b += 1;
	}

	b -= 35778; // Dec 16 1998

	return b;
}


void FS_Restart(int checksumFeed)
{
}
