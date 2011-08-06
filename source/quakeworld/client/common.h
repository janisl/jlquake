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
// comndef.h  -- general definitions

#define	MAX_INFO_STRING	196
#define	MAX_SERVERINFO_STRING	512
#define	MAX_LOCALINFO_STRING	32768

//============================================================================

typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;


void ClearLink (link_t *l);
void RemoveLink (link_t *l);
void InsertLinkBefore (link_t *l, link_t *before);
void InsertLinkAfter (link_t *l, link_t *after);

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (qintptr)&(((t *)0)->m)))

//============================================================================

struct usercmd_s;

extern struct usercmd_s nullcmd;

void MSG_WriteDeltaUsercmd (QMsg *sb, struct usercmd_s *from, struct usercmd_s *cmd);

void MSG_ReadDeltaUsercmd (struct usercmd_s *from, struct usercmd_s *cmd);

//============================================================================

void COM_Init (void);
void COM_InitArgv2(int argc, char **argv);


//============================================================================

extern int com_filesize;

byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize);
byte *COM_LoadHunkFile (const char *path);
void COM_Gamedir (char *dir);

extern	QCvar*		registered;
extern qboolean		standard_quake, rogue, hipnotic;

byte	COM_BlockSequenceCheckByte (byte *base, int length, int sequence, unsigned mapchecksum);
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

int build_number( void );
