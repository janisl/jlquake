// quakedef.h -- primary header for server

//define	PARANOID			// speed sapping error checking

#include <setjmp.h>

#include "../../common/qcommon.h"
#include "bothdefs.h"

#include "common.h"
#include "sys.h"

#include "net.h"

#include "server.h"

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct
{
	const char* basedir;
	int argc;
	char** argv;
} quakeparms_t;


//=============================================================================

//
// host
//
extern quakeparms_t host_parms;

extern qboolean host_initialized;			// true if into command execution
extern double realtime;					// not bounded in any way, changed at
										// start of every frame, never reset

void SV_Error(const char* error, ...);
void SV_Init(quakeparms_t* parms);
