// cl_tent.c -- client side temporary entities

// HEADER FILES ------------------------------------------------------------

#include "quakedef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// PUBLIC FUNCTION DEFINITIONS ---------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static void ParseStream(int type);

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

//==========================================================================
//
// CreateStream--for use mainly external to cl_tent (i.e. cl_effect)
//
// generally pass in 0 for skin
//
//==========================================================================

void CreateStream(int type, int ent, int flags, int tag, float duration, int skin, vec3_t source, vec3_t dest)
{
	qhandle_t		models[4];

	models[1] = models[2] = models[3] = 0;
	switch(type)
	{
	case TE_STREAM_CHAIN:
		models[0] = R_RegisterModel("models/stchain.mdl");
		break;
	case TE_STREAM_SUNSTAFF1:
		models[0] = R_RegisterModel("models/stsunsf1.mdl");
		models[1] = R_RegisterModel("models/stsunsf2.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_SUNSTAFF2:
		models[0] = R_RegisterModel("models/stsunsf5.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_LIGHTNING:
		models[0] = R_RegisterModel("models/stlghtng.mdl");
		break;
	case TE_STREAM_LIGHTNING_SMALL:
		models[0] = R_RegisterModel("models/stltng2.mdl");
		break;
	case TE_STREAM_FAMINE:
		models[0] = R_RegisterModel("models/fambeam.mdl");
		break;
	case TE_STREAM_COLORBEAM:
		models[0] = R_RegisterModel("models/stclrbm.mdl");
		break;
	case TE_STREAM_ICECHUNKS:
		models[0] = R_RegisterModel("models/stice.mdl");
		break;
	case TE_STREAM_GAZE:
		models[0] = R_RegisterModel("models/stmedgaz.mdl");
		break;
	default:
		Sys_Error("CreateStream: bad type");
	}

	CLH2_CreateStream(type, ent, tag, flags, skin, duration * 1000, source, dest, models);
}
//==========================================================================
//
// ParseStream
//
//==========================================================================

static void ParseStream(int type)
{
	int				ent;
	int				tag;
	int				flags;
	int				skin;
	vec3_t			source;
	vec3_t			dest;
	qhandle_t		models[4];

	ent = net_message.ReadShort();
	flags = net_message.ReadByte();
	tag = flags&15;
	int duration = net_message.ReadByte() * 50;
	skin = 0;
	if(type == TE_STREAM_COLORBEAM)
	{
		skin = net_message.ReadByte();
	}
	source[0] = net_message.ReadCoord();
	source[1] = net_message.ReadCoord();
	source[2] = net_message.ReadCoord();
	dest[0] = net_message.ReadCoord();
	dest[1] = net_message.ReadCoord();
	dest[2] = net_message.ReadCoord();

	models[1] = models[2] = models[3] = 0;
	switch(type)
	{
	case TE_STREAM_CHAIN:
		models[0] = R_RegisterModel("models/stchain.mdl");
		break;
	case TE_STREAM_SUNSTAFF1:
		models[0] = R_RegisterModel("models/stsunsf1.mdl");
		models[1] = R_RegisterModel("models/stsunsf2.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_SUNSTAFF2:
		models[0] = R_RegisterModel("models/stsunsf5.mdl");
		models[2] = R_RegisterModel("models/stsunsf3.mdl");
		models[3] = R_RegisterModel("models/stsunsf4.mdl");
		break;
	case TE_STREAM_LIGHTNING:
		models[0] = R_RegisterModel("models/stlghtng.mdl");
		break;
	case TE_STREAM_LIGHTNING_SMALL:
		models[0] = R_RegisterModel("models/stltng2.mdl");
		break;
	case TE_STREAM_FAMINE:
		models[0] = R_RegisterModel("models/fambeam.mdl");
		break;
	case TE_STREAM_COLORBEAM:
		models[0] = R_RegisterModel("models/stclrbm.mdl");
		break;
	case TE_STREAM_ICECHUNKS:
		models[0] = R_RegisterModel("models/stice.mdl");
		break;
	case TE_STREAM_GAZE:
		models[0] = R_RegisterModel("models/stmedgaz.mdl");
		break;
	default:
		Sys_Error("ParseStream: bad type");
	}

	CLH2_CreateStream(type, ent, tag, flags, skin, duration, source, dest, models);
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
	case TE_WIZSPIKE:	// spike hitting wall
		CLH2_ParseWizSpike(net_message);
		break;
	case TE_KNIGHTSPIKE:	// spike hitting wall
		CLH2_ParseKnightSpike(net_message);
		break;
	case TE_SPIKE:			// spike hitting wall
		CLH2_ParseSpike(net_message);
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		CLH2_ParseSuperSpike(net_message);
		break;
	case TE_DRILLA_EXPLODE:
		CLHW_ParseDrillaExplode(net_message);
		break;
	case TE_EXPLOSION:			// rocket explosion
		CLHW_ParseExplosion(net_message);
		break;
	case TE_TAREXPLOSION:			// tarbaby explosion
		CLHW_ParseTarExplosion(net_message);
		break;
	case TE_LIGHTNING1:				// lightning bolts
	case TE_LIGHTNING2:
	case TE_LIGHTNING3:
		CLH2_ParseBeam(net_message);
		break;

	case TE_STREAM_CHAIN:
	case TE_STREAM_SUNSTAFF1:
	case TE_STREAM_SUNSTAFF2:
	case TE_STREAM_LIGHTNING:
	case TE_STREAM_LIGHTNING_SMALL:
	case TE_STREAM_COLORBEAM:
	case TE_STREAM_ICECHUNKS:
	case TE_STREAM_GAZE:
	case TE_STREAM_FAMINE:
		ParseStream(type);
		break;

	case TE_LAVASPLASH:	
		CLH2_ParseLavaSplash(net_message);
		break;
	case TE_TELEPORT:
		CLH2_ParseTeleport(net_message);
		break;
	case TE_GUNSHOT:			// bullet hitting wall
	case TE_BLOOD:				// bullets hitting body
		CLHW_ParseGunShot(net_message);
		break;
	case TE_LIGHTNINGBLOOD:		// lightning hitting body
		CLHW_ParseLightningBlood(net_message);
		break;
	case TE_BIGGRENADE:
		CLHW_ParseBigGrenade(net_message);
		break;
	case TE_CHUNK:
		CLHW_ParseChunk(net_message);
		break;
	case TE_CHUNK2:
		CLHW_ParseChunk2(net_message);
		break;
	case TE_XBOWHIT:
		CLHW_ParseXBowHit(net_message);
		break;
	case TE_METEORHIT:
		CLHW_ParseMeteorHit(net_message);
		break;
	case TE_HWBONEPOWER:
		CLHW_ParseBonePower(net_message);
		break;
	case TE_HWBONEPOWER2:
		CLHW_ParseBonePower2(net_message);
		break;
	case TE_HWRAVENDIE:
		CLHW_ParseRavenDie(net_message);
		break;
	case TE_HWRAVENEXPLODE:
		CLHW_ParseRavenExplode(net_message);
		break;
	case TE_ICEHIT:
		CLHW_ParseIceHit(net_message);
		break;

	case TE_ICESTORM:
		{
			int				ent;
			vec3_t			center;
			qhandle_t		models[2];
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
					models[0] = R_RegisterModel("models/stice.mdl");

					vec3_t source;
					VectorCopy(center, source);
					source[0] += rand() % 100 - 50;
					source[1] += rand() % 100 - 50;

					vec3_t dest;
					VectorCopy(source, dest);
					dest[2] += 128;

					CLH2_CreateStream(TE_STREAM_ICECHUNKS, ent, i, i + H2STREAM_ATTACHED, 0, 300, source, dest, models);
				}

			}
		}

		break;
	case TE_HWMISSILEFLASH:
		CLHW_ParseMissileFlash(net_message);
		break;

	case TE_SUNSTAFF_CHEAP:
		{
			int				ent;
			vec3_t			points[4];
			int				reflect_count;
			short int		tempVal;
			h2entity_state_t	*state;
			int				i, j;

			qhandle_t		models[4];


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
					models[0] = R_RegisterModel("models/stsunsf1.mdl");
					models[1] = R_RegisterModel("models/stsunsf2.mdl");
					models[2] = R_RegisterModel("models/stsunsf3.mdl");
					models[3] = R_RegisterModel("models/stsunsf4.mdl");

					CLH2_CreateStream(TE_STREAM_SUNSTAFF1, ent, i, !i ? i + H2STREAM_ATTACHED : i, 0, 500, points[i], points[i + 1], models);
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

	case TE_LIGHTNING_HAMMER:
		{
			int				ent;
			qhandle_t		models[4];
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
					models[0] = R_RegisterModel("models/stlghtng.mdl");

					vec3_t source;
					VectorCopy(state->origin, source);
					source[0] += rand() % 30 - 15;
					source[1] += rand() % 30 - 15;

					vec3_t dest;
					VectorCopy(source, dest);
					dest[0] += rand() % 80 - 40;
					dest[1] += rand() % 80 - 40;
					dest[2] += 64 + (rand() % 48);

					CLH2_CreateStream(TE_STREAM_LIGHTNING, ent, i, i, 0, 500, source, dest, models);
				}

			}
		}
		break;
	case TE_HWTELEPORT:
		CLHW_ParseTeleport(net_message);
		break;
	case TE_SWORD_EXPLOSION:
		{
			vec3_t			pos;
			int				ent;
			qhandle_t		models[4];
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

					models[0] = R_RegisterModel("models/stlghtng.mdl");

					CLH2_CreateStream(TE_STREAM_LIGHTNING, ent, i, i, 0, 500, source, dest, models);
				}
			}
			CLHW_SwordExplosion(pos);
		}
		break;

	case TE_AXE_BOUNCE:
		CLHW_ParseAxeBounce(net_message);
		break;
	case TE_AXE_EXPLODE:
		CLHW_ParseAxeExplode(net_message);
		break;
	case TE_TIME_BOMB:
		CLHW_ParseTimeBomb(net_message);
		break;
	case TE_FIREBALL:
		CLHW_ParseFireBall(net_message);
		break;

	case TE_SUNSTAFF_POWER:
		{
			int				ent;
			qhandle_t		models[4];
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

				models[0] = R_RegisterModel("models/stsunsf2.mdl");
				models[1] = R_RegisterModel("models/stsunsf1.mdl");
				models[2] = R_RegisterModel("models/stsunsf3.mdl");
				models[3] = R_RegisterModel("models/stsunsf4.mdl");

				CLH2_CreateStream(TE_STREAM_SUNSTAFF2, ent, 0, 0, 0, 800, vel, pos, models);
			}
		}
		break;

	case TE_PURIFY2_EXPLODE:
		CLHW_ParsePurify2Explode(net_message);
		break;
	case TE_PLAYER_DEATH:
		CLHW_ParsePlayerDeath(net_message);
		break;
	case TE_PURIFY1_EFFECT:
		CLHW_ParsePurify1Effect(net_message);
		break;
	case TE_TELEPORT_LINGER:
		CLHW_ParseTeleportLinger(net_message);
		break;
	case TE_LINE_EXPLOSION:
		CLHW_ParseLineExplosion(net_message);
		break;
	case TE_METEOR_CRUSH:
		CLHW_ParseMeteorCrush(net_message);
		break;
	case TE_ACIDBALL:
		CLHW_ParseAcidBall(net_message);
		break;
	case TE_ACIDBLOB:
		CLHW_ParseAcidBlob(net_message);
		break;
	case TE_FIREWALL:
		CLHW_ParseFireWall(net_message);
		break;
	case TE_FIREWALL_IMPACT:
		CLHW_ParseFireWallImpact(net_message);
		break;
	case TE_HWBONERIC:
		CLHW_ParseBoneRic(net_message);
		break;
	case TE_POWERFLAME:
		CLHW_ParsePowerFlame(net_message);
		break;
	case TE_BLOODRAIN:
		CLHW_ParseBloodRain(net_message);
		break;
	case TE_AXE:
		CLHW_ParseAxe(net_message);
		break;
	case TE_PURIFY2_MISSILE:
		CLHW_ParsePurify2Missile(net_message);
		break;
	case TE_SWORD_SHOT:
		CLHW_ParseSwordShot(net_message);
		break;
	case TE_ICESHOT:
		CLHW_ParseIceShot(net_message);
		break;
	case TE_METEOR:
		CLHW_ParseMeteor(net_message);
		break;
	case TE_LIGHTNINGBALL:
		CLHW_ParseLightningBall(net_message);
		break;
	case TE_MEGAMETEOR:
		CLHW_ParseMegaMeteor(net_message);
		break;

	case TE_CUBEBEAM:
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

			type = TE_STREAM_COLORBEAM;

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
				CreateStream(TE_STREAM_COLORBEAM, ent, flags, tag, duration, skin, source, dest);
			}
		}
		break;

	case TE_LIGHTNINGEXPLODE:
		{
			int				ent;
			qhandle_t		models[4];
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
				models[0] = R_RegisterModel("models/stlghtng.mdl");

				tempAng = (rand()%628)/100.0;
				tempPitch = (rand()%628)/100.0;

				vec3_t dest;
				VectorCopy(pos, dest);
				dest[0] += 75.0 * cos(tempAng) * cos(tempPitch);
				dest[1] += 75.0 * sin(tempAng) * cos(tempPitch);
				dest[2] += 75.0 * sin(tempPitch);

				CLH2_CreateStream(TE_STREAM_LIGHTNING, ent, i, i, 0, 500, pos, dest, models);
			}
		}
		break;

	case TE_ACID_BALL_FLY:
		CLHW_ParseAcidBallFly(net_message);
		break;
	case TE_ACID_BLOB_FLY:
		CLHW_ParseAcidBlobFly(net_message);
		break;

	case TE_CHAINLIGHTNING:
		{
			vec3_t			points[12];
			int				numTargs = 0;
			int				oldNum;
			int				temp;

			int				ent;
			qhandle_t		models[4];

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
				models[0] = R_RegisterModel("models/stlghtng.mdl");

				CLH2_CreateStream(TE_STREAM_LIGHTNING, ent, temp, temp, 0, 300, points[temp], points[temp + 1], models);

				CLHW_ChainLightningExplosion(points[temp + 1]);
			}

		}
		break;


	default:
		Sys_Error ("CL_ParseTEnt: bad type");
	}
}

