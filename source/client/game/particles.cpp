//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../client.h"

cparticle_t particles[MAX_PARTICLES];
int cl_numparticles = MAX_PARTICLES;
cparticle_t* active_particles;
cparticle_t* free_particles;

Cvar* snow_flurry;
Cvar* snow_active;

vec3_t avelocities[NUMVERTEXNORMALS];

static unsigned d_8to24TranslucentTable[256];

void CL_ClearParticles()
{
	if ((GGameType & GAME_Hexen2) && !(GGameType & GAME_HexenWorld))
	{
		snow_flurry = Cvar_Get("snow_flurry", "1", CVAR_ARCHIVE);
		snow_active = Cvar_Get("snow_active", "1", CVAR_ARCHIVE);
	}

	active_particles = NULL;
	free_particles = &particles[0];
	for (int i = 0; i < cl_numparticles - 1; i++)
	{
		particles[i].next = &particles[i + 1];
	}
	particles[cl_numparticles - 1].next = NULL;

	unsigned* table = d_8to24TranslucentTable;
	for (int i = 0; i < 16; i++)
	{
		int c = ColorIndex[i];

		int r = r_palette[c][0];
		int g = r_palette[c][1];
		int b = r_palette[c][2];

		for (int p = 0; p < 16; p++)
		{
			int v = (ColorPercent[15 - p] << 24) + (r << 0) + (g << 8) + (b << 16);
			*table++ = v;
		}
	}
}

cparticle_t* CL_AllocParticle()
{
	if (!free_particles)
	{
		return NULL;
	}
	cparticle_t* p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;
	p->time = cl_common->serverTime;
	return p;
}

static void CL_AddParticle(cparticle_t* p, cparticle_t*& active, cparticle_t*& tail)
{
	float time = (cl_common->serverTime - p->time) * 0.001;
	float alpha;
	if (p->type == pt_q2static)
	{
		if (p->alphavel != INSTANT_PARTICLE)
		{
			alpha = p->alpha + time * p->alphavel;
			if (alpha <= 0)
			{
				// faded out
				p->next = free_particles;
				free_particles = p;
				return;
			}
		}
		else
		{
			alpha = p->alpha;
		}
	}
	else
	{
		if (p->die - cl_common->serverTime < 0)
		{
			p->next = free_particles;
			free_particles = p;
			return;
		}
	}

	p->next = NULL;
	if (!tail)
	{
		active = p;
	}
	else
	{
		tail->next = p;
	}
	tail = p;

	if (p->color < 0 || p->color > 511)
	{
		Log::write("Invalid color for particle type %d\n", (int)p->type);
		return;
	}

	const byte* colour;
	byte theAlpha;
	if (p->type == pt_q2static)
	{
		if (alpha > 1.0)
		{
			alpha = 1;
		}

		colour = r_palette[p->color];
		theAlpha = (int)(alpha * 255);
	}
	else if (p->color <= 255)
	{
		colour = r_palette[p->color];
		if (((GGameType & GAME_QuakeWorld) && p->type == pt_q1fire) ||
			((GGameType & GAME_HexenWorld) && p->type == pt_h2fire))
		{
			theAlpha = 255 * (6 - p->ramp) / 6;
		}
		else
		{
			theAlpha = 255;
		}
	}
	else
	{
		colour = (byte *)&d_8to24TranslucentTable[p->color - 256];
		theAlpha = colour[3];
	}

	float time2 = time * time;

	vec3_t origin;
	if (p->type == pt_q2static)
	{
		origin[0] = p->org[0] + p->vel[0] * time + p->accel[0] * time2;
		origin[1] = p->org[1] + p->vel[1] * time + p->accel[1] * time2;
		origin[2] = p->org[2] + p->vel[2] * time + p->accel[2] * time2;
	}
	else
	{
		VectorCopy(p->org, origin);
	}

	QParticleTexture texture;
	float size;
	if (p->type == pt_h2snow)
	{
		if (p->count>=69)
		{
			texture = PARTTEX_Snow1;
		}
		else if (p->count>=40)
		{
			texture = PARTTEX_Snow2;
		}
		else if (p->count>=30)
		{
			texture = PARTTEX_Snow3;
		}
		else
		{
			texture = PARTTEX_Snow4;
		}
		size = p->count / 10;
	}
	else
	{
		texture = PARTTEX_Default;
		size = 1;
	}

	if (p->type == pt_h2rain)
	{
		float vel0 = p->vel[0] * .001;
		float vel1 = p->vel[1] * .001;
		float vel2 = p->vel[2] * .001;
		for (int i = 0; i < 4; i++)
		{
			R_AddParticleToScene(origin, colour[0], colour[1], colour[2], theAlpha, size, texture);

			origin[0] += vel0;
			origin[1] += vel1;
			origin[2] += vel2;
		}
	}
	else
	{
		R_AddParticleToScene(origin, colour[0], colour[1], colour[2], theAlpha, size, texture);
	}
}

