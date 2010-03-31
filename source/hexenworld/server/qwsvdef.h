// quakedef.h -- primary header for server

#define	QUAKE_GAME			// as opposed to utilities

//define	PARANOID			// speed sapping error checking

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "bothdefs.h"

#include "common.h"
#include "bspfile.h"
#include "sys.h"
#include "zone.h"
#include "mathlib.h"

#include "cvar.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
typedef void efrag_t;
#include "gl_model.h"
#include "crc.h"
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

extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

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
