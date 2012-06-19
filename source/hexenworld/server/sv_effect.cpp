
//**************************************************************************
//**
//** sv_effect.c
//**
//** Client side effects.
//**
//** $Header: /HexenWorld/Server/sv_effect.c 39    5/25/98 1:30p Mgummelt $
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "qwsvdef.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern Cvar* sv_ce_scale;
extern Cvar* sv_ce_max_size;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------


void SV_ClearEffects(void)
{
	Com_Memset(sv.h2_Effects,0,sizeof(sv.h2_Effects));
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
void SV_SendEffect(QMsg* sb, int index)
{
	qboolean DoTest;
	vec3_t TestO;
	int TestDistance;
	int i;

	if (sv_ce_scale->value > 0)
	{
		DoTest = true;
	}
	else
	{
		DoTest = false;
	}

	VectorCopy(vec3_origin, TestO);

	switch (sv.h2_Effects[index].type)
	{
	case HWCE_HWSHEEPINATOR:
	case HWCE_HWXBOWSHOOT:
		VectorCopy(sv.h2_Effects[index].Xbow.origin[5], TestO);
		TestDistance = 900;
		break;
	case HWCE_SCARABCHAIN:
		VectorCopy(sv.h2_Effects[index].Chain.origin, TestO);
		TestDistance = 900;
		break;

	case HWCE_TRIPMINE:
		VectorCopy(sv.h2_Effects[index].Chain.origin, TestO);
//			DoTest = false;
		break;

	//ACHTUNG!!!!!!! setting DoTest to false here does not insure that effect will be sent to everyone!
	case HWCE_TRIPMINESTILL:
		TestDistance = 10000;
		DoTest = false;
		break;

	case HWCE_RAIN:
		TestDistance = 10000;
		DoTest = false;
		break;

	case HWCE_FOUNTAIN:
		TestDistance = 10000;
		DoTest = false;
		break;

	case HWCE_QUAKE:
		VectorCopy(sv.h2_Effects[index].Quake.origin, TestO);
		TestDistance = 700;
		break;

	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK1:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_FLAMESTREAM:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
	case HWCE_RIPPLE:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		TestDistance = 250;
		break;

	case HWCE_SM_WHITE_FLASH:
	case HWCE_YELLOWRED_FLASH:
	case HWCE_BLUESPARK:
	case HWCE_YELLOWSPARK:
	case HWCE_SM_CIRCLE_EXP:
	case HWCE_BG_CIRCLE_EXP:
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
	case HWCE_ACID_HIT:
	case HWCE_LBALL_EXPL:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_FBOOM:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
	case HWCE_BOMB:
	case HWCE_FLOOR_EXPLOSION3:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		TestDistance = 250;
		break;

	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO);
		TestDistance = 250;
		break;


	case HWCE_RIDER_DEATH:
		DoTest = false;
		break;

	case HWCE_TELEPORTERPUFFS:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO);
		TestDistance = 350;
		break;

	case HWCE_TELEPORTERBODY:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO);
		TestDistance = 350;
		break;

	case HWCE_DEATHBUBBLES:
		if (sv.h2_Effects[index].Bubble.owner < 0 || sv.h2_Effects[index].Bubble.owner >= sv.qh_num_edicts)
		{
			return;
		}
		VectorCopy(PROG_TO_EDICT(sv.h2_Effects[index].Bubble.owner)->GetOrigin(), TestO);
		TestDistance = 400;
		break;

	case HWCE_HWDRILLA:
	case HWCE_BONESHARD:
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWRAVENPOWER:

		VectorCopy(sv.h2_Effects[index].Missile.origin, TestO);
		TestDistance = 900;
		break;

	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
		VectorCopy(sv.h2_Effects[index].Missile.origin, TestO);
		TestDistance = 600;
		break;
	default:
