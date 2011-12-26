// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static sfxHandle_t		clh2_sfx_buzzbee;

static sfxHandle_t		clh2_sfx_hammersound;

/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	CLH2_InitTEntsCommon();
	clh2_sfx_buzzbee = S_RegisterSound("assassin/scrbfly.wav");

	clh2_sfx_hammersound = S_RegisterSound("paladin/axblade.wav");
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

//*****************************************************************************
//
//						FLAG UPDATE FUNCTIONS
//
//*****************************************************************************

void CL_UpdateHammer(refEntity_t *ent, int edict_num)
{
	int testVal, testVal2;
	// do this every .3 seconds
	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%3))
		{
			//S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, clh2_sfx_hammersound, 1, 1);
			S_StartSound(ent->origin, edict_num, 2, clh2_sfx_hammersound, 1, 1);
		}
	}
}

void CL_UpdateBug(refEntity_t *ent)
{
	int testVal, testVal2;
	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
	// do this every .1 seconds
//		if(!(testVal%3))
//		{
			S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, clh2_sfx_buzzbee, 1, 1);
//		}
	}
}
