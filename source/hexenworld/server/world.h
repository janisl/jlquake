// world.h

#define	MOVE_NORMAL		0
#define	MOVE_NOMONSTERS	1
#define	MOVE_MISSILE	2
#define	MOVE_WATER		3
#define	MOVE_PHASE		4

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	qhlink_t	trigger_edicts;
	qhlink_t	solid_edicts;
} areanode_t;

#define	AREA_DEPTH	4
#define	AREA_NODES	32

extern	areanode_t	sv_areanodes[AREA_NODES];


void SV_ClearWorld (void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict (qhedict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
// flags ent->v.modified

void SV_LinkEdict (qhedict_t *ent, qboolean touch_triggers);
// Needs to be called any time an entity changes origin, mins, maxs, or solid
// flags ent->v.modified
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers

int SV_PointContents (vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all

qhedict_t	*SV_TestEntityPosition (qhedict_t *ent);

q1trace_t SV_Move (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, qhedict_t *passedict);
// mins and maxs are reletive

// if the entire move stays in a solid volume, trace.allsolid will be set

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// nomonsters is used for line of sight or edge testing, where mosnters
// shouldn't be considered solid objects

// passedict is explicitly excluded from clipping checks (normally NULL)


qhedict_t	*SV_TestPlayerPosition (qhedict_t *ent, vec3_t origin);
