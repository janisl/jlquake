// quakedef.h -- primary header for server

//define	PARANOID			// speed sapping error checking

#include "../../common/qcommon.h"
#include "bothdefs.h"

#include "common.h"

#include "server.h"

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

void SV_Error(const char* error, ...);
void COM_InitServer(quakeparms_t* parms);
