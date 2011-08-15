/*
 * $Header: /H2 Mission Pack/R_PART.C 54    4/01/98 6:43p Jmonroe $
 */

#include "quakedef.h"

#define	SFL_FLUFFY			1	// All largish flakes
#define	SFL_MIXED			2	// Mixed flakes
#define	SFL_HALF_BRIGHT		4	// All flakes start darker
#define	SFL_NO_MELT			8	// Flakes don't melt when his surface, just go away
#define	SFL_IN_BOUNDS		16	// Flakes cannot leave the bounds of their box
#define	SFL_NO_TRANS		32	// All flakes start non-translucent


int		ramp1[8] = { 416,416+2,416+4,416+6,416+8,416+10,416+12,416+14};
int		ramp2[8] = { 384+4,384+6,384+8,384+10,384+12,384+13,384+14,384+15};
int		ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};
int		ramp4[16] = { 416,416+1,416+2,416+3,416+4,416+5,416+6,416+7,416+8,416+9,416+10,416+11,416+12,416+13,416+14,416+15};
int		ramp5[16] = { 400,400+1,400+2,400+3,400+4,400+5,400+6,400+7,400+8,400+9,400+10,400+11,400+12,400+13,400+14,400+15};
int		ramp6[16] = { 256,256+1,256+2,256+3,256+4,256+5,256+6,256+7,256+8,256+9,256+10,256+11,256+12,256+13,256+14,256+15};
int		ramp7[16] = { 384,384+1,384+2,384+3,384+4,384+5,384+6,384+7,384+8,384+9,384+10,384+11,384+12,384+13,384+14,384+15};
int     ramp8[16] = {175, 174, 173, 172, 171, 170, 169, 168, 167, 166, 13, 14, 15, 16, 17, 18};
int		ramp9[16] = { 416,416+1,416+2,416+3,416+4,416+5,416+6,416+7,416+8,416+9,416+10,416+11,416+12,416+13,416+14,416+15};
int		ramp10[16] = { 432,432+1,432+2,432+3,432+4,432+5,432+6,432+7,432+8,432+9,432+10,432+11,432+12,432+13,432+14,432+15};
int		ramp11[8] = { 424,424+1,424+2,424+3,424+4,424+5,424+6,424+7};
int		ramp12[8] = { 136,137,138,139,140,141,142,143};

ptype_t hexen2ParticleTypeTable[] =
{
	pt_h2static,
	pt_h2grav,
	pt_h2fastgrav,
	pt_h2slowgrav,
	pt_h2fire,
	pt_h2explode,
	pt_h2explode2,
	pt_h2blob,
	pt_h2blob2,
	pt_h2rain,
	pt_h2c_explode,
	pt_h2c_explode2,
	pt_h2spit,
	pt_h2fireball,
	pt_h2ice,
	pt_h2spell,
	pt_h2test,
	pt_h2quake,
	pt_h2rd,
	pt_h2vorpal,
	pt_h2setstaff,
	pt_h2magicmissile,
	pt_h2boneshard,
	pt_h2scarab,
	pt_h2acidball,
	pt_h2darken,
	pt_h2snow,
	pt_h2gravwell,
	pt_h2redfire,
};

static vec3_t		rider_origin;

void R_RunParticleEffect3 (vec3_t org, vec3_t box, int color, ptype_t effect, int count);
void R_RunParticleEffect4 (vec3_t org, float radius, int color, ptype_t effect, int count);

void R_DarkFieldParticles (entity_t *ent)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;
	vec3_t		org;

	org[0] = ent->origin[0];
	org[1] = ent->origin[1];
	org[2] = ent->origin[2];
	for (i=-16 ; i<16 ; i+=8)
		for (j=-16 ; j<16 ; j+=8)
			for (k=0 ; k<32 ; k+=8)
			{
				p = CL_AllocParticle();
				if (!p)
					return;
		
				p->die = cl.serverTimeFloat + 0.2 + (rand()&7) * 0.02;
				p->color = 150 + rand()%6;
				p->type = pt_h2slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = net_message.ReadChar() * (1.0/16);
	msgcount = net_message.ReadByte ();
	color = net_message.ReadByte ();

if (msgcount == 255)
	count = 1024;
else
	count = msgcount;
	
	R_RunParticleEffect (org, dir, color, count);
}

