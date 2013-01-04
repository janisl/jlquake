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

int h2ramp1[8] = { 416,416 + 2,416 + 4,416 + 6,416 + 8,416 + 10,416 + 12,416 + 14};
int h2ramp2[8] = { 384 + 4,384 + 6,384 + 8,384 + 10,384 + 12,384 + 13,384 + 14,384 + 15};
int h2ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};
int h2ramp4[16] = { 416,416 + 1,416 + 2,416 + 3,416 + 4,416 + 5,416 + 6,416 + 7,416 + 8,416 + 9,416 + 10,416 + 11,416 + 12,416 + 13,416 + 14,416 + 15};
int h2ramp5[16] = { 400,400 + 1,400 + 2,400 + 3,400 + 4,400 + 5,400 + 6,400 + 7,400 + 8,400 + 9,400 + 10,400 + 11,400 + 12,400 + 13,400 + 14,400 + 15};
int h2ramp6[16] = { 256,256 + 1,256 + 2,256 + 3,256 + 4,256 + 5,256 + 6,256 + 7,256 + 8,256 + 9,256 + 10,256 + 11,256 + 12,256 + 13,256 + 14,256 + 15};
int h2ramp7[16] = { 384,384 + 1,384 + 2,384 + 3,384 + 4,384 + 5,384 + 6,384 + 7,384 + 8,384 + 9,384 + 10,384 + 11,384 + 12,384 + 13,384 + 14,384 + 15};
int h2ramp8[16] = {175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 13, 14, 15, 16, 17, 18};
int h2ramp9[16] = { 416,416 + 1,416 + 2,416 + 3,416 + 4,416 + 5,416 + 6,416 + 7,416 + 8,416 + 9,416 + 10,416 + 11,416 + 12,416 + 13,416 + 14,416 + 15};
int h2ramp10[16] = { 432,432 + 1,432 + 2,432 + 3,432 + 4,432 + 5,432 + 6,432 + 7,432 + 8,432 + 9,432 + 10,432 + 11,432 + 12,432 + 13,432 + 14,432 + 15};
int h2ramp11[8] = { 424,424 + 1,424 + 2,424 + 3,424 + 4,424 + 5,424 + 6,424 + 7};
int h2ramp12[8] = { 136,137,138,139,140,141,142,143};
int h2ramp13[16] = { 144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159};

vec3_t rider_origin;

void CLH2_DarkFieldParticles(const vec3_t origin)
{
	for (int i = -16; i < 16; i += 8)
	{
		for (int j = -16; j < 16; j += 8)
		{
			for (int k = 0; k < 32; k += 8)
			{
				cparticle_t* p = CL_AllocParticle();
				if (!p)
				{
					return;
				}
				p->die = cl.serverTime + 200 + (rand() & 7) * 20;
				p->color = 150 + rand() % 6;
				p->type = pt_h2slowgrav;

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

void CLH2_ParticleExplosion(const vec3_t origin)
{
	for (int i = 0; i < 1024; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 5000;
		p->color = h2ramp1[0];
		p->ramp = rand() & 3;
		p->type = i & 1 ? pt_h2explode : pt_h2explode2;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 31) - 16);
			p->vel[j] = (rand() & 511) - 256;
		}
	}
}

//	Not present in Hexen 2
void CLH2_BlobExplosion(const vec3_t origin)
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
			p->type = pt_h2blob_hw;
			p->color = 66 + rand() % 6;
		}
		else
		{
			p->type = pt_h2blob2_hw;
			p->color = 150 + rand() % 6;
		}
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() % 32) - 16);
			p->vel[j] = (rand() % 512) - 256;
		}
	}
}

void CLH2_RunParticleEffect(const vec3_t origin, const vec3_t direction, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 100 * (rand() % 5);
		p->color = 256 + 16 + 12 + (rand() & 3);
		p->type = pt_h2slowgrav;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + ((rand() & 15) - 8);
			p->vel[j] = direction[j] * 15;
		}
	}
}

void CLH2_RunParticleEffect2(const vec3_t origin, const vec3_t directionMin, const vec3_t directionMax, int colour, ptype_t effect, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 2000;
		p->color = colour;
		p->type = effect;
		p->ramp = 0;
		for (int j = 0; j < 3; j++)
		{
			float num = rand() / (float)RAND_MAX;
			if (GGameType & GAME_HexenWorld)
			{
				p->org[j] = origin[j] + ((rand() & 15) - 8);
			}
			else
			{
				p->org[j] = origin[j] + ((rand() & 8) - 4);
			}
			p->vel[j] = directionMin[j] + ((directionMax[j] - directionMin[j]) * num);
		}
	}
}

