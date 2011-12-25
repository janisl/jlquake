
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

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

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
			CLHW_CreateRavenDeath(ent->state.origin);
		}
		break;
	case HWCE_HWRAVENSTAFF:
		if(cl.h2_Effects[index].Missile.entity_index > -1)
		{
			ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
			CLHW_CreateExplosionWithSound(ent->state.origin);
		}
		break;
	}
	CLH2_FreeEffect(index);
}

void CLHW_UpdateEffectDeathBubbles(int index, float frametime)
{
	cl_common->h2_Effects[index].Bubble.time_amount += frametime;
	if (cl_common->h2_Effects[index].Bubble.time_amount > 0.1)//10 bubbles a sec
	{
		cl_common->h2_Effects[index].Bubble.time_amount = 0;
		cl_common->h2_Effects[index].Bubble.count--;
		h2entity_state_t* es = CLH2_FindState(cl_common->h2_Effects[index].Bubble.owner);
		if (es)
		{
			vec3_t org;
			VectorCopy(es->origin, org);
			VectorAdd(org, cl_common->h2_Effects[index].Bubble.offset, org);

			if (CM_PointContentsQ1(org, 0) != BSP29CONTENTS_WATER) 
			{
				//not in water anymore
				CLH2_FreeEffect(index);
				return;
			}
			else
			{
				CLHW_SpawnDeathBubble(org);
			}
		}
	}
	if (cl_common->h2_Effects[index].Bubble.count <= 0)
	{
		CLH2_FreeEffect(index);
	}
}

void CLHW_UpdateEffectScarabChain(int index, float frametime)
{
	cl.h2_Effects[index].Chain.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

	h2entity_state_t* es;
	switch (cl.h2_Effects[index].Chain.state)
	{
	case 0://zooming in toward owner
		es = CLH2_FindState(cl.h2_Effects[index].Chain.owner);
		if (cl.h2_Effects[index].Chain.sound_time <= cl.serverTimeFloat)
		{
			cl.h2_Effects[index].Chain.sound_time = cl.serverTimeFloat + 0.5;
			S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabhome, 1, 1);
		}
		if (es)
		{
			vec3_t org;
			VectorCopy(es->origin,org);
			org[2] += cl.h2_Effects[index].Chain.height;
			VectorSubtract(org, ent->state.origin, org);
			if (fabs(VectorNormalize(org)) < 500 * frametime)
			{
				S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabgrab, 1, 1);
				cl.h2_Effects[index].Chain.state = 1;
				VectorCopy(es->origin, ent->state.origin);
				ent->state.origin[2] += cl.h2_Effects[index].Chain.height;
				CLHW_XbowImpactPuff(ent->state.origin, cl.h2_Effects[index].Chain.material);
			}
			else
			{
				VectorScale(org, 500 * frametime, org);
				VectorAdd(ent->state.origin, org, ent->state.origin);
			}
		}
		break;
	case 1://attached--snap to owner's pos
		es = CLH2_FindState(cl.h2_Effects[index].Chain.owner);
		if (es)
		{
			VectorCopy(es->origin, ent->state.origin);
			ent->state.origin[2] += cl.h2_Effects[index].Chain.height;
		}
		break;
	case 2://unattaching, server needs to set this state
		{
		vec3_t org;
		VectorCopy(ent->state.origin,org);
		VectorSubtract(cl.h2_Effects[index].Chain.origin, org, org);
		if (fabs(VectorNormalize(org)) > 350 * frametime)//closer than 30 is too close?
		{
			VectorScale(org, 350 * frametime, org);
			VectorAdd(ent->state.origin, org, ent->state.origin);
		}
		else//done--flash & git outa here (change type to redflash)
		{
			S_StartSound(ent->state.origin, CLH2_TempSoundChannel(), 1, clh2_fxsfx_scarabbyebye, 1, 1);
			cl.h2_Effects[index].Flash.entity_index = cl.h2_Effects[index].Chain.ent1;
			cl.h2_Effects[index].type = HWCE_RED_FLASH;
			VectorCopy(ent->state.origin, cl.h2_Effects[index].Flash.origin);
			cl.h2_Effects[index].Flash.reverse = 0;
			ent->model = R_RegisterModel("models/redspt.spr");
			ent->state.frame = 0;
			ent->state.drawflags = H2DRF_TRANSLUCENT;
		}
		}
		break;
	}

	CLH2_LinkEffectEntity(ent);

	CLH2_CreateStreamChain(cl.h2_Effects[index].Chain.ent1, cl.h2_Effects[index].Chain.tag, 1, 0, 100, cl.h2_Effects[index].Chain.origin, ent->state.origin);
}

void CLHW_UpdateEffectTripMineStill(int index, float frametime)
{
	cl.h2_Effects[index].Chain.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

	CLH2_LinkEffectEntity(ent);

	CLH2_CreateStreamChain(cl.h2_Effects[index].Chain.ent1, 1, 1, 0, 100, cl.h2_Effects[index].Chain.origin, ent->state.origin);
}

void CLHW_UpdateEffectTripMine(int index, float frametime)
{
	cl.h2_Effects[index].Chain.time_amount += frametime;
	effect_entity_t* ent = &EffectEntities[cl.h2_Effects[index].Chain.ent1];

	ent->state.origin[0] += frametime * cl.h2_Effects[index].Chain.velocity[0];
	ent->state.origin[1] += frametime * cl.h2_Effects[index].Chain.velocity[1];
	ent->state.origin[2] += frametime * cl.h2_Effects[index].Chain.velocity[2];

	CLH2_LinkEffectEntity(ent);

	CLH2_CreateStreamChain(cl.h2_Effects[index].Chain.ent1, 1, 1, 0, 100, cl.h2_Effects[index].Chain.origin, ent->state.origin);
}

void CL_UpdateEffects()
{
	float frametime = host_frametime;
	if (!frametime)
	{
		return;
	}

	for (int index = 0; index < MAX_EFFECTS_H2; index++)
	{
		if (!cl.h2_Effects[index].type)
		{
			continue;
		}
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
			CLH2_UpdateEffectRiderDeath(index, frametime);
			break;
		case HWCE_TELEPORTERPUFFS:
			CLH2_UpdateEffectTeleporterPuffs(index, frametime);
			break;
		case HWCE_TELEPORTERBODY:
			CLH2_UpdateEffectTeleporterBody(index, frametime);
			break;
		case HWCE_HWDRILLA:
			CLHW_UpdateEffectDrilla(index, frametime);
			break;
		case HWCE_HWXBOWSHOOT:
			CLHW_UpdateEffectXBowShot(index, frametime);
			break;
		case HWCE_HWSHEEPINATOR:
			CLHW_UpdateEffectSheepinator(index, frametime);
			break;
		case HWCE_DEATHBUBBLES:
			CLHW_UpdateEffectDeathBubbles(index, frametime);
			break;
		case HWCE_SCARABCHAIN:
			CLHW_UpdateEffectScarabChain(index, frametime);
			break;
		case HWCE_TRIPMINESTILL:
			CLHW_UpdateEffectTripMineStill(index, frametime);
			break;
		case HWCE_TRIPMINE:
			CLHW_UpdateEffectTripMine(index, frametime);
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
			CLHW_UpdateEffectMissileStar(index, frametime);
			break;
		case HWCE_HWEIDOLONSTAR:
			CLHW_UpdateEffectEidolonStar(index, frametime);
			break;
		}
	}
}
