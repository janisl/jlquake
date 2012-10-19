// client.h

// edict->movetype values
#define QHMOVETYPE_NONE         0		// never moves
#define QHMOVETYPE_ANGLENOCLIP  1
#define QHMOVETYPE_ANGLECLIP        2
#define QHMOVETYPE_WALK         3		// gravity
#define QHMOVETYPE_STEP         4		// gravity, special edge handling
#define QHMOVETYPE_FLY          5
#define QHMOVETYPE_TOSS         6		// gravity
#define QHMOVETYPE_PUSH         7		// no clip to world, push and crush
#define QHMOVETYPE_NOCLIP           8
#define QHMOVETYPE_FLYMISSILE       9		// extra size to monsters
#define QHMOVETYPE_BOUNCE           10
#define H2MOVETYPE_BOUNCEMISSILE    11		// bounce w/o gravity
#define H2MOVETYPE_FOLLOW           12		// track movement of aiment
#define H2MOVETYPE_PUSHPULL     13		// pushable/pullable object
#define H2MOVETYPE_SWIM         14		// should keep the object in water

//=============================================================================

extern qboolean nomaster;

//
// cl_main
//
void CL_Init(void);

void CLQHW_ReadPackets(void);

void CL_WriteToServer(hwusercmd_t* cmd);
