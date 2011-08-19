
#include "quakedef.h"

ptype_t hexenWorldParticleTypeTable[] =
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
	pt_h2darken,
	pt_h2grensmoke,
	pt_h2redfire,
	pt_h2acidball,
	pt_h2bluestep,
};

void R_SuccubusInvincibleParticles (refEntity_t *ent)
{
	int			count;
	cparticle_t	*p;
	vec3_t		ent_angles,forward,org;
	
	ent_angles[0]=ent_angles[2]=0;
	ent_angles[1]=cl.serverTimeFloat*12;
	forward[0]=cos(ent_angles[1])*32;
	forward[1]=sin(ent_angles[1])*32;
	forward[2]=0;
	VectorCopy(ent->origin,org);
	org[2]+=28;

	count=140*host_frametime;
	while(count>0)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->ramp = 0;
		p->die = cl.serverTime + 2000;
		p->color = 416;
		p->type = pt_h2fireball;
		
		p->org[0] = org[0] + forward[0] + rand()%4-2;
		p->org[1] = org[1] + forward[1] + rand()%4-2;
		p->org[2] = org[2] + forward[2] + rand()%4-2;
		p->vel[0] = rand()%20-10;
		p->vel[1] = rand()%20-10;
		p->vel[2] = rand()%25+20;
		count--;
	}

	count=60*host_frametime;
	while(count>0)
	{
		p = CL_AllocParticle();
		if (!p)
			return;

		p->ramp = 0;
		p->die = cl.serverTime + 2000;
		p->color = 135;
		p->type = pt_h2redfire;
		
		p->org[0] = org[0] + forward[0] + rand()%4-2;
		p->org[1] = org[1] + forward[1] + rand()%4-2;
		p->org[2] = org[2] + forward[2] + rand()%4-2;
		p->vel[0] = rand()%20-10;
		p->vel[1] = rand()%20-10;
		p->vel[2] = 0;
		count--;
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
	int			i, count, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = net_message.ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = net_message.ReadChar() * (1.0/16);
	count = net_message.ReadByte ();
	color = net_message.ReadByte ();

	if (count == 255)
	{
		CLH2_ParticleExplosion(org);
	}
	else
	{
		CLH2_RunParticleEffect(org, dir, count);
	}
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
		dmin[i] = net_message.ReadFloat ();
	for (i=0 ; i<3 ; i++)
		dmax[i] = net_message.ReadFloat ();
	color = net_message.ReadShort ();
	msgcount = net_message.ReadByte ();
	effect = net_message.ReadByte ();

	CLH2_RunParticleEffect2 (org, dmin, dmax, color, hexenWorldParticleTypeTable[effect], msgcount);
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

	CLH2_RunParticleEffect3 (org, box, color, hexenWorldParticleTypeTable[effect], msgcount);
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

	CLH2_RunParticleEffect4 (org, radius, color, hexenWorldParticleTypeTable[effect], msgcount);
}

void R_ParseRainEffect(void)
{
	vec3_t		org, e_size;
	short		color,count;
	int			x_dir, y_dir;

	org[0] = net_message.ReadCoord();
	org[1] = net_message.ReadCoord();
	org[2] = net_message.ReadCoord();
	e_size[0] = net_message.ReadCoord();
	e_size[1] = net_message.ReadCoord();
	e_size[2] = net_message.ReadCoord();
	x_dir = net_message.ReadAngle();
	y_dir = net_message.ReadAngle();
	color = net_message.ReadShort();
	count = net_message.ReadShort();

	CLH2_RainEffect (org,e_size,x_dir,y_dir,color,count);
}

/*
===============
CLH2_LavaSplash

===============
*/
void R_TargetBallEffect (vec3_t org)
{
	int			i;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=0 ; i < 40 * host_frametime ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;
		if(v_targDist < 60)
		{
			p->die = cl.serverTime + (rand()&3) * 20 + (230 * (1.0 - (.23 * (v_targDist - 24.0)/36.0)));
		}
		else
		{
			p->die = cl.serverTime + (300 * ((256.0 - v_targDist)/256.0)) + (rand()&7) * 20;
		}
		p->color = 7 + (rand()%24);
		p->type = pt_h2slowgrav;
		
		dir[0] = (rand()&63) - 31;
		dir[1] = (rand()&63) - 31;
		dir[2] = 256;

		p->org[0] = org[0] + (rand()&3) - 2;
		p->org[1] = org[1] + (rand()&3) - 2;
		p->org[2] = org[2] + (rand()&3);

		VectorNormalize (dir);					
		vel = 50 + (rand()&63);
		VectorScale (dir, vel, p->vel);
	}
}


