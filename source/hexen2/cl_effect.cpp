
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
	Com_Memset(sv.Effects,0,sizeof(sv.Effects));
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects()
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
		case H2CE_RAIN:
		case H2CE_SNOW:
			DoTest = false;
			break;

		case H2CE_FOUNTAIN:
			DoTest = false;
			break;

		case H2CE_QUAKE:
			VectorCopy(sv.Effects[index].Quake.origin, TestO1);
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
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
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
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case H2CE_WHITE_FLASH:
		case H2CE_BLUE_FLASH:
		case H2CE_SM_BLUE_FLASH:
		case H2CE_RED_FLASH:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case H2CE_RIDER_DEATH:
			DoTest = false;
			break;

		case H2CE_GRAVITYWELL:
			DoTest = false;
			break;

		case H2CE_TELEPORTERPUFFS:
			VectorCopy(sv.Effects[index].Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case H2CE_TELEPORTERBODY:
			VectorCopy(sv.Effects[index].Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case H2CE_BONESHARD:
		case H2CE_BONESHRAPNEL:
			VectorCopy(sv.Effects[index].Missile.origin, TestO1);
			TestDistance = 600;
			break;
		case H2CE_CHUNK:
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
			case H2CE_RAIN:
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
				
			case H2CE_SNOW:
				sb->WriteCoord(sv.Effects[index].Snow.min_org[0]);
				sb->WriteCoord(sv.Effects[index].Snow.min_org[1]);
				sb->WriteCoord(sv.Effects[index].Snow.min_org[2]);
				sb->WriteCoord(sv.Effects[index].Snow.max_org[0]);
				sb->WriteCoord(sv.Effects[index].Snow.max_org[1]);
				sb->WriteCoord(sv.Effects[index].Snow.max_org[2]);
				sb->WriteByte(sv.Effects[index].Snow.flags);
				sb->WriteCoord(sv.Effects[index].Snow.dir[0]);
				sb->WriteCoord(sv.Effects[index].Snow.dir[1]);
				sb->WriteCoord(sv.Effects[index].Snow.dir[2]);
				sb->WriteByte(sv.Effects[index].Snow.count);
				break;
				
			case H2CE_FOUNTAIN:
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
				
			case H2CE_QUAKE:
				sb->WriteCoord(sv.Effects[index].Quake.origin[0]);
				sb->WriteCoord(sv.Effects[index].Quake.origin[1]);
				sb->WriteCoord(sv.Effects[index].Quake.origin[2]);
				sb->WriteFloat(sv.Effects[index].Quake.radius);
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
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[0]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[1]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[2]);
				sb->WriteFloat(sv.Effects[index].Smoke.framelength);
				sb->WriteFloat(sv.Effects[index].Smoke.frame);
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
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				break;
				
			case H2CE_WHITE_FLASH:
			case H2CE_BLUE_FLASH:
			case H2CE_SM_BLUE_FLASH:
			case H2CE_RED_FLASH:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				break;
								
			case H2CE_RIDER_DEATH:
				sb->WriteCoord(sv.Effects[index].RD.origin[0]);
				sb->WriteCoord(sv.Effects[index].RD.origin[1]);
				sb->WriteCoord(sv.Effects[index].RD.origin[2]);
				break;
				
			case H2CE_TELEPORTERPUFFS:
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[0]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[1]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[2]);
				break;
				
			case H2CE_TELEPORTERBODY:
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[0]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[1]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[2]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][0]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][1]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][2]);
				sb->WriteFloat(sv.Effects[index].Teleporter.skinnum);
				break;

			case H2CE_BONESHARD:
			case H2CE_BONESHRAPNEL:
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

			case H2CE_GRAVITYWELL:
				sb->WriteCoord(sv.Effects[index].GravityWell.origin[0]);
				sb->WriteCoord(sv.Effects[index].GravityWell.origin[1]);
				sb->WriteCoord(sv.Effects[index].GravityWell.origin[2]);
				sb->WriteShort(sv.Effects[index].GravityWell.color);
				sb->WriteFloat(sv.Effects[index].GravityWell.lifetime);
				break;

			case H2CE_CHUNK:
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

	for(index=0;index<MAX_EFFECTS_H2;index++)
		if (sv.Effects[index].type)
			SV_SendEffect(sb,index);
}

