// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static sfxHandle_t		cl_sfx_buzzbee;

static sfxHandle_t		cl_sfx_icestorm;
static sfxHandle_t		cl_sfx_sunstaff;
static sfxHandle_t		cl_sfx_sunhit;
static sfxHandle_t		cl_sfx_lightning1;
static sfxHandle_t		cl_sfx_lightning2;
static sfxHandle_t		cl_sfx_hammersound;
static sfxHandle_t		cl_sfx_tornado;



/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	CLH2_InitTEntsCommon();
	cl_sfx_buzzbee = S_RegisterSound("assassin/scrbfly.wav");

	cl_sfx_icestorm = S_RegisterSound("crusader/blizzard.wav");
	cl_sfx_sunstaff = S_RegisterSound("crusader/sunhum.wav");
	cl_sfx_sunhit = S_RegisterSound("crusader/sunhit.wav");
	cl_sfx_lightning1 = S_RegisterSound("crusader/lghtn1.wav");
	cl_sfx_lightning2 = S_RegisterSound("crusader/lghtn2.wav");
	cl_sfx_hammersound = S_RegisterSound("paladin/axblade.wav");
	cl_sfx_tornado = S_RegisterSound("crusader/tornado.wav");
}

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int type;
	vec3_t pos, vel;
	int i;

	type = net_message.ReadByte();
	switch (type)
	{
	case H2TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(net_message);
		break;
	case H2TE_KNIGHTSPIKE:	// spike hitting wall
		CLH2_ParseKnightSpike(net_message);
		break;
	case H2TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(net_message);
		break;
	case H2TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(net_message);
		break;
	case HWTE_DRILLA_EXPLODE:
		CLHW_ParseDrillaExplode(net_message);
		break;
	case H2TE_EXPLOSION:			// rocket explosion
		CLHW_ParseExplosion(net_message);
		break;
	case HWTE_TAREXPLOSION:			// tarbaby explosion
		CLHW_ParseTarExplosion(net_message);
		break;
	case H2TE_LIGHTNING1:				// lightning bolts
	case H2TE_LIGHTNING2:
	case H2TE_LIGHTNING3:
		CLH2_ParseBeam(net_message);
		break;

	case H2TE_STREAM_CHAIN:
	case H2TE_STREAM_SUNSTAFF1:
	case H2TE_STREAM_SUNSTAFF2:
	case H2TE_STREAM_LIGHTNING:
	case HWTE_STREAM_LIGHTNING_SMALL:
	case H2TE_STREAM_COLORBEAM:
	case H2TE_STREAM_ICECHUNKS:
	case H2TE_STREAM_GAZE:
	case H2TE_STREAM_FAMINE:
		CLH2_ParseStream(net_message, type);
		break;

	case H2TE_LAVASPLASH:	
		CLH2_ParseLavaSplash(net_message);
		break;
	case H2TE_TELEPORT:
		CLH2_ParseTeleport(net_message);
		break;
	case H2TE_GUNSHOT:			// bullet hitting wall
	case HWTE_BLOOD:				// bullets hitting body
		CLHW_ParseGunShot(net_message);
		break;
	case HWTE_LIGHTNINGBLOOD:		// lightning hitting body
		CLHW_ParseLightningBlood(net_message);
		break;
	case HWTE_BIGGRENADE:
		CLHW_ParseBigGrenade(net_message);
		break;
	case HWTE_CHUNK:
		CLHW_ParseChunk(net_message);
		break;
	case HWTE_CHUNK2:
		CLHW_ParseChunk2(net_message);
		break;
	case HWTE_XBOWHIT:
		CLHW_ParseXBowHit(net_message);
		break;
	case HWTE_METEORHIT:
		CLHW_ParseMeteorHit(net_message);
		break;
	case HWTE_HWBONEPOWER:
		CLHW_ParseBonePower(net_message);
		break;
	case HWTE_HWBONEPOWER2:
		CLHW_ParseBonePower2(net_message);
		break;
	case HWTE_HWRAVENDIE:
		CLHW_ParseRavenDie(net_message);
		break;
	case HWTE_HWRAVENEXPLODE:
		CLHW_ParseRavenExplode(net_message);
		break;
	case HWTE_ICEHIT:
		CLHW_ParseIceHit(net_message);
		break;

	case HWTE_ICESTORM:
		{
			int				ent;
			vec3_t			center;
			h2entity_state_t	*state;
			static float	playIceSound = .6;

			ent = net_message.ReadShort();

			state = CLH2_FindState(ent);
			if (state)
			{
				VectorCopy(state->origin, center);

				playIceSound+=host_frametime;
				if(playIceSound >= .6)
				{
					S_StartSound(center, CLH2_TempSoundChannel(), 0, cl_sfx_icestorm, 1, 1);
					playIceSound -= .6;
				}

				for (i = 0; i < 5; i++)
				{	// make some ice beams...
					vec3_t source;
					VectorCopy(center, source);
					source[0] += rand() % 100 - 50;
					source[1] += rand() % 100 - 50;

					vec3_t dest;
					VectorCopy(source, dest);
					dest[2] += 128;

					CLH2_CreateStreamIceChunks(ent, i, i + H2STREAM_ATTACHED, 0, 300, source, dest);
				}

			}
		}

		break;
	case HWTE_HWMISSILEFLASH:
		CLHW_ParseMissileFlash(net_message);
		break;

	case HWTE_SUNSTAFF_CHEAP:
		{
			int				ent;
			vec3_t			points[4];
			int				reflect_count;
			short int		tempVal;
			h2entity_state_t	*state;
			int				i, j;


			ent = net_message.ReadShort();
			reflect_count = net_message.ReadByte();

			state = CLH2_FindState(ent);
			if (state)
			{
				// read in up to 4 points for up to 3 beams
				for(i = 0; i < 3; i++)
				{
					tempVal = net_message.ReadCoord();
					points[0][i] = tempVal;
				}
				for(i = 1; i < 2 + reflect_count; i++)
				{
					for(j = 0; j < 3; j++)
					{
						tempVal = net_message.ReadCoord();
						points[i][j] = tempVal;
					}
				}
				
				// actually create the sun model pieces
				for ( i = 0; i < reflect_count + 1; i++)
				{
					CLH2_CreateStreamSunstaff1(ent, i, !i ? i + H2STREAM_ATTACHED : i, 0, 500, points[i], points[i + 1]);
				}
			}
			else
			{	// read in everything to keep everything in sync
				for(i = 0; i < (2 + reflect_count)*3; i++)
				{
					tempVal = net_message.ReadShort();
				}
			}
		}
		break;

	case HWTE_LIGHTNING_HAMMER:
		{
			int				ent;
			h2entity_state_t	*state;

			ent = net_message.ReadShort();

			state = CLH2_FindState(ent);

			if (state)
			{
				if(rand()&1)
				{
					S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
				}
				else
				{
					S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
				}

				for (i = 0; i < 5; i++)
				{	// make some lightning
					vec3_t source;
					VectorCopy(state->origin, source);
					source[0] += rand() % 30 - 15;
					source[1] += rand() % 30 - 15;

					vec3_t dest;
					VectorCopy(source, dest);
					dest[0] += rand() % 80 - 40;
					dest[1] += rand() % 80 - 40;
					dest[2] += 64 + (rand() % 48);

					CLH2_CreateStreamLightning(ent, i, i, 0, 500, source, dest);
				}

			}
		}
		break;
	case HWTE_HWTELEPORT:
		CLHW_ParseTeleport(net_message);
		break;
	case HWTE_SWORD_EXPLOSION:
		{
			vec3_t			pos;
			int				ent;
			h2entity_state_t	*state;

			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();
			ent = net_message.ReadShort();

			state = CLH2_FindState(ent);

			if (state)
			{
				if(rand()&1)
				{
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
				}
				else
				{
					S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
				}

				for (i = 0; i < 5; i++)
				{
					// make some lightning
					vec3_t source;
					VectorCopy(pos, source);
					source[0] += rand() % 30 - 15;
					source[1] += rand() % 30 - 15;

					vec3_t dest;
					VectorCopy(source, dest);
					dest[0] += rand() % 80 - 40;
					dest[1] += rand() % 80 - 40;
					dest[2] += 64 + (rand() % 48);

					CLH2_CreateStreamLightning(ent, i, i, 0, 500, source, dest);
				}
			}
			CLHW_SwordExplosion(pos);
		}
		break;

	case HWTE_AXE_BOUNCE:
		CLHW_ParseAxeBounce(net_message);
		break;
	case HWTE_AXE_EXPLODE:
		CLHW_ParseAxeExplode(net_message);
		break;
	case HWTE_TIME_BOMB:
		CLHW_ParseTimeBomb(net_message);
		break;
	case HWTE_FIREBALL:
		CLHW_ParseFireBall(net_message);
		break;

	case HWTE_SUNSTAFF_POWER:
		{
			int				ent;
			h2entity_state_t	*state;

			ent = net_message.ReadShort();

			vel[0] = net_message.ReadCoord();
			vel[1] = net_message.ReadCoord();
			vel[2] = net_message.ReadCoord();
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			CLHW_SunStaffExplosions(pos);

			state = CLH2_FindState(ent);

			if (state)
			{
				S_StartSound(state->origin, CLH2_TempSoundChannel(), 0, cl_sfx_sunstaff, 1, 1);

				CLH2_CreateStreamSunstaffPower(ent, vel, pos);
			}
		}
		break;

	case HWTE_PURIFY2_EXPLODE:
		CLHW_ParsePurify2Explode(net_message);
		break;
	case HWTE_PLAYER_DEATH:
		CLHW_ParsePlayerDeath(net_message);
		break;
	case HWTE_PURIFY1_EFFECT:
		CLHW_ParsePurify1Effect(net_message);
		break;
	case HWTE_TELEPORT_LINGER:
		CLHW_ParseTeleportLinger(net_message);
		break;
	case HWTE_LINE_EXPLOSION:
		CLHW_ParseLineExplosion(net_message);
		break;
	case HWTE_METEOR_CRUSH:
		CLHW_ParseMeteorCrush(net_message);
		break;
	case HWTE_ACIDBALL:
		CLHW_ParseAcidBall(net_message);
		break;
	case HWTE_ACIDBLOB:
		CLHW_ParseAcidBlob(net_message);
		break;
	case HWTE_FIREWALL:
		CLHW_ParseFireWall(net_message);
		break;
	case HWTE_FIREWALL_IMPACT:
		CLHW_ParseFireWallImpact(net_message);
		break;
	case HWTE_HWBONERIC:
		CLHW_ParseBoneRic(net_message);
		break;
	case HWTE_POWERFLAME:
		CLHW_ParsePowerFlame(net_message);
		break;
	case HWTE_BLOODRAIN:
		CLHW_ParseBloodRain(net_message);
		break;
	case HWTE_AXE:
		CLHW_ParseAxe(net_message);
		break;
	case HWTE_PURIFY2_MISSILE:
		CLHW_ParsePurify2Missile(net_message);
		break;
	case HWTE_SWORD_SHOT:
		CLHW_ParseSwordShot(net_message);
		break;
	case HWTE_ICESHOT:
		CLHW_ParseIceShot(net_message);
		break;
	case HWTE_METEOR:
		CLHW_ParseMeteor(net_message);
		break;
	case HWTE_LIGHTNINGBALL:
		CLHW_ParseLightningBall(net_message);
		break;
	case HWTE_MEGAMETEOR:
		CLHW_ParseMegaMeteor(net_message);
		break;

	case HWTE_CUBEBEAM:
		{
			int				type;
			int				ent;
			int				ent2;
			int				tag;
			int				flags;
			int				skin;
			vec3_t			source;
			vec3_t			dest;
			vec3_t			smokeDir;
			float			duration;
			h2entity_state_t	*state;
			h2entity_state_t	*state2;

			type = H2TE_STREAM_COLORBEAM;

			ent = net_message.ReadShort();
			ent2 = net_message.ReadShort();
			flags = 0;
			tag = 0;
			duration = 0.1;
			skin = rand()%5;
//				source[0] = net_message.ReadCoord();
//				source[1] = net_message.ReadCoord();
//				source[2] = net_message.ReadCoord();
//				dest[0] = net_message.ReadCoord();
//				dest[1] = net_message.ReadCoord();
//				dest[2] = net_message.ReadCoord();

			state = CLH2_FindState(ent);
			state2 = CLH2_FindState(ent2);

			if (state || state2)
			{
				if (state)
				{
					VectorCopy(state->origin, source);
				}
				else//don't know where the damn cube is--prolly won't see beam anyway then, so put it all at the target
				{
					VectorCopy(state2->origin, source);
					source[2]+=10;
				}

				if (state2)
				{
					VectorCopy(state2->origin, dest);//in case they're both valid, copy me again
					dest[2]+=30;
				}
				else//don't know where the damn victim is--prolly won't see beam anyway then, so put it all at the cube
				{
					VectorCopy(source, dest);
					dest[2]+=10;
				}

				VectorSet(smokeDir,0,0,100);
				S_StartSound(source, CLH2_TempSoundChannel(), 0, cl_sfx_sunstaff, 1, 1);
				S_StartSound(dest, CLH2_TempSoundChannel(), 0, cl_sfx_sunhit, 1, 1);
				CLH2_SunStaffTrail(dest, source);
				CLH2_CreateStreamColourBeam(ent, tag, flags, skin, duration * 1000, source, dest);
			}
		}
		break;

	case HWTE_LIGHTNINGEXPLODE:
		{
			int				ent;
			h2entity_state_t	*state;
			float			tempAng, tempPitch;

			ent = net_message.ReadShort();

			state = CLH2_FindState(ent);
			pos[0] = net_message.ReadCoord();
			pos[1] = net_message.ReadCoord();
			pos[2] = net_message.ReadCoord();

			if(rand()&1)
			{
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning1, 1, 1);
			}
			else
			{
				S_StartSound(pos, CLH2_TempSoundChannel(), 0, cl_sfx_lightning2, 1, 1);
			}

			for (i = 0; i < 10; i++)
			{	// make some lightning
				tempAng = (rand()%628)/100.0;
				tempPitch = (rand()%628)/100.0;

				vec3_t dest;
				VectorCopy(pos, dest);
				dest[0] += 75.0 * cos(tempAng) * cos(tempPitch);
				dest[1] += 75.0 * sin(tempAng) * cos(tempPitch);
				dest[2] += 75.0 * sin(tempPitch);

				CLH2_CreateStreamLightning(ent, i, i, 0, 500, pos, dest);
			}
		}
		break;

	case HWTE_ACID_BALL_FLY:
		CLHW_ParseAcidBallFly(net_message);
		break;
	case HWTE_ACID_BLOB_FLY:
		CLHW_ParseAcidBlobFly(net_message);
		break;

	case HWTE_CHAINLIGHTNING:
		{
			vec3_t			points[12];
			int				numTargs = 0;
			int				oldNum;
			int				temp;

			int				ent;

			ent = net_message.ReadShort();

			do
			{
				points[numTargs][0] = net_message.ReadCoord();
				points[numTargs][1] = net_message.ReadCoord();
				points[numTargs][2] = net_message.ReadCoord();

				oldNum = numTargs;

				if(points[numTargs][0]||points[numTargs][1]||points[numTargs][2])
				{
					if(numTargs < 9)
					{
						numTargs++;
					}
				}
			}while(points[oldNum][0]||points[oldNum][1]||points[oldNum][2]);

			if(numTargs == 0)
			{
				break;
			}

			for(temp = 0; temp < numTargs - 1; temp++)
			{
				// make the connecting lightning...
				CLH2_CreateStreamLightning(ent, temp, temp, 0, 300, points[temp], points[temp + 1]);
				CLHW_ChainLightningExplosion(points[temp + 1]);
			}

		}
		break;


	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}