void CL_AddParticles()
{
	cparticle_t* active = NULL;
	cparticle_t* tail = NULL;
	cparticle_t* next;

	for (cparticle_t* p = active_particles; p; p = next)
	{
		next = p->next;
		CL_AddParticle(p, active, tail);
	}

	active_particles = active;
}

static void CL_UpdateShowParticle(cparticle_t* p, int killTime, float frametime)
{
	if (p->vel[0] == 0 && p->vel[1] == 0 && p->vel[2] == 0)
	{
		//Stopped moving
		if (p->color == 256 + 31)//Most translucent white
		{
			//Go away
			p->die = killTime;
		}
		else
		{
			//Count fifty and fade in translucency once each time
			p->ramp += 1;
			if(p->ramp >= 7)
			{
				p->color += 1;//Get more translucent
				p->ramp = 0;
			}
		}
	}
	else
	{
		//FIXME: If flake going fast enough, can go through, do a check in increments ot 10, max?
		//if not in_bounds Get length of diff, add in increments of 4 & check solid

		if (Cvar_VariableValue("snow_flurry") == 1 && (rand() & 31))
		{
			//Add flurry movement
			float snow_speed = p->vel[0] * p->vel[0] + p->vel[1] * p->vel[1] + p->vel[2]*p->vel[2];
			snow_speed = sqrt(snow_speed);

			vec3_t save_vel;
			VectorCopy(p->vel, save_vel);

			save_vel[0] += ((rand() * (2.0 / RAND_MAX)) - 1) * 30;
			save_vel[1] += ((rand() * (2.0 / RAND_MAX)) - 1) * 30;
			if ((rand() & 7) || p->vel[2] > 10)
			{
				save_vel[2] += ((rand() * (2.0 / RAND_MAX)) - 1) * 30;
			}
			VectorNormalize(save_vel);
			VectorScale(save_vel, snow_speed, p->vel);//retain speed but use new dir
		}

		vec3_t diff;
		VectorScale(p->vel, frametime, diff);
		VectorAdd(p->org, diff, p->org);
		
		if (p->flags & SFL_IN_BOUNDS)
		{
			//Always stay inside the boundry!
			if (p->org[0] < p->minOrg[0] || p->org[0] > p->maxOrg[0] ||
				p->org[1] < p->minOrg[1] || p->org[1] > p->maxOrg[1] ||
				p->org[2] < p->minOrg[2] || p->org[2] > p->maxOrg[2])
			{
				p->die = killTime;
			}
		}
		else
		{
			//IF hit solid, go to last position, no velocity, fade out.
			int contents = CM_PointContentsQ1(p->org, 0);
			if (contents != BSP29CONTENTS_EMPTY)
			{
				if (p->flags & SFL_NO_MELT)
				{
					//Don't melt, just die
					p->die = killTime;
				}
				else
				{
					//still have small prob of snow melting on emitter
					VectorScale(diff, 0.2, p->vel);
					int i = 6;
					while (contents != BSP29CONTENTS_EMPTY)
					{
						p->org[0] -= p->vel[0];
						p->org[1] -= p->vel[1];
						p->org[2] -= p->vel[2];
						i--;//no infinite loops
						if (!i)
						{
							p->die = killTime;	//should never happen now!
							break;
						}
						contents = CM_PointContentsQ1(p->org, 0);
					}
					p->vel[0] = p->vel[1] = p->vel[2] = 0;
					p->ramp = 0;
				}
			}
		}
	}
}

