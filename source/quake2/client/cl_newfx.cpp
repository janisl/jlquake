/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_newfx.c -- MORE entity effects parsing and management

#include "client.h"

//=============
//=============
void CL_Flashlight (int ent, vec3_t pos)
{
	cdlight_t	*dl;

	dl = CL_AllocDlight (ent);
	VectorCopy (pos,  dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cl.serverTime + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}

/*
======
CL_ColorFlash - flash of light
======
*/
void CL_ColorFlash (vec3_t pos, int ent, int intensity, float r, float g, float b)
{
	cdlight_t	*dl;

	dl = CL_AllocDlight (ent);
	VectorCopy (pos,  dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cl.serverTime + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}

void CLQ2_HeatbeamPaticles(vec3_t start, vec3_t forward)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	vec3_t		right, up;
	int			i;
	float		c, s;
	vec3_t		dir;
	float		ltime;
	float		step = 32.0, rstep;
	float		start_pt;
	float		rot;
	float		variance;
	vec3_t		end;

	VectorMA (start, 4096, forward, end);

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	// FIXME - pmm - these might end up using old values?
//	MakeNormalVectors (vec, right, up);
	VectorSubtract(vec3_origin, cl.refdef.viewaxis[1], right);
	VectorCopy (cl.refdef.viewaxis[2], up);
	VectorMA (move, -0.5, right, move);
	VectorMA (move, -0.5, up, move);

	ltime = (float) cl.serverTime/1000.0;
	start_pt = fmod(ltime*96.0f,step);
	VectorMA (move, start_pt, vec, move);

	VectorScale (vec, step, vec);

//	Com_Printf ("%f\n", ltime);
	rstep = M_PI/10.0;
	for (i=start_pt ; i<len ; i+=step)
	{
		if (i>step*5) // don't bother after the 5th ring
			break;

		for (rot = 0; rot < M_PI*2; rot += rstep)
		{

			p = CL_AllocParticle();
			if (!p)
				return;

			p->type = pt_q2static;
			
			VectorClear (p->accel);
//			rot+= fmod(ltime, 12.0)*M_PI;
//			c = cos(rot)/2.0;
//			s = sin(rot)/2.0;
//			variance = 0.4 + ((float)rand()/(float)RAND_MAX) *0.2;
			variance = 0.5;
			c = cos(rot)*variance;
			s = sin(rot)*variance;
			
			// trim it so it looks like it's starting at the origin
			if (i < 10)
			{
				VectorScale (right, c*(i/10.0), dir);
				VectorMA (dir, s*(i/10.0), up, dir);
			}
			else
			{
				VectorScale (right, c, dir);
				VectorMA (dir, s, up, dir);
			}
		
			p->alpha = 0.5;
	//		p->alphavel = -1.0 / (1+frand()*0.2);
			p->alphavel = -1000.0;
	//		p->color = 0x74 + (rand()&7);
			p->color = 223 - (rand()&7);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + dir[j]*3;
	//			p->vel[j] = dir[j]*6;
				p->vel[j] = 0;
			}
		}
		VectorAdd (move, vec, move);
	}
}