/*
===============
R_ParseParticleEffect2

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect2 (void)
{
	vec3_t		org, dmin, dmax;
	int			i, msgcount, color, effect;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dmin[i] = net_message.ReadFloat();
	for (i=0 ; i<3 ; i++)
		dmax[i] = net_message.ReadFloat();
	color = net_message.ReadShort ();
	msgcount = net_message.ReadByte ();
	effect = net_message.ReadByte ();

	R_RunParticleEffect2 (org, dmin, dmax, color, hexen2ParticleTypeTable[effect], msgcount);
}

/*
===============
R_ParseParticleEffect3

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect3 (void)
{
	vec3_t		org, box;
	int			i, msgcount, color, effect;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	for (i=0 ; i<3 ; i++)
		box[i] = net_message.ReadByte ();
	color = net_message.ReadShort ();
	msgcount = net_message.ReadByte ();
	effect = net_message.ReadByte ();

	R_RunParticleEffect3 (org, box, color, hexen2ParticleTypeTable[effect], msgcount);
}

/*
===============
R_ParseParticleEffect4

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect4 (void)
{
	vec3_t		org;
	int			i, msgcount, color, effect;
	float		radius;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	radius = net_message.ReadByte();
	color = net_message.ReadShort ();
	msgcount = net_message.ReadByte ();
	effect = net_message.ReadByte ();

	R_RunParticleEffect4 (org, radius, color, hexen2ParticleTypeTable[effect], msgcount);
}

/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion (vec3_t org)
{
	int			i, j;
	cparticle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1)
		{
			p->type = pt_h2explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&31)-16);
				p->vel[j] = (rand()&511)-256;
			}
		}
		else
		{
			p->type = pt_h2explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&31)-16);
				p->vel[j] = (rand()&511)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	cparticle_t	*p;
	
	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		if (count == 1024)
		{	// rocket explosion
			p->die = cl.serverTimeFloat + 5;
			p->color = ramp1[0];
			p->ramp = rand()&3;
			if (i & 1)
			{
				p->type = pt_h2explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()&31)-16);
					p->vel[j] = (rand()&511)-256;
				}
			}
			else
			{
				p->type = pt_h2explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()&31)-16);
					p->vel[j] = (rand()&511)-256;
				}
			}
		}
		else
		{
			p->die = cl.serverTimeFloat + 0.1*(rand()%5);
//			p->color = (color&~7) + (rand()&7);
//			p->color = 265 + (rand() % 9);
			p->color = 256 + 16 + 12 + (rand() & 3);
			p->type = pt_h2slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&15)-8);
				p->vel[j] = dir[j]*15;// + (rand()%300)-150;
			}
		}
	}
}



/*
===============
R_RunParticleEffect2

===============
*/
void R_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, ptype_t effect, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		num;

	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 2;//0.1*(rand()%5);
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j=0 ; j<3 ; j++)
		{
			num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
			p->org[j] = org[j] + ((rand()&8)-4); //added randomness to org
			p->vel[j] = dmin[j] + ((dmax[j] - dmin[j]) * num);
		}

	}
}

/*
===============
R_RunParticleEffect3

===============
*/
void R_RunParticleEffect3 (vec3_t org, vec3_t box, int color, ptype_t effect, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		num;

	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 2;//0.1*(rand()%5);
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j=0 ; j<3 ; j++)
		{
			num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
			p->org[j] = org[j] + ((rand()&15)-8);
			p->vel[j] = (box[j] * num * 2) - box[j];
		}
	}
}

