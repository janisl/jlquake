// comndef.h  -- general definitions

typedef unsigned char 		byte;
#define _DEF_BYTE_

// KJB Undefined true and false defined in SciTech's DEBUG.H header
#undef true
#undef false

typedef enum {false, true}	qboolean;

#define	MAX_INFO_STRING	196
#define	MAX_SERVERINFO_STRING	512
#define	MAX_LOCALINFO_STRING	32768

//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte	*data;
	int		maxsize;
	int		cursize;
} sizebuf_t;

void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, void *data, int length);
void SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

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

#ifndef NULL
#define NULL ((void *)0)
#endif

#define Q_MAXCHAR ((char)0x7f)
#define Q_MAXSHORT ((short)0x7fff)
#define Q_MAXINT	((int)0x7fffffff)
#define Q_MAXLONG ((int)0x7fffffff)
#define Q_MAXFLOAT ((int)0x7fffffff)

#define Q_MINCHAR ((char)0x80)
#define Q_MINSHORT ((short)0x8000)
#define Q_MININT 	((int)0x80000000)
#define Q_MINLONG ((int)0x80000000)
#define Q_MINFLOAT ((int)0x7fffffff)

//============================================================================

extern	qboolean		bigendien;

extern	short	(*BigShort) (short l);
extern	short	(*LittleShort) (short l);
extern	int	(*BigLong) (int l);
extern	int	(*LittleLong) (int l);
extern	float	(*BigFloat) (float l);
extern	float	(*LittleFloat) (float l);

//============================================================================

struct usercmd_s;

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteString (sizebuf_t *sb, char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSG_WriteUsercmd (sizebuf_t *sb, struct usercmd_s *cmd, qboolean long_msg);

extern	int			msg_readcount;
extern	qboolean	msg_badread;		// set if a read goes beyond end of message

void MSG_BeginReading (void);
int MSG_ReadChar (void);
int MSG_ReadByte (void);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
float MSG_ReadFloat (void);
char *MSG_ReadString (void);
char *MSG_ReadStringLine (void);

float MSG_ReadCoord (void);
float MSG_ReadAngle (void);
float MSG_ReadAngle16 (void);
void MSG_ReadUsercmd (struct usercmd_s *cmd, qboolean long_msg);

//============================================================================

/*
void Q_memset (void *dest, int fill, int count);
void Q_memcpy (void *dest, void *src, int count);
int Q_memcmp (void *m1, void *m2, int count);
void Q_strcpy (char *dest, char *src);
void Q_strncpy (char *dest, char *src, int count);
int Q_strlen (char *str);
char *Q_strrchr (char *s, char c);
void Q_strcat (char *dest, char *src);
int Q_strcmp (char *s1, char *s2);
int Q_strncmp (char *s1, char *s2, int count);
int	Q_atoi (char *str);
float Q_atof (char *str);
*/
static __inline int Q_strncasecmp (char *s1, char *s2, int n)
{
#ifdef _WIN32
	return strnicmp (s1, s2, n);
#else
	return strncasecmp (s1, s2, n);
#endif
}

static __inline int Q_strcasecmp (char *s1, char *s2)
{
#ifdef _WIN32
	return stricmp (s1, s2);
#else
	return strcasecmp (s1, s2);
#endif
}
#define Q_atoi	atoi
#define Q_atof	atof
#define strcmpi	Q_strcasecmp
#define Q_memset	memset
#define Q_strcpy	strcpy
#define Q_strncpy	strncpy
#define Q_strcmp	strcmp
#define Q_strrchr	strrchr
#define _strnicmp	Q_strncasecmp
#define _stricmp	Q_strcasecmp

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

char	*va(char *format, ...);
// does a varargs printf into a temp buffer


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