//			Sys_Error ("SV_SendEffect: bad type");
		PR_RunError("SV_SendEffect: bad type");
		break;
	}

	sv.multicast.WriteByte(hwsvc_start_effect);
	sv.multicast.WriteByte(index);
	sv.multicast.WriteByte(sv.h2_Effects[index].type);

	switch (sv.h2_Effects[index].type)
	{
	case HWCE_RAIN:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.min_org[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.min_org[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.min_org[2]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.max_org[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.max_org[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.max_org[2]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.e_size[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.e_size[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.e_size[2]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.dir[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.dir[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Rain.dir[2]);
		sv.multicast.WriteShort(sv.h2_Effects[index].Rain.color);
		sv.multicast.WriteShort(sv.h2_Effects[index].Rain.count);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Rain.wait);
		break;

	case HWCE_FOUNTAIN:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Fountain.pos[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Fountain.pos[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Fountain.pos[2]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Fountain.angle[0]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Fountain.angle[1]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Fountain.angle[2]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Fountain.movedir[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Fountain.movedir[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Fountain.movedir[2]);
		sv.multicast.WriteShort(sv.h2_Effects[index].Fountain.color);
		sv.multicast.WriteByte(sv.h2_Effects[index].Fountain.cnt);
		break;

	case HWCE_QUAKE:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Quake.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Quake.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Quake.origin[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Quake.radius);
		break;

	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK1:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_FLAMESTREAM:
	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
	case HWCE_RIPPLE:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Smoke.velocity[0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Smoke.velocity[1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Smoke.velocity[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Smoke.framelength);
		break;

	case HWCE_SM_WHITE_FLASH:
	case HWCE_YELLOWRED_FLASH:
	case HWCE_BLUESPARK:
	case HWCE_YELLOWSPARK:
	case HWCE_SM_CIRCLE_EXP:
	case HWCE_BG_CIRCLE_EXP:
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
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:

		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
		break;

	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
		break;


	case HWCE_RIDER_DEATH:
		sv.multicast.WriteCoord(sv.h2_Effects[index].RD.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].RD.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].RD.origin[2]);
		break;

	case HWCE_TELEPORTERPUFFS:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Teleporter.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Teleporter.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Teleporter.origin[2]);
		break;

	case HWCE_TELEPORTERBODY:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Teleporter.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Teleporter.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Teleporter.origin[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Teleporter.skinnum);
		break;
	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.velocity[0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.velocity[1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.velocity[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.angle[0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.angle[1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.angle[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.avelocity[0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.avelocity[1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.avelocity[2]);

		break;
	case HWCE_BONESHARD:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
	case HWCE_HWRAVENPOWER:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.velocity[0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.velocity[1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Missile.velocity[2]);
		break;
	case HWCE_HWDRILLA:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Missile.angle[0]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Missile.angle[1]);
		sv.multicast.WriteShort(sv.h2_Effects[index].Missile.speed);
		break;
	case HWCE_DEATHBUBBLES:
		sv.multicast.WriteShort(sv.h2_Effects[index].Bubble.owner);
		sv.multicast.WriteByte(sv.h2_Effects[index].Bubble.offset[0]);
		sv.multicast.WriteByte(sv.h2_Effects[index].Bubble.offset[1]);
		sv.multicast.WriteByte(sv.h2_Effects[index].Bubble.offset[2]);
		sv.multicast.WriteByte(sv.h2_Effects[index].Bubble.count);
		break;
	case HWCE_SCARABCHAIN:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Chain.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Chain.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Chain.origin[2]);
		sv.multicast.WriteShort(sv.h2_Effects[index].Chain.owner + sv.h2_Effects[index].Chain.material);
		sv.multicast.WriteByte(sv.h2_Effects[index].Chain.tag);
		break;
	case HWCE_TRIPMINESTILL:
	case HWCE_TRIPMINE:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Chain.origin[0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Chain.origin[1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Chain.origin[2]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Chain.velocity[0]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Chain.velocity[1]);
		sv.multicast.WriteFloat(sv.h2_Effects[index].Chain.velocity[2]);
		break;
	case HWCE_HWSHEEPINATOR:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][2]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.angle[0]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.angle[1]);

		//now send the guys that have turned
		sv.multicast.WriteByte(sv.h2_Effects[index].Xbow.turnedbolts);
		sv.multicast.WriteByte(sv.h2_Effects[index].Xbow.activebolts);
		for (i = 0; i < 5; i++)
		{
			if ((1 << i) & sv.h2_Effects[index].Xbow.turnedbolts)
			{
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][0]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][1]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][2]);
				sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][0]);
				sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][1]);
			}
		}
		break;
	case HWCE_HWXBOWSHOOT:
		sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][0]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][1]);
		sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[5][2]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.angle[0]);
		sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.angle[1]);
//				sv.multicast.WriteFloat(sv.h2_Effects[index].Xbow.angle[2]);
		sv.multicast.WriteByte(sv.h2_Effects[index].Xbow.bolts);
		sv.multicast.WriteByte(sv.h2_Effects[index].Xbow.randseed);

		//now send the guys that have turned
		sv.multicast.WriteByte(sv.h2_Effects[index].Xbow.turnedbolts);
		sv.multicast.WriteByte(sv.h2_Effects[index].Xbow.activebolts);
		for (i = 0; i < 5; i++)
		{
			if ((1 << i) & sv.h2_Effects[index].Xbow.turnedbolts)
			{
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][0]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][1]);
				sv.multicast.WriteCoord(sv.h2_Effects[index].Xbow.origin[i][2]);
				sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][0]);
				sv.multicast.WriteAngle(sv.h2_Effects[index].Xbow.vel[i][1]);
			}
		}
		break;
	default:
//			Sys_Error ("SV_SendEffect: bad type");
		PR_RunError("SV_SendEffect: bad type");
		break;
	}

	if (sb)
	{
		sb->WriteData(sv.multicast._data, sv.multicast.cursize);
		sv.multicast.Clear();
	}
	else
	{
		if (DoTest)
		{
			SVQH_Multicast(TestO, MULTICAST_PVS_R);
		}
		else
		{
			SVQH_Multicast(TestO, MULTICAST_ALL_R);
		}
		sv.h2_Effects[index].client_list = clients_multicast;
	}
}

void SV_UpdateEffects(QMsg* sb)
{
	int index;

	for (index = 0; index < MAX_EFFECTS_H2; index++)
		if (sv.h2_Effects[index].type)
		{
			SV_SendEffect(sb,index);
		}
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
void SV_ParseEffect(QMsg* sb)
{
	int index;
	byte effect;

	effect = G_FLOAT(OFS_PARM0);

	for (index = 0; index < MAX_EFFECTS_H2; index++)
		if (!sv.h2_Effects[index].type ||
			(sv.h2_Effects[index].expire_time && sv.h2_Effects[index].expire_time <= sv.qh_time))
		{
			break;
		}

	if (index >= MAX_EFFECTS_H2)
	{
		PR_RunError("MAX_EFFECTS_H2 reached");
		return;
	}

//	Con_Printf("Effect #%d\n",index);

	Com_Memset(&sv.h2_Effects[index],0,sizeof(struct h2EffectT));

	sv.h2_Effects[index].type = effect;
	G_FLOAT(OFS_RETURN) = index;

	switch (effect)
	{
	case HWCE_RAIN:
		VectorCopy(G_VECTOR(OFS_PARM1),sv.h2_Effects[index].Rain.min_org);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Rain.max_org);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Rain.e_size);
		VectorCopy(G_VECTOR(OFS_PARM4),sv.h2_Effects[index].Rain.dir);
		sv.h2_Effects[index].Rain.color = G_FLOAT(OFS_PARM5);
		sv.h2_Effects[index].Rain.count = G_FLOAT(OFS_PARM6);
		sv.h2_Effects[index].Rain.wait = G_FLOAT(OFS_PARM7);

		sv.h2_Effects[index].Rain.next_time = 0;
		break;

	case HWCE_FOUNTAIN:
		VectorCopy(G_VECTOR(OFS_PARM1),sv.h2_Effects[index].Fountain.pos);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Fountain.angle);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Fountain.movedir);
		sv.h2_Effects[index].Fountain.color = G_FLOAT(OFS_PARM4);
		sv.h2_Effects[index].Fountain.cnt = G_FLOAT(OFS_PARM5);
		break;

	case HWCE_QUAKE:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Quake.origin);
		sv.h2_Effects[index].Quake.radius = G_FLOAT(OFS_PARM2);
		break;

	case HWCE_WHITE_SMOKE:
	case HWCE_GREEN_SMOKE:
	case HWCE_GREY_SMOKE:
	case HWCE_RED_SMOKE:
	case HWCE_SLOW_WHITE_SMOKE:
	case HWCE_TELESMK1:
	case HWCE_TELESMK2:
	case HWCE_GHOST:
	case HWCE_REDCLOUD:
	case HWCE_RIPPLE:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Smoke.velocity);
		sv.h2_Effects[index].Smoke.framelength = G_FLOAT(OFS_PARM3);

		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case HWCE_ACID_MUZZFL:
	case HWCE_FLAMESTREAM:
	case HWCE_FLAMEWALL:
	case HWCE_FLAMEWALL2:
	case HWCE_ONFIRE:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Smoke.velocity);
		sv.h2_Effects[index].Smoke.framelength = 0.05;
		sv.h2_Effects[index].Smoke.frame = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case HWCE_SM_WHITE_FLASH:
	case HWCE_YELLOWRED_FLASH:
	case HWCE_BLUESPARK:
	case HWCE_YELLOWSPARK:
	case HWCE_SM_CIRCLE_EXP:
	case HWCE_BG_CIRCLE_EXP:
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
	case HWCE_ACID_HIT:
	case HWCE_ACID_SPLAT:
	case HWCE_ACID_EXPL:
	case HWCE_LBALL_EXPL:
	case HWCE_FIREWALL_SMALL:
	case HWCE_FIREWALL_MEDIUM:
	case HWCE_FIREWALL_LARGE:
	case HWCE_FBOOM:
	case HWCE_BOMB:
	case HWCE_BRN_BOUNCE:
	case HWCE_LSHOCK:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case HWCE_WHITE_FLASH:
	case HWCE_BLUE_FLASH:
	case HWCE_SM_BLUE_FLASH:
	case HWCE_HWSPLITFLASH:
	case HWCE_RED_FLASH:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Flash.origin);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case HWCE_RIDER_DEATH:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].RD.origin);
		break;

	case HWCE_TELEPORTERPUFFS:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Teleporter.origin);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case HWCE_TELEPORTERBODY:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Teleporter.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Teleporter.velocity[0]);
		sv.h2_Effects[index].Teleporter.skinnum = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case HWCE_BONESHRAPNEL:
	case HWCE_HWBONEBALL:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Missile.velocity);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Missile.angle);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Missile.avelocity);

		sv.h2_Effects[index].expire_time = sv.qh_time + 10;
		break;
	case HWCE_BONESHARD:
	case HWCE_HWRAVENSTAFF:
	case HWCE_HWMISSILESTAR:
	case HWCE_HWEIDOLONSTAR:
	case HWCE_HWRAVENPOWER:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Missile.velocity);
		sv.h2_Effects[index].expire_time = sv.qh_time + 10;
		break;
	case HWCE_DEATHBUBBLES:
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Bubble.offset);
		sv.h2_Effects[index].Bubble.owner = G_EDICTNUM(OFS_PARM1);
		sv.h2_Effects[index].Bubble.count = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].expire_time = sv.qh_time + 30;
		break;
	case HWCE_HWDRILLA:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Missile.angle);
		sv.h2_Effects[index].Missile.speed = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].expire_time = sv.qh_time + 10;
		break;
	case HWCE_TRIPMINESTILL:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chain.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Chain.velocity);
		sv.h2_Effects[index].expire_time = sv.qh_time + 70;
		break;
	case HWCE_TRIPMINE:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chain.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Chain.velocity);
		sv.h2_Effects[index].expire_time = sv.qh_time + 10;
		break;
	case HWCE_SCARABCHAIN:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Chain.origin);
		sv.h2_Effects[index].Chain.owner = G_EDICTNUM(OFS_PARM2);
		sv.h2_Effects[index].Chain.material = G_INT(OFS_PARM3);
		sv.h2_Effects[index].Chain.tag = G_INT(OFS_PARM4);
		sv.h2_Effects[index].Chain.state = 0;
		sv.h2_Effects[index].expire_time = sv.qh_time + 15;
		break;
	case HWCE_HWSHEEPINATOR:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[0]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[1]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[2]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[3]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[4]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[5]);
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Xbow.angle);
		sv.h2_Effects[index].Xbow.bolts = 5;
		sv.h2_Effects[index].Xbow.activebolts = 31;
		sv.h2_Effects[index].Xbow.randseed = 0;
		sv.h2_Effects[index].Xbow.turnedbolts = 0;
		sv.h2_Effects[index].expire_time = sv.qh_time + 7;
		break;
	case HWCE_HWXBOWSHOOT:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[0]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[1]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[2]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[3]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[4]);
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Xbow.origin[5]);
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Xbow.angle);
		sv.h2_Effects[index].Xbow.bolts = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].Xbow.randseed = G_FLOAT(OFS_PARM4);
		sv.h2_Effects[index].Xbow.turnedbolts = 0;
		if (sv.h2_Effects[index].Xbow.bolts == 3)
		{
			sv.h2_Effects[index].Xbow.activebolts = 7;
		}
		else
		{
			sv.h2_Effects[index].Xbow.activebolts = 31;
		}
		sv.h2_Effects[index].expire_time = sv.qh_time + 15;
		break;
	default:
//			Sys_Error ("SV_ParseEffect: bad type");
		PR_RunError("SV_SendEffect: bad type");
	}

	SV_SendEffect(sb,index);
}

// this random generator can have its effects duplicated on the client
// side by passing the randomseed over the network, as opposed to sending
// all the generated values
static unsigned int randomseed;
void SV_setseed(int seed)
{
	randomseed = seed;
}

float SV_seedrand(void)
{
	randomseed = (randomseed * 877 + 573) % 9968;
	return (float)randomseed / 9968;
}


// this will create several effects and store the ids in the array
static float MultiEffectIds[10];
static int MultiEffectIdCount;

void SV_ParseMultiEffect(QMsg* sb)
{
	int index, count;
	byte effect;
	vec3_t orig, vel;

	MultiEffectIdCount = 0;
	effect = G_FLOAT(OFS_PARM0);
	switch (effect)
	{
	case HWCE_HWRAVENPOWER:
		// need to set aside 3 effect ids

		sb->WriteByte(hwsvc_multieffect);
		sb->WriteByte(effect);

		VectorCopy(G_VECTOR(OFS_PARM1), orig);
		sb->WriteCoord(orig[0]);
		sb->WriteCoord(orig[1]);
		sb->WriteCoord(orig[2]);
		VectorCopy(G_VECTOR(OFS_PARM2), vel);
		sb->WriteCoord(vel[0]);
		sb->WriteCoord(vel[1]);
		sb->WriteCoord(vel[2]);
		for (count = 0; count < 3; count++)
		{
			for (index = 0; index < MAX_EFFECTS_H2; index++)
				if (!sv.h2_Effects[index].type ||
					(sv.h2_Effects[index].expire_time && sv.h2_Effects[index].expire_time <= sv.qh_time))
				{
					break;
				}
			if (index >= MAX_EFFECTS_H2)
			{
				PR_RunError("MAX_EFFECTS_H2 reached");
				return;
			}
			sb->WriteByte(index);
			sv.h2_Effects[index].type = HWCE_HWRAVENPOWER;
			VectorCopy(orig, sv.h2_Effects[index].Missile.origin);
			VectorCopy(vel, sv.h2_Effects[index].Missile.velocity);
			sv.h2_Effects[index].expire_time = sv.qh_time + 10;
			MultiEffectIds[count] = index;
		}
		break;
	default:
		PR_RunError("SV_ParseMultiEffect: bad type");
	}
}

float SV_GetMultiEffectId(void)
{
	MultiEffectIdCount++;
	return MultiEffectIds[MultiEffectIdCount - 1];
}
