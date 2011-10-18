// chase.c -- chase camera code

#include "quakedef.h"

Cvar*	chase_back;
Cvar*	chase_up;
Cvar*	chase_right;
Cvar*	chase_active;

vec3_t	chase_pos;
vec3_t	chase_angles;

vec3_t	chase_dest;
vec3_t	chase_dest_angles;


void Chase_Init (void)
{
	chase_back = Cvar_Get("chase_back", "100", 0);
	chase_up = Cvar_Get("chase_up", "16", 0);
	chase_right = Cvar_Get("chase_right", "0", 0);
	chase_active = Cvar_Get("chase_active", "0", 0);
}

void Chase_Reset (void)
{
	// for respawning and teleporting
//	start position 12 units behind head
}

void TraceLine (vec3_t start, vec3_t end, vec3_t impact)
{
	q1trace_t	trace;

	Com_Memset(&trace, 0, sizeof(trace));
	CM_HullCheckQ1(0, start, end, &trace);

	VectorCopy (trace.endpos, impact);
}

void Chase_Update (void)
{
	int		i;
	vec3_t	forward, up, right;
	vec3_t	dest, stop;


	// if can't see player, reset
	AngleVectors (cl.viewangles, forward, right, up);

	// calc exact destination
	for (i=0 ; i<3 ; i++)
		chase_dest[i] = cl.refdef.vieworg[i] 
		- forward[i]*chase_back->value
		- right[i]*chase_right->value;
	chase_dest[2] = cl.refdef.vieworg[2] + chase_up->value;

	// find the spot the player is looking at
	VectorMA (cl.refdef.vieworg, 4096, forward, dest);
	TraceLine (cl.refdef.vieworg, dest, stop);

	// calculate pitch to look at the same spot from camera
	VectorSubtract(stop, cl.refdef.vieworg, cl.refdef.viewaxis[0]);
	VectorNormalize(cl.refdef.viewaxis[0]);
	CrossProduct(cl.refdef.viewaxis[2], cl.refdef.viewaxis[0], cl.refdef.viewaxis[1]);

	// move towards destination
	VectorCopy (chase_dest, cl.refdef.vieworg);
}

