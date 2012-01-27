// comndef.h  -- general definitions

#define	MAX_SERVERINFO_STRING	512
#define	MAX_LOCALINFO_STRING	32768

//============================================================================

struct qhlink_t;

void ClearLink (qhlink_t *l);
void RemoveLink (qhlink_t *l);
void InsertLinkBefore (qhlink_t *l, qhlink_t *before);
void InsertLinkAfter (qhlink_t *l, qhlink_t *after);

// (type *)STRUCT_FROM_LINK(qhlink_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,h2entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (qintptr)&(((t *)0)->m)))

//============================================================================

void MSG_WriteUsercmd (QMsg *sb, hwusercmd_t* cmd, qboolean long_msg);

void MSG_ReadUsercmd (hwusercmd_t* cmd, qboolean long_msg);

//============================================================================

void COM_Init (const char *path);
void COM_InitArgv2(int argc, char **argv);


//============================================================================

extern int com_filesize;

byte *COM_LoadStackFile (const char *path, void *buffer, int bufsize);
byte *COM_LoadHunkFile (const char *path);
void COM_Gamedir (char *dir);

extern	Cvar*	registered;
extern qboolean		standard_quake, rogue, hipnotic;

extern qboolean cl_siege;
extern byte cl_fraglimit;
extern float cl_timelimit;
extern float cl_server_time_offset;