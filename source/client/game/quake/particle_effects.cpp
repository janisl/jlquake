//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../particles.h"
#include "../../client_main.h"
#include "../../../common/common_defs.h"

int q1ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
int q1ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
int q1ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };

void CLQ1_ParticleExplosion(const vec3_t origin)
{
	for (int i = 0; i < 1024; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 5000;
		p->color = q1ramp1[0];
		p->ramp = rand() & 3;
		p->type = i & 1 ? pt_q1explode : pt_q1explode2;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 512) - 256;
		}
	}
}

void CLQ1_BlobExplosion(const vec3_t origin)
{
	for (int i = 0; i < 1024; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 1000 + (rand() & 8) * 50;
		if (i & 1)
		{
			p->type = pt_q1blob;
			p->color = 66 + rand() % 6;
		}
		else
		{
			p->type = pt_q1blob2;
			p->color = 150 + rand() % 6;
		}
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 512) - 256;
		}
	}
}

//	Not present in QuakeWorld
void CLQ1_ParticleExplosion2(const vec3_t origin, int colorStart, int colorLength)
{
	int colorMod = 0;
	for (int i = 0; i < 512; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 300;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;
		p->type = pt_q1blob;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 512) - 256;
		}
	}
}

static void RunParticleEffect(const vec3_t origin, const vec3_t direction, int colour, int count, int scale, ptype_t type)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 100 * (rand() % 5);
		p->color = (colour & ~7) + (rand() & 7);
		p->type = type;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + scale * ((rand() & 15) - 8);
			p->vel[j] = direction[j] * 15;
		}
	}
}

void CLQ1_RunParticleEffect(const vec3_t origin, const vec3_t direction, int colour, int count)
{
	if (!(GGameType & GAME_QuakeWorld))
	{
		RunParticleEffect(origin, direction, colour, count, 1, pt_q1slowgrav);
	}
	else
	{
		if (count > 130)
		{
			RunParticleEffect(origin, direction, colour, count, 3, pt_q1grav);
		}
		else if (count > 20)
		{
			RunParticleEffect(origin, direction, colour, count, 2, pt_q1grav);
		}
		else
		{
			RunParticleEffect(origin, direction, colour, count, 1, pt_q1grav);
		}
	}
}

void CLQ1_LavaSplash(const vec3_t origin)
{
	for (int i = -16; i < 16; i++)
	{
		for (int j = -16; j < 16; j++)
		{
			for (int k = 0; k < 1; k++)
			{
				cparticle_t* p = CL_AllocParticle();
				if (!p)
				{
					return;
				}
				p->die = cl.serverTime + 2000 + (rand() & 31) * 20;
				p->color = 224 + (rand() & 7);
				p->type = GGameType & GAME_QuakeWorld ? pt_q1grav : pt_q1slowgrav;

				vec3_t dir;
				dir[0] = j * 8 + (rand() & 7);
				dir[1] = i * 8 + (rand() & 7);
				dir[2] = 256;

				p->org[0] = origin[0] + dir[0];
				p->org[1] = origin[1] + dir[1];
				p->org[2] = origin[2] + (rand() & 63);

				VectorNormalize(dir);
				float vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);
			}
		}
	}
}

void CLQ1_TeleportSplash(const vec3_t origin)
{
	for (int i = -16; i < 16; i += 4)
	{
		for (int j = -16; j < 16; j += 4)
		{
			for (int k = -24; k < 32; k += 4)
			{
				cparticle_t* p = CL_AllocParticle();
				if (!p)
				{
					return;
				}
				p->die = cl.serverTime + 200 + (rand() & 7) * 20;
				p->color = 7 + (rand() & 7);
				p->type = GGameType & GAME_QuakeWorld ? pt_q1grav : pt_q1slowgrav;

				vec3_t dir;
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				p->org[0] = origin[0] + i + (rand() & 3);
				p->org[1] = origin[1] + j + (rand() & 3);
				p->org[2] = origin[2] + k + (rand() & 3);

				VectorNormalize(dir);
				float vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);
			}
		}
	}
}

//	Not present in QuakeWorld
void CLQ1_BrightFieldParticles(const vec3_t origin)
{
	float dist = 64;
	float beamlength = 16;

	if (!avelocities[0][0])
	{
		for (int i = 0; i < NUMVERTEXNORMALS * 3; i++)
		{
			avelocities[0][i] = (rand() & 255) * 0.01;
		}
	}

	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		float angle = cl.serverTime * avelocities[i][0] / 1000.0;
		float sy = sin(angle);
		float cy = cos(angle);
		angle = cl.serverTime * avelocities[i][1] / 1000.0;
		float sp = sin(angle);
		float cp = cos(angle);

		vec3_t forward;
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		p->die = cl.serverTime + 10;
		p->color = 0x6f;
		p->type = pt_q1explode;

		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * beamlength;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * beamlength;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * beamlength;
	}
}

void CLQ1_TrailParticles(vec3_t start, const vec3_t end, int type)
{
	vec3_t vec;
	float len;
	int j;
	cparticle_t* p;
	static int tracercount;

	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);
	while (len > 0)
	{
		len -= 3;

		p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		VectorCopy(vec3_origin, p->vel);
		p->die = cl.serverTime + 2000;

		switch (type)
		{
		case 0:	// rocket trail
			p->ramp = (rand() & 3);
			p->color = q1ramp3[(int)p->ramp];
			p->type = pt_q1fire;
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
			break;

		case 1:	// smoke smoke
			p->ramp = (rand() & 3) + 2;
			p->color = q1ramp3[(int)p->ramp];
			p->type = pt_q1fire;
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
			break;

		case 2:	// blood
			p->type = GGameType & GAME_QuakeWorld ? pt_q1slowgrav : pt_q1grav;
			p->color = 67 + (rand() & 3);
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
			break;

		case 3:
		case 5:	// tracer
			p->die = cl.serverTime + 500;
			p->type = pt_q1static;
			if (type == 3)
			{
				p->color = 52 + ((tracercount & 4) << 1);
			}
			else
			{
				p->color = 230 + ((tracercount & 4) << 1);
			}

			tracercount++;

			VectorCopy(start, p->org);
			if (tracercount & 1)
			{
				p->vel[0] = 30 * vec[1];
				p->vel[1] = 30 * -vec[0];
			}
			else
			{
				p->vel[0] = 30 * -vec[1];
				p->vel[1] = 30 * vec[0];
			}
			break;

		case 4:	// slight blood
			p->type = GGameType & GAME_QuakeWorld ? pt_q1slowgrav : pt_q1grav;
			p->color = 67 + (rand() & 3);
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() % 6) - 3);
			len -= 3;
			break;

		case 6:	// voor trail
			p->color = 9 * 16 + 8 + (rand() & 3);
			p->type = pt_q1static;
			p->die = cl.serverTime + 300;
			for (j = 0; j < 3; j++)
				p->org[j] = start[j] + ((rand() & 15) - 8);
			break;
		}


		VectorAdd(start, vec, start);
	}
}
