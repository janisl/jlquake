/*
Copyright (C) 1997-2001 Id Software, Inc.

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
	
// q_shared.h -- included first by ALL program modules

#include "../../core/core.h"

#include <assert.h>
#include <time.h>

// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages



#define	ERR_FATAL			0		// exit the entire game with a popup window
#define	ERR_DROP			1		// print to console and disconnect from game
#define	ERR_DISCONNECT		2		// don't kill server

// destination class for gi.multicast()
typedef enum
{
MULTICAST_ALL,
MULTICAST_PHS,
MULTICAST_PVS,
MULTICAST_ALL_R,
MULTICAST_PHS_R,
MULTICAST_PVS_R
} multicast_t;


//=============================================

void Com_PageInMemory (byte *buffer, int size);

//=============================================

//
// key / value info strings
//
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

/*
==============================================================

SYSTEM SPECIFIC

==============================================================
*/

extern	int	curtime;		// time returned by last Sys_Milliseconds
inline int Sys_Milliseconds_()
{
	curtime = Sys_Milliseconds();
	return curtime;
}

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

/*
** pass in an attribute mask of things you wish to REJECT
*/
char	*Sys_FindFirst (char *path, unsigned musthave, unsigned canthave );
char	*Sys_FindNext ( unsigned musthave, unsigned canthave );
void	Sys_FindClose (void);


// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (const char *error, ...);
void Com_Printf (const char *msg, ...);


/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(BSP38CONTENTS_SOLID|BSP38CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(BSP38CONTENTS_SOLID|BSP38CONTENTS_PLAYERCLIP|BSP38CONTENTS_WINDOW|BSP38CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(BSP38CONTENTS_SOLID|BSP38CONTENTS_PLAYERCLIP|BSP38CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(BSP38CONTENTS_SOLID|BSP38CONTENTS_MONSTERCLIP|BSP38CONTENTS_WINDOW|BSP38CONTENTS_MONSTER)
#define	MASK_WATER				(BSP38CONTENTS_WATER|BSP38CONTENTS_LAVA|BSP38CONTENTS_SLIME)
#define	MASK_OPAQUE				(BSP38CONTENTS_SOLID|BSP38CONTENTS_SLIME|BSP38CONTENTS_LAVA)
#define	MASK_SHOT				(BSP38CONTENTS_SOLID|BSP38CONTENTS_MONSTER|BSP38CONTENTS_WINDOW|BSP38CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(BSP38CONTENTS_CURRENT_0|BSP38CONTENTS_CURRENT_90|BSP38CONTENTS_CURRENT_180|BSP38CONTENTS_CURRENT_270|BSP38CONTENTS_CURRENT_UP|BSP38CONTENTS_CURRENT_DOWN)


// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2


//
// button bits
//
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_ANY			128			// any key whatsoever

#define	MAXTOUCH	32
typedef struct
{
	// state (in / out)
	q2pmove_state_t	s;

	// command (in)
	q2usercmd_t		cmd;
	qboolean		snapinitial;	// if s has been changed outside pmove

	// results (out)
	int			numtouch;
	struct edict_s	*touchents[MAXTOUCH];

	vec3_t		viewangles;			// clamped
	float		viewheight;

	vec3_t		mins, maxs;			// bounding box size

	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;

	// callbacks to test the world
	q2trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
} pmove_t;

// ROGUE


// dmflags->value flags
#define	DF_NO_HEALTH		0x00000001	// 1
#define	DF_NO_ITEMS			0x00000002	// 2
#define	DF_WEAPONS_STAY		0x00000004	// 4
#define	DF_NO_FALLING		0x00000008	// 8
#define	DF_INSTANT_ITEMS	0x00000010	// 16
#define	DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_MODELTEAMS		0x00000080	// 128
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define	DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384
#define DF_FIXED_FOV		0x00008000	// 32768

// RAFAEL
#define	DF_QUADFIRE_DROP	0x00010000	// 65536

//ROGUE
#define DF_NO_MINES			0x00020000
#define DF_NO_STACK_DOUBLE	0x00040000
#define DF_NO_NUKES			0x00080000
#define DF_NO_SPHERES		0x00100000
//ROGUE

/*
ROGUE - VERSIONS
1234	08/13/1998		Activision
1235	08/14/1998		Id Software
1236	08/15/1998		Steve Tietze
1237	08/15/1998		Phil Dobranski
1238	08/15/1998		John Sheley
1239	08/17/1998		Barrett Alexander
1230	08/17/1998		Brandon Fish
1245	08/17/1998		Don MacAskill
1246	08/17/1998		David "Zoid" Kirsch
1247	08/17/1998		Manu Smith
1248	08/17/1998		Geoff Scully
1249	08/17/1998		Andy Van Fossen
1240	08/20/1998		Activision Build 2
1256	08/20/1998		Ranger Clan
1257	08/20/1998		Ensemble Studios
1258	08/21/1998		Robert Duffy
1259	08/21/1998		Stephen Seachord
1250	08/21/1998		Stephen Heaslip
1267	08/21/1998		Samir Sandesara
1268	08/21/1998		Oliver Wyman
1269	08/21/1998		Steven Marchegiano
1260	08/21/1998		Build #2 for Nihilistic
1278	08/21/1998		Build #2 for Ensemble

9999	08/20/1998		Internal Use
*/
#define ROGUE_VERSION_ID		1278

#define ROGUE_VERSION_STRING	"08/21/1998 Beta 2 for Ensemble"
