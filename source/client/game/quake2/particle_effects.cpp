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
#include "../../utils.h"

void CLQ2_ParticleEffect(vec3_t origin, vec3_t direction, int colour, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colour + (rand() & 7);

		float d = rand() & 31;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 7) - 4) + d * direction[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_ParticleEffect2(vec3_t origin, vec3_t direction, int colour, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colour;

		float d = rand() & 7;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 7) - 4) + d * direction[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_ParticleEffect3(vec3_t origin, vec3_t direction, int colour, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colour;

		float d = rand() & 7;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 7) - 4) + d * direction[j];
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_TeleporterParticles(vec3_t origin)
{
	for (int i = 0; i < 8; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = 0xdb;

		for (int j = 0; j < 2; j++)
		{
			p->org[j] = origin[j] - 16 + (rand() & 31);
			p->vel[j] = crand() * 14;
		}

		p->org[2] = origin[2] - 8 + (rand() & 7);
		p->vel[2] = 80 + (rand() & 7);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.5;
	}
}

void CLQ2_PlayerSpawnParticles(vec3_t origin, int baseColour)
{
	for (int i = 0; i < 500; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		p->color = baseColour + (rand() & 7);

		p->org[0] = origin[0] - 16 + frand() * 32;
		p->org[1] = origin[1] - 16 + frand() * 32;
		p->org[2] = origin[2] - 24 + frand() * 56;

		for (int j = 0; j < 3; j++)
		{
			p->vel[j] = crand() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frand() * 0.3);
	}
}

void CLQ2_ItemRespawnParticles(vec3_t origin)
{
	for (int i = 0; i < 64; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = 0xd4 + (rand() & 3);	// green

		p->org[0] = origin[0] + crand() * 8;
		p->org[1] = origin[1] + crand() * 8;
		p->org[2] = origin[2] + crand() * 8;

		for (int j = 0; j < 3; j++)
		{
			p->vel[j] = crand() * 8;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 0.2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frand() * 0.3);
	}
}

void CLQ2_ExplosionParticles(vec3_t origin)
{
	for (int i = 0; i < 256; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = 0xe0 + (rand() & 7);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_BigTeleportParticles(vec3_t origin)
{
	static int colortable[4] = {2 * 8, 13 * 8, 21 * 8, 18 * 8};

	for (int i = 0; i < 4096; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colortable[rand() & 3];

		float angle = idMath::TWO_PI * (rand() & 1023) / 1023.0;
		float dist = rand() & 31;
		p->org[0] = origin[0] + cos(angle) * dist;
		p->vel[0] = cos(angle) * (70 + (rand() & 63));
		p->accel[0] = -cos(angle) * 100;

		p->org[1] = origin[1] + sin(angle) * dist;
		p->vel[1] = sin(angle) * (70 + (rand() & 63));
		p->accel[1] = -sin(angle) * 100;

		p->org[2] = origin[2] + 8 + (rand() % 90);
		p->vel[2] = -100 + (rand() & 31);
		p->accel[2] = PARTICLE_GRAVITY * 4;
		p->alpha = 1.0;

		p->alphavel = -0.3 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_BlasterParticles(vec3_t origin, vec3_t direction)
{
	for (int i = 0; i < 40; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = 0xe0 + (rand() & 7);

		float d = rand() & 15;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 7) - 4) + d * direction[j];
			p->vel[j] = direction[j] * 30 + crand() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_BlasterTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	int dec = 5;
	VectorScale(vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xe0;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void CLQ2_FlagTrail(vec3_t start, vec3_t end, float colour)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	int dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8 + frand() * 0.2);
		p->color = colour;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void CLQ2_DiminishingTrail(vec3_t start, vec3_t end, q2centity_t* old, int flags)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	float dec = 0.5;
	VectorScale(vec, dec, vec);

	float orgscale;
	float velscale;
	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		// drop less particles as it flies
		if ((rand() & 1023) < old->trailcount)
		{
			cparticle_t* p = CL_AllocParticle();
			if (!p)
			{
				return;
			}
			p->type = pt_q2static;
			VectorClear(p->accel);

			if (flags & Q2EF_GIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1 + frand() * 0.4);
				p->color = 0xe8 + (rand() & 7);
				for (int j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & Q2EF_GREENGIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1 + frand() * 0.4);
				p->color = 0xdb + (rand() & 7);
				for (int j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0;
				p->alphavel = -1.0 / (1 + frand() * 0.2);
				p->color = 4 + (rand() & 7);
				for (int j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crand() * orgscale;
					p->vel[j] = crand() * velscale;
				}
				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;
		if (old->trailcount < 100)
		{
			old->trailcount = 100;
		}
		VectorAdd(move, vec, move);
	}
}

void CLQ2_RocketTrail(vec3_t start, vec3_t end, q2centity_t* old)
{
	// smoke
	CLQ2_DiminishingTrail(start, end, old, Q2EF_ROCKET);

	// fire
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	float dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if ((rand() & 7) == 0)
		{
			cparticle_t* p = CL_AllocParticle();
			if (!p)
			{
				return;
			}
			p->type = pt_q2static;

			VectorClear(p->accel);

			p->alpha = 1.0;
			p->alphavel = -1.0 / (1 + frand() * 0.2);
			p->color = 0xdc + (rand() & 3);
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + crand() * 5;
				p->vel[j] = crand() * 20;
			}
			p->accel[2] = -PARTICLE_GRAVITY;
		}
		VectorAdd(move, vec, move);
	}
}

void CLQ2_RailTrail(vec3_t start, vec3_t end)
{
	byte clr = 0x74;

	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	vec3_t right, up;
	MakeNormalVectors(vec, right, up);

	for (int i = 0; i < len; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		p->type = pt_q2static;

		VectorClear(p->accel);

		float d = i * 0.1;
		float c = cos(d);
		float s = sin(d);

		vec3_t dir;
		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frand() * 0.2);
		p->color = clr + (rand() & 7);
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd(move, vec, move);
	}

	float dec = 0.75;
	VectorScale(vec, dec, vec);
	VectorCopy(start, move);

	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.6 + frand() * 0.2);
		p->color = rand() & 15;

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 3;
			p->vel[j] = crand() * 3;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void CLQ2_IonripperTrail(vec3_t start, vec3_t ent)
{
	bool left = false;

	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(ent, start, vec);
	float len = VectorNormalize(vec);

	int dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 0.5;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xe4 + (rand() & 3);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j];
			p->accel[j] = 0;
		}
		if (left)
		{
			left = false;
			p->vel[0] = 10;
		}
		else
		{
			left = true;
			p->vel[0] = -10;
		}

		p->vel[1] = 0;
		p->vel[2] = 0;

		VectorAdd(move, vec, move);
	}
}