void CLH2_RunParticleEffect3(const vec3_t origin, const vec3_t box, int colour, ptype_t effect, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 2000;
		p->color = colour;
		p->type = effect;
		p->ramp = 0;
		for (int j = 0; j < 3; j++)
		{
			float num = rand() / (float)RAND_MAX;
			p->org[j] = origin[j] + ((rand() & 15) - 8);
			p->vel[j] = (box[j] * num * 2) - box[j];
		}
	}
}

void CLH2_RunParticleEffect4(const vec3_t origin, float radius, int colour, ptype_t effect, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 2000;
		p->color = colour;
		p->type = effect;
		p->ramp = 0;
		for (int j = 0; j < 3; j++)
		{
			float num = rand() / (float)RAND_MAX;
			p->org[j] = origin[j] + ((rand() & 15) - 8);
			p->vel[j] = (radius * num * 2) - radius;
		}
	}
}

//	Not present in Hexen 2
void CLH2_SplashParticleEffect(const vec3_t origin, float radius, int colour, ptype_t effect, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 2000;
		p->color = colour;
		p->type = effect;
		p->ramp = 0;
		for (int j = 0; j < 3; j++)
		{
			float num = rand() / (float)RAND_MAX;
			if (j == 2)
			{
				p->vel[j] = (radius * num * 4) + radius;
				p->org[j] = origin[j] + 3;
			}
			else
			{
				p->vel[j] = (radius * num * 2) - radius;
				p->org[j] = origin[j] + ((rand() % 64) - 32);
			}
		}
	}
}

void CLH2_LavaSplash(const vec3_t origin)
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
				p->type = pt_h2slowgrav;

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

void CLH2_TeleportSplash(const vec3_t origin)
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
				p->type = pt_h2slowgrav;

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

void CLH2_RunQuakeEffect(const vec3_t origin, float distance)
{
	for (int i = 0; i < 100; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 300 * (rand() % 5);
		p->color = (rand() & 3) + ((rand() % 3) * 16) + (13 * 16) + 256 + 11;
		p->type = pt_h2quake;
		p->ramp = 0;

		float num = rand() / (float)RAND_MAX;
		float num2 = distance * num;
		num = rand() / (float)RAND_MAX;
		p->org[0] = origin[0] + cos(num * 2 * M_PI) * num2;
		p->org[1] = origin[1] + sin(num * 2 * M_PI) * num2;
		p->org[2] = origin[2];

		num = rand() / (float)RAND_MAX;
		p->vel[0] = (num * 40) - 20;
		num = rand() / (float)RAND_MAX;
		p->vel[1] = (num * 40) - 20;
		num = rand() / (float)RAND_MAX;
		p->vel[2] = 65 * num + 80;
	}
}

void CLH2_SunStaffTrail(vec3_t source, const vec3_t destination)
{
	vec3_t vec;
	VectorSubtract(destination, source, vec);
	float length = VectorNormalize(vec);

	while (length > 0)
	{
		length -= 10;

		cparticle_t* p = CL_AllocParticle();
		if (p == NULL)
		{
			return;
		}

		p->die = cl.serverTime + 2000;

		p->ramp = rand() & 3;
		p->color = h2ramp6[(int)p->ramp];
		p->type = pt_h2spit;

		for (int i = 0; i < 3; i++)
		{
			p->org[i] = source[i] + ((rand() & 3) - 2);
		}

		p->vel[0] = (rand() % 10) - 5;
		p->vel[1] = (rand() % 10) - 5;
		p->vel[2] = (rand() % 10);

		VectorAdd(source, vec, source);
	}
}

void CLH2_RiderParticles(int count, const vec3_t origin)
{
	VectorCopy(origin, rider_origin);

	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 4000;
		p->color = 256 + 16 + 15;
		p->type = pt_h2rd;
		p->ramp = 0;

		VectorCopy(origin, p->org);

		float angle = (rand() % 360) / (2 * M_PI);
		float radius = 300 + (rand() & 255);
		p->org[0] += sin(angle) * radius;
		p->org[1] += cos(angle) * radius;
		p->org[2] += (rand() & 255) - 30;

		p->vel[0] = (rand() & 255) - 127;
		p->vel[1] = (rand() & 255) - 127;
		p->vel[2] = (rand() & 255) - 127;
	}
}