/*
===============
R_RunParticleEffect4

===============
*/
void R_RunParticleEffect4 (vec3_t org, float radius, int color, ptype_t effect, int count)
{
	int			i, j;
	cparticle_t	*p;
	float		num;

	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
		{
			return;
		}

		p->die = cl.serverTimeFloat + 2;//0.1*(rand()%5);
		p->color = color;
		p->type = effect;
		p->ramp = 0;
		for (j=0 ; j<3 ; j++)
		{
			num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
			p->org[j] = org[j] + ((rand()&15)-8);
			p->vel[j] = (radius * num * 2) - radius;
		}
	}
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash (vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i++)
		for (j=-16 ; j<16 ; j++)
			for (k=0 ; k<1 ; k++)
			{
				p = CL_AllocParticle();
				if (!p)
					return;
		
				p->die = cl.serverTimeFloat + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_h2slowgrav;
				
				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash (vec3_t org)
{
	int			i, j, k;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=4)
		for (j=-16 ; j<16 ; j+=4)
			for (k=-24 ; k<32 ; k+=4)
			{
				p = CL_AllocParticle();
				if (!p)
					return;
		
				p->die = cl.serverTimeFloat + 0.2 + (rand()&7) * 0.02;
				p->color = 7 + (rand()&7);
				p->type = pt_h2slowgrav;
				
				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;
	
				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_RunQuakeEffect

===============
*/
void R_RunQuakeEffect (vec3_t org, float distance)
{
	int			i;
	cparticle_t	*p;
	float		num,num2;

	for (i=0 ; i<100 ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 0.3*(rand()%5);
		p->color = (rand() &3) + ((rand() % 3)*16) + (13 * 16) + 256 + 11;
		p->type = pt_h2quake;
		p->ramp = 0;

		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		num2 = distance * num;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->org[0] = org[0] + cos(num * 2 * M_PI)*num2;
		p->org[1] = org[1] + sin(num * 2 * M_PI)*num2;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->org[2] = org[2] + 15*num;
		p->org[2] = org[2];

		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->vel[0] = (num * 40) - 20;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->vel[1] = (num * 40) - 20;
		num = rand()*(1.0/RAND_MAX);//(rand ()&0x7fff) / ((float)0x7fff);
		p->vel[2] = 65*num + 80;
	}
}

//==========================================================================
//
// R_SunStaffTrail
//
//==========================================================================

void R_SunStaffTrail(vec3_t source, vec3_t dest)
{
	int i;
	vec3_t vec, dist;
	float length, size;
	cparticle_t *p;

	VectorSubtract(dest, source, vec);
	length = VectorNormalize(vec);
	dist[0] = vec[0];
	dist[1] = vec[1];
	dist[2] = vec[2];

	size = 10;

	while(length > 0)
	{
		length -= size;

		if((p = CL_AllocParticle()) == NULL)
		{
			return;
		}

		p->die = cl.serverTimeFloat+2;

		p->ramp = rand()&3;
		p->color = ramp6[(int)(p->ramp)];

		p->type = pt_h2spit;

		for(i = 0; i < 3; i++)
		{
			p->org[i] = source[i] + ((rand()&3)-2);
		}

		p->vel[0] = (rand()%10)-5;
		p->vel[1] = (rand()%10)-5;
		p->vel[2] = (rand()%10);

		VectorAdd(source, dist, source);
	}
}

void RiderParticle(int count, vec3_t origin)
{
	int			i;
	cparticle_t	*p;
	float radius,angle;

	VectorCopy(origin, rider_origin);

	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 4;
		p->color = 256+16+15;
		p->type = pt_h2rd;
		p->ramp = 0;

		VectorCopy(origin,p->org);

		//num = (rand ()&0x7fff) / ((float)0x7fff);
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

void GravityWellParticle(int count, vec3_t origin, int color)
{
	int			i;
	cparticle_t	*p;
	float radius,angle;

	VectorCopy(origin, rider_origin);

	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 4;
		p->color = color + (rand() & 15);
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

//==========================================================================
//
// R_RocketTrail
//
//==========================================================================

void R_RocketTrail (vec3_t start, vec3_t end, int type)
{
	vec3_t	vec, dist;
	float	len,size,lifetime;
	int			j;
	cparticle_t	*p;
	static int tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	dist[0] = vec[0];
	dist[1] = vec[1];
	dist[2] = vec[2];
	size = 1;  
	lifetime = 2;
	switch(type)
	{
		case 9: // Spit
			break;

		case 8: // Ice
			size *= 5*3;
			dist[0] *= 5*3;
			dist[1] *= 5*3;
			dist[2] *= 5*3;
			break;

		case rt_acidball: // Ice
			size=5;
			lifetime = .8;
			break;

		default:
			size = 3;
			dist[0] *= 3;
			dist[1] *= 3;
			dist[2] *= 3;
			break;

	}

	while (len > 0)
	{
		len -= size;

		p = CL_AllocParticle();
		if (!p)
			return;
		
		VectorCopy (vec3_origin, p->vel);
		p->die = cl.serverTimeFloat + lifetime;

		switch(type)
		{
			case rt_rocket_trail: // rocket trail
				p->ramp = (rand()&3);
				p->color = ramp3[(int)p->ramp];
				p->type = pt_h2fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_smoke: // smoke smoke
				p->ramp = (rand()&3) + 2;
				p->color = ramp3[(int)p->ramp];
				p->type = pt_h2fire;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;

			case rt_blood: // blood
				p->type = pt_h2slowgrav;
				p->color = 134 + (rand()&7);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				break;


			case rt_tracer:;
			case rt_tracer2:;// tracer
				p->die = cl.serverTimeFloat + 0.5;
				p->type = pt_h2static;
				if (type == 3)
					p->color = 130 + (rand() & 6);
//			p->color = 243 + (rand() & 3);
				else
					p->color = 230 + ((tracercount&4)<<1);
			
				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;
			
			case rt_slight_blood:// slight blood
				p->type = pt_h2slowgrav;
				p->color = 134 + (rand()&7);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);
				len -= size;
				break;

			case rt_bloodshot:// bloodshot trail
				p->type = pt_h2darken;
				p->color = 136 + (rand()&5);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				len -= size;
				break;

			case rt_voor_trail:// voor trail
				p->color = 9*16 + 8 + (rand()&3);
				p->type = pt_h2static;
				p->die = cl.serverTimeFloat + 0.3;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&15)-8);
				break;

			case rt_fireball: // Fireball
				p->ramp = rand()&3;
				p->color = ramp4[(int)(p->ramp)];
				p->type = pt_h2fireball;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);		
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 200) - 100;
				p->vel[1] = (rand() % 200) - 100;
				p->vel[2] = (rand() % 200) - 100;
				break;

			case rt_acidball: // Acid ball
				p->ramp = rand()&3;
				p->color = ramp10[(int)(p->ramp)];
				p->type = pt_h2acidball;
				p->die = cl.serverTimeFloat + 0.5;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 40) - 20;
				p->vel[1] = (rand() % 40) - 20;
				p->vel[2] = (rand() % 40) - 20;
				break;

			case rt_ice: // Ice
				p->ramp = rand()&3;
				p->color = ramp5[(int)(p->ramp)];
				p->type = pt_h2ice;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 16) - 8;
				p->vel[1] = (rand() % 16) - 8;
				p->vel[2] = (rand() % 20) - 40;
				break;

			case rt_spit: // Spit
				p->ramp = rand()&3;
				p->color = ramp6[(int)(p->ramp)];
				p->type = pt_h2spit;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->org[2] += 2; // compensate for model
				p->vel[0] = (rand() % 10) - 5;
				p->vel[1] = (rand() % 10) - 5;
				p->vel[2] = (rand() % 10);
				break;

			case rt_spell: // Spell
				p->ramp = rand()&3;
				p->color = ramp6[(int)(p->ramp)];
				p->type = pt_h2spell;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				p->vel[0] = (rand() % 10) - 5;
				p->vel[1] = (rand() % 10) - 5;
				p->vel[2] = (rand() % 10);
				p->vel[0] = vec[0]*-10;
				p->vel[1] = vec[1]*-10;
				p->vel[2] = vec[2]*-10;
				break;

			case rt_vorpal: // vorpal missile
				p->type = pt_h2vorpal;
				p->color = 44 + (rand()&3) + 256;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%48)-24);

				p->org[2] = start[2] + ((rand()&15)-8);

				break;

			case rt_setstaff: // set staff
				p->type = pt_h2setstaff;
				p->color = ramp9[0];
				p->ramp = rand()&3;

				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%6)-3);

				p->org[2] = start[2] + ((rand()%10)-5);

				p->vel[0] = (rand() &7) - 4;
				p->vel[1] = (rand() &7) - 4;
				break;

			case rt_magicmissile: // magic missile
				p->type = pt_h2magicmissile;
				p->color = 148 + (rand()&11);
				p->ramp = rand()&3;
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%48)-24);

				p->org[2] = start[2] + ((rand()%48)-24);

				p->vel[2] = -((rand()&15)+8);
				break;

			case rt_boneshard: // bone shard
				p->type = pt_h2boneshard;
				p->color = 368 + (rand()&16);
				for (j=0 ; j<2 ; j++)
					p->org[j] = start[j] + ((rand()%48)-24);

				p->org[2] = start[2] + ((rand()%48)-24);

				p->vel[2] = -((rand()&15)+8);
				break;

			case rt_scarab: // scarab staff
				p->type = pt_h2scarab;
				p->color = 250 + (rand()&3);
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + (rand()&7);

				p->vel[2] = -(rand()&7);
				break;

		}		
		VectorAdd (start, dist, start);
	}
}

