// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

void CLH2_UpdateTEnts()
{
	CLH2_UpdateExplosions();
	CLH2_UpdateStreams();
	CLHW_UpdateTargetBall();
}