//	Not present in HexenWorld
void CLH2_GravityWellParticle(int count, const vec3_t origin, int colour)
{
	int i;
	cparticle_t* p;
	float radius,angle;

	VectorCopy(origin, rider_origin);

	for (i = 0; i < count; i++)
	{
		p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		p->die = cl.serverTime + 4000;
		p->color = colour + (rand() & 15);
		p->type = pt_h2gravwell;
		p->ramp = 0;

		VectorCopy(origin,p->org);

		angle = (rand() % 360) / (2 * M_PI);
		radius = 300 + (rand() & 255);
		p->org[0] += sin(angle) * radius;
		p->org[1] += cos(angle) * radius;
		p->org[2] += (rand() & 255) - 30;

		p->vel[0] = (rand() & 255) - 127;
		p->vel[1] = (rand() & 255) - 127;
		p->vel[2] = (rand() & 255) - 127;
	}
}

void CLH2_RainEffect(const vec3_t origin, const vec3_t size, int xDirection, int yDirection, int colour, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->vel[0] = xDirection;	//X and Y motion
		p->vel[1] = yDirection;
		p->vel[2] = -(rand() % 956);
		if (p->vel[2] > -256)
		{
			p->vel[2] += -256;
		}

		float z_time = -(size[2] / p->vel[2]);
		p->die = cl.serverTime + z_time * 1000;
		p->color = colour;
		p->ramp = rand() & 3;

		p->type = pt_h2rain;

		int holdint = (int)size[0];
		p->org[0] = origin[0] + (rand() % holdint);
		holdint = (int)size[1];
		p->org[1] = origin[1] + (rand() % holdint);
		p->org[2] = origin[2];
	}
}

void CLH2_RainEffect2(const vec3_t org, const vec3_t e_size, int x_dir, int y_dir, int color, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->vel[0] = x_dir;	//X and Y motion
		p->vel[1] = y_dir;
		p->vel[2] = -(rand() % 500);
		if (p->vel[2] > -128)
		{
			p->vel[2] += -128;
		}

		float z_time = -(e_size[2] / p->vel[2]);
		p->die = cl.serverTime + z_time * 1000;
		p->color = color;
		p->ramp = (rand() & 3);

		p->type = pt_h2rain;
		int holdint = (int)e_size[0];
		p->org[0] = org[0] + (rand() % holdint);
		holdint = (int)e_size[1];
		p->org[1] = org[1] + (rand() % holdint);
		p->org[2] = org[2] + (rand() % 16);
	}
}

void CLH2_SnowEffect(const vec3_t minOrigin, const vec3_t maxOrigin, int flags, const vec3_t direction, int count)
{
	count *= snow_active->value;
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->vel[0] = direction[0];	//X and Y motion
		p->vel[1] = direction[1];
		p->vel[2] = direction[2] * ((rand() & 15) + 7) / 10;

		p->flags = flags;

		if ((rand() & 0x7f) <= 1)	//have a console variable 'happy_snow' that makes all snowflakes happy snow!
		{
			p->count = 69;	//happy snow!
		}
		else if (flags & SFL_FLUFFY || (flags & SFL_MIXED && (rand() & 3)))
		{
			p->count = (rand() & 31) + 10;	//From 10 to 41 scale, will be divided
		}
		else
		{
			p->count = 10;
		}

		if (flags & SFL_HALF_BRIGHT)//Start darker
		{
			p->color = 26 + (rand() % 5);
		}
		else
		{
			p->color = 18 + (rand() % 12);
		}

		if (!(flags & SFL_NO_TRANS))//Start translucent
		{
			p->color += 256;
		}

		p->die = cl.serverTime + 7000;
		p->ramp = (rand() & 3);
		p->type = pt_h2snow;

		int j = 50;
		int contents;
		p->org[2] = maxOrigin[2];
		do
		{
			//Make sure it doesn't start in a solid
			int holdint = maxOrigin[0] - minOrigin[0];
			p->org[0] = minOrigin[0] + (rand() % holdint);
			holdint = maxOrigin[1] - minOrigin[1];
			p->org[1] = minOrigin[1] + (rand() % holdint);
			contents = CM_PointContentsQ1(p->org, 0);
			j--;//No infinite loops
		}
		while (contents != BSP29CONTENTS_EMPTY && j);
		if (contents != BSP29CONTENTS_EMPTY)
		{
			common->FatalError("Snow entity top plane is not in an empty area (sorry!)");
		}

		VectorCopy(minOrigin, p->minOrg);
		VectorCopy(maxOrigin, p->maxOrg);
	}
}

