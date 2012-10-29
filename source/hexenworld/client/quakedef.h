// quakedef.h -- primary header for client

#include "../../common/qcommon.h"

//define	PARANOID			// speed sapping error checking

#include "bothdefs.h"

#include "common.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	int argc;
	const char** argv;
} quakeparms_t;


//=============================================================================

//
// host
//
extern quakeparms_t host_parms;

extern double host_frametime;
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void Host_Init(quakeparms_t* parms);
void Host_Frame(float time);
