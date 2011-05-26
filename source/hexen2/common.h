// comndef.h  -- general definitions

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

void COM_Init (char *path);
void COM_InitArgv2(int argc, char **argv);


//============================================================================

extern int com_filesize;
struct cache_user_s;

byte *COM_LoadStackFile (char *path, void *buffer, int bufsize);
byte *COM_LoadHunkFile (char *path);


extern	QCvar*	registered;
