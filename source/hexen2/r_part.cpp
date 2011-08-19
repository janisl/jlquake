/*
 * $Header: /H2 Mission Pack/R_PART.C 54    4/01/98 6:43p Jmonroe $
 */

#include "quakedef.h"

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
		CLH2_RunParticleEffect (org, dir, count);
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
		dmin[i] = net_message.ReadFloat();
	for (i=0 ; i<3 ; i++)
		dmax[i] = net_message.ReadFloat();
	color = net_message.ReadShort ();
	msgcount = net_message.ReadByte ();
	effect = net_message.ReadByte ();

	CLH2_RunParticleEffect2 (org, dmin, dmax, color, hexen2ParticleTypeTable[effect], msgcount);
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

	CLH2_RunParticleEffect3 (org, box, color, hexen2ParticleTypeTable[effect], msgcount);
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

	CLH2_RunParticleEffect4 (org, radius, color, hexen2ParticleTypeTable[effect], msgcount);
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
		if (p->die - cl.serverTime < 0)
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
					p->die = killTime;
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
						p->die = killTime;
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
							p->die = killTime;
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
									p->die = killTime;	//should never happen now!
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
			if ((int)p->ramp >=8)
			{
				p->die = killTime;
			}
			else
			{
				p->color = h2ramp1[(int)p->ramp];
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
				p->die = killTime;
			}
			else
			{
				p->color = h2ramp2[(int)p->ramp];
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
				p->die = killTime;
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
				p->die = killTime;
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
				p->die = killTime;
			}
			else
			{
				p->color = h2ramp4[(int)p->ramp];
			}
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
			if ((int)p->ramp >= 16)
			{
				p->die = killTime;
			}
			else
			{
				p->color = h2ramp6[(int)p->ramp];
			}
//			p->vel[2] += grav*2;
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
//			p->vel[2] += grav*2;
			break;

		case pt_h2test:
			p->vel[2] += 1.3;
			p->ramp += time3;
			if ((int)p->ramp >= 13 || ((int)p->ramp > 10 && (int)p->vel[2] < 20) )
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

		case pt_h2magicmissile:
			--p->color; 
			if ((int)p->color < 149)
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
			if ((int)p->color < 368)
			{
				p->die = killTime;
			}
			break;

		case pt_h2scarab:
			--p->color; 
			if ((int)p->color < 250)
			{
				p->die = killTime;
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
					p->die = killTime;
				}
			}
			break;
		}
	}
}