void CLQ2_BubbleTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	float dec = 32;
	VectorScale(vec, dec, vec);

	for (int i = 0; i < len; i += dec)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		p->type = pt_q2static;

		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frand() * 0.2);
		p->color = 4 + (rand() & 7);
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 5;
		}
		p->vel[2] += 6;

		VectorAdd(move, vec, move);
	}
}

void CLQ2_FlyParticles(vec3_t origin, int count)
{
	enum { BEAMLENGTH = 16 };

	if (count > NUMVERTEXNORMALS)
	{
		count = NUMVERTEXNORMALS;
	}

	if (!avelocities[0][0])
	{
		for (int i = 0; i < NUMVERTEXNORMALS * 3; i++)
		{
			avelocities[0][i] = (rand() & 255) * 0.01;
		}
	}

	float ltime = (float)cl.serverTime / 1000.0;
	for (int i = 0; i < count; i += 2)
	{
		float angle = ltime * avelocities[i][0];
		float sy = sin(angle);
		float cy = cos(angle);
		angle = ltime * avelocities[i][1];
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
		p->type = pt_q2static;

		float dist = sin(ltime + i) * 64;
		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear(p->vel);
		VectorClear(p->accel);

		p->color = 0;

		p->alpha = 1;
		p->alphavel = -100;
	}
}

void CLQ2_BfgParticles(vec3_t origin)
{
	enum { BEAMLENGTH = 16 };

	if (!avelocities[0][0])
	{
		for (int i = 0; i < NUMVERTEXNORMALS * 3; i++)
		{
			avelocities[0][i] = (rand() & 255) * 0.01;
		}
	}

	float ltime = (float)cl.serverTime / 1000.0;
	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		float angle = ltime * avelocities[i][0];
		float sy = sin(angle);
		float cy = cos(angle);
		angle = ltime * avelocities[i][1];
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
		p->type = pt_q2static;

		float dist = sin(ltime + i) * 64;
		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * BEAMLENGTH;

		VectorClear(p->vel);
		VectorClear(p->accel);

		vec3_t v;
		VectorSubtract(p->org, origin, v);
		dist = VectorLength(v) / 90.0;
		p->color = floor(0xd0 + dist * 7);

		p->alpha = 1.0 - dist;
		p->alphavel = -100;
	}
}

