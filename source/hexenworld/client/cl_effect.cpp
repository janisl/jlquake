
//**************************************************************************
//**
//** cl_effect.c
//**
//** Client side effects.
//**
//** $Header: /HexenWorld/Client/cl_effect.c 89    5/25/98 1:29p Mgummelt $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------
extern void CreateStream(int type, int ent, int flags, int tag, float duration, int skin, vec3_t source, vec3_t dest);
extern void CLTENT_XbowImpact(vec3_t pos, vec3_t vel, int chType, int damage, int arrowType);//so xbow effect can use tents
extern void CLTENT_SpawnDeathBubble(vec3_t pos);
h2entity_state_t *FindState(int EntNum);

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

// these are in cl_tent.c
void CreateRavenDeath(vec3_t pos);
void CreateExplosionWithSound(vec3_t pos);

void CL_EndEffect(void)
{
	int index;
	effect_entity_t* ent;

	index = net_message.ReadByte();

	switch(cl.h2_Effects[index].type )
	{
	case HWCE_HWRAVENPOWER:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			CreateRavenDeath(ent->state.origin);
		}
		break;
	case HWCE_HWRAVENSTAFF:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			CreateExplosionWithSound(ent->state.origin);
		}
		break;
	}
	CLH2_FreeEffect(index);
}

void XbowImpactPuff(vec3_t origin, int material)//hopefully can use this with xbow & chain both
{
	int		part_color;

	switch(material)
	{
	case H2XBOW_IMPACT_REDFLESH:
		part_color = 256 + 8 * 16 + rand()%9;				//Blood red
		break;
	case H2XBOW_IMPACT_STONE:
		part_color = 256 + 20 + rand()%8;			// Gray
		break;
	case H2XBOW_IMPACT_METAL:
		part_color = 256 + (8 * 15);			// Sparks
		break;
	case H2XBOW_IMPACT_WOOD:
		part_color = 256 + (5 * 16) + rand()%8;			// Wood chunks
		break;
	case H2XBOW_IMPACT_ICE:
		part_color = 406+rand()%8;				// Ice particles
		break;
	case H2XBOW_IMPACT_GREENFLESH:
		part_color = 256 + 183 + rand()%8;		// Spider's have green blood
		break;
	default:
		part_color = 256 + (3 * 16) + 4;		// Dust Brown
		break;
	}

	CLH2_RunParticleEffect4 (origin, 24, part_color, pt_h2fastgrav, 20);
}