/*
===============
R_RainEffect

===============
*/
void R_RainEffect (vec3_t org,vec3_t e_size,int x_dir, int y_dir,int color,int count)
{
	int i,holdint;
	cparticle_t	*p;
	float z_time;
	
	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;
		
		p->vel[0] = x_dir;  //X and Y motion
		p->vel[1] = y_dir;
		p->vel[2] = -((rand()% 956)) ;
		if (p->vel[2] > -256)
		{
			p->vel[2] += -256;
		}
		
		z_time = -(e_size[2]/p->vel[2]);
		p->color = color;
		p->die = cl.serverTimeFloat + z_time;
		p->ramp = (rand()&3);
		//p->veer = veer;
		
		p->type = pt_h2rain;
		
		holdint=e_size[0];
		p->org[0] = org[0] + (rand() % holdint);
		holdint=e_size[1];
		p->org[1] = org[1] + (rand() % holdint);
		p->org[2] = org[2];
		
	}
}

/*
===============
R_SnowEffect

MG
===============
*/
void R_SnowEffect (vec3_t org1,vec3_t org2,int flags,vec3_t alldir,int count)
{
	int i,j,holdint;
	cparticle_t	*p;
	
	count *= Cvar_VariableValue("snow_active");
	for (i=0 ; i<count ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;
		
		p->vel[0] = alldir[0];  //X and Y motion
		p->vel[1] = alldir[1];
		p->vel[2] = alldir[2] * ((rand() & 15) + 7)/10;
		
		p->flags = flags;

		if ((rand() & 0x7f) <= 1)//have a console variable 'happy_snow' that makes all snowflakes happy snow!
			p->count = 69;	//happy snow!
		else if(flags & SFL_FLUFFY || (flags&SFL_MIXED && (rand()&3)))
			p->count = (rand()&31)+10;//From 10 to 41 scale, will be divided
		else
			p->count = 10;


		if(flags&SFL_HALF_BRIGHT)//Start darker
			p->color = 26 + (rand()%5);
		else
			p->color = 18 + (rand()%12);

		if(!(flags&SFL_NO_TRANS))//Start translucent
			p->color += 256;

		p->die = cl.serverTimeFloat + 7;
		p->ramp = (rand()&3);
		//p->veer = veer;
		p->type = pt_h2snow;
		
		holdint=org2[0] - org1[0];
		p->org[0] = org1[0] + (rand() % holdint);
		holdint=org2[1] - org1[1];
		p->org[1] = org1[1] + (rand() % holdint);
		p->org[2] = org2[2];
		

		j=50;
		int contents = CM_PointContentsQ1(p->org, 0);
		while(contents!=BSP29CONTENTS_EMPTY && j)
		{//Make sure it doesn't start in a solid
			holdint=org2[0] - org1[0];
			p->org[0] = org1[0] + (rand() % holdint);
			holdint=org2[1] - org1[1];
			p->org[1] = org1[1] + (rand() % holdint);
			j--;//No infinite loops
			contents = CM_PointContentsQ1(p->org, 0);
		}
		if (contents!=BSP29CONTENTS_EMPTY)
			Sys_Error ("Snow entity top plane is not in an empty area (sorry!)");

		VectorCopy(org1,p->minOrg);
		VectorCopy(org2,p->maxOrg);
	}
}
/*
===============
R_ColoredParticleExplosion

===============
*/
void R_ColoredParticleExplosion (vec3_t org,int color,int radius,int counter)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<counter ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->die = cl.serverTimeFloat + 3;
		p->color = color;
		p->ramp = (rand()&3);

		if (i & 1)
		{
			p->type = pt_h2c_explode;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%(radius*2))-radius);
				p->vel[j] = (rand()&511)-256;
			}
		}
		else
		{
			p->type = pt_h2c_explode2;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%(radius*2))-radius);
				p->vel[j] = (rand()&511)-256;
			}
		}
	}
}

