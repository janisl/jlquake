// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	CLH2_InitTEntsCommon();
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	CLH2_UpdateExplosions ();
	CLH2_UpdateStreams();
	CLHW_UpdateTargetBall(v_targDist, v_targAngle, v_targPitch, cl.simorg);
}
