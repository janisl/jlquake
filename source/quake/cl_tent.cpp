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
	int type = net_message.ReadByte();
	switch (type)
	{
	case Q1TE_WIZSPIKE:			// spike hitting wall
		CLQ1_ParseWizSpike(net_message);
		break;
		
	case Q1TE_KNIGHTSPIKE:			// spike hitting wall
		CLQ1_ParseKnightSpike(net_message);
		break;
		
	case Q1TE_SPIKE:			// spike hitting wall
		CLQ1_ParseSpike(net_message);
		break;
	case Q1TE_SUPERSPIKE:			// super spike hitting wall
		CLQ1_ParseSuperSpike(net_message);
		break;
		
	case Q1TE_EXPLOSION:			// rocket explosion
		CLQ1_ParseExplosion(net_message);
		break;
		
	case Q1TE_TAREXPLOSION:			// tarbaby explosion
		CLQ1_ParseTarExplosion(net_message);
		break;

	case Q1TE_EXPLOSION2:				// color mapped explosion
		CLQ1_ParseExplosion2(net_message);
		break;
		
	case Q1TE_LIGHTNING1:				// lightning bolts
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/bolt.mdl"));
		break;
	
	case Q1TE_LIGHTNING2:				// lightning bolts
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/bolt2.mdl"));
		break;
	
	case Q1TE_LIGHTNING3:				// lightning bolts
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/bolt3.mdl"));
		break;
	
	case Q1TE_BEAM:				// grappling hook beam
		CLQ1_ParseBeam(net_message, R_RegisterModel("progs/beam.mdl"));
		break;

	case Q1TE_LAVASPLASH:	
		CLQ1_ParseLavaSplash(net_message);
		break;
	
	case Q1TE_TELEPORT:
		CLQ1_ParseTeleportSplash(net_message);
		break;
		
	case Q1TE_GUNSHOT:			// bullet hitting wall
		CLQ1_ParseGunShot(net_message);
		break;
		
	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}
