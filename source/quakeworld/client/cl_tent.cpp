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
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos;
	int		rnd;
	q1explosion_t	*ex;
	int		cnt;

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
		
	case TE_EXPLOSION:			// rocket explosion
	// particles
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_ParticleExplosion (pos);
		
	// light
		CLQ1_ExplosionLight(pos);
	
	// sound
		S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
	
	// sprite
		ex = CLQ1_AllocExplosion ();
		VectorCopy (pos, ex->origin);
		ex->start = cl.serverTimeFloat;
		ex->model = R_RegisterModel("progs/s_explod.spr");
		break;
		
	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_BlobExplosion (pos);

		S_StartSound(pos, -1, 0, clq1_sfx_r_exp3, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/bolt.mdl"));
		break;
	
	case TE_LIGHTNING2:				// lightning bolts
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/bolt2.mdl"));
		break;
	
	case TE_LIGHTNING3:				// lightning bolts
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/bolt3.mdl"));
		break;
	
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

	case TE_GUNSHOT:			// bullet hitting wall
		cnt = net_message.ReadByte ();
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 0, 20*cnt);
		break;
		
	case TE_BLOOD:				// bullets hitting body
		cnt = net_message.ReadByte ();
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 73, 20*cnt);
		break;

	case TE_LIGHTNINGBLOOD:		// lightning hitting body
		pos[0] = net_message.ReadCoord ();
		pos[1] = net_message.ReadCoord ();
		pos[2] = net_message.ReadCoord ();
		CLQ1_RunParticleEffect (pos, vec3_origin, 225, 50);
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
	CLQ1_UpdateBeams ();
	CLQ1_UpdateExplosions ();
}
