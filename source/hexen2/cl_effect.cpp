
//**************************************************************************
//**
//** cl_effect.c
//**
//** Client side effects.
//**
//** $Header: /H2 Mission Pack/cl_effect.c 41    3/20/98 12:55p Jmonroe $
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
// SV_SaveEffects(), SV_LoadEffects()
void SV_SendEffect(QMsg* sb, int index)
{
	qboolean DoTest;
	vec3_t TestO1,Diff;
	float Size,TestDistance;
	int i,count;

	if (sb == &sv.qh_reliable_datagram && sv_ce_scale->value > 0)
	{
		DoTest = true;
	}
	else
	{
		DoTest = false;
	}

	VectorCopy(vec3_origin, TestO1);
	TestDistance = 0;

	switch (sv.h2_Effects[index].type)
	{
	case H2CE_RAIN:
	case H2CE_SNOW:
		DoTest = false;
		break;

	case H2CE_FOUNTAIN:
		DoTest = false;
		break;

	case H2CE_QUAKE:
		VectorCopy(sv.h2_Effects[index].Quake.origin, TestO1);
		TestDistance = 700;
		break;

	case H2CE_WHITE_SMOKE:
	case H2CE_GREEN_SMOKE:
	case H2CE_GREY_SMOKE:
	case H2CE_RED_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
	case H2CE_TELESMK1:
	case H2CE_TELESMK2:
	case H2CE_GHOST:
	case H2CE_REDCLOUD:
	case H2CE_FLAMESTREAM:
	case H2CE_ACID_MUZZFL:
	case H2CE_FLAMEWALL:
	case H2CE_FLAMEWALL2:
	case H2CE_ONFIRE:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO1);
		TestDistance = 250;
		break;

	case H2CE_SM_WHITE_FLASH:
	case H2CE_YELLOWRED_FLASH:
	case H2CE_BLUESPARK:
	case H2CE_YELLOWSPARK:
	case H2CE_SM_CIRCLE_EXP:
	case H2CE_BG_CIRCLE_EXP:
	case H2CE_SM_EXPLOSION:
	case H2CE_LG_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION:
	case H2CE_BLUE_EXPLOSION:
	case H2CE_REDSPARK:
	case H2CE_GREENSPARK:
	case H2CE_ICEHIT:
	case H2CE_MEDUSA_HIT:
	case H2CE_MEZZO_REFLECT:
	case H2CE_FLOOR_EXPLOSION2:
	case H2CE_XBOW_EXPLOSION:
	case H2CE_NEW_EXPLOSION:
	case H2CE_MAGIC_MISSILE_EXPLOSION:
	case H2CE_BONE_EXPLOSION:
	case H2CE_BLDRN_EXPL:
	case H2CE_ACID_HIT:
	case H2CE_LBALL_EXPL:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_FBOOM:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
	case H2CE_BOMB:
	case H2CE_FLOOR_EXPLOSION3:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO1);
		TestDistance = 250;
		break;

	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		VectorCopy(sv.h2_Effects[index].Smoke.origin, TestO1);
		TestDistance = 250;
		break;

	case H2CE_RIDER_DEATH:
		DoTest = false;
		break;

	case H2CE_GRAVITYWELL:
		DoTest = false;
		break;

	case H2CE_TELEPORTERPUFFS:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO1);
		TestDistance = 350;
		break;

	case H2CE_TELEPORTERBODY:
		VectorCopy(sv.h2_Effects[index].Teleporter.origin, TestO1);
		TestDistance = 350;
		break;

	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		VectorCopy(sv.h2_Effects[index].Missile.origin, TestO1);
		TestDistance = 600;
		break;
	case H2CE_CHUNK:
		VectorCopy(sv.h2_Effects[index].Chunk.origin, TestO1);
		TestDistance = 600;
		break;

	default:
//			common->FatalError ("SV_SendEffect: bad type");
		PR_RunError("SV_SendEffect: bad type");
		break;
	}

	if (!DoTest)
	{
		count = 1;
	}
	else
	{
		count = svs.qh_maxclients;
		TestDistance = (float)TestDistance * sv_ce_scale->value;
		TestDistance *= TestDistance;
	}


	for (i = 0; i < count; i++)
	{
		if (DoTest)
		{
			if (svs.clients[i].state >= CS_CONNECTED)
			{
				sb = &svs.clients[i].datagram;
				VectorSubtract(svs.clients[i].qh_edict->GetOrigin(),TestO1,Diff);
				Size = (Diff[0] * Diff[0]) + (Diff[1] * Diff[1]) + (Diff[2] * Diff[2]);

				if (Size > TestDistance)
				{
					continue;
				}

				if (sv_ce_max_size->value > 0 && sb->cursize > sv_ce_max_size->value)
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}

		sb->WriteByte(h2svc_start_effect);
		sb->WriteByte(index);
		sb->WriteByte(sv.h2_Effects[index].type);

		switch (sv.h2_Effects[index].type)
		{
		case H2CE_RAIN:
			sb->WriteCoord(sv.h2_Effects[index].Rain.min_org[0]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.min_org[1]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.min_org[2]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.max_org[0]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.max_org[1]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.max_org[2]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.e_size[0]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.e_size[1]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.e_size[2]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.dir[0]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.dir[1]);
			sb->WriteCoord(sv.h2_Effects[index].Rain.dir[2]);
			sb->WriteShort(sv.h2_Effects[index].Rain.color);
			sb->WriteShort(sv.h2_Effects[index].Rain.count);
			sb->WriteFloat(sv.h2_Effects[index].Rain.wait);
			break;

		case H2CE_SNOW:
			sb->WriteCoord(sv.h2_Effects[index].Snow.min_org[0]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.min_org[1]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.min_org[2]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.max_org[0]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.max_org[1]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.max_org[2]);
			sb->WriteByte(sv.h2_Effects[index].Snow.flags);
			sb->WriteCoord(sv.h2_Effects[index].Snow.dir[0]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.dir[1]);
			sb->WriteCoord(sv.h2_Effects[index].Snow.dir[2]);
			sb->WriteByte(sv.h2_Effects[index].Snow.count);
			break;

		case H2CE_FOUNTAIN:
			sb->WriteCoord(sv.h2_Effects[index].Fountain.pos[0]);
			sb->WriteCoord(sv.h2_Effects[index].Fountain.pos[1]);
			sb->WriteCoord(sv.h2_Effects[index].Fountain.pos[2]);
			sb->WriteAngle(sv.h2_Effects[index].Fountain.angle[0]);
			sb->WriteAngle(sv.h2_Effects[index].Fountain.angle[1]);
			sb->WriteAngle(sv.h2_Effects[index].Fountain.angle[2]);
			sb->WriteCoord(sv.h2_Effects[index].Fountain.movedir[0]);
			sb->WriteCoord(sv.h2_Effects[index].Fountain.movedir[1]);
			sb->WriteCoord(sv.h2_Effects[index].Fountain.movedir[2]);
			sb->WriteShort(sv.h2_Effects[index].Fountain.color);
			sb->WriteByte(sv.h2_Effects[index].Fountain.cnt);
			break;

		case H2CE_QUAKE:
			sb->WriteCoord(sv.h2_Effects[index].Quake.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Quake.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Quake.origin[2]);
			sb->WriteFloat(sv.h2_Effects[index].Quake.radius);
			break;

		case H2CE_WHITE_SMOKE:
		case H2CE_GREEN_SMOKE:
		case H2CE_GREY_SMOKE:
		case H2CE_RED_SMOKE:
		case H2CE_SLOW_WHITE_SMOKE:
		case H2CE_TELESMK1:
		case H2CE_TELESMK2:
		case H2CE_GHOST:
		case H2CE_REDCLOUD:
		case H2CE_FLAMESTREAM:
		case H2CE_ACID_MUZZFL:
		case H2CE_FLAMEWALL:
		case H2CE_FLAMEWALL2:
		case H2CE_ONFIRE:
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
			sb->WriteFloat(sv.h2_Effects[index].Smoke.velocity[0]);
			sb->WriteFloat(sv.h2_Effects[index].Smoke.velocity[1]);
			sb->WriteFloat(sv.h2_Effects[index].Smoke.velocity[2]);
			sb->WriteFloat(sv.h2_Effects[index].Smoke.framelength);
			sb->WriteFloat(sv.h2_Effects[index].Smoke.frame);
			break;

		case H2CE_SM_WHITE_FLASH:
		case H2CE_YELLOWRED_FLASH:
		case H2CE_BLUESPARK:
		case H2CE_YELLOWSPARK:
		case H2CE_SM_CIRCLE_EXP:
		case H2CE_BG_CIRCLE_EXP:
		case H2CE_SM_EXPLOSION:
		case H2CE_LG_EXPLOSION:
		case H2CE_FLOOR_EXPLOSION:
		case H2CE_FLOOR_EXPLOSION3:
		case H2CE_BLUE_EXPLOSION:
		case H2CE_REDSPARK:
		case H2CE_GREENSPARK:
		case H2CE_ICEHIT:
		case H2CE_MEDUSA_HIT:
		case H2CE_MEZZO_REFLECT:
		case H2CE_FLOOR_EXPLOSION2:
		case H2CE_XBOW_EXPLOSION:
		case H2CE_NEW_EXPLOSION:
		case H2CE_MAGIC_MISSILE_EXPLOSION:
		case H2CE_BONE_EXPLOSION:
		case H2CE_BLDRN_EXPL:
		case H2CE_ACID_HIT:
		case H2CE_ACID_SPLAT:
		case H2CE_ACID_EXPL:
		case H2CE_LBALL_EXPL:
		case H2CE_FIREWALL_SMALL:
		case H2CE_FIREWALL_MEDIUM:
		case H2CE_FIREWALL_LARGE:
		case H2CE_FBOOM:
		case H2CE_BOMB:
		case H2CE_BRN_BOUNCE:
		case H2CE_LSHOCK:
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
			break;

		case H2CE_WHITE_FLASH:
		case H2CE_BLUE_FLASH:
		case H2CE_SM_BLUE_FLASH:
		case H2CE_RED_FLASH:
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Smoke.origin[2]);
			break;

		case H2CE_RIDER_DEATH:
			sb->WriteCoord(sv.h2_Effects[index].RD.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].RD.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].RD.origin[2]);
			break;

		case H2CE_TELEPORTERPUFFS:
			sb->WriteCoord(sv.h2_Effects[index].Teleporter.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Teleporter.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Teleporter.origin[2]);
			break;

		case H2CE_TELEPORTERBODY:
			sb->WriteCoord(sv.h2_Effects[index].Teleporter.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Teleporter.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Teleporter.origin[2]);
			sb->WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][0]);
			sb->WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][1]);
			sb->WriteFloat(sv.h2_Effects[index].Teleporter.velocity[0][2]);
			sb->WriteFloat(sv.h2_Effects[index].Teleporter.skinnum);
			break;

		case H2CE_BONESHARD:
		case H2CE_BONESHRAPNEL:
			sb->WriteCoord(sv.h2_Effects[index].Missile.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Missile.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Missile.origin[2]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.velocity[0]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.velocity[1]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.velocity[2]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.angle[0]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.angle[1]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.angle[2]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.avelocity[0]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.avelocity[1]);
			sb->WriteFloat(sv.h2_Effects[index].Missile.avelocity[2]);

			break;

		case H2CE_GRAVITYWELL:
			sb->WriteCoord(sv.h2_Effects[index].GravityWell.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].GravityWell.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].GravityWell.origin[2]);
			sb->WriteShort(sv.h2_Effects[index].GravityWell.color);
			sb->WriteFloat(sv.h2_Effects[index].GravityWell.lifetime);
			break;

		case H2CE_CHUNK:
			sb->WriteCoord(sv.h2_Effects[index].Chunk.origin[0]);
			sb->WriteCoord(sv.h2_Effects[index].Chunk.origin[1]);
			sb->WriteCoord(sv.h2_Effects[index].Chunk.origin[2]);
			sb->WriteByte(sv.h2_Effects[index].Chunk.type);
			sb->WriteCoord(sv.h2_Effects[index].Chunk.srcVel[0]);
			sb->WriteCoord(sv.h2_Effects[index].Chunk.srcVel[1]);
			sb->WriteCoord(sv.h2_Effects[index].Chunk.srcVel[2]);
			sb->WriteByte(sv.h2_Effects[index].Chunk.numChunks);

			//common->Printf("Adding %d chunks on server...\n",sv.h2_Effects[index].Chunk.numChunks);
			break;

		default:
			//			common->FatalError ("SV_SendEffect: bad type");
			PR_RunError("SV_SendEffect: bad type");
			break;
		}
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
// SV_SaveEffects(), SV_LoadEffects()
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