/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles()
{
	for (cparticle_t* p = active_particles; p; p = p->next)
	{
		if (p->die < cl.serverTimeFloat)
		{
			continue;
		}

		byte* c;
		byte alpha;
		if (p->color <= 255)
		{
			c = r_palette[p->color];
			alpha = 255;
		}
		else
		{
			c = (byte *)&d_8to24TranslucentTable[p->color - 256];
			alpha = c[3];
		}

		if (p->type == pt_h2rain)
		{
			vec3_t origin;
			VectorCopy(p->org, origin);
			float vel0 = p->vel[0] * .001;
			float vel1 = p->vel[1] * .001;
			float vel2 = p->vel[2] * .001;
			for (int i = 0; i < 4; i++)
			{
				R_AddParticleToScene(origin, c[0], c[1], c[2], alpha, 1, PARTTEX_Default);

				origin[0] += vel0;
				origin[1] += vel1;
				origin[2] += vel2;
 			}
		}
		else if (p->type==pt_h2snow && p->count>=69)
		{
			R_AddParticleToScene(p->org, c[0], c[1], c[2], alpha, p->count / 10, PARTTEX_Snow1);
		}
		else if (p->type==pt_h2snow && p->count>=40)
		{
			R_AddParticleToScene(p->org, c[0], c[1], c[2], alpha, p->count / 10, PARTTEX_Snow2);
		}
		else if (p->type==pt_h2snow && p->count>=30)
		{
			R_AddParticleToScene(p->org, c[0], c[1], c[2], alpha, p->count / 10, PARTTEX_Snow3);
		}
		else if (p->type==pt_h2snow)
		{
			R_AddParticleToScene(p->org, c[0], c[1], c[2], alpha, p->count / 10, PARTTEX_Snow4);
		}
		else
		{
			R_AddParticleToScene(p->org, c[0], c[1], c[2], alpha, 1, PARTTEX_Default);
		}
	}
}

