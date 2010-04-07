// comndef.h  -- general definitions

#define	MAX_INFO_STRING	196
#define	MAX_SERVERINFO_STRING	512
#define	MAX_LOCALINFO_STRING	32768

//============================================================================

void *SZ_GetSpace (QMsg *buf, int length);
void SZ_Print (QMsg *buf, char *data);	// strcats onto the sizebuf

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

void MSG_WriteUsercmd (QMsg *sb, struct usercmd_s *cmd, qboolean long_msg);

void MSG_ReadUsercmd (struct usercmd_s *cmd, qboolean long_msg);

//============================================================================

/*
int Q_memcmp (void *m1, void *m2, int count);
*/

#define Com_Memset	memset
#define Com_Memcpy  memcpy

//============================================================================

extern	char		com_token[1024];
extern	qboolean	com_eof;

char *COM_Parse (char *data);


extern	int		com_argc;
extern	char	**com_argv;

int COM_CheckParm (char *parm);
void COM_AddParm (char *parm);

void COM_Init (char *path);
void COM_InitArgv (int argc, char **argv);

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
void COM_FileBase (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);


//============================================================================

extern int com_filesize;
struct cache_user_s;

extern	char	com_gamedir[MAX_OSPATH];
extern	char	com_savedir[MAX_OSPATH];

void COM_WriteFile (char *filename, void *data, int len);
int COM_FOpenFile (char *filename, FILE **file, qboolean override_pack);
void COM_CloseFile (FILE *h);

byte *COM_LoadStackFile (char *path, void *buffer, int bufsize);
byte *COM_LoadTempFile (char *path);
byte *COM_LoadHunkFile (char *path);
void COM_LoadCacheFile (char *path, struct cache_user_s *cu);
void COM_CreatePath (char *path);
void COM_Gamedir (char *dir);

extern	struct cvar_s	registered;
extern	struct cvar_s	oem;
extern qboolean		standard_quake, rogue, hipnotic;
extern qboolean		com_portals;

char *Info_ValueForKey (char *s, char *key);
void Info_RemoveKey (char *s, char *key);
void Info_RemovePrefixedKeys (char *start, char prefix);
void Info_SetValueForKey (char *s, char *key, char *value, int maxsize);
void Info_SetValueForStarKey (char *s, char *key, char *value, int maxsize);
void Info_Print (char *s);

extern qboolean cl_siege;
extern byte cl_fraglimit;
extern float cl_timelimit;
extern float cl_server_time_offset;