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

#include "quakedef.h"

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord();
	for (i=0 ; i<3 ; i++)
		dir[i] = net_message.ReadChar () * (1.0/16);
	count = net_message.ReadByte ();
	color = net_message.ReadByte ();

	if (count == 255)
	{
		// rocket explosion
		CLQ1_ParticleExplosion(org);
	}
	else
	{
		CLQ1_RunParticleEffect (org, dir, color, count);
	}
}

/*
===============
R_UpdateParticles
===============
*/
extern	Cvar*	sv_gravity;

void R_UpdateParticles (void)
{
	cparticle_t		*p;
	float			grav;
	int				i;
	float			time2, time3;
	float			time1;
	float			dvel;
	float			frametime;
	
	frametime = cl.serverTimeFloat - cl.oldtime;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * sv_gravity->value * 0.05;
	dvel = 4*frametime;
	int killTime = cl.serverTime - 1;

	for (p=active_particles ; p ; p=p->next)
	{
		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_q1static:
			break;
		case pt_q1fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = killTime;
			else
				p->color = q1ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_q1explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = killTime;
			else
				p->color = q1ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_q1explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = killTime;
			else
				p->color = q1ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_q1blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_q1blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_q1grav:
		case pt_q1slowgrav:
			p->vel[2] -= grav;
			break;
		}
	}
}
