// comndef.h  -- general definitions

//============================================================================

struct qhlink_t;

void ClearLink(qhlink_t* l);
void RemoveLink(qhlink_t* l);
void InsertLinkBefore(qhlink_t* l, qhlink_t* before);
void InsertLinkAfter(qhlink_t* l, qhlink_t* after);

// (type *)STRUCT_FROM_LINK(qhlink_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,h2entity_t,order)
// FIXME: remove this mess!
#define STRUCT_FROM_LINK(l,t,m) ((t*)((byte*)l - (qintptr) & (((t*)0)->m)))

//============================================================================

void COM_Init(const char* path);
void COM_InitArgv2(int argc, char** argv);


//============================================================================

extern int com_filesize;

byte* COM_LoadStackFile(const char* path, void* buffer, int bufsize);
byte* COM_LoadHunkFile(const char* path);


extern Cvar* registered;