/*
===============
brightfieldsource

===============
*/

void R_BrightFieldSource (vec3_t org)
{
	int			i;
	cparticle_t	*p;
	float		vel;
	vec3_t		dir;
	float		height;

	height = cos(cl.serverTimeFloat * 4.0) * 25;

	for (i=0 ; i < 120 * host_frametime ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;
		p->die = cl.serverTime + 500;
		p->color = 143;
		//p->type = pt_h2slowgrav;
		p->type = pt_h2quake;
		
		
		dir[0] = (rand()%256) - 128;
		dir[1] = (rand()%256) - 128;
		dir[2] = 32;

		p->org[0] = org[0] + (rand()&15) - 7;
		p->org[1] = org[1] + (rand()&15) - 7;
		p->org[2] = org[2] + height;// + (rand()&3);

		VectorNormalize (dir);					
		vel = 70 + (rand()&31);
		VectorScale (dir, vel, p->vel);
	}

	for (i=0 ; i < 120 * host_frametime ; i++)
	{
		p = CL_AllocParticle();
		if (!p)
			return;
		p->die = cl.serverTime + 500;
		p->color = 159;
		//p->type = pt_h2slowgrav;
		p->type = pt_h2quake;
		
		dir[0] = (rand()%256) - 128;
		dir[1] = (rand()%256) - 128;
		dir[2] = 32;

		p->org[0] = org[0] + (rand()&15) - 7;
		p->org[1] = org[1] + (rand()&15) - 7;
		p->org[2] = org[2] - height;// + (rand()&3);

		VectorNormalize (dir);					
		vel = 70 + (rand()&31);
		VectorScale (dir, vel, p->vel);
	}
}

static void AddParticle(cparticle_t *p)
{
	if (p->color < 0 || p->color > 511)
	{
		Con_Printf("Invalid color for particle type %d\n",(int)p->type);
		return;
	}

	byte* at;
	byte theAlpha;

	if (p->color <= 255)
	{
		at = r_palette[p->color];
		if (p->type==pt_h2fire)
			theAlpha = 255*(6-p->ramp)/6;
		else
			theAlpha = 255;
	}
	else
	{
		at = (byte *)&d_8to24TranslucentTable[p->color - 256];
		theAlpha = at[3];
	}

	R_AddParticleToScene(p->org, at[0], at[1], at[2], theAlpha, 1, PARTTEX_Default);
}

/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles()
{
	cparticle_t		temp_p;
	float			vel0, vel1, vel2;
    
	for (cparticle_t* p = active_particles; p; p = p->next)
	{
		if (p->die - cl.serverTime < 0)
		{
			continue;
		}

		if (p->type == pt_h2rain)
		{
			temp_p = *p;

			vel0 = temp_p.vel[0]*.001;
			vel1 = temp_p.vel[1]*.001;
			vel2 = temp_p.vel[2]*.001;
			for(int i = 0; i < 4; i++)
			{
				AddParticle(&temp_p);

				temp_p.org[0] += vel0;
				temp_p.org[1] += vel1;
				temp_p.org[2] += vel2;
 			}
		}
		else
		{
			AddParticle(p);
		}
	}
}