void CL_ReviseEffect(void)	// be sure to read, in the switch statement, everything
							// in this message, even if the effect is not the right kind or invalid,
							// or else client is sure to crash.	
{
	int index,type,revisionCode;
	int curEnt,material,takedamage;
	effect_entity_t* ent;
	vec3_t	forward,right,up,pos;
	float	dist,speed;
	h2entity_state_t	*es;


	index = net_message.ReadByte ();
	type = net_message.ReadByte ();

	if (cl.h2_Effects[index].type==type)
		switch(type)
		{
		case HWCE_SCARABCHAIN://attach to new guy or retract if new guy is world
			curEnt = net_message.ReadShort();
			if (cl.h2_Effects[index].type==type)
			{
				cl.h2_Effects[index].Chain.material = curEnt>>12;
				curEnt &= 0x0fff;

				if (curEnt)
				{
					cl.h2_Effects[index].Chain.state = 1;
					cl.h2_Effects[index].Chain.owner = curEnt;
					es = FindState(cl.h2_Effects[index].Chain.owner);
					if (es)
					{
						ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];
						XbowImpactPuff(ent->state.origin, cl.h2_Effects[index].Chain.material);
					}
				}
				else
					cl.h2_Effects[index].Chain.state = 2;
			}
			break;
		case HWCE_HWXBOWSHOOT:
			revisionCode = net_message.ReadByte();
			//this is one packed byte!
			//highest bit: for impact revision, indicates whether damage is done
			//				for redirect revision, indicates whether new origin was sent
			//next 3 high bits: for all revisions, indicates which bolt is to be revised
			//highest 3 of the low 4 bits: for impact revision, indicates the material that was hit
			//lowest bit: indicates whether revision is of impact or redirect variety


			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact effect: 
			{
				cl.h2_Effects[index].Xbow.activebolts &= ~(1<<curEnt);
				dist = net_message.ReadCoord();
				if (cl.h2_Effects[index].Xbow.ent[curEnt]!= -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];

					//make sure bolt is in correct position
					VectorCopy(cl.h2_Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,dist,forward);
					VectorAdd(cl.h2_Effects[index].Xbow.origin[curEnt],forward,ent->state.origin);

					material = (revisionCode & 14);
					takedamage = (revisionCode & 128);

					if (takedamage)
					{
						cl.h2_Effects[index].Xbow.gonetime[curEnt] = cl.serverTimeFloat;
					}
					else
					{
						cl.h2_Effects[index].Xbow.gonetime[curEnt] += cl.serverTimeFloat;
					}
					
					VectorCopy(cl.h2_Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,8,forward);

					// do a particle effect here, with the color depending on chType
					XbowImpactPuff(ent->state.origin,material);

					// impact sound:
					switch (material)
					{
					case H2XBOW_IMPACT_GREENFLESH:
					case H2XBOW_IMPACT_REDFLESH:
					case H2XBOW_IMPACT_MUMMY:
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_arr2flsh, 1, 1);
						break;
					case H2XBOW_IMPACT_WOOD:
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_arr2wood, 1, 1);
						break;
					default:
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 0, clh2_fxsfx_met2stn, 1, 1);
						break;
					}

					CLTENT_XbowImpact(ent->state.origin, forward, material, takedamage, cl.h2_Effects[index].Xbow.bolts);
				}
			}
			else
			{
				if (cl.h2_Effects[index].Xbow.ent[curEnt]!=-1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];
					ent->state.angles[0] = net_message.ReadAngle();
					if (ent->state.angles[0] < 0)
						ent->state.angles[0] += 360;
					ent->state.angles[0]*=-1;
					ent->state.angles[1] = net_message.ReadAngle();
					if (ent->state.angles[1] < 0)
						ent->state.angles[1] += 360;
					ent->state.angles[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(ent->state.angles,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorCopy(cl.h2_Effects[index].Xbow.origin[curEnt],ent->state.origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
				}
			}
			break;

		case HWCE_HWSHEEPINATOR:
			revisionCode = net_message.ReadByte();
			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact
			{
				dist = net_message.ReadCoord();
				cl.h2_Effects[index].Xbow.activebolts &= ~(1<<curEnt);
				if (cl.h2_Effects[index].Xbow.ent[curEnt] != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];

					//make sure bolt is in correct position
					VectorCopy(cl.h2_Effects[index].Xbow.vel[curEnt],forward);
					VectorNormalize(forward);
					VectorScale(forward,dist,forward);
					VectorAdd(cl.h2_Effects[index].Xbow.origin[curEnt],forward,ent->state.origin);
					CLH2_ColouredParticleExplosion(ent->state.origin,(rand()%16)+144/*(144,159)*/,20,30);
				}
			}
			else//direction change
			{
				if (cl.h2_Effects[index].Xbow.ent[curEnt] != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[curEnt]];
					ent->state.angles[0] = net_message.ReadAngle();
					if (ent->state.angles[0] < 0)
						ent->state.angles[0] += 360;
					ent->state.angles[0]*=-1;
					ent->state.angles[1] = net_message.ReadAngle();
					if (ent->state.angles[1] < 0)
						ent->state.angles[1] += 360;
					ent->state.angles[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(ent->state.angles,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorCopy(cl.h2_Effects[index].Xbow.origin[curEnt],ent->state.origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					if (revisionCode &128)//new origin
					{
						cl.h2_Effects[index].Xbow.origin[curEnt][0]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][1]=net_message.ReadCoord();
						cl.h2_Effects[index].Xbow.origin[curEnt][2]=net_message.ReadCoord();
					}

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Xbow.vel[curEnt]);
					VectorScale(forward,speed,cl.h2_Effects[index].Xbow.vel[curEnt]);
				}
			}
			break;


		case HWCE_HWDRILLA:
			revisionCode = net_message.ReadByte();
			if (revisionCode == 0)//impact
			{
				pos[0] = net_message.ReadCoord();
				pos[1] = net_message.ReadCoord();
				pos[2] = net_message.ReadCoord();
				material = net_message.ReadByte();

				//throw lil bits of victim at entry
				XbowImpactPuff(pos,material);

				if ((material == H2XBOW_IMPACT_GREENFLESH) || (material == H2XBOW_IMPACT_GREENFLESH))
				{//meaty sound and some chunks too
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, clh2_fxsfx_drillameat, 1, 1);
					
					//todo: the chunks
				}

				//lil bits at exit
				VectorCopy(cl.h2_Effects[index].Missile.velocity,forward);
				VectorNormalize(forward);
				VectorScale(forward,36,forward);
				VectorAdd(forward,pos,pos);
				XbowImpactPuff(pos,material);
			}
			else//turn
			{
				if (cl.h2_Effects[index].Missile.entity_index!=-1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
					ent->state.angles[0] = net_message.ReadAngle();
					if (ent->state.angles[0] < 0)
						ent->state.angles[0] += 360;
					ent->state.angles[0]*=-1;
					ent->state.angles[1] = net_message.ReadAngle();
					if (ent->state.angles[1] < 0)
						ent->state.angles[1] += 360;
					ent->state.angles[2] = 0;

					cl.h2_Effects[index].Missile.origin[0]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[1]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[2]=net_message.ReadCoord();

					AngleVectors(ent->state.angles,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Missile.velocity);
					VectorScale(forward,speed,cl.h2_Effects[index].Missile.velocity);
					VectorCopy(cl.h2_Effects[index].Missile.origin,ent->state.origin);
				}
				else
				{
					pos[0] = net_message.ReadAngle();
					if (pos[0] < 0)
						pos[0] += 360;
					pos[0]*=-1;
					pos[1] = net_message.ReadAngle();
					if (pos[1] < 0)
						pos[1] += 360;
					pos[2] = 0;

					cl.h2_Effects[index].Missile.origin[0]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[1]=net_message.ReadCoord();
					cl.h2_Effects[index].Missile.origin[2]=net_message.ReadCoord();

					AngleVectors(pos,forward,right,up);
					speed = VectorLength(cl.h2_Effects[index].Missile.velocity);
					VectorScale(forward,speed,cl.h2_Effects[index].Missile.velocity);
				}
			}
			break;
		}
	else
	{
//		Con_DPrintf("Received Unrecognized Effect Update!\n");
		switch(type)
		{
		case HWCE_SCARABCHAIN://attach to new guy or retract if new guy is world
			curEnt = net_message.ReadShort();
			break;
		case HWCE_HWXBOWSHOOT:
			revisionCode = net_message.ReadByte();

			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact effect: 
			{
				net_message.ReadCoord();
			}
			else
			{
				net_message.ReadAngle();
				net_message.ReadAngle();
				if (revisionCode &128)//new origin
				{
					net_message.ReadCoord();
					net_message.ReadCoord();
					net_message.ReadCoord();

					// create a clc message to retrieve effect information
//					cls.netchan.message.WriteByte(clc_get_effect);
//					cls.netchan.message.WriteByte(index);
				}
			}
			break;
		case HWCE_HWSHEEPINATOR:
			revisionCode = net_message.ReadByte();
			curEnt = (revisionCode>>4)&7;
			if (revisionCode & 1)//impact
			{
				net_message.ReadCoord();
			}
			else//direction change
			{
				net_message.ReadAngle();
				net_message.ReadAngle();
				if (revisionCode &128)//new origin
				{
					net_message.ReadCoord();
					net_message.ReadCoord();
					net_message.ReadCoord();

					// create a clc message to retrieve effect information
//					cls.netchan.message.WriteByte(clc_get_effect);
//					cls.netchan.message.WriteByte(index);
				}
			}
			break;
		case HWCE_HWDRILLA:
			revisionCode = net_message.ReadByte();
			if (revisionCode == 0)//impact
			{
				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadByte();
			}
			else//turn
			{
				net_message.ReadAngle();
				net_message.ReadAngle();

				net_message.ReadCoord();
				net_message.ReadCoord();
				net_message.ReadCoord();

				// create a clc message to retrieve effect information
//				cls.netchan.message.WriteByte(clc_get_effect);
//				cls.netchan.message.WriteByte(index);

			}
			break;
		}
	}
}