extern	Cvar*	sv_gravity;

void R_UpdateParticles (void)
{
	cparticle_t		*p, *kill;
	float			grav,grav2,percent;
	int				i;
	float			time2, time3, time4;
	float			time1;
	float			dvel;
	float			frametime;
	float			vel0, vel1, vel2;
	float			colindex;
	vec3_t			diff;

	if (cls.state == ca_disconnected)
		return;

	frametime = cl.serverTimeFloat - cl.oldtime;
//	Con_Printf("%10.5f\n",frametime);
	time4 = frametime * 20;
	time3 = frametime * 15;
	time2 = frametime * 10;
	time1 = frametime * 5;
	grav = frametime * sv_gravity->value * 0.05;
	grav2 = frametime * sv_gravity->value * 0.025;
	dvel = 4*frametime;
	percent = (frametime / HX_FRAME_TIME);
	
	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.serverTimeFloat)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.serverTimeFloat)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}

		if (p->type==pt_h2rain)
		{
			vel0 = p->vel[0]*.001;
			vel1 = p->vel[1]*.001;
			vel2 = p->vel[2]*.001;
			for(i=0;i<4;i++)
			{
				p->org[0] += vel0;
				p->org[1] += vel1;
				p->org[2] += vel2;
			}
			p->org[0] += p->vel[0]*(frametime-.004);
			p->org[1] += p->vel[1]*(frametime-.004);
			p->org[2] += p->vel[2]*(frametime-.004);
		}
		else if (p->type==pt_h2snow)
		{
			if(p->vel[0]==0&&p->vel[1]==0&&p->vel[2]==0)
			{//Stopped moving
				if(p->color==256+31)//Most translucent white
				{//Go away
					p->die=-1;
				}
				else
				{//Count fifty and fade in translucency once each time
					p->ramp+=1;
					if(p->ramp>=7)
					{
						p->color+=1;//Get more translucent
						p->ramp=0;
					}
				}
			}
			else
			{//FIXME: If flake going fast enough, can go through, do a check in increments ot 10, max?
				//if not in_bounds Get length of diff, add in increments of 4 & check solid

				if (Cvar_VariableValue("snow_flurry")==1)
				if(rand()&31)
				{//Add flurry movement
					float			snow_speed;
					vec3_t			save_vel;
					snow_speed = p->vel[0] * p->vel[0] + p->vel[1] * p->vel[1] + p->vel[2]*p->vel[2];
					snow_speed = sqrt(snow_speed);
					
					VectorCopy(p->vel,save_vel);
					
					save_vel[0] += ( (rand()*(2.0/RAND_MAX)) - 1)*30;
					save_vel[1] += ( (rand()*(2.0/RAND_MAX)) - 1)*30;
					if((rand()&7)||p->vel[2]>10)
						save_vel[2] += ( (rand()*(2.0/RAND_MAX)) - 1)*30;
					
					VectorNormalize(save_vel);
					VectorScale(save_vel,snow_speed,p->vel);//retain speed but use new dir
				}

				VectorScale(p->vel,frametime,diff);
				VectorAdd(p->org,diff,p->org);
				
				if(p->flags&SFL_IN_BOUNDS)
				{//Always stay inside the boundry!
					if(p->org[0]<p->minOrg[0]||p->org[0]>p->maxOrg[0]||
						p->org[1]<p->minOrg[1]||p->org[1]>p->maxOrg[1]||
						p->org[2]<p->minOrg[2]||p->org[2]>p->maxOrg[2])
					{
						p->die=-1;
					}
				}
				else
				{
					//IF hit solid, go to last position, no velocity, fade out.
					int contents = CM_PointContentsQ1(p->org, 0);
					if (contents != BSP29CONTENTS_EMPTY) //||in_solid==true
					{
						if(p->flags&SFL_NO_MELT)
						{//Don't melt, just die
							p->die=-1;
						}
						else
						{//still have small prob of snow melting on emitter
							VectorScale(diff,0.2,p->vel);
							i=6;
							while (contents != BSP29CONTENTS_EMPTY)
							{
								p->org[0] -= p->vel[0];
								p->org[1] -= p->vel[1];
								p->org[2] -= p->vel[2];
								i--;//no infinite loops
								if (!i)
								{
									p->die=-1;	//should never happen now!
									break;
								}
								contents = CM_PointContentsQ1(p->org, 0);
							}
							p->vel[0]=p->vel[1]=p->vel[2]=0;
							p->ramp=0;
						}
					}
				}
			}
		}
		else
		{
			p->org[0] += p->vel[0]*frametime;
			p->org[1] += p->vel[1]*frametime;
			p->org[2] += p->vel[2]*frametime;
		}

		switch (p->type)
		{
		case pt_h2static:
			break;

		case pt_h2fire:
			p->ramp += time1;
			if ((int)p->ramp >= 6)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp3[(int)p->ramp];
			}
			p->vel[2] += grav;
			break;

		case pt_h2explode:
			p->ramp += time2;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp1[(int)p->ramp];
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] += p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;

		case pt_h2explode2:
			p->ramp += time3;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp2[(int)p->ramp];
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] -= p->vel[i]*frametime;
			}
			p->vel[2] -= grav;
			break;

      case pt_h2c_explode:
			p->ramp += time2;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else if (time2)
			{
				p->color--;
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] += p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;

      case pt_h2c_explode2:
			p->ramp += time3;
			if ((int)p->ramp >=8)
			{
				p->die = -1;
			}
			else if (time3)
			{
				p->color -= 2;
			}
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] -= p->vel[i]*frametime;
			}
			p->vel[2] -= grav;
			break;