void CL_UpdateStreams(void)
{
	int				i;
	h2stream_t		*stream;
	vec3_t			dist;
	vec3_t			org;
	vec3_t			discard, right, up;
	float			cosTime, sinTime, lifeTime, cos2Time, sin2Time;
	float			d;
	int				segmentCount;
	int				offset;
	h2entity_state_t	*state;

	// Update streams
	for(i = 0, stream = clh2_Streams; i < MAX_STREAMS_H2; i++, stream++)
	{
		if(!stream->models[0])// || stream->endTime < cl.time)
		{ // Inactive
			continue;
		}

		if(stream->endTime < cl_common->serverTime * 0.001)
		{ // Inactive
			if(stream->type!=TE_STREAM_LIGHTNING&&stream->type!=TE_STREAM_LIGHTNING_SMALL)
				continue;
			else if(stream->endTime + 0.25 < cl_common->serverTime * 0.001)
				continue;
		}

		if(stream->flags&H2STREAM_ATTACHED&&stream->endTime >= cl_common->serverTime * 0.001)
			{ // Attach the start position to owner
			state = CLH2_FindState(stream->entity);
			if (state)
			{
				VectorAdd(state->origin, stream->offset, stream->source);
			}
		}

		VectorSubtract(stream->dest, stream->source, dist);
		vec3_t angles;
		VecToAnglesBuggy(dist, angles);

		VectorCopy(stream->source, org);
		d = VectorNormalize(dist);

		if(stream->type == TE_STREAM_SUNSTAFF2)
		{
			AngleVectors(angles, discard, right, up);

			lifeTime = ((stream->endTime - cl_common->serverTime * 0.001)/.8);
			cosTime = cos(cl_common->serverTime * 0.001*5);
			sinTime = sin(cl_common->serverTime * 0.001*5);
			cos2Time = cos(cl_common->serverTime * 0.001*5 + 3.14);
			sin2Time = sin(cl_common->serverTime * 0.001*5 + 3.14);
		}

		segmentCount = 0;
		if(stream->type == TE_STREAM_ICECHUNKS)
		{
			offset = (int)(cl_common->serverTime * 0.001*40)%30;
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*offset;
			}
		}
		while(d > 0)
		{
			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(org, ent.origin);
			ent.hModel = stream->models[0];
			switch (stream->type)
			{
			case TE_STREAM_CHAIN:
				angles[2] = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF1:
				angles[2] = (int)(cl_common->serverTime * 0.001*10)%360;
				//ent->frame = (int)(cl.time*20)%20;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl_common->serverTime * 0.001)/.3), 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[1];
				angles[2] = (int)(cl_common->serverTime * 0.001*50)%360;
				//stream->endTime = cl.time+0.3;	// FIXME
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 50 + 100 * ((stream->endTime - cl_common->serverTime * 0.001)/.5), 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_SUNSTAFF2:
				angles[2] = (int)(cl_common->serverTime * 0.001*100)%360;
				VectorMA(ent.origin, cosTime * (40 * lifeTime), right,  ent.origin);
				VectorMA(ent.origin, sinTime * (40 * lifeTime), up,  ent.origin);
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);

				Com_Memset(&ent, 0, sizeof(ent));
				ent.reType = RT_MODEL;
				VectorCopy(org, ent.origin);
				ent.hModel = stream->models[0];
				angles[2] = (int)(cl_common->serverTime * 0.001*100)%360;
				VectorMA(ent.origin, cos2Time * (40 * lifeTime), right,  ent.origin);
				VectorMA(ent.origin, sin2Time * (40 * lifeTime), up,  ent.origin);
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				R_AddRefEntityToScene(&ent);

				for (int ix = 0; ix < 2; ix++)
				{
					Com_Memset(&ent, 0, sizeof(ent));
					ent.reType = RT_MODEL;
					VectorCopy(org, ent.origin);
					if (i)
					{
						VectorMA(ent.origin, cos2Time * (40 * lifeTime), right,  ent.origin);
						VectorMA(ent.origin, sin2Time * (40 * lifeTime), up,  ent.origin);
					}
					else
					{
						VectorMA(ent.origin, cosTime * (40 * lifeTime), right,  ent.origin);
						VectorMA(ent.origin, sinTime * (40 * lifeTime), up,  ent.origin);
					}
					ent.hModel = stream->models[1];
					angles[2] = (int)(cl_common->serverTime * 0.001*20)%360;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 100 + 150 * lifeTime, 0, 128, H2MLS_ABSLIGHT);
					R_AddRefEntityToScene(&ent);
				}
				break;
			case TE_STREAM_LIGHTNING:
				if(stream->endTime < cl_common->serverTime * 0.001)
				{//fixme: keep last non-translucent frame and angle
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl_common->serverTime * 0.001)*192, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_LIGHTNING_SMALL:
				if(stream->endTime < cl_common->serverTime * 0.001)
				{
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128 + (stream->endTime - cl_common->serverTime * 0.001)*192, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
				}
				else
				{
					angles[2] = rand()%360;
					ent.frame = rand()%6;
					CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				}
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_FAMINE:
				angles[2] = rand()%360;
				ent.frame = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_COLORBEAM:
				angles[2] = 0;
				ent.skinNum = stream->skin;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				CLH2_HandleCustomSkin(&ent, -1);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_GAZE:
				angles[2] = 0;
				ent.frame = (int)(cl_common->serverTime * 0.001*40)%36;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			case TE_STREAM_ICECHUNKS:
				angles[2] = rand()%360;
				ent.frame = rand()%5;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 128, H2MLS_ABSLIGHT);
				R_AddRefEntityToScene(&ent);
				break;
			default:
				angles[2] = 0;
				CLH2_SetRefEntAxis(&ent, angles, vec3_origin, 0, 0, 0, 0);
				R_AddRefEntityToScene(&ent);
			}
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*30;
			}
			d -= 30;
			segmentCount++;
		}
		if(stream->type == TE_STREAM_SUNSTAFF1)
		{
			if(stream->lastTrailTime+0.2 < cl_common->serverTime * 0.001)
			{
				stream->lastTrailTime = cl_common->serverTime * 0.001;
				CLH2_SunStaffTrail(stream->source, stream->dest);
			}

			refEntity_t ent;
			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[2];
			//ent->frame = (int)(cl.time*20)%20;
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 80 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT);
			R_AddRefEntityToScene(&ent);

			Com_Memset(&ent, 0, sizeof(ent));
			ent.reType = RT_MODEL;
			VectorCopy(stream->dest, ent.origin);
			ent.hModel = stream->models[3];
			CLH2_SetRefEntAxis(&ent, vec3_origin, vec3_origin, 150 + (rand() & 15), 0, 128, H2MLS_ABSLIGHT|H2DRF_TRANSLUCENT);
			R_AddRefEntityToScene(&ent);
		}
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
	CL_UpdateStreams();
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