void UpdateMissilePath(vec3_t oldorg, vec3_t neworg, vec3_t newvel, float time)
{
	vec3_t endpos;	//the position it should be at currently
	float delta;

	delta = cl.serverTimeFloat - time;
	
	VectorMA(neworg, delta, newvel, endpos); 
	VectorCopy(neworg, oldorg);	//set orig, maybe vel too
}


void CL_TurnEffect(void)
{
	int index;
	effect_entity_t* ent;
	vec3_t pos, vel;
	float time;

	index = net_message.ReadByte ();
	time = net_message.ReadFloat ();
	pos[0] = net_message.ReadCoord();
	pos[1] = net_message.ReadCoord();
	pos[2] = net_message.ReadCoord();
	vel[0] = net_message.ReadCoord();
	vel[1] = net_message.ReadCoord();
	vel[2] = net_message.ReadCoord();
	switch(cl.h2_Effects[index].type)
	{
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			UpdateMissilePath(ent->state.origin, pos, vel, time);
			VectorCopy(vel, cl.h2_Effects[index].Missile.velocity);
			VecToAnglesBuggy(cl.h2_Effects[index].Missile.velocity, cl.h2_Effects[index].Missile.angle);
		}
		break;
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
		if(cl.h2_Effects[index].Star.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Star.entity_index];
			UpdateMissilePath(ent->state.origin, pos, vel, time);
			VectorCopy(vel, cl.h2_Effects[index].Star.velocity);
		}
		break;
	case 0:
		// create a clc message to retrieve effect information
