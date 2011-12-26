// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static sfxHandle_t		cl_sfx_buzzbee;

static sfxHandle_t		cl_sfx_hammersound;
static sfxHandle_t		cl_sfx_tornado;



/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	CLH2_InitTEntsCommon();
	cl_sfx_buzzbee = S_RegisterSound("assassin/scrbfly.wav");

	cl_sfx_hammersound = S_RegisterSound("paladin/axblade.wav");
	cl_sfx_tornado = S_RegisterSound("crusader/tornado.wav");
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
			//S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, cl_sfx_hammersound, 1, 1);
			S_StartSound(ent->origin, edict_num, 2, cl_sfx_hammersound, 1, 1);
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
			S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, cl_sfx_buzzbee, 1, 1);
//		}
	}
}

void CL_UpdateIceStorm(refEntity_t *ent, int edict_num)
{
	vec3_t			center, side1;
	vec3_t			side2 = {160, 160, 128};
	h2entity_state_t	*state;
	static float	playIceSound = .6;

	state = CLH2_FindState(edict_num);
	if (state)
	{
		VectorCopy(state->origin, center);

		// handle the particles
		VectorCopy(center, side1);
		side1[0] -= 80;
		side1[1] -= 80;
		side1[2] += 104;
		CLH2_RainEffect2(side1, side2, rand()%400-200, rand()%400-200, rand()%15 + 9*16, (int)(30*20*host_frametime));

		playIceSound+=host_frametime;
		if(playIceSound >= .6)
		{
			S_StartSound(center, CLH2_TempSoundChannel(), 0, clh2_sfx_icestorm, 1, 1);
			playIceSound -= .6;
		}
	}

	// toss little ice chunks
	if(rand()%100 < host_frametime * 100.0 * 3)
	{
		CLHW_CreateIceChunk(ent->origin);
	}
}
