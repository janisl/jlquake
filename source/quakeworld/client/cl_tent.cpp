/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_tent.c -- client side temporary entities

#include "quakedef.h"


#define	MAX_BEAMS	8
typedef struct
{
	int			entity;
	qhandle_t	model;
	float		endtime;
	vec3_t		start, end;
} beam_t;

beam_t		cl_beams[MAX_BEAMS];

#define	MAX_EXPLOSIONS	8
typedef struct
{
	vec3_t		origin;
	float		start;
	qhandle_t	model;
} explosion_t;

explosion_t	cl_explosions[MAX_EXPLOSIONS];


static sfxHandle_t	cl_sfx_wizhit;
static sfxHandle_t	cl_sfx_knighthit;
static sfxHandle_t	cl_sfx_tink1;
static sfxHandle_t	cl_sfx_ric1;
static sfxHandle_t	cl_sfx_ric2;
static sfxHandle_t	cl_sfx_ric3;
static sfxHandle_t	cl_sfx_r_exp3;

/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	cl_sfx_wizhit = S_RegisterSound("wizard/hit.wav");
	cl_sfx_knighthit = S_RegisterSound("hknight/hit.wav");
	cl_sfx_tink1 = S_RegisterSound("weapons/tink1.wav");
	cl_sfx_ric1 = S_RegisterSound("weapons/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound("weapons/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound("weapons/ric3.wav");
	cl_sfx_r_exp3 = S_RegisterSound("weapons/r_exp3.wav");
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	Com_Memset(&cl_beams, 0, sizeof(cl_beams));
	Com_Memset(&cl_explosions, 0, sizeof(cl_explosions));
}

/*
=================
CL_AllocExplosion
=================
*/
explosion_t *CL_AllocExplosion (void)
{
	int		i;
	float	time;
	int		index;
	
	for (i=0 ; i<MAX_EXPLOSIONS ; i++)
		if (!cl_explosions[i].model)
			return &cl_explosions[i];
// find the oldest explosion
	time = cl.time;
	index = 0;

	for (i=0 ; i<MAX_EXPLOSIONS ; i++)
		if (cl_explosions[i].start < time)
		{
			time = cl_explosions[i].start;
			index = i;
		}
	return &cl_explosions[index];
}

/*
=================
CL_ParseBeam
=================
*/
static void CL_ParseBeam (qhandle_t m)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	ent = net_message.ReadShort();
	
	start[0] = net_message.ReadCoord ();
	start[1] = net_message.ReadCoord ();
	start[2] = net_message.ReadCoord ();
	
	end[0] = net_message.ReadCoord ();
	end[1] = net_message.ReadCoord ();
	end[2] = net_message.ReadCoord ();

// override any beam with the same entity
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.time + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}
	}
	Con_Printf ("beam list overflow!\n");	
}

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos;
	dlight_t	*dl;
	int		rnd;
	explosion_t	*ex;
	int		cnt;

	type = net_message.ReadByte ();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 20, 30);
		S_StartSound(pos, -1, 0, cl_sfx_wizhit, 1, 1);
		break;
		
	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 226, 20);
		S_StartSound(pos, -1, 0, cl_sfx_knighthit, 1, 1);
		break;
		
	case TE_SPIKE:			// spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound(pos, -1, 0, cl_sfx_tink1, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(pos, -1, 0, cl_sfx_ric1, 1, 1);
			else if (rnd == 2)
				S_StartSound(pos, -1, 0, cl_sfx_ric2, 1, 1);
			else
				S_StartSound(pos, -1, 0, cl_sfx_ric3, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound(pos, -1, 0, cl_sfx_tink1, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(pos, -1, 0, cl_sfx_ric1, 1, 1);
			else if (rnd == 2)
				S_StartSound(pos, -1, 0, cl_sfx_ric2, 1, 1);
			else
				S_StartSound(pos, -1, 0, cl_sfx_ric3, 1, 1);
		}
		break;
		
	case TE_EXPLOSION:			// rocket explosion
	// particles
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_ParticleExplosion (pos);
		
	// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 0.5;
		dl->decay = 300;
		dl->color[0] = 0.2;
		dl->color[1] = 0.1;
		dl->color[2] = 0.05;
		dl->color[3] = 0.7;
	
	// sound
		S_StartSound(pos, -1, 0, cl_sfx_r_exp3, 1, 1);
	
	// sprite
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->origin);
		ex->start = cl.time;
		ex->model = Mod_ForName("progs/s_explod.spr", true);
		break;
		
	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_BlobExplosion (pos);

		S_StartSound(pos, -1, 0, cl_sfx_r_exp3, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam(Mod_ForName("progs/bolt.mdl", true));
		break;
	
	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam(Mod_ForName("progs/bolt2.mdl", true));
		break;
	
	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam(Mod_ForName("progs/bolt3.mdl", true));
		break;
	
	case TE_LAVASPLASH:	
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_LavaSplash (pos);
		break;
	
	case TE_TELEPORT:
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_TeleportSplash (pos);
		break;

	case TE_GUNSHOT:			// bullet hitting wall
		cnt = net_message.ReadByte ();
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 0, 20*cnt);
		break;
		
	case TE_BLOOD:				// bullets hitting body
		cnt = net_message.ReadByte ();
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 73, 20*cnt);
		break;

	case TE_LIGHTNINGBLOOD:		// lightning hitting body
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		R_RunParticleEffect (pos, vec3_origin, 225, 50);
		break;

	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}

/*
=================
CL_UpdateBeams
=================
*/
void CL_UpdateBeams (void)
{
	int			i;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	float		yaw, pitch;
	float		forward;

// update lightning
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

	// if coming from the player, update the start position
		if (b->entity == cl.playernum+1)	// entity 0 is the world
		{
			VectorCopy (cl.simorg, b->start);
//			b->start[2] -= 22;	// adjust for view height
		}

	// calculate pitch and yaw
		VectorSubtract (b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (int) (atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}

	// add new entities for the lightning
		VectorCopy (b->start, org);
		d = VectorNormalize(dist);
		while (d > 0)
		{
			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));

			ent.reType = RT_MODEL;
			VectorCopy(org, ent.origin);
			ent.hModel = b->model;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand() % 360;
			CL_SetRefEntAxis(&ent, angles);
			R_AddRefEntToScene(&ent);

			for (i=0 ; i<3 ; i++)
				org[i] += dist[i]*30;
			d -= 30;
		}
	}
	
}

/*
=================
CL_UpdateExplosions
=================
*/
void CL_UpdateExplosions (void)
{
	int			i;
	int			f;
	explosion_t	*ex;

	for (i=0, ex=cl_explosions ; i< MAX_EXPLOSIONS ; i++, ex++)
	{
		if (!ex->model)
			continue;
		f = 10*(cl.time - ex->start);
		if (f >= Mod_GetNumFrames(ex->model))
		{
			ex->model = 0;
			continue;
		}

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));

		ent.reType = RT_MODEL;
		VectorCopy(ex->origin, ent.origin);
		ent.hModel = ex->model;
		ent.frame = f;
		CL_SetRefEntAxis(&ent, vec3_origin);
		R_AddRefEntToScene(&ent);
	}
}

/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	CL_UpdateBeams ();
	CL_UpdateExplosions ();
}