//		cls.netchan.message.WriteByte(clc_get_effect);
//		cls.netchan.message.WriteByte(index);
//		Con_Printf("CL_TurnEffect: null effect %d\n", index);
		break;
	default:
		Con_Printf ("CL_TurnEffect: bad type %d\n", cl.h2_Effects[index].type);
		break;
	}

}

void CL_UpdateEffects(void)
{
	int			index;
	float		frametime;
	vec3_t		org,org2,old_origin;
	effect_entity_t	*ent, *ent2;
	int			i;
	h2entity_state_t	*es;

	frametime = host_frametime;
	if (!frametime) return;
//	Con_Printf("Here at %f\n",cl.time);

	for(index=0;index<MAX_EFFECTS_H2;index++)
	{
		if (!cl.h2_Effects[index].type) 
			continue;

		switch(cl.h2_Effects[index].type)
		{
			case HWCE_RAIN:
				CLH2_UpdateEffectRain(index, frametime);
				break;
			case HWCE_FOUNTAIN:
				CLH2_UpdateEffectFountain(index);
				break;
			case HWCE_QUAKE:
				CLH2_UpdateEffectQuake(index);
				break;
			case HWCE_RIPPLE:
				CLHW_UpdateEffectRipple(index, frametime);
				break;
			case HWCE_WHITE_SMOKE:
			case HWCE_GREEN_SMOKE:
			case HWCE_GREY_SMOKE:
			case HWCE_RED_SMOKE:
			case HWCE_SLOW_WHITE_SMOKE:
			case HWCE_TELESMK2:
			case HWCE_GHOST:
			case HWCE_REDCLOUD:
			case HWCE_FLAMESTREAM:
			case HWCE_ACID_MUZZFL:
			case HWCE_FLAMEWALL:
			case HWCE_FLAMEWALL2:
			case HWCE_ONFIRE:
				CLH2_UpdateEffectSmoke(index, frametime);
				break;
			case HWCE_TELESMK1:
				CLHW_UpdateEffectTeleportSmoke1(index, frametime);
				break;
			case HWCE_SM_WHITE_FLASH:
			case HWCE_YELLOWRED_FLASH:
			case HWCE_BLUESPARK:
			case HWCE_YELLOWSPARK:
			case HWCE_SM_CIRCLE_EXP:
			case HWCE_SM_EXPLOSION:
			case HWCE_SM_EXPLOSION2:
			case HWCE_BG_EXPLOSION:
			case HWCE_FLOOR_EXPLOSION:
			case HWCE_BLUE_EXPLOSION:
			case HWCE_REDSPARK:
			case HWCE_GREENSPARK:
			case HWCE_ICEHIT:
			case HWCE_MEDUSA_HIT:
			case HWCE_MEZZO_REFLECT:
			case HWCE_FLOOR_EXPLOSION2:
			case HWCE_XBOW_EXPLOSION:
			case HWCE_NEW_EXPLOSION:
			case HWCE_MAGIC_MISSILE_EXPLOSION:
			case HWCE_BONE_EXPLOSION:
			case HWCE_BLDRN_EXPL:
			case HWCE_BRN_BOUNCE:
			case HWCE_ACID_HIT:
			case HWCE_ACID_SPLAT:
			case HWCE_ACID_EXPL:
			case HWCE_LBALL_EXPL:
			case HWCE_FBOOM:
			case HWCE_BOMB:
			case HWCE_FIREWALL_SMALL:
			case HWCE_FIREWALL_MEDIUM:
			case HWCE_FIREWALL_LARGE:
				CLH2_UpdateEffectExplosion(index, frametime);
				break;
			case HWCE_BG_CIRCLE_EXP:
				CLH2_UpdateEffectBigCircleExplosion(index, frametime);
				break;
			case HWCE_WHITE_FLASH:
			case HWCE_BLUE_FLASH:
			case HWCE_SM_BLUE_FLASH:
			case HWCE_HWSPLITFLASH:
			case HWCE_RED_FLASH:
				CLH2_UpdateEffectFlash(index, frametime);
				break;
			case HWCE_RIDER_DEATH:
				cl.h2_Effects[index].RD.time_amount += frametime;
				if (cl.h2_Effects[index].RD.time_amount >= 1)
				{
					cl.h2_Effects[index].RD.stage++;
					cl.h2_Effects[index].RD.time_amount -= 1;
				}

				VectorCopy(cl.h2_Effects[index].RD.origin,org);
				org[0] += sin(cl.h2_Effects[index].RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.h2_Effects[index].RD.time_amount * 2 * M_PI) * 30;

				if (cl.h2_Effects[index].RD.stage <= 6)
				{
					CLH2_RiderParticles(cl.h2_Effects[index].RD.stage+1,org);
				}
				else
				{
					// To set the rider's origin point for the particles
					CLH2_RiderParticles(0,org);
					if (cl.h2_Effects[index].RD.stage == 7) 
					{
						cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
						cl.cshifts[CSHIFT_BONUS].percent = 256;
					}
					else if (cl.h2_Effects[index].RD.stage > 13) 
					{
						CLH2_FreeEffect(index);
					}
				}
				break;
			case HWCE_TELEPORTERPUFFS:
				CLH2_UpdateEffectTeleporterPuffs(index, frametime);
				break;
			case HWCE_TELEPORTERBODY:
				CLH2_UpdateEffectTeleporterBody(index, frametime);
				break;

			case HWCE_HWDRILLA:
				cl.h2_Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];

				if ((int)(cl.serverTimeFloat) != (int)(cl.serverTimeFloat - host_frametime))
				{
					S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_drillaspin, 1, 1);
				}

				ent->state.angles[0] += frametime * cl.h2_Effects[index].Missile.avelocity[0];
				ent->state.angles[1] += frametime * cl.h2_Effects[index].Missile.avelocity[1];
				ent->state.angles[2] += frametime * cl.h2_Effects[index].Missile.avelocity[2];

				VectorCopy(ent->state.origin,old_origin);

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Missile.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Missile.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Missile.velocity[2];

				CLH2_TrailParticles (old_origin, ent->state.origin, rt_setstaff);

				CLH2_LinkEffectEntity(ent);
				break;
			case HWCE_HWXBOWSHOOT:
				cl.h2_Effects[index].Xbow.time_amount += frametime;
				for (i=0;i<cl.h2_Effects[index].Xbow.bolts;i++)
				{
					if (cl.h2_Effects[index].Xbow.ent[i] != -1)//only update valid effect ents
					{
						if (cl.h2_Effects[index].Xbow.activebolts & (1<<i))//bolt in air, simply update position
						{
							ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

							ent->state.origin[0] += frametime * cl.h2_Effects[index].Xbow.vel[i][0];
							ent->state.origin[1] += frametime * cl.h2_Effects[index].Xbow.vel[i][1];
							ent->state.origin[2] += frametime * cl.h2_Effects[index].Xbow.vel[i][2];

							CLH2_LinkEffectEntity(ent);
						}
						else if (cl.h2_Effects[index].Xbow.bolts == 5)//fiery bolts don't just go away
						{
							if (cl.h2_Effects[index].Xbow.state[i] == 0)//waiting to explode state
							{
								if (cl.h2_Effects[index].Xbow.gonetime[i] > cl.serverTimeFloat)//fiery bolts stick around for a while
								{
									ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];
									CLH2_LinkEffectEntity(ent);
								}
								else//when time's up on fiery guys, they explode
								{
									//set state to exploding
									cl.h2_Effects[index].Xbow.state[i] = 1;

									ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

									//move bolt back a little to make explosion look better
									VectorNormalize(cl.h2_Effects[index].Xbow.vel[i]);
									VectorScale(cl.h2_Effects[index].Xbow.vel[i],-8,cl.h2_Effects[index].Xbow.vel[i]);
									VectorAdd(ent->state.origin,cl.h2_Effects[index].Xbow.vel[i],ent->state.origin);

									//turn bolt entity into an explosion
									ent->model = R_RegisterModel("models/xbowexpl.spr");
									ent->state.frame = 0;

									//set frame change counter
									cl.h2_Effects[index].Xbow.gonetime[i] = cl.serverTimeFloat + HX_FRAME_TIME * 2;

									//play explosion sound
									S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_explode, 1, 1);

									CLH2_LinkEffectEntity(ent);
								}
							}
							else if (cl.h2_Effects[index].Xbow.state[i] == 1)//fiery bolt exploding state
							{
								ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

								//increment frame if it's time
								while(cl.h2_Effects[index].Xbow.gonetime[i] <= cl.serverTimeFloat)
								{
									ent->state.frame++;
									cl.h2_Effects[index].Xbow.gonetime[i] += HX_FRAME_TIME * 0.75;
								}


								if (ent->state.frame >= R_ModelNumFrames(ent->model))
								{
									cl.h2_Effects[index].Xbow.state[i] = 2;//if anim is over, set me to inactive state
								}
								else
									CLH2_LinkEffectEntity(ent);
							}
						}
					}
				}
				break;
			case HWCE_HWSHEEPINATOR:
				cl.h2_Effects[index].Xbow.time_amount += frametime;
				for (i=0;i<cl.h2_Effects[index].Xbow.bolts;i++)
				{
					if (cl.h2_Effects[index].Xbow.ent[i] != -1)//only update valid effect ents
					{
						if (cl.h2_Effects[index].Xbow.activebolts & (1<<i))//bolt in air, simply update position
						{
							ent = &EffectEntities[cl.h2_Effects[index].Xbow.ent[i]];

							ent->state.origin[0] += frametime * cl.h2_Effects[index].Xbow.vel[i][0];
							ent->state.origin[1] += frametime * cl.h2_Effects[index].Xbow.vel[i][1];
							ent->state.origin[2] += frametime * cl.h2_Effects[index].Xbow.vel[i][2];

							CLH2_RunParticleEffect4(ent->state.origin,7,(rand()%15)+144,pt_h2explode2,(rand()%5)+1);

							CLH2_LinkEffectEntity(ent);
						}
					}
				}
				break;
			case HWCE_DEATHBUBBLES:
				cl.h2_Effects[index].Bubble.time_amount += frametime;
				if (cl.h2_Effects[index].Bubble.time_amount > 0.1)//10 bubbles a sec
				{
					cl.h2_Effects[index].Bubble.time_amount = 0;
					cl.h2_Effects[index].Bubble.count--;
					es = FindState(cl.h2_Effects[index].Bubble.owner);
					if (es)
					{
						VectorCopy(es->origin,org);
						VectorAdd(org,cl.h2_Effects[index].Bubble.offset,org);

						if (CM_PointContentsQ1(org, 0) != BSP29CONTENTS_WATER) 
						{
							//not in water anymore
							CLH2_FreeEffect(index);
							break;
						}
						else
						{
							CLTENT_SpawnDeathBubble(org);
						}
					}
				}
				if (cl.h2_Effects[index].Bubble.count <= 0)
					CLH2_FreeEffect(index);
				break;
			case HWCE_SCARABCHAIN:
				cl.h2_Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

				switch (cl.h2_Effects[index].Chain.state)
				{
				case 0://zooming in toward owner
					es = FindState(cl.h2_Effects[index].Chain.owner);
					if (cl.h2_Effects[index].Chain.sound_time <= cl.serverTimeFloat)
					{
						cl.h2_Effects[index].Chain.sound_time = cl.serverTimeFloat + 0.5;
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabhome, 1, 1);
					}
					if (es)
					{
						VectorCopy(es->origin,org);
						org[2]+=cl.h2_Effects[index].Chain.height;
						VectorSubtract(org,ent->state.origin,org);
						if (fabs(VectorNormalize(org))<500*frametime)
						{
							S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabgrab, 1, 1);
							cl.h2_Effects[index].Chain.state = 1;
							VectorCopy(es->origin, ent->state.origin);
							ent->state.origin[2] += cl.h2_Effects[index].Chain.height;
							XbowImpactPuff(ent->state.origin, cl.h2_Effects[index].Chain.material);
						}
						else
						{
							VectorScale(org,500*frametime,org);
							VectorAdd(ent->state.origin,org,ent->state.origin);
						}
					}
					break;
				case 1://attached--snap to owner's pos
					es = FindState(cl.h2_Effects[index].Chain.owner);
					if (es)
					{
						VectorCopy(es->origin, ent->state.origin);
						ent->state.origin[2] += cl.h2_Effects[index].Chain.height;
					}
					break;
				case 2://unattaching, server needs to set this state
					VectorCopy(ent->state.origin,org);
					VectorSubtract(cl.h2_Effects[index].Chain.origin,org,org);
					if (fabs(VectorNormalize(org))>350*frametime)//closer than 30 is too close?
					{
						VectorScale(org,350*frametime,org);
						VectorAdd(ent->state.origin,org,ent->state.origin);
					}
					else//done--flash & git outa here (change type to redflash)
					{
						S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabbyebye, 1, 1);
						cl.h2_Effects[index].Flash.entity_index = cl.h2_Effects[index].Chain.ent1;
						cl.h2_Effects[index].type = HWCE_RED_FLASH;
						VectorCopy(ent->state.origin,cl.h2_Effects[index].Flash.origin);
						cl.h2_Effects[index].Flash.reverse = 0;
						ent->model = R_RegisterModel("models/redspt.spr");
						ent->state.frame = 0;
						ent->state.drawflags = H2DRF_TRANSLUCENT;
					}
					break;
				}

				CLH2_LinkEffectEntity(ent);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.h2_Effects[index].Chain.origin, org);
				VectorCopy(ent->state.origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.h2_Effects[index].Chain.ent1, 1, cl.h2_Effects[index].Chain.tag, 0.1, 0, org, org2);

				break;
			case HWCE_TRIPMINESTILL:
				cl.h2_Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