void CLH2_ColouredParticleExplosion(const vec3_t org, int color, int radius, int count)
{
	for (int i = 0; i < count; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 3000;
		p->color = color;
		p->ramp = (rand() & 3);
		p->type = i & 1 ? pt_h2c_explode : pt_h2c_explode2;
		for (int j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((rand() % (radius * 2)) - radius);
			p->vel[j] = (rand() & 511) - 256;
		}
	}
}

void CLH2_TrailParticles(vec3_t start, const vec3_t end, int type)
{
	static int tracercount;

	vec3_t vec;
	VectorSubtract(end, start, vec);
	float len = VectorNormalize(vec);
	vec3_t dist;
	dist[0] = vec[0];
	dist[1] = vec[1];
	dist[2] = vec[2];
	float size = 1;
	float lifetime = 2;
	switch (type)
	{
	case rt_spit:
		break;

	case rt_ice:
		size = 5 * 3;
		break;

	case rt_acidball:
		size = 5;
		lifetime = 0.8;
		break;

	case rt_grensmoke:
		size = 5;
		break;

	case rt_purify:
		size = 5;
		lifetime = 0.5;
		break;

	default:
		size = 3;
		break;
	}
	VectorScale(dist, size, dist);

	while (len > 0)
	{
		len -= size;

		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		VectorCopy(vec3_origin, p->vel);
		p->die = cl.serverTime + lifetime * 1000;

		switch (type)
		{
		case rt_grensmoke:	//smoke trail for grenade
			p->color = 283 + (rand() & 3);
			p->type = pt_h2grensmoke;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() % 6) - 3);
			}
			break;

		case rt_rocket_trail:	// rocket trail
			p->ramp = (rand() & 3);
			p->color = h2ramp3[(int)p->ramp];
			p->type = pt_h2fire;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() % 6) - 3);
			}
			break;

		case rt_smoke:	// smoke smoke
			p->ramp = (rand() & 3) + 2;
			p->color = h2ramp3[(int)p->ramp];
			p->type = pt_h2fire;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() % 6) - 3);
			}
			break;

		case rt_blood:	// blood
			if (GGameType & GAME_HexenWorld)
			{
				p->type = pt_h2darken;
				p->color = 136 + (rand() % 5);
				for (int j = 0; j < 3; j++)
				{
					p->org[j] = start[j] + ((rand() & 3) - 2);
				}
				len -= size;
			}
			else
			{
				p->type = pt_h2slowgrav;
				p->color = 134 + (rand() & 7);
				for (int j = 0; j < 3; j++)
				{
					p->org[j] = start[j] + ((rand() % 6) - 3);
				}
			}
			break;

		case rt_tracer:
		case rt_tracer2:// tracer
			p->die = cl.serverTime + 500;
			p->type = pt_h2static;
			if (type == rt_tracer)
			{
				p->color = 130 + (rand() & 6);
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

		case rt_slight_blood:	// slight blood
			p->type = pt_h2slowgrav;
			if (GGameType & GAME_HexenWorld)
			{
				p->color = 138 + (rand() & 3);
			}
			else
			{
				p->color = 134 + (rand() & 7);
			}
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() % 6) - 3);
			}
			len -= size;
			break;

		case rt_bloodshot:	// bloodshot trail
			p->type = pt_h2darken;
			p->color = 136 + (rand() & 5);
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 3) - 2);
			}
			len -= size;
			break;

		case rt_voor_trail:	// voor trail
			p->color = 9 * 16 + 8 + (rand() & 3);
			p->type = pt_h2static;
			p->die = cl.serverTime + 300;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 15) - 8);
			}
			break;

		case rt_fireball:	// Fireball
			p->ramp = rand() & 3;
			p->color = h2ramp4[(int)(p->ramp)];
			p->type = pt_h2fireball;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 3) - 2);
			}
			p->org[2] += 2;	// compensate for model
			p->vel[0] = (rand() % 200) - 100;
			p->vel[1] = (rand() % 200) - 100;
			p->vel[2] = (rand() % 200) - 100;
			break;

		case rt_acidball:	// Acid ball
			p->ramp = rand() & 3;
			p->color = h2ramp10[(int)(p->ramp)];
			p->type = pt_h2acidball;
			if (!(GGameType & GAME_HexenWorld))
			{
				p->die = cl.serverTime + 500;
			}
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 3) - 2);
			}
			p->org[2] += 2;	// compensate for model
			if (GGameType & GAME_HexenWorld)
			{
				p->vel[0] = (rand() % 20) - 10;
				p->vel[1] = (rand() % 20) - 10;
				p->vel[2] = (rand() % 20) - 10;
			}
			else
			{
				p->vel[0] = (rand() % 40) - 20;
				p->vel[1] = (rand() % 40) - 20;
				p->vel[2] = (rand() % 40) - 20;
			}
			break;

		case rt_ice:// Ice
			p->ramp = rand() & 3;
			p->color = h2ramp5[(int)(p->ramp)];
			p->type = pt_h2ice;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 3) - 2);
			}
			p->org[2] += 2;	// compensate for model
			p->vel[0] = (rand() % 16) - 8;
			p->vel[1] = (rand() % 16) - 8;
			p->vel[2] = (rand() % 20) - 40;
			break;

		case rt_spit:	// Spit
			p->ramp = rand() & 3;
			p->color = h2ramp6[(int)(p->ramp)];
			p->type = pt_h2spit;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 3) - 2);
			}
			p->org[2] += 2;	// compensate for model
			p->vel[0] = (rand() % 10) - 5;
			p->vel[1] = (rand() % 10) - 5;
			p->vel[2] = (rand() % 10);
			break;

		case rt_spell:	// Spell
			p->ramp = rand() & 3;
			p->color = h2ramp6[(int)(p->ramp)];
			p->type = pt_h2spell;
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() & 3) - 2);
			}
			p->vel[0] = (rand() % 10) - 5;
			p->vel[1] = (rand() % 10) - 5;
			p->vel[2] = (rand() % 10);
			p->vel[0] = vec[0] * -10;
			p->vel[1] = vec[1] * -10;
			p->vel[2] = vec[2] * -10;
			break;

		case rt_vorpal:	// vorpal missile
			p->type = pt_h2vorpal;
			p->color = 44 + (rand() & 3) + 256;
			for (int j = 0; j < 2; j++)
			{
				p->org[j] = start[j] + ((rand() % 48) - 24);
			}
			p->org[2] = start[2] + ((rand() & 15) - 8);
			break;

		case rt_setstaff:	// set staff
			p->type = pt_h2setstaff;
			p->color = h2ramp9[0];
			p->ramp = rand() & 3;

			for (int j = 0; j < 2; j++)
			{
				p->org[j] = start[j] + ((rand() % 6) - 3);
			}

			p->org[2] = start[2] + ((rand() % 10) - 5);

			p->vel[0] = (rand() & 7) - 4;
			p->vel[1] = (rand() & 7) - 4;
			break;

		case rt_purify:	// purifier
			p->type = pt_h2setstaff;
			p->color = h2ramp9[0];
			p->ramp = rand() & 3;

			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + ((rand() % 3) - 1);
			}

			p->vel[0] = 0;
			p->vel[1] = 0;
			break;

		case rt_magicmissile:	// magic missile
			p->type = pt_h2magicmissile;
			p->color = 148 + (rand() & 11);
			p->ramp = rand() & 3;
			for (int j = 0; j < 2; j++)
			{
				p->org[j] = start[j] + ((rand() % 48) - 24);
			}
			p->org[2] = start[2] + ((rand() % 48) - 24);

			p->vel[2] = -((rand() & 15) + 8);
			break;

		case rt_boneshard:	// bone shard
			p->type = pt_h2boneshard;
			p->color = 368 + (rand() & 16);
			for (int j = 0; j < 2; j++)
			{
				p->org[j] = start[j] + ((rand() % 48) - 24);
			}
			p->org[2] = start[2] + ((rand() % 48) - 24);

			p->vel[2] = -((rand() & 15) + 8);
			break;

		case rt_scarab:	// scarab staff
			p->type = pt_h2scarab;
			p->color = 250 + (rand() & 3);
			for (int j = 0; j < 3; j++)
			{
				p->org[j] = start[j] + (rand() & 7);
			}

			p->vel[2] = -(rand() & 7);
			break;
		}
		VectorAdd(start, dist, start);
	}
}

