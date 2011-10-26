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

/*
=================
CL_ParseBeam
=================
*/
static void CL_ParseBeam(qhandle_t m)
{
	int		ent;
	vec3_t	start, end;
	q1beam_t	*b;
	int		i;
	
	ent = net_message.ReadShort();
	
	start[0] = net_message.ReadCoord ();
	start[1] = net_message.ReadCoord ();
	start[2] = net_message.ReadCoord ();
	
	end[0] = net_message.ReadCoord ();
	end[1] = net_message.ReadCoord ();
	end[2] = net_message.ReadCoord ();

// override any beam with the same entity
	for (i=0, b=clq1_beams ; i< MAX_BEAMS_Q1 ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.serverTimeFloat + 0.2;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			return;
		}

// find a free beam
	for (i=0, b=clq1_beams ; i< MAX_BEAMS_Q1 ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTimeFloat)
		{
			b->entity = ent;
			b->model = m;
			b->endtime = cl.serverTimeFloat + 0.2;
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
	int		rnd;
	int		colorStart, colorLength;

	type = net_message.ReadByte ();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 20, 30);
		S_StartSound(pos, -1, 0, clq1_sfx_wizhit, 1, 1);
		break;
		
	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 226, 20);
		S_StartSound(pos, -1, 0, clq1_sfx_knighthit, 1, 1);
		break;
		
	case TE_SPIKE:			// spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 0, 10);
		if ( rand() % 5 )
			S_StartSound(pos, -1, 0, clq1_sfx_tink1, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(pos, -1, 0, clq1_sfx_ric1, 1, 1);
			else if (rnd == 2)
				S_StartSound(pos, -1, 0, clq1_sfx_ric2, 1, 1);
			else
				S_StartSound(pos, -1, 0, clq1_sfx_ric3, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound(pos, -1, 0, clq1_sfx_tink1, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound(pos, -1, 0, clq1_sfx_ric1, 1, 1);
			else if (rnd == 2)
				S_StartSound(pos, -1, 0, clq1_sfx_ric2, 1, 1);
			else
				S_StartSound(pos, -1, 0, clq1_sfx_ric3, 1, 1);
		}
		break;
		
	case TE_GUNSHOT:			// bullet hitting wall
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 0, 20);
		break;
		
	case TE_EXPLOSION:			// rocket explosion
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_ParticleExplosion (pos);
		CLQ1_ExplosionLight(pos);
		S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
		break;
		
	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_BlobExplosion (pos);

		S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CL_ParseBeam(R_RegisterModel("progs/bolt.mdl"));
		break;
	
	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam(R_RegisterModel("progs/bolt2.mdl"));
		break;
	
	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam(R_RegisterModel("progs/bolt3.mdl"));
		break;
	
// PGM 01/21/97 
	case TE_BEAM:				// grappling hook beam
		CL_ParseBeam(R_RegisterModel("progs/beam.mdl"));
		break;
// PGM 01/21/97

	case TE_LAVASPLASH:	
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_LavaSplash (pos);
		break;
	
	case TE_TELEPORT:
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_TeleportSplash (pos);
		break;
		
	case TE_EXPLOSION2:				// color mapped explosion
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		colorStart = net_message.ReadByte ();
		colorLength = net_message.ReadByte ();
		CLQ1_ParticleExplosion2 (pos, colorStart, colorLength);
		CLQ1_ExplosionLight(pos);
		S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
		break;
		
	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	int			i;
	q1beam_t		*b;
	vec3_t		dist, org;
	float		d;
	float		yaw, pitch;
	float		forward;

// update lightning
	for (i=0, b=clq1_beams ; i< MAX_BEAMS_Q1 ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTimeFloat)
			continue;

	// if coming from the player, update the start position
		if (b->entity == cl.viewentity)
		{
			VectorCopy (cl_entities[cl.viewentity].origin, b->start);
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
			VectorCopy (org, ent.origin);
			ent.hModel = b->model;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand()%360;
			CL_SetRefEntAxis(&ent, angles);
			R_AddRefEntityToScene(&ent);

			for (i=0 ; i<3 ; i++)
				org[i] += dist[i]*30;
			d -= 30;
		}
	}
	
}


