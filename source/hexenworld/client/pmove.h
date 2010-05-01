
#define	MAX_PHYSENTS	64
typedef struct
{
	vec3_t	origin;
	cmodel_t	*model;		// only for bsp models
	vec3_t	mins, maxs;	// only for non-bsp models
	vec3_t	angles;
	int		info;		// for client or server to identify
} physent_t;


typedef struct
{
	int		sequence;	// just for debugging prints

	// player state
	vec3_t	origin;
	vec3_t	angles;
	vec3_t	velocity;
	int		oldbuttons;
	float		waterjumptime;
	float		teleport_time;
	qboolean	dead;
	int		spectator;
	float	hasted;
	int movetype;
	qboolean crouched;

	// world state
	int		numphysent;
	physent_t	physents[MAX_PHYSENTS];	// 0 should be the world

	// input
	usercmd_t	cmd;

	// results
	int		numtouch;
	int		touchindex[MAX_PHYSENTS];
} playermove_t;

typedef struct {
	float	gravity;
	float	stopspeed;
	float	maxspeed;
	float	spectatormaxspeed;
	float	accelerate;
	float	airaccelerate;
	float	wateraccelerate;
	float	friction;
	float	waterfriction;
	float	entgravity;
} movevars_t;


extern	movevars_t		movevars;
extern	playermove_t	pmove;
extern	int		onground;
extern	int		waterlevel;
extern	int		watertype;

void PlayerMove (void);
void Pmove_Init (void);

qboolean PM_TestPlayerPosition(vec3_t point);
trace_t PM_PlayerMove(vec3_t start, vec3_t stop);