void CLHW_BrightFieldParticles(vec3_t origin)
{
	float height = cos(cl.serverTime * 4.0 / 1000.0) * 25;

	for (int i = 0; i < 120 * cls.frametime / 1000; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 500;
		p->color = 143;
		p->type = pt_h2quake;

		vec3_t dir;
		dir[0] = (rand() % 256) - 128;
		dir[1] = (rand() % 256) - 128;
		dir[2] = 32;

		p->org[0] = origin[0] + (rand() & 15) - 7;
		p->org[1] = origin[1] + (rand() & 15) - 7;
		p->org[2] = origin[2] + height;

		VectorNormalize(dir);
		float vel = 70 + (rand() & 31);
		VectorScale(dir, vel, p->vel);
	}

	for (int i = 0; i < 120 * cls.frametime / 1000; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->die = cl.serverTime + 500;
		p->color = 159;
		p->type = pt_h2quake;

		vec3_t dir;
		dir[0] = (rand() % 256) - 128;
		dir[1] = (rand() % 256) - 128;
		dir[2] = 32;

		p->org[0] = origin[0] + (rand() & 15) - 7;
		p->org[1] = origin[1] + (rand() & 15) - 7;
		p->org[2] = origin[2] - height;

		VectorNormalize(dir);
		float vel = 70 + (rand() & 31);
		VectorScale(dir, vel, p->vel);
	}
}