//	common->Printf("Effect #%d\n",index);

	Com_Memset(&sv.h2_Effects[index],0,sizeof(struct h2EffectT));

	sv.h2_Effects[index].type = effect;
	G_FLOAT(OFS_RETURN) = index;

	switch (effect)
	{
	case H2CE_RAIN:
		VectorCopy(G_VECTOR(OFS_PARM1),sv.h2_Effects[index].Rain.min_org);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Rain.max_org);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Rain.e_size);
		VectorCopy(G_VECTOR(OFS_PARM4),sv.h2_Effects[index].Rain.dir);
		sv.h2_Effects[index].Rain.color = G_FLOAT(OFS_PARM5);
		sv.h2_Effects[index].Rain.count = G_FLOAT(OFS_PARM6);
		sv.h2_Effects[index].Rain.wait = G_FLOAT(OFS_PARM7);

		sv.h2_Effects[index].Rain.next_time = 0;
		break;

	case H2CE_SNOW:
		VectorCopy(G_VECTOR(OFS_PARM1),sv.h2_Effects[index].Snow.min_org);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Snow.max_org);
		sv.h2_Effects[index].Snow.flags = G_FLOAT(OFS_PARM3);
		VectorCopy(G_VECTOR(OFS_PARM4),sv.h2_Effects[index].Snow.dir);
		sv.h2_Effects[index].Snow.count = G_FLOAT(OFS_PARM5);

		sv.h2_Effects[index].Snow.next_time = 0;
		break;

	case H2CE_FOUNTAIN:
		VectorCopy(G_VECTOR(OFS_PARM1),sv.h2_Effects[index].Fountain.pos);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Fountain.angle);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Fountain.movedir);
		sv.h2_Effects[index].Fountain.color = G_FLOAT(OFS_PARM4);
		sv.h2_Effects[index].Fountain.cnt = G_FLOAT(OFS_PARM5);
		break;

	case H2CE_QUAKE:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Quake.origin);
		sv.h2_Effects[index].Quake.radius = G_FLOAT(OFS_PARM2);
		break;

	case H2CE_WHITE_SMOKE:
	case H2CE_GREEN_SMOKE:
	case H2CE_GREY_SMOKE:
	case H2CE_RED_SMOKE:
	case H2CE_SLOW_WHITE_SMOKE:
	case H2CE_TELESMK1:
	case H2CE_TELESMK2:
	case H2CE_GHOST:
	case H2CE_REDCLOUD:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Smoke.velocity);
		sv.h2_Effects[index].Smoke.framelength = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].Smoke.frame = 0;
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case H2CE_ACID_MUZZFL:
	case H2CE_FLAMESTREAM:
	case H2CE_FLAMEWALL:
	case H2CE_FLAMEWALL2:
	case H2CE_ONFIRE:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
		VectorCopy(G_VECTOR(OFS_PARM2), sv.h2_Effects[index].Smoke.velocity);
		sv.h2_Effects[index].Smoke.framelength = 0.05;
		sv.h2_Effects[index].Smoke.frame = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case H2CE_SM_WHITE_FLASH:
	case H2CE_YELLOWRED_FLASH:
	case H2CE_BLUESPARK:
	case H2CE_YELLOWSPARK:
	case H2CE_SM_CIRCLE_EXP:
	case H2CE_BG_CIRCLE_EXP:
	case H2CE_SM_EXPLOSION:
	case H2CE_LG_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION:
	case H2CE_FLOOR_EXPLOSION3:
	case H2CE_BLUE_EXPLOSION:
	case H2CE_REDSPARK:
	case H2CE_GREENSPARK:
	case H2CE_ICEHIT:
	case H2CE_MEDUSA_HIT:
	case H2CE_MEZZO_REFLECT:
	case H2CE_FLOOR_EXPLOSION2:
	case H2CE_XBOW_EXPLOSION:
	case H2CE_NEW_EXPLOSION:
	case H2CE_MAGIC_MISSILE_EXPLOSION:
	case H2CE_BONE_EXPLOSION:
	case H2CE_BLDRN_EXPL:
	case H2CE_ACID_HIT:
	case H2CE_ACID_SPLAT:
	case H2CE_ACID_EXPL:
	case H2CE_LBALL_EXPL:
	case H2CE_FIREWALL_SMALL:
	case H2CE_FIREWALL_MEDIUM:
	case H2CE_FIREWALL_LARGE:
	case H2CE_FBOOM:
	case H2CE_BOMB:
	case H2CE_BRN_BOUNCE:
	case H2CE_LSHOCK:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Smoke.origin);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case H2CE_WHITE_FLASH:
	case H2CE_BLUE_FLASH:
	case H2CE_SM_BLUE_FLASH:
	case H2CE_RED_FLASH:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Flash.origin);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case H2CE_RIDER_DEATH:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].RD.origin);
		break;

	case H2CE_GRAVITYWELL:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].GravityWell.origin);
		sv.h2_Effects[index].GravityWell.color = G_FLOAT(OFS_PARM2);
		sv.h2_Effects[index].GravityWell.lifetime = G_FLOAT(OFS_PARM3);
		break;

	case H2CE_TELEPORTERPUFFS:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Teleporter.origin);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case H2CE_TELEPORTERBODY:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Teleporter.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Teleporter.velocity[0]);
		sv.h2_Effects[index].Teleporter.skinnum = G_FLOAT(OFS_PARM3);
		sv.h2_Effects[index].expire_time = sv.qh_time + 1;
		break;

	case H2CE_BONESHARD:
	case H2CE_BONESHRAPNEL:
		VectorCopy(G_VECTOR(OFS_PARM1), sv.h2_Effects[index].Missile.origin);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Missile.velocity);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Missile.angle);
		VectorCopy(G_VECTOR(OFS_PARM2),sv.h2_Effects[index].Missile.avelocity);

		sv.h2_Effects[index].expire_time = sv.qh_time + 10;
		break;

	case H2CE_CHUNK:
		VectorCopy(G_VECTOR(OFS_PARM1),sv.h2_Effects[index].Chunk.origin);
		sv.h2_Effects[index].Chunk.type = G_FLOAT(OFS_PARM2);
		VectorCopy(G_VECTOR(OFS_PARM3),sv.h2_Effects[index].Chunk.srcVel);
		sv.h2_Effects[index].Chunk.numChunks = G_FLOAT(OFS_PARM4);

		sv.h2_Effects[index].expire_time = sv.qh_time + 3;
		break;


	default:
//			common->FatalError ("SV_ParseEffect: bad type");
		PR_RunError("SV_SendEffect: bad type");
	}

	SV_SendEffect(sb,index);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects()
void SV_SaveEffects(fileHandle_t FH)
{
	int index,count;

	for (index = count = 0; index < MAX_EFFECTS_H2; index++)
		if (sv.h2_Effects[index].type)
		{
			count++;
		}

	FS_Printf(FH,"Effects: %d\n",count);

	for (index = count = 0; index < MAX_EFFECTS_H2; index++)
		if (sv.h2_Effects[index].type)
		{
			FS_Printf(FH,"Effect: %d %d %f: ",index,sv.h2_Effects[index].type,sv.h2_Effects[index].expire_time);

			switch (sv.h2_Effects[index].type)
			{
			case H2CE_RAIN:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.min_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.min_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.min_org[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.max_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.max_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.max_org[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.e_size[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.e_size[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.e_size[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.dir[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.dir[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Rain.dir[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Rain.color);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Rain.count);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Rain.wait);
				break;

			case H2CE_SNOW:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.min_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.min_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.min_org[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.max_org[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.max_org[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.max_org[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Snow.flags);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.dir[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.dir[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Snow.dir[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Snow.count);
				break;

			case H2CE_FOUNTAIN:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.pos[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.pos[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.pos[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.angle[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.angle[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.angle[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.movedir[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.movedir[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Fountain.movedir[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Fountain.color);
				FS_Printf(FH, "%d\n", sv.h2_Effects[index].Fountain.cnt);
				break;

			case H2CE_QUAKE:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Quake.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Quake.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Quake.origin[2]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Quake.radius);
				break;

			case H2CE_WHITE_SMOKE:
			case H2CE_GREEN_SMOKE:
			case H2CE_GREY_SMOKE:
			case H2CE_RED_SMOKE:
			case H2CE_SLOW_WHITE_SMOKE:
			case H2CE_TELESMK1:
			case H2CE_TELESMK2:
			case H2CE_GHOST:
			case H2CE_REDCLOUD:
			case H2CE_ACID_MUZZFL:
			case H2CE_FLAMESTREAM:
			case H2CE_FLAMEWALL:
			case H2CE_FLAMEWALL2:
			case H2CE_ONFIRE:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.velocity[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.velocity[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.velocity[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.framelength);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Smoke.frame);
				break;

			case H2CE_SM_WHITE_FLASH:
			case H2CE_YELLOWRED_FLASH:
			case H2CE_BLUESPARK:
			case H2CE_YELLOWSPARK:
			case H2CE_SM_CIRCLE_EXP:
			case H2CE_BG_CIRCLE_EXP:
			case H2CE_SM_EXPLOSION:
			case H2CE_LG_EXPLOSION:
			case H2CE_FLOOR_EXPLOSION:
			case H2CE_FLOOR_EXPLOSION3:
			case H2CE_BLUE_EXPLOSION:
			case H2CE_REDSPARK:
			case H2CE_GREENSPARK:
			case H2CE_ICEHIT:
			case H2CE_MEDUSA_HIT:
			case H2CE_MEZZO_REFLECT:
			case H2CE_FLOOR_EXPLOSION2:
			case H2CE_XBOW_EXPLOSION:
			case H2CE_NEW_EXPLOSION:
			case H2CE_MAGIC_MISSILE_EXPLOSION:
			case H2CE_BONE_EXPLOSION:
			case H2CE_BLDRN_EXPL:
			case H2CE_BRN_BOUNCE:
			case H2CE_LSHOCK:
			case H2CE_ACID_HIT:
			case H2CE_ACID_SPLAT:
			case H2CE_ACID_EXPL:
			case H2CE_LBALL_EXPL:
			case H2CE_FIREWALL_SMALL:
			case H2CE_FIREWALL_MEDIUM:
			case H2CE_FIREWALL_LARGE:
			case H2CE_FBOOM:
			case H2CE_BOMB:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Smoke.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Smoke.origin[2]);
				break;

			case H2CE_WHITE_FLASH:
			case H2CE_BLUE_FLASH:
			case H2CE_SM_BLUE_FLASH:
			case H2CE_RED_FLASH:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Flash.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Flash.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Flash.origin[2]);
				break;

			case H2CE_RIDER_DEATH:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].RD.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].RD.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].RD.origin[2]);
				break;

			case H2CE_GRAVITYWELL:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].GravityWell.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].GravityWell.origin[1]);
				FS_Printf(FH, "%f", sv.h2_Effects[index].GravityWell.origin[2]);
				FS_Printf(FH, "%d", sv.h2_Effects[index].GravityWell.color);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].GravityWell.lifetime);
				break;
			case H2CE_TELEPORTERPUFFS:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Teleporter.origin[2]);
				break;

			case H2CE_TELEPORTERBODY:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Teleporter.origin[1]);
				FS_Printf(FH, "%f\n", sv.h2_Effects[index].Teleporter.origin[2]);
				break;

			case H2CE_BONESHARD:
			case H2CE_BONESHRAPNEL:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.origin[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.velocity[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.velocity[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.velocity[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.angle[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.angle[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.angle[2]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.avelocity[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.avelocity[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Missile.avelocity[2]);
				break;

			case H2CE_CHUNK:
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.origin[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.origin[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.origin[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Chunk.type);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.srcVel[0]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.srcVel[1]);
				FS_Printf(FH, "%f ", sv.h2_Effects[index].Chunk.srcVel[2]);
				FS_Printf(FH, "%d ", sv.h2_Effects[index].Chunk.numChunks);
				break;

			default:
				PR_RunError("SV_SaveEffect: bad type");
				break;
			}

		}
}

static int GetInt(char*& Data)
{
	//	Skip whitespace.
	while (*Data && *Data <= ' ')
		Data++;
	char Tmp[32];
	int Len = 0;
	while ((*Data >= '0' && *Data <= '9') || *Data == '-')
	{
		if (Len >= (int)sizeof(Tmp) - 1)
		{
			Tmp[31] = 0;
			common->FatalError("Number too long %s", Tmp);
		}
		Tmp[Len] = *Data++;
		Len++;
	}
	Tmp[Len] = 0;
	return String::Atoi(Tmp);
}

static float GetFloat(char*& Data)
{
	//	Skip whitespace.
	while (*Data && *Data <= ' ')
		Data++;
	char Tmp[32];
	int Len = 0;
	while ((*Data >= '0' && *Data <= '9') || *Data == '-' || *Data == '.')
	{
		if (Len >= (int)sizeof(Tmp) - 1)
		{
			Tmp[31] = 0;
			common->FatalError("Number too long %s", Tmp);
		}
		Tmp[Len] = *Data++;
		Len++;
	}
	Tmp[Len] = 0;
	return String::Atof(Tmp);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects()
char* SV_LoadEffects(char* Data)
{
	int index,Total,count;

	// Since the map is freshly loaded, clear out any effects as a result of
	// the loading
	SV_ClearEffects();

	if (String::NCmp(Data, "Effects: ", 9))
	{
		throw DropException("Effects expected");
	}
	Data += 9;
	Total = GetInt(Data);

	for (count = 0; count < Total; count++)
	{
		//	Skip whitespace.
		while (*Data && *Data <= ' ')
			Data++;
		if (String::NCmp(Data, "Effect: ", 8))
		{
			common->FatalError("Effect expected");
		}
		Data += 8;
		index = GetInt(Data);
		sv.h2_Effects[index].type = GetInt(Data);
		sv.h2_Effects[index].expire_time = GetFloat(Data);
		if (*Data != ':')
		{
			common->FatalError("Colon expected");
		}
		Data++;

		switch (sv.h2_Effects[index].type)
		{
		case H2CE_RAIN:
			sv.h2_Effects[index].Rain.min_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.min_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.min_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.max_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.max_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.max_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.e_size[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.e_size[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.e_size[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.dir[0] = GetFloat(Data);
			sv.h2_Effects[index].Rain.dir[1] = GetFloat(Data);
			sv.h2_Effects[index].Rain.dir[2] = GetFloat(Data);
			sv.h2_Effects[index].Rain.color = GetInt(Data);
			sv.h2_Effects[index].Rain.count = GetInt(Data);
			sv.h2_Effects[index].Rain.wait = GetFloat(Data);
			break;

		case H2CE_SNOW:
			sv.h2_Effects[index].Snow.min_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Snow.min_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Snow.min_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Snow.max_org[0] = GetFloat(Data);
			sv.h2_Effects[index].Snow.max_org[1] = GetFloat(Data);
			sv.h2_Effects[index].Snow.max_org[2] = GetFloat(Data);
			sv.h2_Effects[index].Snow.flags = GetInt(Data);
			sv.h2_Effects[index].Snow.dir[0] = GetFloat(Data);
			sv.h2_Effects[index].Snow.dir[1] = GetFloat(Data);
			sv.h2_Effects[index].Snow.dir[2] = GetFloat(Data);
			sv.h2_Effects[index].Snow.count = GetInt(Data);
			break;

		case H2CE_FOUNTAIN:
			sv.h2_Effects[index].Fountain.pos[0] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.pos[1] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.pos[2] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.angle[0] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.angle[1] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.angle[2] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.movedir[0] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.movedir[1] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.movedir[2] = GetFloat(Data);
			sv.h2_Effects[index].Fountain.color = GetInt(Data);
			sv.h2_Effects[index].Fountain.cnt = GetInt(Data);
			break;

		case H2CE_QUAKE:
			sv.h2_Effects[index].Quake.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Quake.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Quake.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Quake.radius = GetFloat(Data);
			break;

		case H2CE_WHITE_SMOKE:
		case H2CE_GREEN_SMOKE:
		case H2CE_GREY_SMOKE:
		case H2CE_RED_SMOKE:
		case H2CE_SLOW_WHITE_SMOKE:
		case H2CE_TELESMK1:
		case H2CE_TELESMK2:
		case H2CE_GHOST:
		case H2CE_REDCLOUD:
		case H2CE_ACID_MUZZFL:
		case H2CE_FLAMESTREAM:
		case H2CE_FLAMEWALL:
		case H2CE_FLAMEWALL2:
		case H2CE_ONFIRE:
			sv.h2_Effects[index].Smoke.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.velocity[0] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.velocity[1] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.velocity[2] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.framelength = GetFloat(Data);
			sv.h2_Effects[index].Smoke.frame = GetFloat(Data);
			break;

		case H2CE_SM_WHITE_FLASH:
		case H2CE_YELLOWRED_FLASH:
		case H2CE_BLUESPARK:
		case H2CE_YELLOWSPARK:
		case H2CE_SM_CIRCLE_EXP:
		case H2CE_BG_CIRCLE_EXP:
		case H2CE_SM_EXPLOSION:
		case H2CE_LG_EXPLOSION:
		case H2CE_FLOOR_EXPLOSION:
		case H2CE_FLOOR_EXPLOSION3:
		case H2CE_BLUE_EXPLOSION:
		case H2CE_REDSPARK:
		case H2CE_GREENSPARK:
		case H2CE_ICEHIT:
		case H2CE_MEDUSA_HIT:
		case H2CE_MEZZO_REFLECT:
		case H2CE_FLOOR_EXPLOSION2:
		case H2CE_XBOW_EXPLOSION:
		case H2CE_NEW_EXPLOSION:
		case H2CE_MAGIC_MISSILE_EXPLOSION:
		case H2CE_BONE_EXPLOSION:
		case H2CE_BLDRN_EXPL:
		case H2CE_BRN_BOUNCE:
		case H2CE_LSHOCK:
		case H2CE_ACID_HIT:
		case H2CE_ACID_SPLAT:
		case H2CE_ACID_EXPL:
		case H2CE_LBALL_EXPL:
		case H2CE_FBOOM:
		case H2CE_FIREWALL_SMALL:
		case H2CE_FIREWALL_MEDIUM:
		case H2CE_FIREWALL_LARGE:
		case H2CE_BOMB:
			sv.h2_Effects[index].Smoke.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Smoke.origin[2] = GetFloat(Data);
			break;

		case H2CE_WHITE_FLASH:
		case H2CE_BLUE_FLASH:
		case H2CE_SM_BLUE_FLASH:
		case H2CE_RED_FLASH:
			sv.h2_Effects[index].Flash.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Flash.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Flash.origin[2] = GetFloat(Data);
			break;

		case H2CE_RIDER_DEATH:
			sv.h2_Effects[index].RD.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].RD.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].RD.origin[2] = GetFloat(Data);
			break;

		case H2CE_GRAVITYWELL:
			sv.h2_Effects[index].GravityWell.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].GravityWell.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].GravityWell.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].GravityWell.color = GetInt(Data);
			sv.h2_Effects[index].GravityWell.lifetime = GetFloat(Data);
			break;

		case H2CE_TELEPORTERPUFFS:
			sv.h2_Effects[index].Teleporter.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[2] = GetFloat(Data);
			break;

		case H2CE_TELEPORTERBODY:
			sv.h2_Effects[index].Teleporter.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Teleporter.origin[2] = GetFloat(Data);
			break;

		case H2CE_BONESHARD:
		case H2CE_BONESHRAPNEL:
			sv.h2_Effects[index].Missile.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Missile.velocity[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.velocity[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.velocity[2] = GetFloat(Data);
			sv.h2_Effects[index].Missile.angle[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.angle[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.angle[2] = GetFloat(Data);
			sv.h2_Effects[index].Missile.avelocity[0] = GetFloat(Data);
			sv.h2_Effects[index].Missile.avelocity[1] = GetFloat(Data);
			sv.h2_Effects[index].Missile.avelocity[2] = GetFloat(Data);
			break;

		case H2CE_CHUNK:
			sv.h2_Effects[index].Chunk.origin[0] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.origin[1] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.origin[2] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.type = GetInt(Data);
			sv.h2_Effects[index].Chunk.srcVel[0] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.srcVel[1] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.srcVel[2] = GetFloat(Data);
			sv.h2_Effects[index].Chunk.numChunks = GetInt(Data);
			break;

		default:
			PR_RunError("SV_SaveEffect: bad type");
			break;
		}
	}
	return Data;
}