void CLQ2_TrapParticles(vec3_t origin)
{
	origin[2] -= 14;
	vec3_t start;
	VectorCopy(origin, start);
	vec3_t end;
	VectorCopy(origin, end);
	origin[2] += 14;
	end[2] += 64;

	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	int dec = 5;
	VectorScale(vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xe0;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 15;
			p->accel[j] = 0;
		}
		p->accel[2] = PARTICLE_GRAVITY;

		VectorAdd(move, vec, move);
	}

	vec3_t org;
	VectorCopy(origin, org);

	for (int i = -2; i <= 2; i += 4)
	{
		for (int j = -2; j <= 2; j += 4)
		{
			for (int k = -2; k <= 2; k += 4)
			{
				cparticle_t* p = CL_AllocParticle();
				if (!p)
				{
					return;
				}
				p->type = pt_q2static;

				p->color = 0xe0 + (rand() & 3);

				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand() & 7) * 0.02);

				p->org[0] = org[0] + i + ((rand() & 23) * crand());
				p->org[1] = org[1] + j + ((rand() & 23) * crand());
				p->org[2] = org[2] + k + ((rand() & 23) * crand());

				vec3_t dir;
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				VectorNormalize(dir);
				float vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
		}
	}
}

void CLQ2_BFGExplosionParticles(vec3_t origin)
{
	for (int i = 0; i < 256; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = 0xd0 + (rand() & 7);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_TeleportParticles(vec3_t origin)
{
	for (int i = -16; i <= 16; i += 4)
	{
		for (int j = -16; j <= 16; j += 4)
		{
			for (int k = -16; k <= 32; k += 4)
			{
				cparticle_t* p = CL_AllocParticle();
				if (!p)
				{
					return;
				}
				p->type = pt_q2static;

				p->color = 7 + (rand() & 7);

				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand() & 7) * 0.02);

				p->org[0] = origin[0] + i + (rand() & 3);
				p->org[1] = origin[1] + j + (rand() & 3);
				p->org[2] = origin[2] + k + (rand() & 3);

				vec3_t dir;
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				VectorNormalize(dir);
				float vel = 50 + (rand() & 63);
				VectorScale(dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
		}
	}
}

void CLQ2_DebugTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	vec3_t right, up;
	MakeNormalVectors(vec, right, up);

	float dec = 3;
	VectorScale(vec, dec, vec);
	VectorCopy(start, move);

	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		VectorClear(p->accel);
		VectorClear(p->vel);
		p->alpha = 1.0;
		p->alphavel = -0.1;
		p->color = 0x74 + (rand() & 7);
		VectorCopy(move, p->org);
		VectorAdd(move, vec, move);
	}
}

void CLQ2_ForceWall(vec3_t start, vec3_t end, int colour)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	VectorScale(vec, 4, vec);

	while (len > 0)
	{
		len -= 4;

		if (frand() > 0.3)
		{
			cparticle_t* p = CL_AllocParticle();
			if (!p)
			{
				return;
			}
			p->type = pt_q2static;
			VectorClear(p->accel);

			p->alpha = 1.0;
			p->alphavel =  -1.0 / (3.0 + frand() * 0.5);
			p->color = colour;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + crand() * 3;
				p->accel[j] = 0;
			}
			p->vel[0] = 0;
			p->vel[1] = 0;
			p->vel[2] = -40 - (crand() * 10);
		}

		VectorAdd(move, vec, move);
	}
}

//	(lets you control the # of bubbles by setting the distance between the spawns)
void CLQ2_BubbleTrail2(vec3_t start, vec3_t end, int distance)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	float dec = distance;
	VectorScale(vec, dec, vec);

	for (int i = 0; i < len; i += dec)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		p->type = pt_q2static;

		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frand() * 0.1);
		p->color = 4 + (rand() & 7);
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 2;
			p->vel[j] = crand() * 10;
		}
		p->org[2] -= 4;
		p->vel[2] += 20;

		VectorAdd(move, vec, move);
	}
}

void CLQ2_HeatbeamPaticles(vec3_t start, vec3_t forward)
{
	float step = 32.0;

	vec3_t end;
	VectorMA(start, 4096, forward, end);

	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	vec3_t right;
	VectorSubtract(vec3_origin, cl.refdef.viewaxis[1], right);
	vec3_t up;
	VectorCopy(cl.refdef.viewaxis[2], up);
	VectorMA(move, -0.5, right, move);
	VectorMA(move, -0.5, up, move);

	float ltime = (float)cl.serverTime / 1000.0;
	float start_pt = fmod(ltime * 96.0f, step);
	VectorMA(move, start_pt, vec, move);

	VectorScale(vec, step, vec);

	float rstep = idMath::PI / 10.0;
	for (int i = start_pt; i < len; i += step)
	{
		if (i > step * 5)	// don't bother after the 5th ring
		{
			break;
		}

		for (float rot = 0; rot < idMath::TWO_PI; rot += rstep)
		{
			cparticle_t* p = CL_AllocParticle();
			if (!p)
			{
				return;
			}

			p->type = pt_q2static;

			VectorClear(p->accel);
			float variance = 0.5;
			float c = cos(rot) * variance;
			float s = sin(rot) * variance;

			// trim it so it looks like it's starting at the origin
			vec3_t dir;
			if (i < 10)
			{
				VectorScale(right, c * (i / 10.0), dir);
				VectorMA(dir, s * (i / 10.0), up, dir);
			}
			else
			{
				VectorScale(right, c, dir);
				VectorMA(dir, s, up, dir);
			}

			p->alpha = 0.5;
			p->alphavel = -1000.0;
			p->color = 223 - (rand() & 7);
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + dir[j] * 3;
				p->vel[j] = 0;
			}
		}
		VectorAdd(move, vec, move);
	}
}