/*
===============
R_UpdateParticles
===============
*/
void R_UpdateParticles (void)
{
	cparticle_t		*p, *kill;
	float			grav,grav2,percent;
	int				i;
	float			time2, time3, time4;
	float			time1;
	float			dvel;
	float			frametime;
	float			vel0;
	vec3_t			diff;
    
	frametime = host_frametime;
	time4 = frametime * 20;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = frametime * movevars.gravity * 0.05;
	grav2 = frametime * movevars.gravity * 0.025;
	dvel = 4*frametime;
	percent = (frametime / HX_FRAME_TIME);
	//	Time for particles that need to be killed ASAP.
	int killTime = cl.serverTime - 1;

	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die - cl.serverTime < 0)
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
			if (kill && kill->die - cl.serverTime < 0)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}


		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_h2static:
			break;
		case pt_h2fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = killTime;
			else
				p->color = h2ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_h2explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = killTime;
			else
				p->color = h2ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_h2explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = killTime;
			else
				p->color = h2ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

      case pt_h2c_explode:
			p->ramp += time2;
			if (p->ramp >= 8 || p->color <= 0)
				p->die = killTime;
			else if (time2)
				p->color--;
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

      case pt_h2c_explode2:
			p->ramp += time3;
			if (p->ramp >= 8 || p->color <= 1)
				p->die = killTime;
			else if (time3)
				p->color -= 2;
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			p->vel[2] -= grav;
			break;

		case pt_h2blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_h2blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_h2grav:
		case pt_h2slowgrav:
			p->vel[2] -= grav;
			break;

		case pt_h2grensmoke:
			p->vel[0] += time3 * ((rand()%3)-1);
			p->vel[1] += time3 * ((rand()%3)-1);
			p->vel[2] += time3 * ((rand()%3)-1);
			break;

		case pt_h2fastgrav:
			p->vel[2] -= grav*4;
			break;

      case pt_h2rain:
			break;

		case pt_h2fireball:
			p->ramp += time3;
			if (p->ramp >= 16)
				p->die = killTime;
			else
				p->color = h2ramp4[(int)p->ramp];
			break;

		case pt_h2acidball:
			p->ramp += time4*1.4;
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
			if (p->ramp >= 16)
				p->die = killTime;
			else
				p->color = h2ramp6[(int)p->ramp];
//			p->vel[2] += grav*2;
			break;

		case pt_h2ice:
			p->ramp += time4;
			if (p->ramp > 15)
				p->die = killTime;
			else
				p->color = h2ramp5[(int)p->ramp];
			p->vel[2] -= grav;
			break;

		case pt_h2spell:
			p->ramp += time2;
			if (p->ramp >= 16)
				p->die = killTime;
			else
				p->color = h2ramp7[(int)p->ramp];
//			p->vel[2] += grav*2;
			break;

		case pt_h2test:
			p->vel[2] += 1.3;
			p->ramp += time3;
			if (p->ramp >= 13 || (p->ramp > 10 && p->vel[2] < 20) )
				p->die = killTime;
			else
				p->color = h2ramp8[(int)p->ramp];
			break;

		case pt_h2quake:
			p->vel[0] *= 1.05;
			p->vel[1] *= 1.05;
			p->vel[2] -= grav*4;
			if(p->color < 160 && p->color > 143)
			{
				p->color = 152 + 7 * ((p->die - cl.serverTime) * 2 / 1000);
			}
			if(p->color < 144 && p->color > 127)
			{
				p->color = 136 + 7 * ((p->die - cl.serverTime) * 2 / 1000);
			}
			break;

		case pt_h2rd:
			if (!frametime) break;

			p->ramp += percent;
			if (p->ramp > 50) 
			{
				p->ramp = 50;
				p->die = killTime;
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

		case pt_h2vorpal:
			--p->color; 
			if (p->color <= 37 + 256)
				p->die = killTime;
			break;

		case pt_h2setstaff:
			p->ramp += time1;
			if (p->ramp >= 16)
				p->die = killTime;
			else
				p->color = h2ramp9[(int)p->ramp];

			p->vel[0] *= 1.08 * percent;
			p->vel[1] *= 1.08 * percent;
			p->vel[2] -= grav2;
			break;

		case pt_h2redfire:
			p->ramp += frametime*3;
			if ((int)p->ramp >= 8)
			{
				p->die = killTime;
			}
			else
			{
				p->color = h2ramp12[(int)p->ramp]+256;
			}

			p->vel[0] *= .9;
			p->vel[1] *= .9;
			p->vel[2] += grav/2;
			break;

		case pt_h2bluestep:
			p->ramp += frametime*8;
			if ((int)p->ramp >= 16)
			{
				p->die = killTime;
			}
			else
			{
				p->color = h2ramp13[(int)p->ramp]+256;
			}

			p->vel[0] *= .9;
			p->vel[1] *= .9;
			p->vel[2] += grav;
			break;

		case pt_h2magicmissile:
			--p->color; 
			if (p->color < 149)
				p->color = 149;
			p->ramp += time1;
			if (p->ramp > 16)
				p->die = killTime;
			break;

		case pt_h2boneshard:
			--p->color; 
			if (p->color < 368)
				p->die = killTime;
			break;
		case pt_h2scarab:
			--p->color; 
			if (p->color < 250)
				p->die = killTime;
			break;

		case pt_h2darken:
			{
				int	colindex;
				p->vel[2] -= grav*2;	//Also gravity
				if(rand()&1)--p->color; 
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
						p->die = killTime;
					}
				}
			}
			break;
		}
	}
}
