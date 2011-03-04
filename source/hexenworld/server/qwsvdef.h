// quakedef.h -- primary header for server

#include "../../../libs/core/core.h"

//define	PARANOID			// speed sapping error checking

#include <setjmp.h>

#include "bothdefs.h"

#include "common.h"
#include "../../../libs/core/bsp29file.h"
#include "sys.h"
#include "zone.h"

#include "net.h"
#include "protocol.h"
#include "cmd.h"
typedef void efrag_t;
#include "gl_model.h"
#include "cl_effect.h"
#include "progs.h"

#include "server.h"
#include "world.h"
#include "pmove.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	char	*basedir;
	char	*cachedir;		// for development over ISDN lines
	int		argc;
	char	**argv;
	void	*membase;
	int		memsize;
} quakeparms_t;


//=============================================================================

//
// host
//
extern	quakeparms_t host_parms;

extern	QCvar*		sys_nostdout;
extern	QCvar*		developer;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void SV_Error (char *error, ...);
void SV_Init (quakeparms_t *parms);

void Con_Printf (char *fmt, ...);
void Con_DPrintf (char *fmt, ...);

extern	unsigned int defLosses;	// Defenders losses in Siege
extern	unsigned int attLosses;	// Attackers Losses in Siege

#define HULL_IMPLICIT		0	//Choose the hull based on bounding box- like in Quake
#define HULL_POINT			1	//0 0 0, 0 0 0
#define HULL_PLAYER			2	//'-16 -16 0', '16 16 56'
#define HULL_SCORPION		3	//'-24 -24 -20', '24 24 20'
#define HULL_CROUCH			4	//'-16 -16 0', '16 16 28'
#define HULL_HYDRA			5	//'-28 -28 -24', '28 28 24'
#define HULL_GOLEM			6	//???,???

void PR_LoadStrings(void);