/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	CLH2_UpdateExplosions ();
	CLH2_UpdateStreams();
	CLHW_UpdateTargetBall(v_targDist, v_targAngle, v_targPitch, cl.simorg);
}

//*****************************************************************************
//
//						FLAG UPDATE FUNCTIONS
//
//*****************************************************************************

void CL_UpdateHammer(refEntity_t *ent, int edict_num)
{
	int testVal, testVal2;
	// do this every .3 seconds
	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
		if(!(testVal%3))
		{
			//S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, cl_sfx_hammersound, 1, 1);
			S_StartSound(ent->origin, edict_num, 2, cl_sfx_hammersound, 1, 1);
		}
	}
}

void CL_UpdateBug(refEntity_t *ent)
{
	int testVal, testVal2;
	testVal = (int)(cl_common->serverTime * 0.001 * 10.0);
	testVal2 = (int)((cl_common->serverTime * 0.001 - host_frametime)*10.0);

	if(testVal != testVal2)
	{
	// do this every .1 seconds
//		if(!(testVal%3))
//		{
			S_StartSound(ent->origin, CLH2_TempSoundChannel(), 1, cl_sfx_buzzbee, 1, 1);
//		}
	}
}

void CL_UpdateIceStorm(refEntity_t *ent, int edict_num)
{
	vec3_t			center, side1;
	vec3_t			side2 = {160, 160, 128};
	h2entity_state_t	*state;
	static float	playIceSound = .6;

	state = CLH2_FindState(edict_num);
	if (state)
	{
		VectorCopy(state->origin, center);

		// handle the particles
		VectorCopy(center, side1);
		side1[0] -= 80;
		side1[1] -= 80;
		side1[2] += 104;
		CLH2_RainEffect2(side1, side2, rand()%400-200, rand()%400-200, rand()%15 + 9*16, (int)(30*20*host_frametime));

		playIceSound+=host_frametime;
		if(playIceSound >= .6)
		{
			S_StartSound(center, CLH2_TempSoundChannel(), 0, cl_sfx_icestorm, 1, 1);
			playIceSound -= .6;
		}
	}

	// toss little ice chunks
	if(rand()%100 < host_frametime * 100.0 * 3)
	{
		CLHW_CreateIceChunk(ent->origin);
	}
}