// All changes need to be in SV_SendEffect(), SV_ParseEffect(),
// SV_SaveEffects(), SV_LoadEffects()
void SV_ParseEffect(QMsg *sb)
{
	int index;
	byte effect;

	effect = G_FLOAT(OFS_PARM0);

	for(index=0;index<MAX_EFFECTS_H2;index++)
		if (!sv.Effects[index].type || 
			(sv.Effects[index].expire_time && sv.Effects[index].expire_time <= sv.time)) 
			break;
		
	if (index >= MAX_EFFECTS_H2)
	{
		PR_RunError ("MAX_EFFECTS_H2 reached");
		return;
	}

//	Con_Printf("Effect #%d\n",index);

	Com_Memset(&sv.Effects[index],0,sizeof(struct h2EffectT));

	sv.Effects[index].type = effect;
	G_FLOAT(OFS_RETURN) = index;

	switch(effect)
	{
		case H2CE_RAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Rain.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Rain.max_org);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Rain.e_size);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].Rain.dir);
			sv.Effects[index].Rain.color = G_FLOAT(OFS_PARM5);
			sv.Effects[index].Rain.count = G_FLOAT(OFS_PARM6);
			sv.Effects[index].Rain.wait = G_FLOAT(OFS_PARM7);

			sv.Effects[index].Rain.next_time = 0;
			break;

		case H2CE_SNOW:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Snow.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Snow.max_org);
			sv.Effects[index].Snow.flags = G_FLOAT(OFS_PARM3);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].Snow.dir);
			sv.Effects[index].Snow.count = G_FLOAT(OFS_PARM5);

			sv.Effects[index].Snow.next_time = 0;
			break;

		case H2CE_FOUNTAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Fountain.pos);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Fountain.angle);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Fountain.movedir);
			sv.Effects[index].Fountain.color = G_FLOAT(OFS_PARM4);
			sv.Effects[index].Fountain.cnt = G_FLOAT(OFS_PARM5);
			break;

		case H2CE_QUAKE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Quake.origin);
			sv.Effects[index].Quake.radius = G_FLOAT(OFS_PARM2);
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
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].Smoke.velocity);
			sv.Effects[index].Smoke.framelength = G_FLOAT(OFS_PARM3);
			sv.Effects[index].Smoke.frame = 0;
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case H2CE_ACID_MUZZFL:
		case H2CE_FLAMESTREAM:
		case H2CE_FLAMEWALL:
		case H2CE_FLAMEWALL2:
		case H2CE_ONFIRE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].Smoke.velocity);
			sv.Effects[index].Smoke.framelength = 0.05;
			sv.Effects[index].Smoke.frame = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
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
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case H2CE_WHITE_FLASH:
		case H2CE_BLUE_FLASH:
		case H2CE_SM_BLUE_FLASH:
		case H2CE_RED_FLASH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Flash.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case H2CE_RIDER_DEATH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].RD.origin);
			break;

		case H2CE_GRAVITYWELL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].GravityWell.origin);
			sv.Effects[index].GravityWell.color = G_FLOAT(OFS_PARM2);
			sv.Effects[index].GravityWell.lifetime = G_FLOAT(OFS_PARM3);
			break;

		case H2CE_TELEPORTERPUFFS:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Teleporter.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case H2CE_TELEPORTERBODY:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Teleporter.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Teleporter.velocity[0]);
			sv.Effects[index].Teleporter.skinnum = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case H2CE_BONESHARD:
		case H2CE_BONESHRAPNEL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Missile.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Missile.velocity);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Missile.angle);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Missile.avelocity);

			sv.Effects[index].expire_time = sv.time + 10;
			break;

		case H2CE_CHUNK:
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
// SV_SaveEffects(), SV_LoadEffects()
void SV_SaveEffects(fileHandle_t FH)
{
	int index,count;

	for(index=count=0;index<MAX_EFFECTS_H2;index++)
		if (sv.Effects[index].type)
			count++;

	FS_Printf(FH,"Effects: %d\n",count);

	for(index=count=0;index<MAX_EFFECTS_H2;index++)
		if (sv.Effects[index].type)
		{
			FS_Printf(FH,"Effect: %d %d %f: ",index,sv.Effects[index].type,sv.Effects[index].expire_time);

			switch(sv.Effects[index].type)
			{
				case H2CE_RAIN:
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

				case H2CE_SNOW:
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.min_org[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.min_org[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.min_org[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.max_org[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.max_org[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.max_org[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Snow.flags);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.dir[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.dir[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Snow.dir[2]);
					FS_Printf(FH, "%d ", sv.Effects[index].Snow.count);
					break;

				case H2CE_FOUNTAIN:
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

				case H2CE_QUAKE:
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[2]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Quake.radius);
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
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.framelength);
					FS_Printf(FH, "%f\n", sv.Effects[index].Smoke.frame);
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
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Smoke.origin[2]);
					break;

				case H2CE_WHITE_FLASH:
				case H2CE_BLUE_FLASH:
				case H2CE_SM_BLUE_FLASH:
				case H2CE_RED_FLASH:
					FS_Printf(FH, "%f ", sv.Effects[index].Flash.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Flash.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Flash.origin[2]);
					break;

				case H2CE_RIDER_DEATH:
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].RD.origin[2]);
					break;

				case H2CE_GRAVITYWELL:
					FS_Printf(FH, "%f ", sv.Effects[index].GravityWell.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].GravityWell.origin[1]);
					FS_Printf(FH, "%f", sv.Effects[index].GravityWell.origin[2]);
					FS_Printf(FH, "%d", sv.Effects[index].GravityWell.color);
					FS_Printf(FH, "%f\n", sv.Effects[index].GravityWell.lifetime);
					break;
				case H2CE_TELEPORTERPUFFS:
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Teleporter.origin[2]);
					break;

				case H2CE_TELEPORTERBODY:
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Teleporter.origin[2]);
					break;

				case H2CE_BONESHARD:
				case H2CE_BONESHRAPNEL:
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

				case H2CE_CHUNK:
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
// SV_SaveEffects(), SV_LoadEffects()
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
			case H2CE_RAIN:
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

			case H2CE_SNOW:
				sv.Effects[index].Snow.min_org[0] = GetFloat(Data);
				sv.Effects[index].Snow.min_org[1] = GetFloat(Data);
				sv.Effects[index].Snow.min_org[2] = GetFloat(Data);
				sv.Effects[index].Snow.max_org[0] = GetFloat(Data);
				sv.Effects[index].Snow.max_org[1] = GetFloat(Data);
				sv.Effects[index].Snow.max_org[2] = GetFloat(Data);
				sv.Effects[index].Snow.flags = GetInt(Data);
				sv.Effects[index].Snow.dir[0] = GetFloat(Data);
				sv.Effects[index].Snow.dir[1] = GetFloat(Data);
				sv.Effects[index].Snow.dir[2] = GetFloat(Data);
				sv.Effects[index].Snow.count = GetInt(Data);
				break;

			case H2CE_FOUNTAIN:
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

			case H2CE_QUAKE:
				sv.Effects[index].Quake.origin[0] = GetFloat(Data);
				sv.Effects[index].Quake.origin[1] = GetFloat(Data);
				sv.Effects[index].Quake.origin[2] = GetFloat(Data);
				sv.Effects[index].Quake.radius = GetFloat(Data);
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
				sv.Effects[index].Smoke.origin[0] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[1] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[2] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[0] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[1] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[2] = GetFloat(Data);
				sv.Effects[index].Smoke.framelength = GetFloat(Data);
				sv.Effects[index].Smoke.frame = GetFloat(Data);
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
				sv.Effects[index].Smoke.origin[0] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[1] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[2] = GetFloat(Data);
				break;

			case H2CE_WHITE_FLASH:
			case H2CE_BLUE_FLASH:
			case H2CE_SM_BLUE_FLASH:
			case H2CE_RED_FLASH:
				sv.Effects[index].Flash.origin[0] = GetFloat(Data);
				sv.Effects[index].Flash.origin[1] = GetFloat(Data);
				sv.Effects[index].Flash.origin[2] = GetFloat(Data);
				break;

			case H2CE_RIDER_DEATH:
				sv.Effects[index].RD.origin[0] = GetFloat(Data);
				sv.Effects[index].RD.origin[1] = GetFloat(Data);
				sv.Effects[index].RD.origin[2] = GetFloat(Data);
				break;

			case H2CE_GRAVITYWELL:
				sv.Effects[index].GravityWell.origin[0] = GetFloat(Data);
				sv.Effects[index].GravityWell.origin[1] = GetFloat(Data);
				sv.Effects[index].GravityWell.origin[2] = GetFloat(Data);
				sv.Effects[index].GravityWell.color = GetInt(Data);
				sv.Effects[index].GravityWell.lifetime = GetFloat(Data);
				break;

			case H2CE_TELEPORTERPUFFS:
				sv.Effects[index].Teleporter.origin[0] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[1] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[2] = GetFloat(Data);
				break;

			case H2CE_TELEPORTERBODY:
				sv.Effects[index].Teleporter.origin[0] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[1] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[2] = GetFloat(Data);
				break;

			case H2CE_BONESHARD:
			case H2CE_BONESHRAPNEL:
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

			case H2CE_CHUNK:
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

void CL_EndEffect(void)
{
	int index;

	index = net_message.ReadByte();

	CLH2_FreeEffect(index);
}

void CL_LinkEntity(effect_entity_t* ent)
{
	refEntity_t rent;
	Com_Memset(&rent, 0, sizeof(rent));
	rent.reType = RT_MODEL;
	VectorCopy(ent->state.origin, rent.origin);
	rent.hModel = ent->model;
	rent.frame = ent->state.frame;
	rent.skinNum = ent->state.skinnum;
	CL_SetRefEntAxis(&rent, ent->state.angles, vec3_origin, ent->state.scale, 0, ent->state.abslight, ent->state.drawflags);
	R_HandleCustomSkin(&rent, -1);
	R_AddRefEntityToScene(&rent);
}

void CL_UpdateEffects(void)
{
	int index,cur_frame;
	vec3_t mymin,mymax;
	float frametime;
	vec3_t	org,org2,alldir;
	int x_dir,y_dir;
	effect_entity_t* ent;
	float distance,smoketime;
	int i;
	vec3_t snow_org;

	if (cls.state == ca_disconnected)
		return;

	frametime = cl.serverTimeFloat - cl.oldtime;
	if (!frametime) return;
//	Con_Printf("Here at %f\n",cl.time);

	for(index=0;index<MAX_EFFECTS_H2;index++)
	{
		if (!cl.h2_Effects[index].type) 
			continue;

		switch(cl.h2_Effects[index].type)
		{
			case H2CE_RAIN:
				org[0] = cl.h2_Effects[index].Rain.min_org[0];
				org[1] = cl.h2_Effects[index].Rain.min_org[1];
				org[2] = cl.h2_Effects[index].Rain.max_org[2];

				org2[0] = cl.h2_Effects[index].Rain.e_size[0];
				org2[1] = cl.h2_Effects[index].Rain.e_size[1];
				org2[2] = cl.h2_Effects[index].Rain.e_size[2];

				x_dir = cl.h2_Effects[index].Rain.dir[0];
				y_dir = cl.h2_Effects[index].Rain.dir[1];
				
				cl.h2_Effects[index].Rain.next_time += frametime;
				if (cl.h2_Effects[index].Rain.next_time >= cl.h2_Effects[index].Rain.wait)
				{		
					CLH2_RainEffect(org,org2,x_dir,y_dir,cl.h2_Effects[index].Rain.color,
						cl.h2_Effects[index].Rain.count);
					cl.h2_Effects[index].Rain.next_time = 0;
				}
				break;

			case H2CE_SNOW:
				VectorCopy(cl.h2_Effects[index].Snow.min_org,org);
				VectorCopy(cl.h2_Effects[index].Snow.max_org,org2);
				VectorCopy(cl.h2_Effects[index].Snow.dir,alldir);
								
				VectorAdd(org, org2, snow_org);

				snow_org[0] *= 0.5;
				snow_org[1] *= 0.5;
				snow_org[2] *= 0.5;

				snow_org[2] = cl.refdef.vieworg[2];

				VectorSubtract(snow_org, cl.refdef.vieworg, snow_org);
				
				distance = VectorNormalize(snow_org);
				
				cl.h2_Effects[index].Snow.next_time += frametime;
				//jfm:  fixme, check distance to player first
				if (cl.h2_Effects[index].Snow.next_time >= 0.10 && distance < 1024)
				{		
					CLH2_SnowEffect(org,org2,cl.h2_Effects[index].Snow.flags,alldir,
								 cl.h2_Effects[index].Snow.count);

					cl.h2_Effects[index].Snow.next_time = 0;
				}
				break;

			case H2CE_FOUNTAIN:
				mymin[0] = (-3 * cl.h2_Effects[index].Fountain.vright[0] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.h2_Effects[index].Fountain.vforward[0] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (2 * cl.h2_Effects[index].Fountain.vup[0] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymin[1] = (-3 * cl.h2_Effects[index].Fountain.vright[1] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.h2_Effects[index].Fountain.vforward[1] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (2 * cl.h2_Effects[index].Fountain.vup[1] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymin[2] = (-3 * cl.h2_Effects[index].Fountain.vright[2] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (-3 * cl.h2_Effects[index].Fountain.vforward[2] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (2 * cl.h2_Effects[index].Fountain.vup[2] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymin[0] *= 15;
				mymin[1] *= 15;
				mymin[2] *= 15;

				mymax[0] = (3 * cl.h2_Effects[index].Fountain.vright[0] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (3 * cl.h2_Effects[index].Fountain.vforward[0] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (10 * cl.h2_Effects[index].Fountain.vup[0] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymax[1] = (3 * cl.h2_Effects[index].Fountain.vright[1] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (3 * cl.h2_Effects[index].Fountain.vforward[1] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (10 * cl.h2_Effects[index].Fountain.vup[1] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymax[2] = (3 * cl.h2_Effects[index].Fountain.vright[2] * cl.h2_Effects[index].Fountain.movedir[0]) +
						   (3 * cl.h2_Effects[index].Fountain.vforward[2] * cl.h2_Effects[index].Fountain.movedir[1]) +
						   (10 * cl.h2_Effects[index].Fountain.vup[2] * cl.h2_Effects[index].Fountain.movedir[2]);
				mymax[0] *= 15;
				mymax[1] *= 15;
				mymax[2] *= 15;

				CLH2_RunParticleEffect2 (cl.h2_Effects[index].Fountain.pos,mymin,mymax,
					                  cl.h2_Effects[index].Fountain.color,pt_h2fastgrav,cl.h2_Effects[index].Fountain.cnt);

/*				Com_Memset(&test,0,sizeof(test));
				trace = SV_Move (cl.h2_Effects[index].Fountain.pos, mymin, mymax, mymin, false, &test);
				Con_Printf("Fraction is %f\n",trace.fraction);*/
				break;

			case H2CE_QUAKE:
				CLH2_RunQuakeEffect (cl.h2_Effects[index].Quake.origin,cl.h2_Effects[index].Quake.radius);
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
				cl.h2_Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];

				smoketime = cl.h2_Effects[index].Smoke.framelength;
				if (!smoketime)
					smoketime = HX_FRAME_TIME;

				ent->state.origin[0] += (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[0];
				ent->state.origin[1] += (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[1];
				ent->state.origin[2] += (frametime/smoketime) * cl.h2_Effects[index].Smoke.velocity[2];

				while(cl.h2_Effects[index].Smoke.time_amount >= smoketime)
				{
					ent->state.frame++;
					cl.h2_Effects[index].Smoke.time_amount -= smoketime;
				}

				if (ent->state.frame >= R_ModelNumFrames(ent->model))
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);

				break;

			// Just go through animation and then remove
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
			case H2CE_ACID_HIT:
			case H2CE_ACID_SPLAT:
			case H2CE_ACID_EXPL:
			case H2CE_LBALL_EXPL:
			case H2CE_FBOOM:
			case H2CE_BOMB:
			case H2CE_FIREWALL_SMALL:
			case H2CE_FIREWALL_MEDIUM:
			case H2CE_FIREWALL_LARGE:

				cl.h2_Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];

				if (cl.h2_Effects[index].type != H2CE_BG_CIRCLE_EXP)
				{
					while(cl.h2_Effects[index].Smoke.time_amount >= HX_FRAME_TIME)
					{
						ent->state.frame++;
						cl.h2_Effects[index].Smoke.time_amount -= HX_FRAME_TIME;
					}
				}
				else
				{
					while(cl.h2_Effects[index].Smoke.time_amount >= HX_FRAME_TIME * 2)
					{
						ent->state.frame++;
						cl.h2_Effects[index].Smoke.time_amount -= HX_FRAME_TIME * 2;
					}
				}
				if (ent->state.frame >= R_ModelNumFrames(ent->model))
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;


			case H2CE_LSHOCK:
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];
				if(ent->state.skinnum==0)
					ent->state.skinnum=1;
				else if(ent->state.skinnum==1)
					ent->state.skinnum=0;
				ent->state.scale-=10;
				if (ent->state.scale<=10)
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			// Go forward then backward through animation then remove
			case H2CE_WHITE_FLASH:
			case H2CE_BLUE_FLASH:
			case H2CE_SM_BLUE_FLASH:
			case H2CE_RED_FLASH:
				cl.h2_Effects[index].Flash.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Flash.entity_index];

				while(cl.h2_Effects[index].Flash.time_amount >= HX_FRAME_TIME)
				{
					if (!cl.h2_Effects[index].Flash.reverse)
					{
						if (ent->state.frame >= R_ModelNumFrames(ent->model) - 1)  // Ran through forward animation
						{
							cl.h2_Effects[index].Flash.reverse = 1;
							ent->state.frame--;
						}
						else
							ent->state.frame++;

					}	
					else
						ent->state.frame--;

					cl.h2_Effects[index].Flash.time_amount -= HX_FRAME_TIME;
				}

				if ((ent->state.frame <= 0) && (cl.h2_Effects[index].Flash.reverse))
				{
					CLH2_FreeEffect(index);
				}
				else
					CL_LinkEntity(ent);
				break;

			case H2CE_RIDER_DEATH:
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
					CLH2_RiderParticles(cl.h2_Effects[index].RD.stage+1,org);
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
//						cl.h2_Effects[index].RD.stage = 0;
						CLH2_FreeEffect(index);
					}
				}
				break;

			case H2CE_GRAVITYWELL:
			
				cl.h2_Effects[index].GravityWell.time_amount += frametime*2;
				if (cl.h2_Effects[index].GravityWell.time_amount >= 1)
					cl.h2_Effects[index].GravityWell.time_amount -= 1;
		
				VectorCopy(cl.h2_Effects[index].GravityWell.origin,org);
				org[0] += sin(cl.h2_Effects[index].GravityWell.time_amount * 2 * M_PI) * 30;
				org[1] += cos(cl.h2_Effects[index].GravityWell.time_amount * 2 * M_PI) * 30;

				if (cl.h2_Effects[index].GravityWell.lifetime < cl.serverTimeFloat)
				{
					CLH2_FreeEffect(index);
				}
				else
					CLH2_GravityWellParticle(rand()%8,org, cl.h2_Effects[index].GravityWell.color);

				break;

			case H2CE_TELEPORTERPUFFS:
				cl.h2_Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.h2_Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				while(cl.h2_Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->state.frame++;
					cl.h2_Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}
				cur_frame = ent->state.frame;

				if (cur_frame >= R_ModelNumFrames(ent->model))
				{
					CLH2_FreeEffect(index);
					break;
				}

				for (i=0;i<8;++i)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[i]];

					ent->state.origin[0] += (frametime/smoketime) * cl.h2_Effects[index].Teleporter.velocity[i][0];
					ent->state.origin[1] += (frametime/smoketime) * cl.h2_Effects[index].Teleporter.velocity[i][1];
					ent->state.origin[2] += (frametime/smoketime) * cl.h2_Effects[index].Teleporter.velocity[i][2];
					ent->state.frame = cur_frame;

					CL_LinkEntity(ent);
				}
				break;

			case H2CE_TELEPORTERBODY:
				cl.h2_Effects[index].Teleporter.time_amount += frametime;
				smoketime = cl.h2_Effects[index].Teleporter.framelength;

				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				while(cl.h2_Effects[index].Teleporter.time_amount >= HX_FRAME_TIME)
				{
					ent->state.scale -= 15;
					cl.h2_Effects[index].Teleporter.time_amount -= HX_FRAME_TIME;
				}

				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				ent->state.angles[1] += 45;

				if (ent->state.scale <= 10)
				{
					CLH2_FreeEffect(index);
				}
				else
				{
					CL_LinkEntity(ent);
				}
				break;

			case H2CE_BONESHARD:
			case H2CE_BONESHRAPNEL:
				cl.h2_Effects[index].Missile.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];

//		ent->angles[0] = cl.h2_Effects[index].Missile.angle[0];
//		ent->angles[1] = cl.h2_Effects[index].Missile.angle[1];
//		ent->angles[2] = cl.h2_Effects[index].Missile.angle[2];

				ent->state.angles[0] += frametime * cl.h2_Effects[index].Missile.avelocity[0];
				ent->state.angles[1] += frametime * cl.h2_Effects[index].Missile.avelocity[1];
				ent->state.angles[2] += frametime * cl.h2_Effects[index].Missile.avelocity[2];

				ent->state.origin[0] += frametime * cl.h2_Effects[index].Missile.velocity[0];
				ent->state.origin[1] += frametime * cl.h2_Effects[index].Missile.velocity[1];
				ent->state.origin[2] += frametime * cl.h2_Effects[index].Missile.velocity[2];

				CL_LinkEntity(ent);
				break;

			case H2CE_CHUNK:
				cl.h2_Effects[index].Chunk.time_amount -= frametime;
				if(cl.h2_Effects[index].Chunk.time_amount < 0)
				{
					CLH2_FreeEffect(index);	
				}
				else
				{
					for (i=0;i < cl.h2_Effects[index].Chunk.numChunks;i++)
					{
						vec3_t oldorg;
						int			moving = 1;

						ent = &EffectEntities[cl.h2_Effects[index].Chunk.entity_index[i]];

						VectorCopy(ent->state.origin, oldorg);

						ent->state.origin[0] += frametime * cl.h2_Effects[index].Chunk.velocity[i][0];
						ent->state.origin[1] += frametime * cl.h2_Effects[index].Chunk.velocity[i][1];
						ent->state.origin[2] += frametime * cl.h2_Effects[index].Chunk.velocity[i][2];

						if (CM_PointContentsQ1(ent->state.origin, 0) != BSP29CONTENTS_EMPTY) //||in_solid==true
						{
							// bouncing prolly won't work...
							VectorCopy(oldorg, ent->state.origin);

							cl.h2_Effects[index].Chunk.velocity[i][0] = 0;
							cl.h2_Effects[index].Chunk.velocity[i][1] = 0;
							cl.h2_Effects[index].Chunk.velocity[i][2] = 0;

							moving = 0;
						}
						else
						{
							ent->state.angles[0] += frametime * cl.h2_Effects[index].Chunk.avel[i%3][0];
							ent->state.angles[1] += frametime * cl.h2_Effects[index].Chunk.avel[i%3][1];
							ent->state.angles[2] += frametime * cl.h2_Effects[index].Chunk.avel[i%3][2];
						}

						if(cl.h2_Effects[index].Chunk.time_amount < frametime * 3)
						{	// chunk leaves in 3 frames
							ent->state.scale *= .7;
						}

						CL_LinkEntity(ent);

						cl.h2_Effects[index].Chunk.velocity[i][2] -= frametime * 500; // apply gravity

						switch(cl.h2_Effects[index].Chunk.type)
						{
						case H2THINGTYPE_GREYSTONE:
							break;
						case H2THINGTYPE_WOOD:
							break;
						case H2THINGTYPE_METAL:
							break;
						case H2THINGTYPE_FLESH:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_bloodshot);
							break;
						case H2THINGTYPE_FIRE:
							break;
						case H2THINGTYPE_CLAY:
						case H2THINGTYPE_BONE:
							break;
						case H2THINGTYPE_LEAVES:
							break;
						case H2THINGTYPE_HAY:
							break;
						case H2THINGTYPE_BROWNSTONE:
							break;
						case H2THINGTYPE_CLOTH:
							break;
						case H2THINGTYPE_WOOD_LEAF:
							break;
						case H2THINGTYPE_WOOD_METAL:
							break;
						case H2THINGTYPE_WOOD_STONE:
							break;
						case H2THINGTYPE_METAL_STONE:
							break;
						case H2THINGTYPE_METAL_CLOTH:
							break;
						case H2THINGTYPE_WEBS:
							break;
						case H2THINGTYPE_GLASS:
							break;
						case H2THINGTYPE_ICE:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_ice);
							break;
						case H2THINGTYPE_CLEARGLASS:
							break;
						case H2THINGTYPE_REDGLASS:
							break;
						case H2THINGTYPE_ACID:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_acidball);
							break;
						case H2THINGTYPE_METEOR:
							CLH2_TrailParticles (oldorg, ent->state.origin, rt_smoke);
							break;
						case H2THINGTYPE_GREENFLESH:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_acidball);
							break;

						}
					}
				}
				break;
		}
	}
}