/*	//jfm:not used
		case pt_h2blob:
			for (i=0 ; i<3 ; i++)
			{
				p->vel[i] += p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;

		case pt_h2blob2:
			for (i=0 ; i<2 ; i++)
			{
				p->vel[i] -= p->vel[i]*dvel;
			}
			p->vel[2] -= grav;
			break;
*/
		case pt_h2grav:
		case pt_h2slowgrav:
			p->vel[2] -= grav;
			break;

		case pt_h2fastgrav:
			p->vel[2] -= grav*4;
			break;

      case pt_h2rain:
			break;

      case pt_h2snow:
			break;

	  case pt_h2fireball:
			p->ramp += time3;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp4[(int)p->ramp];
			}
			break;

		case pt_h2acidball:
			p->ramp += time4*1.4;
			if ((int)p->ramp >= 23)
			{
				p->die = -1;
			}
			else if ((int)p->ramp >= 15)
			{
				p->color = ramp11[(int)p->ramp - 15];
			}
			else
			{
				p->color = ramp10[(int)p->ramp];
			}
			p->vel[2] -= grav;
			break;

		case pt_h2spit:
			p->ramp += time3;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp6[(int)p->ramp];
			}
//			p->vel[2] += grav*2;
			break;

		case pt_h2ice:
			p->ramp += time4;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp5[(int)p->ramp];
			}
			p->vel[2] -= grav;
			break;

		case pt_h2spell:
			p->ramp += time2;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp7[(int)p->ramp];
			}