void CLHW_SuccubusInvincibleParticles(vec3_t origin)
{
	vec3_t ent_angles;
	ent_angles[0] = ent_angles[2] = 0;
	ent_angles[1] = cl.serverTime * 12.0 / 1000.0;

	vec3_t forward;
	forward[0] = cos(ent_angles[1]) * 32;
	forward[1] = sin(ent_angles[1]) * 32;
	forward[2] = 0;

	vec3_t org;
	VectorCopy(origin, org);
	org[2] += 28;

	int count = 140 * cls.frametime / 1000;
	while (count > 0)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->ramp = 0;
		p->die = cl.serverTime + 2000;
		p->color = 416;
		p->type = pt_h2fireball;

		p->org[0] = org[0] + forward[0] + rand() % 4 - 2;
		p->org[1] = org[1] + forward[1] + rand() % 4 - 2;
		p->org[2] = org[2] + forward[2] + rand() % 4 - 2;
		p->vel[0] = rand() % 20 - 10;
		p->vel[1] = rand() % 20 - 10;
		p->vel[2] = rand() % 25 + 20;
		count--;
	}

	count = 60 * cls.frametime / 1000;
	while (count > 0)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		p->ramp = 0;
		p->die = cl.serverTime + 2000;
		p->color = 135;
		p->type = pt_h2redfire;

		p->org[0] = org[0] + forward[0] + rand() % 4 - 2;
		p->org[1] = org[1] + forward[1] + rand() % 4 - 2;
		p->org[2] = org[2] + forward[2] + rand() % 4 - 2;
		p->vel[0] = rand() % 20 - 10;
		p->vel[1] = rand() % 20 - 10;
		p->vel[2] = 0;
		count--;
	}
}

void CLHW_TargetBallEffectParticles(vec3_t origin, float targetDistance)
{
	for (int i = 0; i < 40 * cls.frametime / 1000; i++)
	{
		cparticle_t* p = CL_AllocParticle();
		if (!p)
		{
			return;
		}
		if (targetDistance < 60)
		{
			p->die = cl.serverTime + (rand() & 3) * 20 + (230 * (1.0 - (0.23 * (targetDistance - 24.0) / 36.0)));
		}
		else
		{
			p->die = cl.serverTime + (300 * ((256.0 - targetDistance) / 256.0)) + (rand() & 7) * 20;
		}
		p->color = 7 + (rand() % 24);
		p->type = pt_h2slowgrav;

		vec3_t dir;
		dir[0] = (rand() & 63) - 31;
		dir[1] = (rand() & 63) - 31;
		dir[2] = 256;

		p->org[0] = origin[0] + (rand() & 3) - 2;
		p->org[1] = origin[1] + (rand() & 3) - 2;
		p->org[2] = origin[2] + (rand() & 3);

		VectorNormalize(dir);
		float vel = 50 + (rand() & 63);
		VectorScale(dir, vel, p->vel);
	}
}