//	Puffs with velocity along direction, with some randomness thrown in
void CLQ2_ParticleSteamEffect(vec3_t org, vec3_t dir, int color, int count, int magnitude)
{
	vec3_t r, u;
	MakeNormalVectors(dir, r, u);

	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = color + (rand() & 7);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + magnitude * 0.1 * crand();
		}
		VectorScale(dir, magnitude, p->vel);
		float d = crand() * magnitude / 3;
		VectorMA(p->vel, d, r, p->vel);
		d = crand() * magnitude / 3;
		VectorMA(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_TrackerTrail(vec3_t start, vec3_t end, int particleColour)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	vec3_t forward;
	VectorCopy(vec, forward);
	vec3_t angle_dir;
	VecToAngles(forward, angle_dir);
	vec3_t right, up;
	AngleVectors(angle_dir, forward, right, up);

	int dec = 3;
	VectorScale(vec, 3, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -2.0;
		p->color = particleColour;
		float dist = DotProduct(move, forward);
		VectorMA(move, 8 * cos(dist), up, p->org);
		for (int j = 0; j < 3; j++)
		{
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
		p->vel[2] = 5;

		VectorAdd(move, vec, move);
	}
}

void CLQ2_Tracker_Shell(vec3_t origin)
{
	for (int i = 0; i < 300; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0;

		vec3_t dir;
		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);

		VectorMA(origin, 40, dir, p->org);
	}
}

void CLQ2_MonsterPlasma_Shell(vec3_t origin)
{
	for (int i = 0; i < 40; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0xe0;

		vec3_t dir;
		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);

		VectorMA(origin, 10, dir, p->org);
	}
}

void CLQ2_WidowSplash(vec3_t origin)
{
	static int colortable[4] = {2 * 8, 13 * 8, 21 * 8, 18 * 8};

	for (int i = 0; i < 256; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colortable[rand() & 3];

		vec3_t dir;
		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		VectorMA(origin, 45.0, dir, p->org);
		VectorMA(vec3_origin, 40.0, dir, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->alpha = 1.0;

		p->alphavel = -0.8 / (0.5 + frand() * 0.3);
	}
}

void CLQ2_Tracker_Explode(vec3_t origin)
{
	for (int i = 0; i < 300; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0;
		p->color = 0;

		vec3_t dir;
		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		vec3_t backdir;
		VectorScale(dir, -1, backdir);

		VectorMA(origin, 64, dir, p->org);
		VectorScale(backdir, 64, p->vel);
	}
}

void CLQ2_TagTrail(vec3_t start, vec3_t end, int colour)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	int dec = 5;
	VectorScale(vec, 5, vec);

	while (len >= 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8 + frand() * 0.2);
		p->color = colour;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand() * 16;
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void CLQ2_ColorExplosionParticles(vec3_t origin, int colour, int run)
{
	for (int i = 0; i < 128; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colour + (rand() % run);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 256) - 128;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.4 / (0.6 + frand() * 0.2);
	}
}

//	like the steam effect, but unaffected by gravity
void CLQ2_ParticleSmokeEffect(vec3_t origin, vec3_t direction, int colour, int count, int magnitude)
{
	vec3_t r, u;
	MakeNormalVectors(direction, r, u);

	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colour + (rand() & 7);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + magnitude * 0.1 * crand();
		}
		VectorScale(direction, magnitude, p->vel);
		float d = crand() * magnitude / 3;
		VectorMA(p->vel, d, r, p->vel);
		d = crand() * magnitude / 3;
		VectorMA(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

//	Wall impact puffs (Green)
void CLQ2_BlasterParticles2(vec3_t origin, vec3_t direction, unsigned int colour)
{
	int count = 40;
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;

		p->color = colour + (rand() & 7);

		float d = rand() & 15;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 7) - 4) + d * direction[j];
			p->vel[j] = direction[j] * 30 + crand() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand() * 0.3);
	}
}

//	Green!
void CLQ2_BlasterTrail2(vec3_t start, vec3_t end)
{
	vec3_t move;
	VectorCopy(start, move);
	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);

	int dec = 5;
	VectorScale(vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->type = pt_q2static;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xd0;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}