//				if (cl.h2_Effects[index].Chain.ent1 < 0)//fixme: remove this!!!
//					Con_DPrintf("OHSHITOHSHIT--bad chain ent\n");

				CLH2_LinkEffectEntity(ent);
//				Con_DPrintf("Chain Ent at: %d %d %d\n",(int)cl.h2_Effects[index].Chain.origin[0],(int)cl.h2_Effects[index].Chain.origin[1],(int)cl.h2_Effects[index].Chain.origin[2]);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.h2_Effects[index].Chain.origin, org);
				VectorCopy(ent->state.origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.h2_Effects[index].Chain.ent1, 1, 1, 0.1, 0, org, org2);

				break;
			case HWCE_TRIPMINE:
				cl.h2_Effects[index].Chain.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Chain.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Chain.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Chain.velocity[2];

				CLH2_LinkEffectEntity(ent);

				//damndamndamn--add stream stuff here!
				VectorCopy(cl.h2_Effects[index].Chain.origin, org);
				VectorCopy(ent->state.origin, org2);
				CreateStream(TE_STREAM_CHAIN, cl.h2_Effects[index].Chain.ent1, 1, 1, 0.1, 0, org, org2);

				break;
			case HWCE_BONESHARD:
			case HWCE_BONESHRAPNEL:
			case HWCE_HWRAVENSTAFF:
				CLH2_UpdateEffectMissile(index, frametime);
				break;
			case HWCE_HWBONEBALL:
				CLHW_UpdateEffectBoneBall(index, frametime);
				break;
			case HWCE_HWRAVENPOWER:
				CLHW_UpdateEffectRavenPower(index, frametime);
				break;
			case HWCE_HWMISSILESTAR:
			case HWCE_HWEIDOLONSTAR:
				// update scale
				if(cl.h2_Effects[index].Star.scaleDir)
				{
					cl.h2_Effects[index].Star.scale += 0.05;
					if(cl.h2_Effects[index].Star.scale >= 1)
					{
						cl.h2_Effects[index].Star.scaleDir = 0;
					}
				}
				else
				{
					cl.h2_Effects[index].Star.scale -= 0.05;
					if(cl.h2_Effects[index].Star.scale <= 0.01)
					{
						cl.h2_Effects[index].Star.scaleDir = 1;
					}
				}
				
				cl.h2_Effects[index].Star.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Star.entity_index];

				ent->state.angles[0] += frametime * cl.h2_Effects[index].Star.avelocity[0];
				ent->state.angles[1] += frametime * cl.h2_Effects[index].Star.avelocity[1];
				ent->state.angles[2] += frametime * cl.h2_Effects[index].Star.avelocity[2];

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Star.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Star.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Star.velocity[2];

				CLH2_LinkEffectEntity(ent);
				
				if (cl.h2_Effects[index].Star.ent1 != -1)
				{
					ent2= &EffectEntities[cl.h2_Effects[index].Star.ent1];
					VectorCopy(ent->state.origin, ent2->state.origin);
					ent2->state.scale = cl.h2_Effects[index].Star.scale;
					ent2->state.angles[1] += frametime * 300;
					ent2->state.angles[2] += frametime * 400;
					CLH2_LinkEffectEntity(ent2);
				}
				if(cl.h2_Effects[index].type == HWCE_HWMISSILESTAR)
				{
					if (cl.h2_Effects[index].Star.ent2 != -1)
					{
						ent2 = &EffectEntities[cl.h2_Effects[index].Star.ent2];
						VectorCopy(ent->state.origin, ent2->state.origin);
						ent2->state.scale = cl.h2_Effects[index].Star.scale;
						ent2->state.angles[1] += frametime * -300;
						ent2->state.angles[2] += frametime * -400;
						CLH2_LinkEffectEntity(ent2);
					}
				}					
				if(rand() % 10 < 3)		
				{
					CLH2_RunParticleEffect4 (ent->state.origin, 7, 148 + rand() % 11, pt_h2grav, 10 + rand() % 10);
				}
				break;
		}
	}
}

