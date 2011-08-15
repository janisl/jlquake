
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

#define CE_NONE				0
#define CE_RAIN				1
#define CE_FOUNTAIN			2
#define CE_QUAKE			3
#define CE_WHITE_SMOKE		4   // whtsmk1.spr
#define CE_BLUESPARK		5	// bspark.spr
#define CE_YELLOWSPARK		6	// spark.spr
#define CE_SM_CIRCLE_EXP	7	// fcircle.spr
#define CE_BG_CIRCLE_EXP	8	// fcircle.spr
#define CE_SM_WHITE_FLASH	9	// sm_white.spr
#define CE_WHITE_FLASH		10	// gryspt.spr
#define CE_YELLOWRED_FLASH  11  // yr_flash.spr
#define CE_BLUE_FLASH       12  // bluflash.spr
#define CE_SM_BLUE_FLASH    13  // bluflash.spr
#define CE_RED_FLASH		14  // redspt.spr
#define CE_SM_EXPLOSION		15  // sm_expld.spr
#define CE_LG_EXPLOSION		16  // bg_expld.spr
#define CE_FLOOR_EXPLOSION	17  // fl_expld.spr
#define CE_RIDER_DEATH		18
#define CE_BLUE_EXPLOSION   19  // xpspblue.spr
#define CE_GREEN_SMOKE      20  // grnsmk1.spr
#define CE_GREY_SMOKE       21  // grysmk1.spr
#define CE_RED_SMOKE        22  // redsmk1.spr
#define CE_SLOW_WHITE_SMOKE 23  // whtsmk1.spr
#define CE_REDSPARK         24  // rspark.spr
#define CE_GREENSPARK       25  // gspark.spr
#define CE_TELESMK1         26  // telesmk1.spr
#define CE_TELESMK2         27  // telesmk2.spr
#define CE_ICEHIT           28  // icehit.spr
#define CE_MEDUSA_HIT       29  // medhit.spr
#define CE_MEZZO_REFLECT    30  // mezzoref.spr
#define CE_FLOOR_EXPLOSION2 31  // flrexpl2.spr
#define CE_XBOW_EXPLOSION   32  // xbowexpl.spr
#define CE_NEW_EXPLOSION    33  // gen_expl.spr
#define CE_MAGIC_MISSILE_EXPLOSION   34  // mm_expld.spr
#define CE_GHOST			35  // ghost.spr
#define CE_BONE_EXPLOSION	36
#define CE_REDCLOUD			37
#define CE_TELEPORTERPUFFS  38
#define CE_TELEPORTERBODY   39
#define CE_BONESHARD		40
#define CE_BONESHRAPNEL		41
#define CE_FLAMESTREAM		42	//Flamethrower
#define CE_SNOW				43
#define CE_GRAVITYWELL		44
#define CE_BLDRN_EXPL		45
#define CE_ACID_MUZZFL		46
#define CE_ACID_HIT			47
#define CE_FIREWALL_SMALL	48
#define CE_FIREWALL_MEDIUM	49
#define CE_FIREWALL_LARGE	50
#define CE_LBALL_EXPL		51
#define	CE_ACID_SPLAT		52
#define	CE_ACID_EXPL		53
#define	CE_FBOOM			54
#define CE_CHUNK			55
#define CE_BOMB				56
#define CE_BRN_BOUNCE		57
#define CE_LSHOCK			58
#define CE_FLAMEWALL		59
#define CE_FLAMEWALL2		60
#define CE_FLOOR_EXPLOSION3 61  
#define CE_ONFIRE			62

#define MAX_EFFECT_ENTITIES		256

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int NewEffectEntity(void);
static void FreeEffectEntity(int index);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern Cvar* sv_ce_scale;
extern Cvar* sv_ce_max_size;

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static entity_t EffectEntities[MAX_EFFECT_ENTITIES];
static qboolean EntityUsed[MAX_EFFECT_ENTITIES];
static int EffectEntityCount;

// CODE --------------------------------------------------------------------

//==========================================================================
//
// CL_InitTEnts
//
//==========================================================================

void CL_ClearEffects(void)
{
	Com_Memset(cl.Effects,0,sizeof(cl.Effects));
	Com_Memset(EntityUsed,0,sizeof(EntityUsed));
	EffectEntityCount = 0;
}

void SV_ClearEffects(void)
{
	Com_Memset(sv.Effects,0,sizeof(sv.Effects));
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_SendEffect(QMsg *sb, int index)
{
	qboolean	DoTest;
	vec3_t		TestO1,Diff;
	float		Size,TestDistance;
	int			i,count;

	if (sb == &sv.reliable_datagram && sv_ce_scale->value > 0)
		DoTest = true;
	else
		DoTest = false;

	VectorCopy(vec3_origin, TestO1);
	TestDistance = 0;

	switch(sv.Effects[index].type)
	{
		case CE_RAIN:
		case CE_SNOW:
			DoTest = false;
			break;

		case CE_FOUNTAIN:
			DoTest = false;
			break;

		case CE_QUAKE:
			VectorCopy(sv.Effects[index].Quake.origin, TestO1);
			TestDistance = 700;
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_FLAMESTREAM:
		case CE_ACID_MUZZFL:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_ACID_HIT:
		case CE_LBALL_EXPL:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_FBOOM:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_BOMB:
		case CE_FLOOR_EXPLOSION3:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CE_RIDER_DEATH:
			DoTest = false;
			break;

		case CE_GRAVITYWELL:
			DoTest = false;
			break;

		case CE_TELEPORTERPUFFS:
			VectorCopy(sv.Effects[index].Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case CE_TELEPORTERBODY:
			VectorCopy(sv.Effects[index].Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			VectorCopy(sv.Effects[index].Missile.origin, TestO1);
			TestDistance = 600;
			break;
		case CE_CHUNK:
			VectorCopy(sv.Effects[index].Chunk.origin, TestO1);
			TestDistance = 600;
			break;

		default:
//			Sys_Error ("SV_SendEffect: bad type");
			PR_RunError ("SV_SendEffect: bad type");
			break;
	}

	if (!DoTest) 
		count = 1;
	else 
	{
		count = svs.maxclients;
		TestDistance = (float)TestDistance * sv_ce_scale->value;
		TestDistance *= TestDistance;
	}

	
	for(i=0;i<count;i++)
	{
		if (DoTest)
		{	
			if (svs.clients[i].active)
			{
				sb = &svs.clients[i].datagram;
				VectorSubtract(svs.clients[i].edict->v.origin,TestO1,Diff);
				Size = (Diff[0]*Diff[0]) + (Diff[1]*Diff[1]) + (Diff[2]*Diff[2]);

				if (Size > TestDistance)
					continue;
				
				if (sv_ce_max_size->value > 0 && sb->cursize > sv_ce_max_size->value)
					continue;
			}
			else continue;
		}
		
		sb->WriteByte(svc_start_effect);
		sb->WriteByte(index);
		sb->WriteByte(sv.Effects[index].type);
		
		switch(sv.Effects[index].type)
		{
			case CE_RAIN:
				sb->WriteCoord(sv.Effects[index].Rain.min_org[0]);
				sb->WriteCoord(sv.Effects[index].Rain.min_org[1]);
				sb->WriteCoord(sv.Effects[index].Rain.min_org[2]);
				sb->WriteCoord(sv.Effects[index].Rain.max_org[0]);
				sb->WriteCoord(sv.Effects[index].Rain.max_org[1]);
				sb->WriteCoord(sv.Effects[index].Rain.max_org[2]);
				sb->WriteCoord(sv.Effects[index].Rain.e_size[0]);
				sb->WriteCoord(sv.Effects[index].Rain.e_size[1]);
				sb->WriteCoord(sv.Effects[index].Rain.e_size[2]);
				sb->WriteCoord(sv.Effects[index].Rain.dir[0]);
				sb->WriteCoord(sv.Effects[index].Rain.dir[1]);
				sb->WriteCoord(sv.Effects[index].Rain.dir[2]);
				sb->WriteShort(sv.Effects[index].Rain.color);
				sb->WriteShort(sv.Effects[index].Rain.count);
				sb->WriteFloat(sv.Effects[index].Rain.wait);
				break;
				
			case CE_SNOW:
				sb->WriteCoord(sv.Effects[index].Rain.min_org[0]);
				sb->WriteCoord(sv.Effects[index].Rain.min_org[1]);
				sb->WriteCoord(sv.Effects[index].Rain.min_org[2]);
				sb->WriteCoord(sv.Effects[index].Rain.max_org[0]);
				sb->WriteCoord(sv.Effects[index].Rain.max_org[1]);
				sb->WriteCoord(sv.Effects[index].Rain.max_org[2]);
				sb->WriteByte(sv.Effects[index].Rain.flags);
				sb->WriteCoord(sv.Effects[index].Rain.dir[0]);
				sb->WriteCoord(sv.Effects[index].Rain.dir[1]);
				sb->WriteCoord(sv.Effects[index].Rain.dir[2]);
				sb->WriteByte(sv.Effects[index].Rain.count);
				//sb->WriteShort(sv.Effects[index].Rain.veer);
				break;
				
			case CE_FOUNTAIN:
				sb->WriteCoord(sv.Effects[index].Fountain.pos[0]);
				sb->WriteCoord(sv.Effects[index].Fountain.pos[1]);
				sb->WriteCoord(sv.Effects[index].Fountain.pos[2]);
				sb->WriteAngle(sv.Effects[index].Fountain.angle[0]);
				sb->WriteAngle(sv.Effects[index].Fountain.angle[1]);
				sb->WriteAngle(sv.Effects[index].Fountain.angle[2]);
				sb->WriteCoord(sv.Effects[index].Fountain.movedir[0]);
				sb->WriteCoord(sv.Effects[index].Fountain.movedir[1]);
				sb->WriteCoord(sv.Effects[index].Fountain.movedir[2]);
				sb->WriteShort(sv.Effects[index].Fountain.color);
				sb->WriteByte(sv.Effects[index].Fountain.cnt);
				break;
				
			case CE_QUAKE:
				sb->WriteCoord(sv.Effects[index].Quake.origin[0]);
				sb->WriteCoord(sv.Effects[index].Quake.origin[1]);
				sb->WriteCoord(sv.Effects[index].Quake.origin[2]);
				sb->WriteFloat(sv.Effects[index].Quake.radius);
				break;
				
			case CE_WHITE_SMOKE:
			case CE_GREEN_SMOKE:
			case CE_GREY_SMOKE:
			case CE_RED_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
			case CE_TELESMK1:
			case CE_TELESMK2:
			case CE_GHOST:
			case CE_REDCLOUD:
			case CE_FLAMESTREAM:
			case CE_ACID_MUZZFL:
			case CE_FLAMEWALL:
			case CE_FLAMEWALL2:
			case CE_ONFIRE:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[0]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[1]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[2]);
				sb->WriteFloat(sv.Effects[index].Smoke.framelength);
				sb->WriteFloat(sv.Effects[index].Smoke.frame);
				break;
				
			case CE_SM_WHITE_FLASH:
			case CE_YELLOWRED_FLASH:
			case CE_BLUESPARK:
			case CE_YELLOWSPARK:
			case CE_SM_CIRCLE_EXP:
			case CE_BG_CIRCLE_EXP:
			case CE_SM_EXPLOSION:
			case CE_LG_EXPLOSION:
			case CE_FLOOR_EXPLOSION:
			case CE_FLOOR_EXPLOSION3:
			case CE_BLUE_EXPLOSION:
			case CE_REDSPARK:
			case CE_GREENSPARK:
			case CE_ICEHIT:
			case CE_MEDUSA_HIT:
			case CE_MEZZO_REFLECT:
			case CE_FLOOR_EXPLOSION2:
			case CE_XBOW_EXPLOSION:
			case CE_NEW_EXPLOSION:
			case CE_MAGIC_MISSILE_EXPLOSION:
			case CE_BONE_EXPLOSION:
			case CE_BLDRN_EXPL:
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:	
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:
			case CE_FBOOM:
			case CE_BOMB:
			case CE_BRN_BOUNCE:
			case CE_LSHOCK:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				break;
				
			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_RED_FLASH:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				break;
								
			case CE_RIDER_DEATH:
				sb->WriteCoord(sv.Effects[index].RD.origin[0]);
				sb->WriteCoord(sv.Effects[index].RD.origin[1]);
				sb->WriteCoord(sv.Effects[index].RD.origin[2]);
				break;
				
			case CE_TELEPORTERPUFFS:
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[0]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[1]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[2]);
				break;
				
			case CE_TELEPORTERBODY:
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[0]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[1]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[2]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][0]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][1]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][2]);
				sb->WriteFloat(sv.Effects[index].Teleporter.skinnum);
				break;

			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
				sb->WriteCoord(sv.Effects[index].Missile.origin[0]);
				sb->WriteCoord(sv.Effects[index].Missile.origin[1]);
				sb->WriteCoord(sv.Effects[index].Missile.origin[2]);
				sb->WriteFloat(sv.Effects[index].Missile.velocity[0]);
				sb->WriteFloat(sv.Effects[index].Missile.velocity[1]);
				sb->WriteFloat(sv.Effects[index].Missile.velocity[2]);
				sb->WriteFloat(sv.Effects[index].Missile.angle[0]);
				sb->WriteFloat(sv.Effects[index].Missile.angle[1]);
				sb->WriteFloat(sv.Effects[index].Missile.angle[2]);
				sb->WriteFloat(sv.Effects[index].Missile.avelocity[0]);
				sb->WriteFloat(sv.Effects[index].Missile.avelocity[1]);
				sb->WriteFloat(sv.Effects[index].Missile.avelocity[2]);
				
				break;

			case CE_GRAVITYWELL:
				sb->WriteCoord(sv.Effects[index].RD.origin[0]);
				sb->WriteCoord(sv.Effects[index].RD.origin[1]);
				sb->WriteCoord(sv.Effects[index].RD.origin[2]);
				sb->WriteShort(sv.Effects[index].RD.color);
				sb->WriteFloat(sv.Effects[index].RD.lifetime);
				break;

			case CE_CHUNK:
				sb->WriteCoord(sv.Effects[index].Chunk.origin[0]);
				sb->WriteCoord(sv.Effects[index].Chunk.origin[1]);
				sb->WriteCoord(sv.Effects[index].Chunk.origin[2]);
				sb->WriteByte(sv.Effects[index].Chunk.type);
				sb->WriteCoord(sv.Effects[index].Chunk.srcVel[0]);
				sb->WriteCoord(sv.Effects[index].Chunk.srcVel[1]);
				sb->WriteCoord(sv.Effects[index].Chunk.srcVel[2]);
				sb->WriteByte(sv.Effects[index].Chunk.numChunks);

				//Con_Printf("Adding %d chunks on server...\n",sv.Effects[index].Chunk.numChunks);
				break;

			default:
	//			Sys_Error ("SV_SendEffect: bad type");
				PR_RunError ("SV_SendEffect: bad type");
				break;
		}
	}
}