static void CL_UpdateParticle(cparticle_t* p, float frametime, float time1, float time2, float time3, float time4, int killTime, float grav, float dvel, float percent, float grav2)
{
	if (p->type != pt_h2snow && p->type != pt_q2static)
	{
		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
	}

	switch (p->type)
	{
	case pt_q1static:
		break;

	case pt_q1fire:
		p->ramp += time1;
		if (p->ramp >= 6)
		{
			p->die = killTime;
		}
		else
		{
			p->color = q1ramp3[(int)p->ramp];
		}
		p->vel[2] += grav;
		break;

	case pt_q1explode:
		p->ramp += time2;
		if (p->ramp >= 8)
		{
			p->die = killTime;
		}
		else
		{
			p->color = q1ramp1[(int)p->ramp];
		}
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] += p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_q1explode2:
		p->ramp += time3;
		if (p->ramp >= 8)
		{
			p->die = killTime;
		}
		else
		{
			p->color = q1ramp2[(int)p->ramp];
		}
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] -= p->vel[i] * frametime;
		}
		p->vel[2] -= grav;
		break;

	case pt_q1blob:
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] += p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_q1blob2:
		for (int i = 0; i < 2; i++)
		{
			p->vel[i] -= p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_q1grav:
	case pt_q1slowgrav:
		p->vel[2] -= grav;
		break;

	case pt_h2static:
		break;

	case pt_h2fire:
		p->ramp += time1;
		if ((int)p->ramp >= 6)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp3[(int)p->ramp];
		}
		p->vel[2] += grav;
		break;

	case pt_h2explode:
		p->ramp += time2;
		if ((int)p->ramp >= 8)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp1[(int)p->ramp];
		}
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] += p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_h2explode2:
		p->ramp += time3;
		if ((int)p->ramp >= 8)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp2[(int)p->ramp];
		}
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] -= p->vel[i] * frametime;
		}
		p->vel[2] -= grav;
		break;

	case pt_h2c_explode:
		p->ramp += time2;
		if ((int)p->ramp >= 8 || p->color <= 0)
		{
			p->die = killTime;
		}
		else if (time2)
		{
			p->color--;
		}
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] += p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_h2c_explode2:
		p->ramp += time3;
		if ((int)p->ramp >= 8 || p->color <= 1)
		{
			p->die = killTime;
		}
		else if (time3)
		{
			p->color -= 2;
		}
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] -= p->vel[i] * frametime;
		}
		p->vel[2] -= grav;
		break;

	case pt_h2blob:
		break;

	case pt_h2blob2:
		break;

	case pt_h2blob_hw:
		for (int i = 0; i < 3; i++)
		{
			p->vel[i] += p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_h2blob2_hw:
		for (int i = 0; i < 2; i++)
		{
			p->vel[i] -= p->vel[i] * dvel;
		}
		p->vel[2] -= grav;
		break;

	case pt_h2grav:
	case pt_h2slowgrav:
		p->vel[2] -= grav;
		break;

	case pt_h2fastgrav:
		p->vel[2] -= grav * 4;
		break;

	case pt_h2rain:
		break;

	case pt_h2snow:
		CL_UpdateShowParticle(p, killTime, frametime);
		break;

	case pt_h2fireball:
		p->ramp += time3;
		if ((int)p->ramp >= 16)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp4[(int)p->ramp];
		}
		break;

	case pt_h2acidball:
		p->ramp += time4 * 1.4;
		if ((int)p->ramp >= 23)
		{
			p->die = killTime;
		}
		else if ((int)p->ramp >= 15)
		{
			p->color = h2ramp11[(int)p->ramp - 15];
		}
		else
		{
			p->color = h2ramp10[(int)p->ramp];
		}
		p->vel[2] -= grav;
		break;

	case pt_h2spit:
		p->ramp += time3;
		if ((int)p->ramp >= 16)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp6[(int)p->ramp];
		}
		break;

	case pt_h2ice:
		p->ramp += time4;
		if ((int)p->ramp >= 16)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp5[(int)p->ramp];
		}
		p->vel[2] -= grav;
		break;

	case pt_h2spell:
		p->ramp += time2;
		if ((int)p->ramp >= 16)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp7[(int)p->ramp];
		}
		break;

	case pt_h2test:
		p->vel[2] += 1.3;
		p->ramp += time3;
		if ((int)p->ramp >= 13 || ((int)p->ramp > 10 && p->vel[2] < 20))
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp8[(int)p->ramp];
		}
		break;

	case pt_h2quake:
		p->vel[0] *= 1.05;
		p->vel[1] *= 1.05;
		p->vel[2] -= grav * 4;
		if (GGameType & GAME_HexenWorld)
		{
			if (p->color < 160 && p->color > 143)
			{
				p->color = 152 + 7 * ((p->die - cl_common->serverTime) * 2 / 1000);
			}
			if (p->color < 144 && p->color > 127)
			{
				p->color = 136 + 7 * ((p->die - cl_common->serverTime) * 2 / 1000);
			}
		}
		break;

	case pt_h2rd:
		if (!frametime)
		{
			break;
		}

		p->ramp += percent;
		if ((int)p->ramp > 50) 
		{
			p->ramp = 50;
			p->die = killTime;
		}
		p->color = 256 + 16 + 16 - (p->ramp / (50 / 16));

		{
			vec3_t diff;
			VectorSubtract(rider_origin, p->org, diff);

			float vel0 = 1 / (51 - p->ramp);
			p->org[0] += diff[0] * vel0;
			p->org[1] += diff[1] * vel0;
			p->org[2] += diff[2] * vel0;
		}
		break;

	case pt_h2gravwell:
		if (!frametime)
		{
			break;
		}

		p->ramp += percent;
		if ((int)p->ramp > 35) 
		{
			p->ramp = 35;
			p->die = killTime;
		}

		{
			vec3_t diff;
			VectorSubtract(rider_origin, p->org, diff);

			float vel0 = 1 / (36 - p->ramp);
			p->org[0] += diff[0] * vel0;
			p->org[1] += diff[1] * vel0;
			p->org[2] += diff[2] * vel0;
		}
		break;

	case pt_h2vorpal:
		--p->color; 
		if (p->color <= 37 + 256)
		{
			p->die = killTime;
		}
		break;

	case pt_h2setstaff:
		p->ramp += time1;
		if ((int)p->ramp >= 16)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp9[(int)p->ramp];
		}

		p->vel[0] *= 1.08 * percent;
		p->vel[1] *= 1.08 * percent;
		p->vel[2] -= grav2;
		break;

	case pt_h2redfire:
		p->ramp += frametime * 3;
		if ((int)p->ramp >= 8)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp12[(int)p->ramp] + 256;
		}

		p->vel[0] *= .9;
		p->vel[1] *= .9;
		p->vel[2] += grav / 2;
		break;

	case pt_h2bluestep:
		p->ramp += frametime * 8;
		if ((int)p->ramp >= 16)
		{
			p->die = killTime;
		}
		else
		{
			p->color = h2ramp13[(int)p->ramp] + 256;
		}

		p->vel[0] *= .9;
		p->vel[1] *= .9;
		p->vel[2] += grav;
		break;

	case pt_h2magicmissile:
		--p->color; 
		if (p->color < 149)
		{
			p->color = 149;
		}
		p->ramp += time1;
		if ((int)p->ramp > 16)
		{
			p->die = killTime;
		}
		break;

	case pt_h2boneshard:
		--p->color; 
		if (p->color < 368)
		{
			p->die = killTime;
		}
		break;

	case pt_h2scarab:
		--p->color; 
		if (p->color < 250)
		{
			p->die = killTime;
		}
		break;

	case pt_h2darken:
		{
			if (GGameType & GAME_HexenWorld)
			{
				p->vel[2] -= grav * 2;	//Also gravity
				if (rand() & 1)
				{
					--p->color;
				}
			}
			else
			{
				p->vel[2] -= grav;	//Also gravity
				--p->color;
			}
			int colindex = 0;
			while (colindex < 224)
			{
				if (colindex == 192 || colindex == 200)
				{
					colindex += 8;
				}
				else
				{
					colindex += 16;
				}
				if (p->color == colindex)
				{
					p->die = killTime;
				}
			}
		}
		break;

	case pt_h2grensmoke:
		p->vel[0] += time3 * ((rand() % 3) - 1);
		p->vel[1] += time3 * ((rand() % 3) - 1);
		p->vel[2] += time3 * ((rand() % 3) - 1);
		break;

	case pt_q2static:
		if (p->alphavel == INSTANT_PARTICLE)
		{
			p->alphavel = 0.0;
			p->alpha = 0.0;
		}
		break;
	}
}

void CL_UpdateParticles(float gravityBase)
{
	float frametime = cls_common->frametime * 0.001;
	float time1 = frametime * 5;
	float time2 = frametime * 10;
	float time3 = frametime * 15;
	float time4 = frametime * 20;
	float dvel = 4 * frametime;
	int killTime = cl_common->serverTime - 1;
	float grav = frametime * gravityBase * 0.05;
	float grav2 = frametime * gravityBase * 0.025;
	float percent = frametime / HX_FRAME_TIME;

	for (cparticle_t* p = active_particles; p; p = p->next)
	{
		CL_UpdateParticle(p, frametime, time1, time2, time3, time4, killTime, grav, dvel, percent, grav2);
	}
}