// this creates multi effects from one packet
void CreateRavenExplosions(vec3_t pos);
void CL_ParseMultiEffect(void)
{
	int type, index, count;
	vec3_t	orig, vel;
	effect_entity_t* ent;

	type = net_message.ReadByte();
	switch(type)
	{
	case HWCE_HWRAVENPOWER:
		orig[0] = net_message.ReadCoord();
		orig[1] = net_message.ReadCoord();
		orig[2] = net_message.ReadCoord();
		vel[0] = net_message.ReadCoord();
		vel[1] = net_message.ReadCoord();
		vel[2] = net_message.ReadCoord();
		for(count=0;count<3;count++)
		{
			index = net_message.ReadByte();
			// create the effect
			cl.h2_Effects[index].type = type;
			VectorCopy(orig, cl.h2_Effects[index].Missile.origin);
			VectorCopy(vel, cl.h2_Effects[index].Missile.velocity);
			VecToAnglesBuggy(cl.h2_Effects[index].Missile.velocity, cl.h2_Effects[index].Missile.angle);
			if ((cl.h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
				VectorCopy(cl.h2_Effects[index].Missile.origin, ent->state.origin);
				VectorCopy(cl.h2_Effects[index].Missile.angle, ent->state.angles);
				ent->model = R_RegisterModel("models/ravproj.mdl");
				S_StartSound(cl.h2_Effects[index].Missile.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_ravengo, 1, 1);
			}
		}
		CreateRavenExplosions(orig);
		break;
	default:
		Sys_Error ("CL_ParseMultiEffect: bad type");

	}	
	
}