void SV_UpdateEffects(QMsg *sb)
{
	int index;

	for(index=0;index<MAX_EFFECTS;index++)
		if (sv.Effects[index].type)
			SV_SendEffect(sb,index);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_ParseEffect(QMsg *sb)
{
	int index;
	byte effect;

	effect = G_FLOAT(OFS_PARM0);

	for(index=0;index<MAX_EFFECTS;index++)
		if (!sv.Effects[index].type || 
			(sv.Effects[index].expire_time && sv.Effects[index].expire_time <= sv.time)) 
			break;
		
	if (index >= MAX_EFFECTS)
	{
		PR_RunError ("MAX_EFFECTS reached");
		return;
	}

//	Con_Printf("Effect #%d\n",index);

	Com_Memset(&sv.Effects[index],0,sizeof(struct EffectT));

	sv.Effects[index].type = effect;
	G_FLOAT(OFS_RETURN) = index;

	switch(effect)
	{
		case CE_RAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Rain.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Rain.max_org);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Rain.e_size);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].Rain.dir);
			sv.Effects[index].Rain.color = G_FLOAT(OFS_PARM5);
			sv.Effects[index].Rain.count = G_FLOAT(OFS_PARM6);
			sv.Effects[index].Rain.wait = G_FLOAT(OFS_PARM7);

			sv.Effects[index].Rain.next_time = 0;
			break;

		case CE_SNOW:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Rain.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Rain.max_org);
			sv.Effects[index].Rain.flags = G_FLOAT(OFS_PARM3);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].Rain.dir);
			sv.Effects[index].Rain.count = G_FLOAT(OFS_PARM5);
			//sv.Effects[index].Rain.veer = G_FLOAT(OFS_PARM6);
			//sv.Effects[index].Rain.wait = 0.10;

			sv.Effects[index].Rain.next_time = 0;
			break;

		case CE_FOUNTAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Fountain.pos);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Fountain.angle);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Fountain.movedir);
			sv.Effects[index].Fountain.color = G_FLOAT(OFS_PARM4);
			sv.Effects[index].Fountain.cnt = G_FLOAT(OFS_PARM5);
			break;

		case CE_QUAKE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Quake.origin);
			sv.Effects[index].Quake.radius = G_FLOAT(OFS_PARM2);
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].Smoke.velocity);
			sv.Effects[index].Smoke.framelength = G_FLOAT(OFS_PARM3);
			sv.Effects[index].Smoke.frame = 0;
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].Smoke.velocity);
			sv.Effects[index].Smoke.framelength = 0.05;
			sv.Effects[index].Smoke.frame = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_FLOOR_EXPLOSION3:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Flash.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_RIDER_DEATH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].RD.origin);
			break;

		case CE_GRAVITYWELL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].RD.origin);
			sv.Effects[index].RD.color = G_FLOAT(OFS_PARM2);
			sv.Effects[index].RD.lifetime = G_FLOAT(OFS_PARM3);
			break;

		case CE_TELEPORTERPUFFS:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Teleporter.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_TELEPORTERBODY:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Teleporter.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Teleporter.velocity[0]);
			sv.Effects[index].Teleporter.skinnum = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Missile.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Missile.velocity);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Missile.angle);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Missile.avelocity);

			sv.Effects[index].expire_time = sv.time + 10;
			break;

		case CE_CHUNK:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Chunk.origin);
			sv.Effects[index].Chunk.type = G_FLOAT(OFS_PARM2);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Chunk.srcVel);
			sv.Effects[index].Chunk.numChunks = G_FLOAT(OFS_PARM4);

			sv.Effects[index].expire_time = sv.time + 3;
			break;


		default:
//			Sys_Error ("SV_ParseEffect: bad type");
			PR_RunError ("SV_SendEffect: bad type");
	}

	SV_SendEffect(sb,index);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void SV_SaveEffects(fileHandle_t FH)
{
	int index,count;

	for(index=count=0;index<MAX_EFFECTS;index++)
		if (sv.Effects[index].type)
			count++;

	FS_Printf(FH,"Effects: %d\n",count);

	for(index=count=0;index<MAX_EFFECTS;index++)
		if (sv.Effects[index].type)
		{
			FS_Printf(FH,"Effect: %d %d %f: ",index,sv.Effects[index].type,sv.Effects[index].expire_time);

			switch(sv.Effects[index].type)
			{
				case CE_RAIN:
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.min_org[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.min_org[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.min_org[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.max_org[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.max_org[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.max_org[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.e_size[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.e_size[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.e_size[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.dir[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.dir[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.dir[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Rain.color);
					FS_Printf(FH, "%d ", sv.Effects[index].Rain.count);
					FS_Printf(FH, "%f\n", sv.Effects[index].Rain.wait);
					break;

				case CE_SNOW:
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.min_org[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.min_org[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.min_org[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.max_org[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.max_org[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.max_org[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Rain.flags);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.dir[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.dir[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Rain.dir[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Rain.count);
					//FS_Printf(FH, "%d ", sv.Effects[index].Rain.veer);
					break;

				case CE_FOUNTAIN:
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.pos[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.pos[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.pos[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.angle[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.angle[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.angle[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.movedir[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.movedir[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Fountain.movedir[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Fountain.color);
					FS_Printf(FH, "%d\n", sv.Effects[index].Fountain.cnt);
					break;

				case CE_QUAKE:
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[2]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Quake.radius);
					break;

				case CE_WHITE_SMOKE:
				case CE_GREEN_SMOKE:
				case CE_GREY_SMOKE:
				case CE_RED_SMOKE:
				case CE_SLOW_WHITE_SMOKE:
				case CE_TELESMK1:
				case CE_TELESMK2:
				case CE_GHOST:
				case CE_REDCLOUD:
				case CE_ACID_MUZZFL:
				case CE_FLAMESTREAM:
				case CE_FLAMEWALL:
				case CE_FLAMEWALL2:
				case CE_ONFIRE:
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.framelength);
					FS_Printf(FH, "%f\n", sv.Effects[index].Smoke.frame);
					break;

				case CE_SM_WHITE_FLASH:
				case CE_YELLOWRED_FLASH:
				case CE_BLUESPARK:
				case CE_YELLOWSPARK:
				case CE_SM_CIRCLE_EXP:
				case CE_BG_CIRCLE_EXP:
				case CE_SM_EXPLOSION:
				case CE_LG_EXPLOSION:
				case CE_FLOOR_EXPLOSION:
				case CE_FLOOR_EXPLOSION3:
				case CE_BLUE_EXPLOSION:
				case CE_REDSPARK:
				case CE_GREENSPARK:
				case CE_ICEHIT:
				case CE_MEDUSA_HIT:
				case CE_MEZZO_REFLECT:
				case CE_FLOOR_EXPLOSION2:
				case CE_XBOW_EXPLOSION:
				case CE_NEW_EXPLOSION:
				case CE_MAGIC_MISSILE_EXPLOSION:
				case CE_BONE_EXPLOSION:
				case CE_BLDRN_EXPL:
				case CE_BRN_BOUNCE:
				case CE_LSHOCK:
				case CE_ACID_HIT:
				case CE_ACID_SPLAT:
				case CE_ACID_EXPL:
				case CE_LBALL_EXPL:
				case CE_FIREWALL_SMALL:
				case CE_FIREWALL_MEDIUM:
				case CE_FIREWALL_LARGE:
				case CE_FBOOM:
				case CE_BOMB:
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Smoke.origin[2]);
					break;

				case CE_WHITE_FLASH:
				case CE_BLUE_FLASH:
				case CE_SM_BLUE_FLASH:
				case CE_RED_FLASH:
					FS_Printf(FH, "%f ", sv.Effects[index].Flash.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Flash.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Flash.origin[2]);
					break;

				case CE_RIDER_DEATH:
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].RD.origin[2]);
					break;

				case CE_GRAVITYWELL:
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[1]);
					FS_Printf(FH, "%f", sv.Effects[index].RD.origin[2]);
					FS_Printf(FH, "%d", sv.Effects[index].RD.color);
					FS_Printf(FH, "%f\n", sv.Effects[index].RD.lifetime);
					break;
				case CE_TELEPORTERPUFFS:
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Teleporter.origin[2]);
					break;

				case CE_TELEPORTERBODY:
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Teleporter.origin[2]);
					break;

				case CE_BONESHARD:
				case CE_BONESHRAPNEL:
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.origin[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.velocity[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.velocity[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.velocity[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.angle[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.angle[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.angle[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.avelocity[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.avelocity[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Missile.avelocity[2]);
					break;

				case CE_CHUNK:
					FS_Printf(FH, "%f ", sv.Effects[index].Chunk.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Chunk.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Chunk.origin[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Chunk.type);
					FS_Printf(FH, "%f ", sv.Effects[index].Chunk.srcVel[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Chunk.srcVel[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Chunk.srcVel[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Chunk.numChunks);
					break;

				default:
					PR_RunError ("SV_SaveEffect: bad type");
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
			Sys_Error("Number too long %s", Tmp);
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
			Sys_Error("Number too long %s", Tmp);
		}
		Tmp[Len] = *Data++;
		Len++;
	}
	Tmp[Len] = 0;
	return String::Atof(Tmp);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
char* SV_LoadEffects(char* Data)
{
	int index,Total,count;

	// Since the map is freshly loaded, clear out any effects as a result of
	// the loading
	SV_ClearEffects();

	if (String::NCmp(Data, "Effects: ", 9))
		throw DropException("Effects expected");
	Data += 9;
	Total = GetInt(Data);

	for(count=0;count<Total;count++)
	{
		//	Skip whitespace.
		while (*Data && *Data <= ' ')
			Data++;
		if (String::NCmp(Data, "Effect: ", 8))
			Sys_Error("Effect expected");
		Data += 8;
		index = GetInt(Data);
		sv.Effects[index].type = GetInt(Data);
		sv.Effects[index].expire_time = GetFloat(Data);
		if (*Data != ':')
			Sys_Error("Colon expected");
		Data++;

		switch(sv.Effects[index].type)
		{
			case CE_RAIN:
				sv.Effects[index].Rain.min_org[0] = GetFloat(Data);
				sv.Effects[index].Rain.min_org[1] = GetFloat(Data);
				sv.Effects[index].Rain.min_org[2] = GetFloat(Data);
				sv.Effects[index].Rain.max_org[0] = GetFloat(Data);
				sv.Effects[index].Rain.max_org[1] = GetFloat(Data);
				sv.Effects[index].Rain.max_org[2] = GetFloat(Data);
				sv.Effects[index].Rain.e_size[0] = GetFloat(Data);
				sv.Effects[index].Rain.e_size[1] = GetFloat(Data);
				sv.Effects[index].Rain.e_size[2] = GetFloat(Data);
				sv.Effects[index].Rain.dir[0] = GetFloat(Data);
				sv.Effects[index].Rain.dir[1] = GetFloat(Data);
				sv.Effects[index].Rain.dir[2] = GetFloat(Data);
				sv.Effects[index].Rain.color = GetInt(Data);
				sv.Effects[index].Rain.count = GetInt(Data);
				sv.Effects[index].Rain.wait = GetFloat(Data);
				break;

			case CE_SNOW:
				sv.Effects[index].Rain.min_org[0] = GetFloat(Data);
				sv.Effects[index].Rain.min_org[1] = GetFloat(Data);
				sv.Effects[index].Rain.min_org[2] = GetFloat(Data);
				sv.Effects[index].Rain.max_org[0] = GetFloat(Data);
				sv.Effects[index].Rain.max_org[1] = GetFloat(Data);
				sv.Effects[index].Rain.max_org[2] = GetFloat(Data);
				sv.Effects[index].Rain.flags = GetInt(Data);
				sv.Effects[index].Rain.dir[0] = GetFloat(Data);
				sv.Effects[index].Rain.dir[1] = GetFloat(Data);
				sv.Effects[index].Rain.dir[2] = GetFloat(Data);
				sv.Effects[index].Rain.count = GetInt(Data);
				break;

			case CE_FOUNTAIN:
				sv.Effects[index].Fountain.pos[0] = GetFloat(Data);
				sv.Effects[index].Fountain.pos[1] = GetFloat(Data);
				sv.Effects[index].Fountain.pos[2] = GetFloat(Data);
				sv.Effects[index].Fountain.angle[0] = GetFloat(Data);
				sv.Effects[index].Fountain.angle[1] = GetFloat(Data);
				sv.Effects[index].Fountain.angle[2] = GetFloat(Data);
				sv.Effects[index].Fountain.movedir[0] = GetFloat(Data);
				sv.Effects[index].Fountain.movedir[1] = GetFloat(Data);
				sv.Effects[index].Fountain.movedir[2] = GetFloat(Data);
				sv.Effects[index].Fountain.color = GetInt(Data);
				sv.Effects[index].Fountain.cnt = GetInt(Data);
				break;

			case CE_QUAKE:
				sv.Effects[index].Quake.origin[0] = GetFloat(Data);
				sv.Effects[index].Quake.origin[1] = GetFloat(Data);
				sv.Effects[index].Quake.origin[2] = GetFloat(Data);
				sv.Effects[index].Quake.radius = GetFloat(Data);
				break;

			case CE_WHITE_SMOKE:
			case CE_GREEN_SMOKE:
			case CE_GREY_SMOKE:
			case CE_RED_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
			case CE_TELESMK1:
			case CE_TELESMK2:
			case CE_GHOST:
			case CE_REDCLOUD:
			case CE_ACID_MUZZFL:
			case CE_FLAMESTREAM:
			case CE_FLAMEWALL:
			case CE_FLAMEWALL2:
			case CE_ONFIRE:
				sv.Effects[index].Smoke.origin[0] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[1] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[2] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[0] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[1] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[2] = GetFloat(Data);
				sv.Effects[index].Smoke.framelength = GetFloat(Data);
				sv.Effects[index].Smoke.frame = GetFloat(Data);
				break;

			case CE_SM_WHITE_FLASH:
			case CE_YELLOWRED_FLASH:
			case CE_BLUESPARK:
			case CE_YELLOWSPARK:
			case CE_SM_CIRCLE_EXP:
			case CE_BG_CIRCLE_EXP:
			case CE_SM_EXPLOSION:
			case CE_LG_EXPLOSION:
			case CE_FLOOR_EXPLOSION:
			case CE_FLOOR_EXPLOSION3:
			case CE_BLUE_EXPLOSION:
			case CE_REDSPARK:
			case CE_GREENSPARK:
			case CE_ICEHIT:
			case CE_MEDUSA_HIT:
			case CE_MEZZO_REFLECT:
			case CE_FLOOR_EXPLOSION2:
			case CE_XBOW_EXPLOSION:
			case CE_NEW_EXPLOSION:
			case CE_MAGIC_MISSILE_EXPLOSION:
			case CE_BONE_EXPLOSION:
			case CE_BLDRN_EXPL:
			case CE_BRN_BOUNCE:
			case CE_LSHOCK:
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:
			case CE_FBOOM:
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:
			case CE_BOMB:
				sv.Effects[index].Smoke.origin[0] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[1] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[2] = GetFloat(Data);
				break;

			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_RED_FLASH:
				sv.Effects[index].Flash.origin[0] = GetFloat(Data);
				sv.Effects[index].Flash.origin[1] = GetFloat(Data);
				sv.Effects[index].Flash.origin[2] = GetFloat(Data);
				break;

			case CE_RIDER_DEATH:
				sv.Effects[index].RD.origin[0] = GetFloat(Data);
				sv.Effects[index].RD.origin[1] = GetFloat(Data);
				sv.Effects[index].RD.origin[2] = GetFloat(Data);
				break;

			case CE_GRAVITYWELL:
				sv.Effects[index].RD.origin[0] = GetFloat(Data);
				sv.Effects[index].RD.origin[1] = GetFloat(Data);
				sv.Effects[index].RD.origin[2] = GetFloat(Data);
				sv.Effects[index].RD.color = GetInt(Data);
				sv.Effects[index].RD.lifetime = GetFloat(Data);
				break;

			case CE_TELEPORTERPUFFS:
				sv.Effects[index].Teleporter.origin[0] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[1] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[2] = GetFloat(Data);
				break;

			case CE_TELEPORTERBODY:
				sv.Effects[index].Teleporter.origin[0] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[1] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[2] = GetFloat(Data);
				break;

			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
				sv.Effects[index].Missile.origin[0] = GetFloat(Data);
				sv.Effects[index].Missile.origin[1] = GetFloat(Data);
				sv.Effects[index].Missile.origin[2] = GetFloat(Data);
				sv.Effects[index].Missile.velocity[0] = GetFloat(Data);
				sv.Effects[index].Missile.velocity[1] = GetFloat(Data);
				sv.Effects[index].Missile.velocity[2] = GetFloat(Data);
				sv.Effects[index].Missile.angle[0] = GetFloat(Data);
				sv.Effects[index].Missile.angle[1] = GetFloat(Data);
				sv.Effects[index].Missile.angle[2] = GetFloat(Data);
				sv.Effects[index].Missile.avelocity[0] = GetFloat(Data);
				sv.Effects[index].Missile.avelocity[1] = GetFloat(Data);
				sv.Effects[index].Missile.avelocity[2] = GetFloat(Data);
				break;

			case CE_CHUNK:
				sv.Effects[index].Chunk.origin[0] = GetFloat(Data);
				sv.Effects[index].Chunk.origin[1] = GetFloat(Data);
				sv.Effects[index].Chunk.origin[2] = GetFloat(Data);
				sv.Effects[index].Chunk.type = GetInt(Data);
				sv.Effects[index].Chunk.srcVel[0] = GetFloat(Data);
				sv.Effects[index].Chunk.srcVel[1] = GetFloat(Data);
				sv.Effects[index].Chunk.srcVel[2] = GetFloat(Data);
				sv.Effects[index].Chunk.numChunks = GetInt(Data);
				break;

			default:
				PR_RunError ("SV_SaveEffect: bad type");
				break;
		}
	}
	return Data;
}

void CL_FreeEffect(int index)
{	
	int i;

	switch(cl.Effects[index].type)
	{
		case CE_RAIN:
			break;

		case CE_SNOW:
			break;

		case CE_FOUNTAIN:
			break;

		case CE_QUAKE:
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			FreeEffectEntity(cl.Effects[index].Smoke.entity_index);
			break;

		// Just go through animation and then remove
		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_FLOOR_EXPLOSION3:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:

			FreeEffectEntity(cl.Effects[index].Smoke.entity_index);
			break;

		// Go forward then backward through animation then remove
		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			FreeEffectEntity(cl.Effects[index].Flash.entity_index);
			break;

		case CE_RIDER_DEATH:
			break;

		case CE_GRAVITYWELL:
			break;

		case CE_TELEPORTERPUFFS:
			for (i=0;i<8;++i)
				FreeEffectEntity(cl.Effects[index].Teleporter.entity_index[i]);
			break;

		case CE_TELEPORTERBODY:
			FreeEffectEntity(cl.Effects[index].Teleporter.entity_index[0]);
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			FreeEffectEntity(cl.Effects[index].Missile.entity_index);
			break;
		case CE_CHUNK:
			//Con_Printf("Freeing a chunk here\n");
			for (i=0;i < cl.Effects[index].Chunk.numChunks;i++)
			{
				if(cl.Effects[index].Chunk.entity_index[i] != -1)
				{
					FreeEffectEntity(cl.Effects[index].Chunk.entity_index[i]);
				}
			}
			break;

	}

	Com_Memset(&cl.Effects[index],0,sizeof(struct EffectT));
}

//==========================================================================
//
// CL_ParseEffect
//
//==========================================================================

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
void CL_ParseEffect(void)
{
	int index,i;
	qboolean ImmediateFree;
	entity_t *ent;
	int dir;
	float	angleval, sinval, cosval;
	float skinnum;
	float final;

	ImmediateFree = false;

	index = net_message.ReadByte();
	if (cl.Effects[index].type)
		CL_FreeEffect(index);

	Com_Memset(&cl.Effects[index],0,sizeof(struct EffectT));

	cl.Effects[index].type = net_message.ReadByte();

	switch(cl.Effects[index].type)
	{
		case CE_RAIN:
			cl.Effects[index].Rain.min_org[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.min_org[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.min_org[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.e_size[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.e_size[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.e_size[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.color = net_message.ReadShort();
			cl.Effects[index].Rain.count = net_message.ReadShort();
			cl.Effects[index].Rain.wait = net_message.ReadFloat();
			break;

		case CE_SNOW:
			cl.Effects[index].Rain.min_org[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.min_org[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.min_org[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.max_org[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.flags = net_message.ReadByte();
			cl.Effects[index].Rain.dir[0] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[1] = net_message.ReadCoord();
			cl.Effects[index].Rain.dir[2] = net_message.ReadCoord();
			cl.Effects[index].Rain.count = net_message.ReadByte();
			//cl.Effects[index].Rain.veer = net_message.ReadShort();
			break;

		case CE_FOUNTAIN:
			cl.Effects[index].Fountain.pos[0] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.pos[1] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.pos[2] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.angle[0] = net_message.ReadAngle ();
			cl.Effects[index].Fountain.angle[1] = net_message.ReadAngle ();
			cl.Effects[index].Fountain.angle[2] = net_message.ReadAngle ();
			cl.Effects[index].Fountain.movedir[0] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.movedir[1] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.movedir[2] = net_message.ReadCoord ();
			cl.Effects[index].Fountain.color = net_message.ReadShort ();
			cl.Effects[index].Fountain.cnt = net_message.ReadByte ();
			AngleVectors (cl.Effects[index].Fountain.angle, 
						  cl.Effects[index].Fountain.vforward,
						  cl.Effects[index].Fountain.vright,
						  cl.Effects[index].Fountain.vup);
			break;

		case CE_QUAKE:
			cl.Effects[index].Quake.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Quake.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Quake.origin[2] = net_message.ReadCoord ();
			cl.Effects[index].Quake.radius = net_message.ReadFloat ();
			break;

		case CE_WHITE_SMOKE:
		case CE_GREEN_SMOKE:
		case CE_GREY_SMOKE:
		case CE_RED_SMOKE:
		case CE_SLOW_WHITE_SMOKE:
		case CE_TELESMK1:
		case CE_TELESMK2:
		case CE_GHOST:
		case CE_REDCLOUD:
		case CE_ACID_MUZZFL:
		case CE_FLAMESTREAM:
		case CE_FLAMEWALL:
		case CE_FLAMEWALL2:
		case CE_ONFIRE:
			cl.Effects[index].Smoke.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Smoke.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Smoke.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Smoke.velocity[2] = net_message.ReadFloat ();

			cl.Effects[index].Smoke.framelength = net_message.ReadFloat ();
			cl.Effects[index].Smoke.frame = net_message.ReadFloat ();

			if ((cl.Effects[index].Smoke.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];
				VectorCopy(cl.Effects[index].Smoke.origin, ent->origin);

				if ((cl.Effects[index].type == CE_WHITE_SMOKE) || 
					(cl.Effects[index].type == CE_SLOW_WHITE_SMOKE))
					ent->model = R_RegisterModel("models/whtsmk1.spr");
				else if (cl.Effects[index].type == CE_GREEN_SMOKE)
					ent->model = R_RegisterModel("models/grnsmk1.spr");
				else if (cl.Effects[index].type == CE_GREY_SMOKE)
					ent->model = R_RegisterModel("models/grysmk1.spr");
				else if (cl.Effects[index].type == CE_RED_SMOKE)
					ent->model = R_RegisterModel("models/redsmk1.spr");
				else if (cl.Effects[index].type == CE_TELESMK1)
					ent->model = R_RegisterModel("models/telesmk1.spr");
				else if (cl.Effects[index].type == CE_TELESMK2)
					ent->model = R_RegisterModel("models/telesmk2.spr");
				else if (cl.Effects[index].type == CE_REDCLOUD)
					ent->model = R_RegisterModel("models/rcloud.spr");
				else if (cl.Effects[index].type == CE_FLAMESTREAM)
					ent->model = R_RegisterModel("models/flamestr.spr");
				else if (cl.Effects[index].type == CE_ACID_MUZZFL)
				{
					ent->model = R_RegisterModel("models/muzzle1.spr");
					ent->drawflags=DRF_TRANSLUCENT|MLS_ABSLIGHT;
					ent->abslight=0.2;
				}
				else if (cl.Effects[index].type == CE_FLAMEWALL)
					ent->model = R_RegisterModel("models/firewal1.spr");
				else if (cl.Effects[index].type == CE_FLAMEWALL2)
					ent->model = R_RegisterModel("models/firewal2.spr");
				else if (cl.Effects[index].type == CE_ONFIRE)
				{
					float rdm = rand() & 3;

					if (rdm < 1)
						ent->model = R_RegisterModel("models/firewal1.spr");
					else if (rdm < 2)
						ent->model = R_RegisterModel("models/firewal2.spr");
					else
						ent->model = R_RegisterModel("models/firewal3.spr");
					
					ent->drawflags = DRF_TRANSLUCENT;
					ent->abslight = 1;
					ent->frame = cl.Effects[index].Smoke.frame;
				}

				if (cl.Effects[index].type != CE_REDCLOUD&&cl.Effects[index].type != CE_ACID_MUZZFL&&cl.Effects[index].type != CE_FLAMEWALL)
					ent->drawflags = DRF_TRANSLUCENT;

				if (cl.Effects[index].type == CE_FLAMESTREAM)
				{
					ent->drawflags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
					ent->abslight = 1;
					ent->frame = cl.Effects[index].Smoke.frame;
				}

				if (cl.Effects[index].type == CE_GHOST)
				{
					ent->model = R_RegisterModel("models/ghost.spr");
					ent->drawflags = DRF_TRANSLUCENT | MLS_ABSLIGHT;
					ent->abslight = .5;
				}
			}
			else
				ImmediateFree = true;
			break;

		case CE_SM_WHITE_FLASH:
		case CE_YELLOWRED_FLASH:
		case CE_BLUESPARK:
		case CE_YELLOWSPARK:
		case CE_SM_CIRCLE_EXP:
		case CE_BG_CIRCLE_EXP:
		case CE_SM_EXPLOSION:
		case CE_LG_EXPLOSION:
		case CE_FLOOR_EXPLOSION:
		case CE_FLOOR_EXPLOSION3:
		case CE_BLUE_EXPLOSION:
		case CE_REDSPARK:
		case CE_GREENSPARK:
		case CE_ICEHIT:
		case CE_MEDUSA_HIT:
		case CE_MEZZO_REFLECT:
		case CE_FLOOR_EXPLOSION2:
		case CE_XBOW_EXPLOSION:
		case CE_NEW_EXPLOSION:
		case CE_MAGIC_MISSILE_EXPLOSION:
		case CE_BONE_EXPLOSION:
		case CE_BLDRN_EXPL:
		case CE_BRN_BOUNCE:
		case CE_LSHOCK:
		case CE_ACID_HIT:
		case CE_ACID_SPLAT:
		case CE_ACID_EXPL:
		case CE_LBALL_EXPL:
		case CE_FBOOM:
		case CE_BOMB:
		case CE_FIREWALL_SMALL:
		case CE_FIREWALL_MEDIUM:
		case CE_FIREWALL_LARGE:
			cl.Effects[index].Smoke.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Smoke.origin[2] = net_message.ReadCoord ();
			if ((cl.Effects[index].Smoke.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];
				VectorCopy(cl.Effects[index].Smoke.origin, ent->origin);

				if (cl.Effects[index].type == CE_BLUESPARK)
					ent->model = R_RegisterModel("models/bspark.spr");
				else if (cl.Effects[index].type == CE_YELLOWSPARK)
					ent->model = R_RegisterModel("models/spark.spr");
				else if (cl.Effects[index].type == CE_SM_CIRCLE_EXP)
					ent->model = R_RegisterModel("models/fcircle.spr");
				else if (cl.Effects[index].type == CE_BG_CIRCLE_EXP)
					ent->model = R_RegisterModel("models/xplod29.spr");
				else if (cl.Effects[index].type == CE_SM_WHITE_FLASH)
					ent->model = R_RegisterModel("models/sm_white.spr");
				else if (cl.Effects[index].type == CE_YELLOWRED_FLASH)
				{
					ent->model = R_RegisterModel("models/yr_flsh.spr");
					ent->drawflags = DRF_TRANSLUCENT;
				}
				else if (cl.Effects[index].type == CE_SM_EXPLOSION)
					ent->model = R_RegisterModel("models/sm_expld.spr");
				else if (cl.Effects[index].type == CE_LG_EXPLOSION)
					ent->model = R_RegisterModel("models/bg_expld.spr");
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION)
					ent->model = R_RegisterModel("models/fl_expld.spr");
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION3)
					ent->model = R_RegisterModel("models/biggy.spr");
				else if (cl.Effects[index].type == CE_BLUE_EXPLOSION)
					ent->model = R_RegisterModel("models/xpspblue.spr");
				else if (cl.Effects[index].type == CE_REDSPARK)
					ent->model = R_RegisterModel("models/rspark.spr");
				else if (cl.Effects[index].type == CE_GREENSPARK)
					ent->model = R_RegisterModel("models/gspark.spr");
				else if (cl.Effects[index].type == CE_ICEHIT)
					ent->model = R_RegisterModel("models/icehit.spr");
				else if (cl.Effects[index].type == CE_MEDUSA_HIT)
					ent->model = R_RegisterModel("models/medhit.spr");
				else if (cl.Effects[index].type == CE_MEZZO_REFLECT)
					ent->model = R_RegisterModel("models/mezzoref.spr");
				else if (cl.Effects[index].type == CE_FLOOR_EXPLOSION2)
					ent->model = R_RegisterModel("models/flrexpl2.spr");
				else if (cl.Effects[index].type == CE_XBOW_EXPLOSION)
					ent->model = R_RegisterModel("models/xbowexpl.spr");
				else if (cl.Effects[index].type == CE_NEW_EXPLOSION)
					ent->model = R_RegisterModel("models/gen_expl.spr");
				else if (cl.Effects[index].type == CE_MAGIC_MISSILE_EXPLOSION)
					ent->model = R_RegisterModel("models/mm_expld.spr");
				else if (cl.Effects[index].type == CE_BONE_EXPLOSION)
					ent->model = R_RegisterModel("models/bonexpld.spr");
				else if (cl.Effects[index].type == CE_BLDRN_EXPL)
					ent->model = R_RegisterModel("models/xplsn_1.spr");
				else if (cl.Effects[index].type == CE_ACID_HIT)
					ent->model = R_RegisterModel("models/axplsn_2.spr");
				else if (cl.Effects[index].type == CE_ACID_SPLAT)
					ent->model = R_RegisterModel("models/axplsn_1.spr");
				else if (cl.Effects[index].type == CE_ACID_EXPL)
				{
					ent->model = R_RegisterModel("models/axplsn_5.spr");
					ent->drawflags = MLS_ABSLIGHT;
					ent->abslight = 1;
				}
				else if (cl.Effects[index].type == CE_FBOOM)
					ent->model = R_RegisterModel("models/fboom.spr");
				else if (cl.Effects[index].type == CE_BOMB)
					ent->model = R_RegisterModel("models/pow.spr");
				else if (cl.Effects[index].type == CE_LBALL_EXPL)
					ent->model = R_RegisterModel("models/Bluexp3.spr");
				else if (cl.Effects[index].type == CE_FIREWALL_SMALL)
					ent->model = R_RegisterModel("models/firewal1.spr");
				else if (cl.Effects[index].type == CE_FIREWALL_MEDIUM)
					ent->model = R_RegisterModel("models/firewal5.spr");
				else if (cl.Effects[index].type == CE_FIREWALL_LARGE)
					ent->model = R_RegisterModel("models/firewal4.spr");
				else if (cl.Effects[index].type == CE_BRN_BOUNCE)
					ent->model = R_RegisterModel("models/spark.spr");
				else if (cl.Effects[index].type == CE_LSHOCK)
				{
					ent->model = R_RegisterModel("models/vorpshok.mdl");
					ent->drawflags=MLS_TORCH;
					ent->angles[2]=90;
					ent->scale=255;
				}
			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CE_WHITE_FLASH:
		case CE_BLUE_FLASH:
		case CE_SM_BLUE_FLASH:
		case CE_RED_FLASH:
			cl.Effects[index].Flash.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Flash.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Flash.origin[2] = net_message.ReadCoord ();
			cl.Effects[index].Flash.reverse = 0;
			if ((cl.Effects[index].Flash.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Flash.entity_index];
				VectorCopy(cl.Effects[index].Flash.origin, ent->origin);

				if (cl.Effects[index].type == CE_WHITE_FLASH)
					ent->model = R_RegisterModel("models/gryspt.spr");
				else if (cl.Effects[index].type == CE_BLUE_FLASH)
					ent->model = R_RegisterModel("models/bluflash.spr");
				else if (cl.Effects[index].type == CE_SM_BLUE_FLASH)
					ent->model = R_RegisterModel("models/sm_blue.spr");
				else if (cl.Effects[index].type == CE_RED_FLASH)
					ent->model = R_RegisterModel("models/redspt.spr");

				ent->drawflags = DRF_TRANSLUCENT;

			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CE_RIDER_DEATH:
			cl.Effects[index].RD.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].RD.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].RD.origin[2] = net_message.ReadCoord ();
			break;

		case CE_GRAVITYWELL:
			cl.Effects[index].RD.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].RD.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].RD.origin[2] = net_message.ReadCoord ();
			cl.Effects[index].RD.color = net_message.ReadShort ();
			cl.Effects[index].RD.lifetime = net_message.ReadFloat ();
			break;

		case CE_TELEPORTERPUFFS:
			cl.Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();
				
			cl.Effects[index].Teleporter.framelength = .05;
			dir = 0;
			for (i=0;i<8;++i)
			{		
				if ((cl.Effects[index].Teleporter.entity_index[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[i]];
					VectorCopy(cl.Effects[index].Teleporter.origin, ent->origin);

					angleval = dir * M_PI*2 / 360;

					sinval = sin(angleval);
					cosval = cos(angleval);

					cl.Effects[index].Teleporter.velocity[i][0] = 10*cosval;
					cl.Effects[index].Teleporter.velocity[i][1] = 10*sinval;
					cl.Effects[index].Teleporter.velocity[i][2] = 0;
					dir += 45;

					ent->model = R_RegisterModel("models/telesmk2.spr");
					ent->drawflags = DRF_TRANSLUCENT;
				}
			}
			break;

		case CE_TELEPORTERBODY:
			cl.Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Teleporter.velocity[0][0] = net_message.ReadFloat ();
			cl.Effects[index].Teleporter.velocity[0][1] = net_message.ReadFloat ();
			cl.Effects[index].Teleporter.velocity[0][2] = net_message.ReadFloat ();

			skinnum = net_message.ReadFloat ();
			
			cl.Effects[index].Teleporter.framelength = .05;
			dir = 0;
			if ((cl.Effects[index].Teleporter.entity_index[0] = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				VectorCopy(cl.Effects[index].Teleporter.origin, ent->origin);

				ent->model = R_RegisterModel("models/teleport.mdl");
				ent->drawflags = SCALE_TYPE_XYONLY | DRF_TRANSLUCENT;
				ent->scale = 100;
				ent->skinnum = skinnum;
			}
			break;

		case CE_BONESHARD:
		case CE_BONESHRAPNEL:
			cl.Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Missile.velocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.velocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.velocity[2] = net_message.ReadFloat ();

			cl.Effects[index].Missile.angle[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.angle[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.angle[2] = net_message.ReadFloat ();

			cl.Effects[index].Missile.avelocity[0] = net_message.ReadFloat ();
			cl.Effects[index].Missile.avelocity[1] = net_message.ReadFloat ();
			cl.Effects[index].Missile.avelocity[2] = net_message.ReadFloat ();

			if ((cl.Effects[index].Missile.entity_index = NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];
				VectorCopy(cl.Effects[index].Missile.origin, ent->origin);
				if (cl.Effects[index].type == CE_BONESHARD)
					ent->model = R_RegisterModel("models/boneshot.mdl");
				else if (cl.Effects[index].type == CE_BONESHRAPNEL)
					ent->model = R_RegisterModel("models/boneshrd.mdl");
			}
			else
				ImmediateFree = true;
			break;

		case CE_CHUNK:
			cl.Effects[index].Chunk.origin[0] = net_message.ReadCoord ();
			cl.Effects[index].Chunk.origin[1] = net_message.ReadCoord ();
			cl.Effects[index].Chunk.origin[2] = net_message.ReadCoord ();

			cl.Effects[index].Chunk.type = net_message.ReadByte ();

			cl.Effects[index].Chunk.srcVel[0] = net_message.ReadCoord ();
			cl.Effects[index].Chunk.srcVel[1] = net_message.ReadCoord ();
			cl.Effects[index].Chunk.srcVel[2] = net_message.ReadCoord ();

			cl.Effects[index].Chunk.numChunks = net_message.ReadByte ();

			cl.Effects[index].Chunk.time_amount = 4.0;

			cl.Effects[index].Chunk.aveScale = 30 + 100 * (cl.Effects[index].Chunk.numChunks / 40.0);

			if(cl.Effects[index].Chunk.numChunks > 16)cl.Effects[index].Chunk.numChunks = 16;

			for (i=0;i < cl.Effects[index].Chunk.numChunks;i++)
			{		
				if ((cl.Effects[index].Chunk.entity_index[i] = NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.Effects[index].Chunk.entity_index[i]];
					VectorCopy(cl.Effects[index].Chunk.origin, ent->origin);

					VectorCopy(cl.Effects[index].Chunk.srcVel, cl.Effects[index].Chunk.velocity[i]);
					VectorScale(cl.Effects[index].Chunk.velocity[i], .80 + ((rand()%4)/10.0), cl.Effects[index].Chunk.velocity[i]);
					// temp modify them...
					cl.Effects[index].Chunk.velocity[i][0] += (rand()%140)-70;
					cl.Effects[index].Chunk.velocity[i][1] += (rand()%140)-70;
					cl.Effects[index].Chunk.velocity[i][2] += (rand()%140)-70;

					// are these in degrees or radians?
					ent->angles[0] = rand()%360;
					ent->angles[1] = rand()%360;
					ent->angles[2] = rand()%360;

					ent->scale = cl.Effects[index].Chunk.aveScale + rand()%40;

					// make this overcomplicated
					final = (rand()%100)*.01;
					if ((cl.Effects[index].Chunk.type==THINGTYPE_GLASS) || (cl.Effects[index].Chunk.type==THINGTYPE_REDGLASS) || 
							(cl.Effects[index].Chunk.type==THINGTYPE_CLEARGLASS) || (cl.Effects[index].Chunk.type==THINGTYPE_WEBS))
					{
						if (final<0.20)
							ent->model = R_RegisterModel ("models/shard1.mdl");
						else if (final<0.40)
							ent->model = R_RegisterModel ("models/shard2.mdl");
						else if (final<0.60)
							ent->model = R_RegisterModel ("models/shard3.mdl");
						else if (final<0.80)
							ent->model = R_RegisterModel ("models/shard4.mdl");
						else 
							ent->model = R_RegisterModel ("models/shard5.mdl");

						if (cl.Effects[index].Chunk.type==THINGTYPE_CLEARGLASS)
						{
							ent->skinnum=1;
							ent->drawflags |= DRF_TRANSLUCENT;
						}
						else if (cl.Effects[index].Chunk.type==THINGTYPE_REDGLASS)
						{
							ent->skinnum=2;
						}
						else if (cl.Effects[index].Chunk.type==THINGTYPE_WEBS)
						{
							ent->skinnum=3;
							ent->drawflags |= DRF_TRANSLUCENT;
						}
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_WOOD)
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/splnter1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/splnter2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/splnter3.mdl");
						else 
							ent->model = R_RegisterModel ("models/splnter4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_METAL)
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/metlchk1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/metlchk2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/metlchk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/metlchk4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_FLESH)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/flesh1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/flesh2.mdl");
						else
							ent->model = R_RegisterModel ("models/flesh3.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_BROWNSTONE)
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/schunk1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/schunk2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/schunk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/schunk4.mdl");
						ent->skinnum = 1;
					}
					else if ((cl.Effects[index].Chunk.type==THINGTYPE_CLAY) || (cl.Effects[index].Chunk.type==THINGTYPE_BONE))
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/clshard1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/clshard2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/clshard3.mdl");
						else 
							ent->model = R_RegisterModel ("models/clshard4.mdl");
						if (cl.Effects[index].Chunk.type==THINGTYPE_BONE)
						{
							ent->skinnum=1;//bone skin is second
						}
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_LEAVES)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/leafchk1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/leafchk2.mdl");
						else 
							ent->model = R_RegisterModel ("models/leafchk3.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_HAY)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/hay1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/hay2.mdl");
						else 
							ent->model = R_RegisterModel ("models/hay3.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_CLOTH)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/clthchk1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/clthchk2.mdl");
						else 
							ent->model = R_RegisterModel ("models/clthchk3.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_WOOD_LEAF)
					{
						if (final < 0.14)
							ent->model = R_RegisterModel ("models/splnter1.mdl");
						else if (final < 0.28)
							ent->model = R_RegisterModel ("models/leafchk1.mdl");
						else if (final < 0.42)
							ent->model = R_RegisterModel ("models/splnter2.mdl");
						else if (final < 0.56)
							ent->model = R_RegisterModel ("models/leafchk2.mdl");
						else if (final < 0.70)
							ent->model = R_RegisterModel ("models/splnter3.mdl");
						else if (final < 0.84)
							ent->model = R_RegisterModel ("models/leafchk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/splnter4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_WOOD_METAL)
					{
						if (final < 0.125)
							ent->model = R_RegisterModel ("models/splnter1.mdl");
						else if (final < 0.25)
							ent->model = R_RegisterModel ("models/metlchk1.mdl");
						else if (final < 0.375)
							ent->model = R_RegisterModel ("models/splnter2.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/metlchk2.mdl");
						else if (final < 0.625)
							ent->model = R_RegisterModel ("models/splnter3.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/metlchk3.mdl");
						else if (final < 0.875)
							ent->model = R_RegisterModel ("models/splnter4.mdl");
						else 
							ent->model = R_RegisterModel ("models/metlchk4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_WOOD_STONE)
					{
						if (final < 0.125)
							ent->model = R_RegisterModel ("models/splnter1.mdl");
						else if (final < 0.25)
							ent->model = R_RegisterModel ("models/schunk1.mdl");
						else if (final < 0.375)
							ent->model = R_RegisterModel ("models/splnter2.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/schunk2.mdl");
						else if (final < 0.625)
							ent->model = R_RegisterModel ("models/splnter3.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/schunk3.mdl");
						else if (final < 0.875)
							ent->model = R_RegisterModel ("models/splnter4.mdl");
						else 
							ent->model = R_RegisterModel ("models/schunk4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_METAL_STONE)
					{
						if (final < 0.125)
							ent->model = R_RegisterModel ("models/metlchk1.mdl");
						else if (final < 0.25)
							ent->model = R_RegisterModel ("models/schunk1.mdl");
						else if (final < 0.375)
							ent->model = R_RegisterModel ("models/metlchk2.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/schunk2.mdl");
						else if (final < 0.625)
							ent->model = R_RegisterModel ("models/metlchk3.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/schunk3.mdl");
						else if (final < 0.875)
							ent->model = R_RegisterModel ("models/metlchk4.mdl");
						else 
							ent->model = R_RegisterModel ("models/schunk4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_METAL_CLOTH)
					{
						if (final < 0.14)
							ent->model = R_RegisterModel ("models/metlchk1.mdl");
						else if (final < 0.28)
							ent->model = R_RegisterModel ("models/clthchk1.mdl");
						else if (final < 0.42)
							ent->model = R_RegisterModel ("models/metlchk2.mdl");
						else if (final < 0.56)
							ent->model = R_RegisterModel ("models/clthchk2.mdl");
						else if (final < 0.70)
							ent->model = R_RegisterModel ("models/metlchk3.mdl");
						else if (final < 0.84)
							ent->model = R_RegisterModel ("models/clthchk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/metlchk4.mdl");
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_ICE)
					{
						ent->model = R_RegisterModel("models/shard.mdl");
						ent->skinnum=0;
						ent->frame = rand()%2;
						ent->drawflags |= DRF_TRANSLUCENT|MLS_ABSLIGHT;
						ent->abslight = 0.5;
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_METEOR)
					{
						ent->model = R_RegisterModel("models/tempmetr.mdl");
						ent->skinnum = 0;
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_ACID)
					{	// no spinning if possible...
						ent->model = R_RegisterModel("models/sucwp2p.mdl");
						ent->skinnum = 0;
					}
					else if (cl.Effects[index].Chunk.type==THINGTYPE_GREENFLESH)
					{	// spider guts
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/sflesh1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/sflesh2.mdl");
						else
							ent->model = R_RegisterModel ("models/sflesh3.mdl");

						ent->skinnum = 0;
					}
					else// if (cl.Effects[index].Chunk.type==THINGTYPE_GREYSTONE)
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/schunk1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/schunk2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/schunk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/schunk4.mdl");
						ent->skinnum = 0;
					}
				}
			}
			for(i=0; i < 3; i++)
			{
				cl.Effects[index].Chunk.avel[i][0] = rand()%850 - 425;
				cl.Effects[index].Chunk.avel[i][1] = rand()%850 - 425;
				cl.Effects[index].Chunk.avel[i][2] = rand()%850 - 425;
			}

			break;

		default:
			Sys_Error ("CL_ParseEffect: bad type");
	}

	if (ImmediateFree)
	{
		cl.Effects[index].type = CE_NONE;
	}
}

void CL_EndEffect(void)
{
	int index;

	index = net_message.ReadByte();

	CL_FreeEffect(index);
}

void CL_LinkEntity(entity_t *ent)
{
	refEntity_t rent;
	Com_Memset(&rent, 0, sizeof(rent));
	rent.reType = RT_MODEL;
	VectorCopy(ent->origin, rent.origin);
	rent.hModel = ent->model;
	rent.frame = ent->frame;
	rent.shaderTime = ent->syncbase;
	rent.skinNum = ent->skinnum;
	CL_SetRefEntAxis(&rent, ent->angles, ent->scale, ent->colorshade, ent->abslight, ent->drawflags);
	R_HandleCustomSkin(&rent, -1);
	R_AddRefEntityToScene(&rent);
}

void R_RunQuakeEffect (vec3_t org, float distance);
void RiderParticle(int count, vec3_t origin);
void GravityWellParticle(int count, vec3_t origin, int color);

void CL_UpdateEffects(void)
{
	int index,cur_frame;
	vec3_t mymin,mymax;
	float frametime;
	vec3_t	org,org2,alldir;
	int x_dir,y_dir;
	entity_t *ent;
	float distance,smoketime;
	int i;
	vec3_t snow_org;

	if (cls.state == ca_disconnected)
		return;

	frametime = cl.serverTimeFloat - cl.oldtime;
	if (!frametime) return;
//	Con_Printf("Here at %f\n",cl.time);

	for(index=0;index<MAX_EFFECTS;index++)
	{
		if (!cl.Effects[index].type) 
			continue;

		switch(cl.Effects[index].type)
		{
			case CE_RAIN:
				org[0] = cl.Effects[index].Rain.min_org[0];
				org[1] = cl.Effects[index].Rain.min_org[1];
				org[2] = cl.Effects[index].Rain.max_org[2];

				org2[0] = cl.Effects[index].Rain.e_size[0];
				org2[1] = cl.Effects[index].Rain.e_size[1];
				org2[2] = cl.Effects[index].Rain.e_size[2];

				x_dir = cl.Effects[index].Rain.dir[0];
				y_dir = cl.Effects[index].Rain.dir[1];
				
				cl.Effects[index].Rain.next_time += frametime;
				if (cl.Effects[index].Rain.next_time >= cl.Effects[index].Rain.wait)
				{		
					R_RainEffect(org,org2,x_dir,y_dir,cl.Effects[index].Rain.color,
						cl.Effects[index].Rain.count);
					cl.Effects[index].Rain.next_time = 0;
				}
				break;

			case CE_SNOW:
				VectorCopy(cl.Effects[index].Rain.min_org,org);
				VectorCopy(cl.Effects[index].Rain.max_org,org2);
				VectorCopy(cl.Effects[index].Rain.dir,alldir);
								
				VectorAdd(org, org2, snow_org);

				snow_org[0] *= 0.5;
				snow_org[1] *= 0.5;
				snow_org[2] *= 0.5;

				snow_org[2] = r_refdef.vieworg[2];

				VectorSubtract(snow_org, r_refdef.vieworg, snow_org);
				
				distance = VectorNormalize(snow_org);
				
				cl.Effects[index].Rain.next_time += frametime;
				//jfm:  fixme, check distance to player first
				if (cl.Effects[index].Rain.next_time >= 0.10 && distance < 1024)
				{		
					R_SnowEffect(org,org2,cl.Effects[index].Rain.flags,alldir,
								 cl.Effects[index].Rain.count);

					cl.Effects[index].Rain.next_time = 0;
				}
				break;

			case CE_FOUNTAIN:
				mymin[0] = (-3 * cl.Effects[index].Fountain.vright[0] * cl.Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].Fountain.vforward[0] * cl.Effects[index].Fountain.movedir[1]) +
						   (2 * cl.Effects[index].Fountain.vup[0] * cl.Effects[index].Fountain.movedir[2]);
				mymin[1] = (-3 * cl.Effects[index].Fountain.vright[1] * cl.Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].Fountain.vforward[1] * cl.Effects[index].Fountain.movedir[1]) +
						   (2 * cl.Effects[index].Fountain.vup[1] * cl.Effects[index].Fountain.movedir[2]);
				mymin[2] = (-3 * cl.Effects[index].Fountain.vright[2] * cl.Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.Effects[index].Fountain.vforward[2] * cl.Effects[index].Fountain.movedir[1]) +
						   (2 * cl.Effects[index].Fountain.vup[2] * cl.Effects[index].Fountain.movedir[2]);
				mymin[0] *= 15;
				mymin[1] *= 15;
				mymin[2] *= 15;

				mymax[0] = (3 * cl.Effects[index].Fountain.vright[0] * cl.Effects[index].Fountain.movedir[0]) +
						   (3 * cl.Effects[index].Fountain.vforward[0] * cl.Effects[index].Fountain.movedir[1]) +
						   (10 * cl.Effects[index].Fountain.vup[0] * cl.Effects[index].Fountain.movedir[2]);
				mymax[1] = (3 * cl.Effects[index].Fountain.vright[1] * cl.Effects[index].Fountain.movedir[0]) +
						   (3 * cl.Effects[index].Fountain.vforward[1] * cl.Effects[index].Fountain.movedir[1]) +
						   (10 * cl.Effects[index].Fountain.vup[1] * cl.Effects[index].Fountain.movedir[2]);
				mymax[2] = (3 * cl.Effects[index].Fountain.vright[2] * cl.Effects[index].Fountain.movedir[0]) +
						   (3 * cl.Effects[index].Fountain.vforward[2] * cl.Effects[index].Fountain.movedir[1]) +
						   (10 * cl.Effects[index].Fountain.vup[2] * cl.Effects[index].Fountain.movedir[2]);
				mymax[0] *= 15;
				mymax[1] *= 15;
				mymax[2] *= 15;

				R_RunParticleEffect2 (cl.Effects[index].Fountain.pos,mymin,mymax,
					                  cl.Effects[index].Fountain.color,pt_h2fastgrav,cl.Effects[index].Fountain.cnt);

/*				Com_Memset(&test,0,sizeof(test));
				trace = SV_Move (cl.Effects[index].Fountain.pos, mymin, mymax, mymin, false, &test);
				Con_Printf("Fraction is %f\n",trace.fraction);*/
				break;

			case CE_QUAKE:
				R_RunQuakeEffect (cl.Effects[index].Quake.origin,cl.Effects[index].Quake.radius);
				break;

			case CE_WHITE_SMOKE:
			case CE_GREEN_SMOKE:
			case CE_GREY_SMOKE:
			case CE_RED_SMOKE:
			case CE_SLOW_WHITE_SMOKE:
			case CE_TELESMK1:
			case CE_TELESMK2:
			case CE_GHOST:
			case CE_REDCLOUD:
			case CE_FLAMESTREAM:
			case CE_ACID_MUZZFL:
			case CE_FLAMEWALL:
			case CE_FLAMEWALL2:
			case CE_ONFIRE:
				cl.Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];

				smoketime = cl.Effects[index].Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME;

				ent->origin[0] += (frametime/smoketime) * cl.Effects[index].Smoke.velocity[0];
				ent->origin[1] += (frametime/smoketime) * cl.Effects[index].Smoke.velocity[1];
				ent->origin[2] += (frametime/smoketime) * cl.Effects[index].Smoke.velocity[2];

				while(cl.Effects[index].Smoke.time_amount >= smoketime)
				{
					ent->frame++;
					cl.Effects[index].Smoke.time_amount -= smoketime;
				}

				if (ent->frame >= R_ModelNumFrames(ent->model))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				break;

			// Just go through animation and then remove
			case CE_SM_WHITE_FLASH:
			case CE_YELLOWRED_FLASH:
			case CE_BLUESPARK:
			case CE_YELLOWSPARK:
			case CE_SM_CIRCLE_EXP:
			case CE_BG_CIRCLE_EXP:
			case CE_SM_EXPLOSION:
			case CE_LG_EXPLOSION:
			case CE_FLOOR_EXPLOSION:
			case CE_FLOOR_EXPLOSION3:
			case CE_BLUE_EXPLOSION:
			case CE_REDSPARK:
			case CE_GREENSPARK:
			case CE_ICEHIT:
			case CE_MEDUSA_HIT:
			case CE_MEZZO_REFLECT:
			case CE_FLOOR_EXPLOSION2:
			case CE_XBOW_EXPLOSION:
			case CE_NEW_EXPLOSION:
			case CE_MAGIC_MISSILE_EXPLOSION:
			case CE_BONE_EXPLOSION:
			case CE_BLDRN_EXPL:
			case CE_BRN_BOUNCE:
			case CE_ACID_HIT:
			case CE_ACID_SPLAT:
			case CE_ACID_EXPL:
			case CE_LBALL_EXPL:
			case CE_FBOOM:
			case CE_BOMB:
			case CE_FIREWALL_SMALL:
			case CE_FIREWALL_MEDIUM:
			case CE_FIREWALL_LARGE:

				cl.Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];

				if (cl.Effects[index].type != CE_BG_CIRCLE_EXP)
				{
					while(cl.Effects[index].Smoke.time_amount >= HX_FRAME_TIME)
					{
						ent->frame++;
						cl.Effects[index].Smoke.time_amount -= HX_FRAME_TIME;
					}
				}
				else
				{
					while(cl.Effects[index].Smoke.time_amount >= HX_FRAME_TIME * 2)
					{
						ent->frame++;
						cl.Effects[index].Smoke.time_amount -= HX_FRAME_TIME * 2;
					}
				}
				if (ent->frame >= R_ModelNumFrames(ent->model))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;


			case CE_LSHOCK:
				ent = &EffectEntities[cl.Effects[index].Smoke.entity_index];
				if(ent->skinnum==0)
					ent->skinnum=1;
				else if(ent->skinnum==1)
					ent->skinnum=0;
				ent->scale-=10;
				if (ent->scale<=10)
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			// Go forward then backward through animation then remove
			case CE_WHITE_FLASH:
			case CE_BLUE_FLASH:
			case CE_SM_BLUE_FLASH:
			case CE_RED_FLASH:
				cl.Effects[index].Flash.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Flash.entity_index];

				while(cl.Effects[index].Flash.time_amount >= HX_FRAME_TIME)
				{
					if (!cl.Effects[index].Flash.reverse)
					{
						if (ent->frame >= R_ModelNumFrames(ent->model) - 1)  // Ran through forward animation
						{
							cl.Effects[index].Flash.reverse = 1;
							ent->frame--;
						}
						else
							ent->frame++;

					}	
					else
						ent->frame--;

					cl.Effects[index].Flash.time_amount -= HX_FRAME_TIME;
				}

				if ((ent->frame <= 0) && (cl.Effects[index].Flash.reverse))
				{
					CL_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			case CE_RIDER_DEATH:
				cl.Effects[index].RD.time_amount += frametime;
				if (cl.Effects[index].RD.time_amount >= 1)
				{
					cl.Effects[index].RD.stage++;
					cl.Effects[index].RD.time_amount -= 1;
				}

				VectorCopy(cl.Effects[index].RD.origin,org);
				org[0] += sin(cl.Effects[index].RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.Effects[index].RD.time_amount * 2 * M_PI) * 30;

				if (cl.Effects[index].RD.stage <= 6)
//					RiderParticle(cl.Effects[index].RD.stage+1,cl.Effects[index].RD.origin);
					RiderParticle(cl.Effects[index].RD.stage+1,org);
				else
				{
					// To set the rider's origin point for the particles
					RiderParticle(0,org);
					if (cl.Effects[index].RD.stage == 7) 
					{
						cl.cshifts[CSHIFT_BONUS].destcolor[0] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[1] = 255;
						cl.cshifts[CSHIFT_BONUS].destcolor[2] = 255;
						cl.cshifts[CSHIFT_BONUS].percent = 256;
					}
					else if (cl.Effects[index].RD.stage > 13) 
					{
//						cl.Effects[index].RD.stage = 0;
						CL_FreeEffect(index);
					}
				}
				break;

			case CE_GRAVITYWELL:
			
				cl.Effects[index].RD.time_amount += frametime*2;
				if (cl.Effects[index].RD.time_amount >= 1)
					cl.Effects[index].RD.time_amount -= 1;
		
				VectorCopy(cl.Effects[index].RD.origin,org);
				org[0] += sin(cl.Effects[index].RD.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.Effects[index].RD.time_amount * 2 * M_PI) * 30;

				if (cl.Effects[index].RD.lifetime < cl.serverTimeFloat)
				{
					CL_FreeEffect(index);
				}
				else
					GravityWellParticle(rand()%8,org, cl.Effects[index].RD.color);

				break;

			case CE_TELEPORTERPUFFS:
				cl.Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				while(cl.Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->frame++;
					cl.Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}
				cur_frame = ent->frame;

				if (cur_frame >= R_ModelNumFrames(ent->model))
				{
					CL_FreeEffect(index);
					break;
				}

				for (i=0;i<8;++i)
				{
					ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[i]];

					ent->origin[0] += (frametime/smoketime) * cl.Effects[index].Teleporter.velocity[i][0];
					ent->origin[1] += (frametime/smoketime) * cl.Effects[index].Teleporter.velocity[i][1];
					ent->origin[2] += (frametime/smoketime) * cl.Effects[index].Teleporter.velocity[i][2];
					ent->frame = cur_frame;

					CL_LinkEntity(ent);
				}
				break;

			case CE_TELEPORTERBODY:
				cl.Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				while(cl.Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->scale -= 15;
					cl.Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}

				ent = &EffectEntities[cl.Effects[index].Teleporter.entity_index[0]];
				ent->angles[1] += 45;

				if (ent->scale <= 10)
				{
					CL_FreeEffect(index);
				}
				else
				{
					CL_LinkEntity(ent);
				}
				break;

			case CE_BONESHARD:
			case CE_BONESHRAPNEL:
				cl.Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.Effects[index].Missile.entity_index];

//		ent->angles[0] = cl.Effects[index].Missile.angle[0];
//		ent->angles[1] = cl.Effects[index].Missile.angle[1];
//		ent->angles[2] = cl.Effects[index].Missile.angle[2];

				ent->angles[0] += frametime * cl.Effects[index].Missile.avelocity[0];
				ent->angles[1] += frametime * cl.Effects[index].Missile.avelocity[1];
				ent->angles[2] += frametime * cl.Effects[index].Missile.avelocity[2];

				ent->origin[0] += frametime * cl.Effects[index].Missile.velocity[0];
				ent->origin[1] += frametime * cl.Effects[index].Missile.velocity[1];
				ent->origin[2] += frametime * cl.Effects[index].Missile.velocity[2];

				CL_LinkEntity(ent);
				break;

			case CE_CHUNK:
				cl.Effects[index].Chunk.time_amount -= frametime;
				if(cl.Effects[index].Chunk.time_amount < 0)
				{
					CL_FreeEffect(index);	
				}
				else
				{
					for (i=0;i < cl.Effects[index].Chunk.numChunks;i++)
					{
						vec3_t oldorg;
						int			moving = 1;

						ent = &EffectEntities[cl.Effects[index].Chunk.entity_index[i]];

						VectorCopy(ent->origin, oldorg);

						ent->origin[0] += frametime * cl.Effects[index].Chunk.velocity[i][0];
						ent->origin[1] += frametime * cl.Effects[index].Chunk.velocity[i][1];
						ent->origin[2] += frametime * cl.Effects[index].Chunk.velocity[i][2];

						if (CM_PointContentsQ1(ent->origin, 0) != BSP29CONTENTS_EMPTY) //||in_solid==true
						{
							// bouncing prolly won't work...
							VectorCopy(oldorg, ent->origin);

							cl.Effects[index].Chunk.velocity[i][0] = 0;
							cl.Effects[index].Chunk.velocity[i][1] = 0;
							cl.Effects[index].Chunk.velocity[i][2] = 0;

							moving = 0;
						}
						else
						{
							ent->angles[0] += frametime * cl.Effects[index].Chunk.avel[i%3][0];
							ent->angles[1] += frametime * cl.Effects[index].Chunk.avel[i%3][1];
							ent->angles[2] += frametime * cl.Effects[index].Chunk.avel[i%3][2];
						}

						if(cl.Effects[index].Chunk.time_amount < frametime * 3)
						{	// chunk leaves in 3 frames
							ent->scale *= .7;
						}

						CL_LinkEntity(ent);

						cl.Effects[index].Chunk.velocity[i][2] -= frametime * 500; // apply gravity

						switch(cl.Effects[index].Chunk.type)
						{
						case THINGTYPE_GREYSTONE:
							break;
						case THINGTYPE_WOOD:
							break;
						case THINGTYPE_METAL:
							break;
						case THINGTYPE_FLESH:
							if(moving)R_RocketTrail (oldorg, ent->origin, 17);
							break;
						case THINGTYPE_FIRE:
							break;
						case THINGTYPE_CLAY:
						case THINGTYPE_BONE:
							break;
						case THINGTYPE_LEAVES:
							break;
						case THINGTYPE_HAY:
							break;
						case THINGTYPE_BROWNSTONE:
							break;
						case THINGTYPE_CLOTH:
							break;
						case THINGTYPE_WOOD_LEAF:
							break;
						case THINGTYPE_WOOD_METAL:
							break;
						case THINGTYPE_WOOD_STONE:
							break;
						case THINGTYPE_METAL_STONE:
							break;
						case THINGTYPE_METAL_CLOTH:
							break;
						case THINGTYPE_WEBS:
							break;
						case THINGTYPE_GLASS:
							break;
						case THINGTYPE_ICE:
							if(moving)R_RocketTrail (oldorg, ent->origin, rt_ice);
							break;
						case THINGTYPE_CLEARGLASS:
							break;
						case THINGTYPE_REDGLASS:
							break;
						case THINGTYPE_ACID:
							if(moving)R_RocketTrail (oldorg, ent->origin, rt_acidball);
							break;
						case THINGTYPE_METEOR:
							R_RocketTrail (oldorg, ent->origin, 1);
							break;
						case THINGTYPE_GREENFLESH:
							if(moving)R_RocketTrail (oldorg, ent->origin, rt_acidball);
							break;

						}
					}
				}
				break;
		}
	}
}

//==========================================================================
//
// NewEffectEntity
//
//==========================================================================

static int NewEffectEntity(void)
{
	entity_t	*ent;
	int counter;

	if(EffectEntityCount == MAX_EFFECT_ENTITIES)
	{
		return -1;
	}

	for(counter=0;counter<MAX_EFFECT_ENTITIES;counter++)
		if (!EntityUsed[counter]) 
			break;

	EntityUsed[counter] = true;
	EffectEntityCount++;
	ent = &EffectEntities[counter];
	Com_Memset(ent, 0, sizeof(*ent));

	return counter;
}

static void FreeEffectEntity(int index)
{
	EntityUsed[index] = false;
	EffectEntityCount--;
}
