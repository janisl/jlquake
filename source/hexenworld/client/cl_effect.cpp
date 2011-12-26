
//**************************************************************************
//**
//** cl_effect.c
//**
//** Client side effects.
//**
//** $Header: /HexenWorld/Client/cl_effect.c 89    5/25/98 1:29p Mgummelt $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

void CL_UpdateEffects()
{
	float frametime = host_frametime;
	if (!frametime)
	{
		return;
	}

	for (int index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (!cl.h2_Effects[index].type)
		{
			continue;
		}
		CLHW_UpdateEffect(index, frametime);
	}
}