//			p->vel[2] += grav*2;
			break;

		case pt_h2test:
			p->vel[2] += 1.3;
			p->ramp += time3;
			if ((int)p->ramp >= 13 || ((int)p->ramp > 10 && (int)p->vel[2] < 20) )
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp8[(int)p->ramp];
			}
			break;

		case pt_h2quake:
			p->vel[0] *= 1.05;
			p->vel[1] *= 1.05;
			p->vel[2] -= grav*4;
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
				p->die = -1;
			}
			p->color = 256+16+16 - (p->ramp/(50/16));

			VectorSubtract(rider_origin, p->org, diff);

/*			p->org[0] += diff[0] * p->ramp / 80;
			p->org[1] += diff[1] * p->ramp / 80;
			p->org[2] += diff[2] * p->ramp / 80;
*/

			vel0 = 1 / (51 - p->ramp);
			p->org[0] += diff[0] * vel0;
			p->org[1] += diff[1] * vel0;
			p->org[2] += diff[2] * vel0;
			
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
				p->die = -1;
			}

			VectorSubtract(rider_origin, p->org, diff);

/*			p->org[0] += diff[0] * p->ramp / 80;
			p->org[1] += diff[1] * p->ramp / 80;
			p->org[2] += diff[2] * p->ramp / 80;
*/

			vel0 = 1 / (36 - p->ramp);
			p->org[0] += diff[0] * vel0;
			p->org[1] += diff[1] * vel0;
			p->org[2] += diff[2] * vel0;
			
			break;

		case pt_h2vorpal:
			--p->color; 
			if ((int)p->color <= 37 + 256)
			{
				p->die = -1;
			}
			break;

		case pt_h2setstaff:
			p->ramp += time1;
			if ((int)p->ramp >= 16)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp9[(int)p->ramp];
			}

			p->vel[0] *= 1.08 * percent;
			p->vel[1] *= 1.08 * percent;
			p->vel[2] -= grav2;
			break;

		case pt_h2redfire:
			p->ramp += frametime*3;
			if ((int)p->ramp >= 8)
			{
				p->die = -1;
			}
			else
			{
				p->color = ramp12[(int)p->ramp]+256;
			}

			p->vel[0] *= .9;
			p->vel[1] *= .9;
			p->vel[2] += grav/2;
			break;

		case pt_h2magicmissile:
			--p->color; 
			if ((int)p->color < 149)
			{
				p->color = 149;
			}
			p->ramp += time1;
			if ((int)p->ramp > 16)
			{
				p->die = -1;
			}
			break;

		case pt_h2boneshard:
			--p->color; 
			if ((int)p->color < 368)
			{
				p->die = -1;
			}
			break;

		case pt_h2scarab:
			--p->color; 
			if ((int)p->color < 250)
			{
				p->die = -1;
			}
			break;

		case pt_h2darken:
			p->vel[2] -= grav;	//Also gravity
			--p->color; 
			colindex=0;
			while(colindex<224)
			{
				if(colindex==192 || colindex == 200)
				{
					colindex+=8;
				}
				else
				{
					colindex+=16;
				}
				if (p->color==colindex)
				{
					p->die = -1;
				}
			}
			break;
		}
	}
}
