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
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

//============================================================================

struct usercmd_s;

extern struct usercmd_s nullcmd;

void MSG_WriteDeltaUsercmd (QMsg *sb, struct usercmd_s *from, struct usercmd_s *cmd);

void MSG_ReadDeltaUsercmd (struct usercmd_s *from, struct usercmd_s *cmd);

//============================================================================

extern	int		com_argc;
extern	char	**com_argv;

int COM_CheckParm (char *parm);
void COM_AddParm (char *parm);

void COM_Init (void);
void COM_InitArgv (int argc, char **argv);


//============================================================================

extern int com_filesize;
struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];

void COM_WriteFile (char *filename, void *data, int len);
int COM_FOpenFile (char *filename, FILE **file);
void COM_CloseFile (FILE *h);

byte *COM_LoadStackFile (char *path, void *buffer, int bufsize);
byte *COM_LoadTempFile (char *path);
byte *COM_LoadHunkFile (char *path);
void COM_LoadCacheFile (char *path, struct cache_user_s *cu);
void COM_CreatePath (char *path);
void COM_Gamedir (char *dir);

extern	struct cvar_s	registered;
extern qboolean		standard_quake, rogue, hipnotic;

char *Info_ValueForKey (char *s, char *key);
void Info_RemoveKey (char *s, char *key);
void Info_RemovePrefixedKeys (char *start, char prefix);
void Info_SetValueForKey (char *s, char *key, char *value, int maxsize);
void Info_SetValueForStarKey (char *s, char *key, char *value, int maxsize);
void Info_Print (char *s);

byte	COM_BlockSequenceCheckByte (byte *base, int length, int sequence, unsigned mapchecksum);
byte	COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

int build_number( void );
