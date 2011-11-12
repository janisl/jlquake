
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
		case CEH2_RAIN:
		case CEH2_SNOW:
			DoTest = false;
			break;

		case CEH2_FOUNTAIN:
			DoTest = false;
			break;

		case CEH2_QUAKE:
			VectorCopy(sv.Effects[index].Quake.origin, TestO1);
			TestDistance = 700;
			break;

		case CEH2_WHITE_SMOKE:
		case CEH2_GREEN_SMOKE:
		case CEH2_GREY_SMOKE:
		case CEH2_RED_SMOKE:
		case CEH2_SLOW_WHITE_SMOKE:
		case CEH2_TELESMK1:
		case CEH2_TELESMK2:
		case CEH2_GHOST:
		case CEH2_REDCLOUD:
		case CEH2_FLAMESTREAM:
		case CEH2_ACID_MUZZFL:
		case CEH2_FLAMEWALL:
		case CEH2_FLAMEWALL2:
		case CEH2_ONFIRE:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CEH2_SM_WHITE_FLASH:
		case CEH2_YELLOWRED_FLASH:
		case CEH2_BLUESPARK:
		case CEH2_YELLOWSPARK:
		case CEH2_SM_CIRCLE_EXP:
		case CEH2_BG_CIRCLE_EXP:
		case CEH2_SM_EXPLOSION:
		case CEH2_LG_EXPLOSION:
		case CEH2_FLOOR_EXPLOSION:
		case CEH2_BLUE_EXPLOSION:
		case CEH2_REDSPARK:
		case CEH2_GREENSPARK:
		case CEH2_ICEHIT:
		case CEH2_MEDUSA_HIT:
		case CEH2_MEZZO_REFLECT:
		case CEH2_FLOOR_EXPLOSION2:
		case CEH2_XBOW_EXPLOSION:
		case CEH2_NEW_EXPLOSION:
		case CEH2_MAGIC_MISSILE_EXPLOSION:
		case CEH2_BONE_EXPLOSION:
		case CEH2_BLDRN_EXPL:
		case CEH2_ACID_HIT:
		case CEH2_LBALL_EXPL:
		case CEH2_FIREWALL_SMALL:
		case CEH2_FIREWALL_MEDIUM:
		case CEH2_FIREWALL_LARGE:
		case CEH2_ACID_SPLAT:
		case CEH2_ACID_EXPL:
		case CEH2_FBOOM:
		case CEH2_BRN_BOUNCE:
		case CEH2_LSHOCK:
		case CEH2_BOMB:
		case CEH2_FLOOR_EXPLOSION3:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CEH2_WHITE_FLASH:
		case CEH2_BLUE_FLASH:
		case CEH2_SM_BLUE_FLASH:
		case CEH2_RED_FLASH:
			VectorCopy(sv.Effects[index].Smoke.origin, TestO1);
			TestDistance = 250;
			break;

		case CEH2_RIDER_DEATH:
			DoTest = false;
			break;

		case CEH2_GRAVITYWELL:
			DoTest = false;
			break;

		case CEH2_TELEPORTERPUFFS:
			VectorCopy(sv.Effects[index].Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case CEH2_TELEPORTERBODY:
			VectorCopy(sv.Effects[index].Teleporter.origin, TestO1);
			TestDistance = 350;
			break;

		case CEH2_BONESHARD:
		case CEH2_BONESHRAPNEL:
			VectorCopy(sv.Effects[index].Missile.origin, TestO1);
			TestDistance = 600;
			break;
		case CEH2_CHUNK:
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
			case CEH2_RAIN:
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
				
			case CEH2_SNOW:
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
				
			case CEH2_FOUNTAIN:
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
				
			case CEH2_QUAKE:
				sb->WriteCoord(sv.Effects[index].Quake.origin[0]);
				sb->WriteCoord(sv.Effects[index].Quake.origin[1]);
				sb->WriteCoord(sv.Effects[index].Quake.origin[2]);
				sb->WriteFloat(sv.Effects[index].Quake.radius);
				break;
				
			case CEH2_WHITE_SMOKE:
			case CEH2_GREEN_SMOKE:
			case CEH2_GREY_SMOKE:
			case CEH2_RED_SMOKE:
			case CEH2_SLOW_WHITE_SMOKE:
			case CEH2_TELESMK1:
			case CEH2_TELESMK2:
			case CEH2_GHOST:
			case CEH2_REDCLOUD:
			case CEH2_FLAMESTREAM:
			case CEH2_ACID_MUZZFL:
			case CEH2_FLAMEWALL:
			case CEH2_FLAMEWALL2:
			case CEH2_ONFIRE:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[0]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[1]);
				sb->WriteFloat(sv.Effects[index].Smoke.velocity[2]);
				sb->WriteFloat(sv.Effects[index].Smoke.framelength);
				sb->WriteFloat(sv.Effects[index].Smoke.frame);
				break;
				
			case CEH2_SM_WHITE_FLASH:
			case CEH2_YELLOWRED_FLASH:
			case CEH2_BLUESPARK:
			case CEH2_YELLOWSPARK:
			case CEH2_SM_CIRCLE_EXP:
			case CEH2_BG_CIRCLE_EXP:
			case CEH2_SM_EXPLOSION:
			case CEH2_LG_EXPLOSION:
			case CEH2_FLOOR_EXPLOSION:
			case CEH2_FLOOR_EXPLOSION3:
			case CEH2_BLUE_EXPLOSION:
			case CEH2_REDSPARK:
			case CEH2_GREENSPARK:
			case CEH2_ICEHIT:
			case CEH2_MEDUSA_HIT:
			case CEH2_MEZZO_REFLECT:
			case CEH2_FLOOR_EXPLOSION2:
			case CEH2_XBOW_EXPLOSION:
			case CEH2_NEW_EXPLOSION:
			case CEH2_MAGIC_MISSILE_EXPLOSION:
			case CEH2_BONE_EXPLOSION:
			case CEH2_BLDRN_EXPL:
			case CEH2_ACID_HIT:
			case CEH2_ACID_SPLAT:
			case CEH2_ACID_EXPL:
			case CEH2_LBALL_EXPL:	
			case CEH2_FIREWALL_SMALL:
			case CEH2_FIREWALL_MEDIUM:
			case CEH2_FIREWALL_LARGE:
			case CEH2_FBOOM:
			case CEH2_BOMB:
			case CEH2_BRN_BOUNCE:
			case CEH2_LSHOCK:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				break;
				
			case CEH2_WHITE_FLASH:
			case CEH2_BLUE_FLASH:
			case CEH2_SM_BLUE_FLASH:
			case CEH2_RED_FLASH:
				sb->WriteCoord(sv.Effects[index].Smoke.origin[0]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[1]);
				sb->WriteCoord(sv.Effects[index].Smoke.origin[2]);
				break;
								
			case CEH2_RIDER_DEATH:
				sb->WriteCoord(sv.Effects[index].RD.origin[0]);
				sb->WriteCoord(sv.Effects[index].RD.origin[1]);
				sb->WriteCoord(sv.Effects[index].RD.origin[2]);
				break;
				
			case CEH2_TELEPORTERPUFFS:
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[0]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[1]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[2]);
				break;
				
			case CEH2_TELEPORTERBODY:
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[0]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[1]);
				sb->WriteCoord(sv.Effects[index].Teleporter.origin[2]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][0]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][1]);
				sb->WriteFloat(sv.Effects[index].Teleporter.velocity[0][2]);
				sb->WriteFloat(sv.Effects[index].Teleporter.skinnum);
				break;

			case CEH2_BONESHARD:
			case CEH2_BONESHRAPNEL:
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

			case CEH2_GRAVITYWELL:
				sb->WriteCoord(sv.Effects[index].GravityWell.origin[0]);
				sb->WriteCoord(sv.Effects[index].GravityWell.origin[1]);
				sb->WriteCoord(sv.Effects[index].GravityWell.origin[2]);
				sb->WriteShort(sv.Effects[index].GravityWell.color);
				sb->WriteFloat(sv.Effects[index].GravityWell.lifetime);
				break;

			case CEH2_CHUNK:
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
// SV_SaveEffects(), SV_LoadEffects(), CL_ParseEffect()
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
		case CEH2_RAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Rain.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Rain.max_org);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Rain.e_size);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].Rain.dir);
			sv.Effects[index].Rain.color = G_FLOAT(OFS_PARM5);
			sv.Effects[index].Rain.count = G_FLOAT(OFS_PARM6);
			sv.Effects[index].Rain.wait = G_FLOAT(OFS_PARM7);

			sv.Effects[index].Rain.next_time = 0;
			break;

		case CEH2_SNOW:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Snow.min_org);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Snow.max_org);
			sv.Effects[index].Snow.flags = G_FLOAT(OFS_PARM3);
			VectorCopy(G_VECTOR(OFS_PARM4),sv.Effects[index].Snow.dir);
			sv.Effects[index].Snow.count = G_FLOAT(OFS_PARM5);

			sv.Effects[index].Snow.next_time = 0;
			break;

		case CEH2_FOUNTAIN:
			VectorCopy(G_VECTOR(OFS_PARM1),sv.Effects[index].Fountain.pos);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Fountain.angle);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Fountain.movedir);
			sv.Effects[index].Fountain.color = G_FLOAT(OFS_PARM4);
			sv.Effects[index].Fountain.cnt = G_FLOAT(OFS_PARM5);
			break;

		case CEH2_QUAKE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Quake.origin);
			sv.Effects[index].Quake.radius = G_FLOAT(OFS_PARM2);
			break;

		case CEH2_WHITE_SMOKE:
		case CEH2_GREEN_SMOKE:
		case CEH2_GREY_SMOKE:
		case CEH2_RED_SMOKE:
		case CEH2_SLOW_WHITE_SMOKE:
		case CEH2_TELESMK1:
		case CEH2_TELESMK2:
		case CEH2_GHOST:
		case CEH2_REDCLOUD:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].Smoke.velocity);
			sv.Effects[index].Smoke.framelength = G_FLOAT(OFS_PARM3);
			sv.Effects[index].Smoke.frame = 0;
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CEH2_ACID_MUZZFL:
		case CEH2_FLAMESTREAM:
		case CEH2_FLAMEWALL:
		case CEH2_FLAMEWALL2:
		case CEH2_ONFIRE:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			VectorCopy(G_VECTOR(OFS_PARM2), sv.Effects[index].Smoke.velocity);
			sv.Effects[index].Smoke.framelength = 0.05;
			sv.Effects[index].Smoke.frame = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CEH2_SM_WHITE_FLASH:
		case CEH2_YELLOWRED_FLASH:
		case CEH2_BLUESPARK:
		case CEH2_YELLOWSPARK:
		case CEH2_SM_CIRCLE_EXP:
		case CEH2_BG_CIRCLE_EXP:
		case CEH2_SM_EXPLOSION:
		case CEH2_LG_EXPLOSION:
		case CEH2_FLOOR_EXPLOSION:
		case CEH2_FLOOR_EXPLOSION3:
		case CEH2_BLUE_EXPLOSION:
		case CEH2_REDSPARK:
		case CEH2_GREENSPARK:
		case CEH2_ICEHIT:
		case CEH2_MEDUSA_HIT:
		case CEH2_MEZZO_REFLECT:
		case CEH2_FLOOR_EXPLOSION2:
		case CEH2_XBOW_EXPLOSION:
		case CEH2_NEW_EXPLOSION:
		case CEH2_MAGIC_MISSILE_EXPLOSION:
		case CEH2_BONE_EXPLOSION:
		case CEH2_BLDRN_EXPL:
		case CEH2_ACID_HIT:
		case CEH2_ACID_SPLAT:
		case CEH2_ACID_EXPL:
		case CEH2_LBALL_EXPL:
		case CEH2_FIREWALL_SMALL:
		case CEH2_FIREWALL_MEDIUM:
		case CEH2_FIREWALL_LARGE:
		case CEH2_FBOOM:
		case CEH2_BOMB:
		case CEH2_BRN_BOUNCE:
		case CEH2_LSHOCK:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Smoke.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CEH2_WHITE_FLASH:
		case CEH2_BLUE_FLASH:
		case CEH2_SM_BLUE_FLASH:
		case CEH2_RED_FLASH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Flash.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CEH2_RIDER_DEATH:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].RD.origin);
			break;

		case CEH2_GRAVITYWELL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].GravityWell.origin);
			sv.Effects[index].GravityWell.color = G_FLOAT(OFS_PARM2);
			sv.Effects[index].GravityWell.lifetime = G_FLOAT(OFS_PARM3);
			break;

		case CEH2_TELEPORTERPUFFS:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Teleporter.origin);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CEH2_TELEPORTERBODY:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Teleporter.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Teleporter.velocity[0]);
			sv.Effects[index].Teleporter.skinnum = G_FLOAT(OFS_PARM3);
			sv.Effects[index].expire_time = sv.time + 1;
			break;

		case CEH2_BONESHARD:
		case CEH2_BONESHRAPNEL:
			VectorCopy(G_VECTOR(OFS_PARM1), sv.Effects[index].Missile.origin);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Missile.velocity);
			VectorCopy(G_VECTOR(OFS_PARM3),sv.Effects[index].Missile.angle);
			VectorCopy(G_VECTOR(OFS_PARM2),sv.Effects[index].Missile.avelocity);

			sv.Effects[index].expire_time = sv.time + 10;
			break;

		case CEH2_CHUNK:
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
				case CEH2_RAIN:
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

				case CEH2_SNOW:
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

				case CEH2_FOUNTAIN:
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

				case CEH2_QUAKE:
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Quake.origin[2]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Quake.radius);
					break;

				case CEH2_WHITE_SMOKE:
				case CEH2_GREEN_SMOKE:
				case CEH2_GREY_SMOKE:
				case CEH2_RED_SMOKE:
				case CEH2_SLOW_WHITE_SMOKE:
				case CEH2_TELESMK1:
				case CEH2_TELESMK2:
				case CEH2_GHOST:
				case CEH2_REDCLOUD:
				case CEH2_ACID_MUZZFL:
				case CEH2_FLAMESTREAM:
				case CEH2_FLAMEWALL:
				case CEH2_FLAMEWALL2:
				case CEH2_ONFIRE:
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[1]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.velocity[2]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.framelength);
					FS_Printf(FH, "%f\n", sv.Effects[index].Smoke.frame);
					break;

				case CEH2_SM_WHITE_FLASH:
				case CEH2_YELLOWRED_FLASH:
				case CEH2_BLUESPARK:
				case CEH2_YELLOWSPARK:
				case CEH2_SM_CIRCLE_EXP:
				case CEH2_BG_CIRCLE_EXP:
				case CEH2_SM_EXPLOSION:
				case CEH2_LG_EXPLOSION:
				case CEH2_FLOOR_EXPLOSION:
				case CEH2_FLOOR_EXPLOSION3:
				case CEH2_BLUE_EXPLOSION:
				case CEH2_REDSPARK:
				case CEH2_GREENSPARK:
				case CEH2_ICEHIT:
				case CEH2_MEDUSA_HIT:
				case CEH2_MEZZO_REFLECT:
				case CEH2_FLOOR_EXPLOSION2:
				case CEH2_XBOW_EXPLOSION:
				case CEH2_NEW_EXPLOSION:
				case CEH2_MAGIC_MISSILE_EXPLOSION:
				case CEH2_BONE_EXPLOSION:
				case CEH2_BLDRN_EXPL:
				case CEH2_BRN_BOUNCE:
				case CEH2_LSHOCK:
				case CEH2_ACID_HIT:
				case CEH2_ACID_SPLAT:
				case CEH2_ACID_EXPL:
				case CEH2_LBALL_EXPL:
				case CEH2_FIREWALL_SMALL:
				case CEH2_FIREWALL_MEDIUM:
				case CEH2_FIREWALL_LARGE:
				case CEH2_FBOOM:
				case CEH2_BOMB:
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Smoke.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Smoke.origin[2]);
					break;

				case CEH2_WHITE_FLASH:
				case CEH2_BLUE_FLASH:
				case CEH2_SM_BLUE_FLASH:
				case CEH2_RED_FLASH:
					FS_Printf(FH, "%f ", sv.Effects[index].Flash.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Flash.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Flash.origin[2]);
					break;

				case CEH2_RIDER_DEATH:
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].RD.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].RD.origin[2]);
					break;

				case CEH2_GRAVITYWELL:
					FS_Printf(FH, "%f ", sv.Effects[index].GravityWell.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].GravityWell.origin[1]);
					FS_Printf(FH, "%f", sv.Effects[index].GravityWell.origin[2]);
					FS_Printf(FH, "%d", sv.Effects[index].GravityWell.color);
					FS_Printf(FH, "%f\n", sv.Effects[index].GravityWell.lifetime);
					break;
				case CEH2_TELEPORTERPUFFS:
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Teleporter.origin[2]);
					break;

				case CEH2_TELEPORTERBODY:
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[0]);
					FS_Printf(FH, "%f ", sv.Effects[index].Teleporter.origin[1]);
					FS_Printf(FH, "%f\n", sv.Effects[index].Teleporter.origin[2]);
					break;

				case CEH2_BONESHARD:
				case CEH2_BONESHRAPNEL:
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

				case CEH2_CHUNK:
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
			case CEH2_RAIN:
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

			case CEH2_SNOW:
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

			case CEH2_FOUNTAIN:
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

			case CEH2_QUAKE:
				sv.Effects[index].Quake.origin[0] = GetFloat(Data);
				sv.Effects[index].Quake.origin[1] = GetFloat(Data);
				sv.Effects[index].Quake.origin[2] = GetFloat(Data);
				sv.Effects[index].Quake.radius = GetFloat(Data);
				break;

			case CEH2_WHITE_SMOKE:
			case CEH2_GREEN_SMOKE:
			case CEH2_GREY_SMOKE:
			case CEH2_RED_SMOKE:
			case CEH2_SLOW_WHITE_SMOKE:
			case CEH2_TELESMK1:
			case CEH2_TELESMK2:
			case CEH2_GHOST:
			case CEH2_REDCLOUD:
			case CEH2_ACID_MUZZFL:
			case CEH2_FLAMESTREAM:
			case CEH2_FLAMEWALL:
			case CEH2_FLAMEWALL2:
			case CEH2_ONFIRE:
				sv.Effects[index].Smoke.origin[0] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[1] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[2] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[0] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[1] = GetFloat(Data);
				sv.Effects[index].Smoke.velocity[2] = GetFloat(Data);
				sv.Effects[index].Smoke.framelength = GetFloat(Data);
				sv.Effects[index].Smoke.frame = GetFloat(Data);
				break;

			case CEH2_SM_WHITE_FLASH:
			case CEH2_YELLOWRED_FLASH:
			case CEH2_BLUESPARK:
			case CEH2_YELLOWSPARK:
			case CEH2_SM_CIRCLE_EXP:
			case CEH2_BG_CIRCLE_EXP:
			case CEH2_SM_EXPLOSION:
			case CEH2_LG_EXPLOSION:
			case CEH2_FLOOR_EXPLOSION:
			case CEH2_FLOOR_EXPLOSION3:
			case CEH2_BLUE_EXPLOSION:
			case CEH2_REDSPARK:
			case CEH2_GREENSPARK:
			case CEH2_ICEHIT:
			case CEH2_MEDUSA_HIT:
			case CEH2_MEZZO_REFLECT:
			case CEH2_FLOOR_EXPLOSION2:
			case CEH2_XBOW_EXPLOSION:
			case CEH2_NEW_EXPLOSION:
			case CEH2_MAGIC_MISSILE_EXPLOSION:
			case CEH2_BONE_EXPLOSION:
			case CEH2_BLDRN_EXPL:
			case CEH2_BRN_BOUNCE:
			case CEH2_LSHOCK:
			case CEH2_ACID_HIT:
			case CEH2_ACID_SPLAT:
			case CEH2_ACID_EXPL:
			case CEH2_LBALL_EXPL:
			case CEH2_FBOOM:
			case CEH2_FIREWALL_SMALL:
			case CEH2_FIREWALL_MEDIUM:
			case CEH2_FIREWALL_LARGE:
			case CEH2_BOMB:
				sv.Effects[index].Smoke.origin[0] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[1] = GetFloat(Data);
				sv.Effects[index].Smoke.origin[2] = GetFloat(Data);
				break;

			case CEH2_WHITE_FLASH:
			case CEH2_BLUE_FLASH:
			case CEH2_SM_BLUE_FLASH:
			case CEH2_RED_FLASH:
				sv.Effects[index].Flash.origin[0] = GetFloat(Data);
				sv.Effects[index].Flash.origin[1] = GetFloat(Data);
				sv.Effects[index].Flash.origin[2] = GetFloat(Data);
				break;

			case CEH2_RIDER_DEATH:
				sv.Effects[index].RD.origin[0] = GetFloat(Data);
				sv.Effects[index].RD.origin[1] = GetFloat(Data);
				sv.Effects[index].RD.origin[2] = GetFloat(Data);
				break;

			case CEH2_GRAVITYWELL:
				sv.Effects[index].GravityWell.origin[0] = GetFloat(Data);
				sv.Effects[index].GravityWell.origin[1] = GetFloat(Data);
				sv.Effects[index].GravityWell.origin[2] = GetFloat(Data);
				sv.Effects[index].GravityWell.color = GetInt(Data);
				sv.Effects[index].GravityWell.lifetime = GetFloat(Data);
				break;

			case CEH2_TELEPORTERPUFFS:
				sv.Effects[index].Teleporter.origin[0] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[1] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[2] = GetFloat(Data);
				break;

			case CEH2_TELEPORTERBODY:
				sv.Effects[index].Teleporter.origin[0] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[1] = GetFloat(Data);
				sv.Effects[index].Teleporter.origin[2] = GetFloat(Data);
				break;

			case CEH2_BONESHARD:
			case CEH2_BONESHRAPNEL:
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

			case CEH2_CHUNK:
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
	effect_entity_t* ent;
	int dir;
	float	angleval, sinval, cosval;
	float skinnum;
	float final;

	ImmediateFree = false;

	index = net_message.ReadByte();
	if (cl.h2_Effects[index].type)
		CLH2_FreeEffect(index);

	Com_Memset(&cl.h2_Effects[index],0,sizeof(struct h2EffectT));

	cl.h2_Effects[index].type = net_message.ReadByte();

	switch(cl.h2_Effects[index].type)
	{
		case CEH2_RAIN:
			CLH2_ParseEffectRain(index, net_message);
			break;

		case CEH2_SNOW:
			CLH2_ParseEffectSnow(index, net_message);
			break;

		case CEH2_FOUNTAIN:
			CLH2_ParseEffectFountain(index, net_message);
			break;

		case CEH2_QUAKE:
			CLH2_ParseEffectQuake(index, net_message);
			break;

		case CEH2_WHITE_SMOKE:
		case CEH2_SLOW_WHITE_SMOKE:
			ImmediateFree = CLH2_ParseEffectWhiteSmoke(index, net_message);
			break;
		case CEH2_GREEN_SMOKE:
			ImmediateFree = !CLH2_ParseEffectGreenSmoke(index, net_message);
			break;
		case CEH2_GREY_SMOKE:
			ImmediateFree = !CLH2_ParseEffectGraySmoke(index, net_message);
			break;
		case CEH2_RED_SMOKE:
			ImmediateFree = !CLH2_ParseEffectRedSmoke(index, net_message);
			break;
		case CEH2_TELESMK1:
			ImmediateFree = !CLH2_ParseEffectTeleportSmoke1(index, net_message);
			break;
		case CEH2_TELESMK2:
			ImmediateFree = !CLH2_ParseEffectTeleportSmoke2(index, net_message);
			break;
		case CEH2_GHOST:
			ImmediateFree = !CLH2_ParseEffectGhost(index, net_message);
			break;
		case CEH2_REDCLOUD:
			ImmediateFree = !CLH2_ParseEffectRedCloud(index, net_message);
			break;
		case CEH2_ACID_MUZZFL:
			ImmediateFree = !CLH2_ParseEffectAcidMuzzleFlash(index, net_message);
			break;
		case CEH2_FLAMESTREAM:
			ImmediateFree = !CLH2_ParseEffectFlameStream(index, net_message);
			break;
		case CEH2_FLAMEWALL:
			ImmediateFree = !CLH2_ParseEffectFlameWall(index, net_message);
			break;
		case CEH2_FLAMEWALL2:
			ImmediateFree = !CLH2_ParseEffectFlameWall2(index, net_message);
			break;
		case CEH2_ONFIRE:
			ImmediateFree = !CLH2_ParseEffectOnFire(index, net_message);
			break;

		case CEH2_SM_WHITE_FLASH:
			ImmediateFree = !CLH2_ParseEffectSmallWhiteFlash(index, net_message);
			break;
		case CEH2_YELLOWRED_FLASH:
			ImmediateFree = !CLH2_ParseEffectYellowRedFlash(index, net_message);
			break;
		case CEH2_BLUESPARK:
			ImmediateFree = !CLH2_ParseEffectBlueSpark(index, net_message);
			break;
		case CEH2_YELLOWSPARK:
		case CEH2_BRN_BOUNCE:
			ImmediateFree = !CLH2_ParseEffectYellowSpark(index, net_message);
			break;
		case CEH2_SM_CIRCLE_EXP:
			ImmediateFree = !CLH2_ParseEffectSmallCircleExplosion(index, net_message);
			break;
		case CEH2_BG_CIRCLE_EXP:
			ImmediateFree = !CLH2_ParseEffectBigCircleExplosion(index, net_message);
			break;
		case CEH2_SM_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectSmallExplosion(index, net_message);
			break;
		case CEH2_LG_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectBigExplosion(index, net_message);
			break;
		case CEH2_FLOOR_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectFloorExplosion(index, net_message);
			break;
		case CEH2_FLOOR_EXPLOSION3:
			ImmediateFree = !CLH2_ParseEffectFloorExplosion3(index, net_message);
			break;
		case CEH2_BLUE_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectBlueExplosion(index, net_message);
			break;
		case CEH2_REDSPARK:
			ImmediateFree = !CLH2_ParseEffectRedSpark(index, net_message);
			break;
		case CEH2_GREENSPARK:
			ImmediateFree = !CLH2_ParseEffectGreenSpark(index, net_message);
			break;
		case CEH2_ICEHIT:
			ImmediateFree = !CLH2_ParseEffectIceHit(index, net_message);
			break;
		case CEH2_MEDUSA_HIT:
			ImmediateFree = !CLH2_ParseEffectMedusaHit(index, net_message);
			break;
		case CEH2_MEZZO_REFLECT:
			ImmediateFree = !CLH2_ParseEffectMezzoReflect(index, net_message);
			break;
		case CEH2_FLOOR_EXPLOSION2:
			ImmediateFree = !CLH2_ParseEffectFloorExplosion2(index, net_message);
			break;
		case CEH2_XBOW_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectXBowExplosion(index, net_message);
			break;
		case CEH2_NEW_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectNewExplosion(index, net_message);
			break;
		case CEH2_MAGIC_MISSILE_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectMagicMissileExplosion(index, net_message);
			break;
		case CEH2_BONE_EXPLOSION:
			ImmediateFree = !CLH2_ParseEffectBoneExplosion(index, net_message);
			break;
		case CEH2_BLDRN_EXPL:
			ImmediateFree = !CLH2_ParseEffectBldrnExplosion(index, net_message);
			break;
		case CEH2_LSHOCK:
			ImmediateFree = !CLH2_ParseEffectLShock(index, net_message);
			break;
		case CEH2_ACID_HIT:
			ImmediateFree = !CLH2_ParseEffectAcidHit(index, net_message);
			break;
		case CEH2_ACID_SPLAT:
			ImmediateFree = !CLH2_ParseEffectAcidSplat(index, net_message);
			break;
		case CEH2_ACID_EXPL:
			ImmediateFree = !CLH2_ParseEffectAcidExplosion(index, net_message);
			break;
		case CEH2_LBALL_EXPL:
			ImmediateFree = !CLH2_ParseEffectLBallExplosion(index, net_message);
			break;
		case CEH2_FBOOM:
			ImmediateFree = !CLH2_ParseEffectFBoom(index, net_message);
			break;
		case CEH2_BOMB:
			ImmediateFree = !CLH2_ParseEffectBomb(index, net_message);
			break;
		case CEH2_FIREWALL_SMALL:
			ImmediateFree = !CLH2_ParseEffectFirewallSall(index, net_message);
			break;
		case CEH2_FIREWALL_MEDIUM:
			ImmediateFree = !CLH2_ParseEffectFirewallMedium(index, net_message);
			break;
		case CEH2_FIREWALL_LARGE:
			ImmediateFree = !CLH2_ParseEffectFirewallLarge(index, net_message);
			break;

		case CEH2_WHITE_FLASH:
		case CEH2_BLUE_FLASH:
		case CEH2_SM_BLUE_FLASH:
		case CEH2_RED_FLASH:
			cl.h2_Effects[index].Flash.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Flash.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Flash.origin[2] = net_message.ReadCoord ();
			cl.h2_Effects[index].Flash.reverse = 0;
			if ((cl.h2_Effects[index].Flash.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Flash.entity_index];
				VectorCopy(cl.h2_Effects[index].Flash.origin, ent->state.origin);

				if (cl.h2_Effects[index].type == CEH2_WHITE_FLASH)
					ent->model = R_RegisterModel("models/gryspt.spr");
				else if (cl.h2_Effects[index].type == CEH2_BLUE_FLASH)
					ent->model = R_RegisterModel("models/bluflash.spr");
				else if (cl.h2_Effects[index].type == CEH2_SM_BLUE_FLASH)
					ent->model = R_RegisterModel("models/sm_blue.spr");
				else if (cl.h2_Effects[index].type == CEH2_RED_FLASH)
					ent->model = R_RegisterModel("models/redspt.spr");

				ent->state.drawflags = H2DRF_TRANSLUCENT;

			}
			else
			{
				ImmediateFree = true;
			}
			break;

		case CEH2_RIDER_DEATH:
			cl.h2_Effects[index].RD.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].RD.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].RD.origin[2] = net_message.ReadCoord ();
			break;

		case CEH2_GRAVITYWELL:
			cl.h2_Effects[index].GravityWell.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].GravityWell.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].GravityWell.origin[2] = net_message.ReadCoord ();
			cl.h2_Effects[index].GravityWell.color = net_message.ReadShort ();
			cl.h2_Effects[index].GravityWell.lifetime = net_message.ReadFloat ();
			break;

		case CEH2_TELEPORTERPUFFS:
			cl.h2_Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();
				
			cl.h2_Effects[index].Teleporter.framelength = .05;
			dir = 0;
			for (i=0;i<8;++i)
			{		
				if ((cl.h2_Effects[index].Teleporter.entity_index[i] = CLH2_NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[i]];
					VectorCopy(cl.h2_Effects[index].Teleporter.origin, ent->state.origin);

					angleval = dir * M_PI*2 / 360;

					sinval = sin(angleval);
					cosval = cos(angleval);

					cl.h2_Effects[index].Teleporter.velocity[i][0] = 10*cosval;
					cl.h2_Effects[index].Teleporter.velocity[i][1] = 10*sinval;
					cl.h2_Effects[index].Teleporter.velocity[i][2] = 0;
					dir += 45;

					ent->model = R_RegisterModel("models/telesmk2.spr");
					ent->state.drawflags = H2DRF_TRANSLUCENT;
				}
			}
			break;

		case CEH2_TELEPORTERBODY:
			cl.h2_Effects[index].Teleporter.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Teleporter.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Teleporter.velocity[0][0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Teleporter.velocity[0][1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Teleporter.velocity[0][2] = net_message.ReadFloat ();

			skinnum = net_message.ReadFloat ();
			
			cl.h2_Effects[index].Teleporter.framelength = .05;
			dir = 0;
			if ((cl.h2_Effects[index].Teleporter.entity_index[0] = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Teleporter.entity_index[0]];
				VectorCopy(cl.h2_Effects[index].Teleporter.origin, ent->state.origin);

				ent->model = R_RegisterModel("models/teleport.mdl");
				ent->state.drawflags = H2SCALE_TYPE_XYONLY | H2DRF_TRANSLUCENT;
				ent->state.scale = 100;
				ent->state.skinnum = skinnum;
			}
			break;

		case CEH2_BONESHARD:
		case CEH2_BONESHRAPNEL:
			cl.h2_Effects[index].Missile.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Missile.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Missile.velocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.velocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.velocity[2] = net_message.ReadFloat ();

			cl.h2_Effects[index].Missile.angle[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.angle[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.angle[2] = net_message.ReadFloat ();

			cl.h2_Effects[index].Missile.avelocity[0] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.avelocity[1] = net_message.ReadFloat ();
			cl.h2_Effects[index].Missile.avelocity[2] = net_message.ReadFloat ();

			if ((cl.h2_Effects[index].Missile.entity_index = CLH2_NewEffectEntity()) != -1)
			{
				ent = &EffectEntities[cl.h2_Effects[index].Missile.entity_index];
				VectorCopy(cl.h2_Effects[index].Missile.origin, ent->state.origin);
				if (cl.h2_Effects[index].type == CEH2_BONESHARD)
					ent->model = R_RegisterModel("models/boneshot.mdl");
				else if (cl.h2_Effects[index].type == CEH2_BONESHRAPNEL)
					ent->model = R_RegisterModel("models/boneshrd.mdl");
			}
			else
				ImmediateFree = true;
			break;

		case CEH2_CHUNK:
			cl.h2_Effects[index].Chunk.origin[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chunk.origin[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chunk.origin[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Chunk.type = net_message.ReadByte ();

			cl.h2_Effects[index].Chunk.srcVel[0] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chunk.srcVel[1] = net_message.ReadCoord ();
			cl.h2_Effects[index].Chunk.srcVel[2] = net_message.ReadCoord ();

			cl.h2_Effects[index].Chunk.numChunks = net_message.ReadByte ();

			cl.h2_Effects[index].Chunk.time_amount = 4.0;

			cl.h2_Effects[index].Chunk.aveScale = 30 + 100 * (cl.h2_Effects[index].Chunk.numChunks / 40.0);

			if(cl.h2_Effects[index].Chunk.numChunks > 16)cl.h2_Effects[index].Chunk.numChunks = 16;

			for (i=0;i < cl.h2_Effects[index].Chunk.numChunks;i++)
			{		
				if ((cl.h2_Effects[index].Chunk.entity_index[i] = CLH2_NewEffectEntity()) != -1)
				{
					ent = &EffectEntities[cl.h2_Effects[index].Chunk.entity_index[i]];
					VectorCopy(cl.h2_Effects[index].Chunk.origin, ent->state.origin);

					VectorCopy(cl.h2_Effects[index].Chunk.srcVel, cl.h2_Effects[index].Chunk.velocity[i]);
					VectorScale(cl.h2_Effects[index].Chunk.velocity[i], .80 + ((rand()%4)/10.0), cl.h2_Effects[index].Chunk.velocity[i]);
					// temp modify them...
					cl.h2_Effects[index].Chunk.velocity[i][0] += (rand()%140)-70;
					cl.h2_Effects[index].Chunk.velocity[i][1] += (rand()%140)-70;
					cl.h2_Effects[index].Chunk.velocity[i][2] += (rand()%140)-70;

					// are these in degrees or radians?
					ent->state.angles[0] = rand()%360;
					ent->state.angles[1] = rand()%360;
					ent->state.angles[2] = rand()%360;

					ent->state.scale = cl.h2_Effects[index].Chunk.aveScale + rand()%40;

					// make this overcomplicated
					final = (rand()%100)*.01;
					if ((cl.h2_Effects[index].Chunk.type==THINGTYPEH2_GLASS) || (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_REDGLASS) || 
							(cl.h2_Effects[index].Chunk.type==THINGTYPEH2_CLEARGLASS) || (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_WEBS))
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

						if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_CLEARGLASS)
						{
							ent->state.skinnum=1;
							ent->state.drawflags |= H2DRF_TRANSLUCENT;
						}
						else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_REDGLASS)
						{
							ent->state.skinnum=2;
						}
						else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_WEBS)
						{
							ent->state.skinnum=3;
							ent->state.drawflags |= H2DRF_TRANSLUCENT;
						}
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_WOOD)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_METAL)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_FLESH)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/flesh1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/flesh2.mdl");
						else
							ent->model = R_RegisterModel ("models/flesh3.mdl");
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_BROWNSTONE)
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/schunk1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/schunk2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/schunk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/schunk4.mdl");
						ent->state.skinnum = 1;
					}
					else if ((cl.h2_Effects[index].Chunk.type==THINGTYPEH2_CLAY) || (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_BONE))
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/clshard1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/clshard2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/clshard3.mdl");
						else 
							ent->model = R_RegisterModel ("models/clshard4.mdl");
						if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_BONE)
						{
							ent->state.skinnum=1;//bone skin is second
						}
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_LEAVES)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/leafchk1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/leafchk2.mdl");
						else 
							ent->model = R_RegisterModel ("models/leafchk3.mdl");
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_HAY)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/hay1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/hay2.mdl");
						else 
							ent->model = R_RegisterModel ("models/hay3.mdl");
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_CLOTH)
					{
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/clthchk1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/clthchk2.mdl");
						else 
							ent->model = R_RegisterModel ("models/clthchk3.mdl");
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_WOOD_LEAF)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_WOOD_METAL)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_WOOD_STONE)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_METAL_STONE)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_METAL_CLOTH)
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
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_ICE)
					{
						ent->model = R_RegisterModel("models/shard.mdl");
						ent->state.skinnum=0;
						ent->state.frame = rand()%2;
						ent->state.drawflags |= H2DRF_TRANSLUCENT|H2MLS_ABSLIGHT;
						ent->state.abslight = 0.5;
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_METEOR)
					{
						ent->model = R_RegisterModel("models/tempmetr.mdl");
						ent->state.skinnum = 0;
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_ACID)
					{	// no spinning if possible...
						ent->model = R_RegisterModel("models/sucwp2p.mdl");
						ent->state.skinnum = 0;
					}
					else if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_GREENFLESH)
					{	// spider guts
						if (final < 0.33)
							ent->model = R_RegisterModel ("models/sflesh1.mdl");
						else if (final < 0.66)
							ent->model = R_RegisterModel ("models/sflesh2.mdl");
						else
							ent->model = R_RegisterModel ("models/sflesh3.mdl");

						ent->state.skinnum = 0;
					}
					else// if (cl.h2_Effects[index].Chunk.type==THINGTYPEH2_GREYSTONE)
					{
						if (final < 0.25)
							ent->model = R_RegisterModel ("models/schunk1.mdl");
						else if (final < 0.50)
							ent->model = R_RegisterModel ("models/schunk2.mdl");
						else if (final < 0.75)
							ent->model = R_RegisterModel ("models/schunk3.mdl");
						else 
							ent->model = R_RegisterModel ("models/schunk4.mdl");
						ent->state.skinnum = 0;
					}
				}
			}
			for(i=0; i < 3; i++)
			{
				cl.h2_Effects[index].Chunk.avel[i][0] = rand()%850 - 425;
				cl.h2_Effects[index].Chunk.avel[i][1] = rand()%850 - 425;
				cl.h2_Effects[index].Chunk.avel[i][2] = rand()%850 - 425;
			}

			break;

		default:
			Sys_Error ("CL_ParseEffect: bad type");
	}

	if (ImmediateFree)
	{
		cl.h2_Effects[index].type = CEH2_NONE;
	}
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
	CL_SetRefEntAxis(&rent, ent->state.angles, ent->state.scale, 0, ent->state.abslight, ent->state.drawflags);
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
			case CEH2_RAIN:
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

			case CEH2_SNOW:
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

			case CEH2_FOUNTAIN:
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

			case CEH2_QUAKE:
				CLH2_RunQuakeEffect (cl.h2_Effects[index].Quake.origin,cl.h2_Effects[index].Quake.radius);
				break;

			case CEH2_WHITE_SMOKE:
			case CEH2_GREEN_SMOKE:
			case CEH2_GREY_SMOKE:
			case CEH2_RED_SMOKE:
			case CEH2_SLOW_WHITE_SMOKE:
			case CEH2_TELESMK1:
			case CEH2_TELESMK2:
			case CEH2_GHOST:
			case CEH2_REDCLOUD:
			case CEH2_FLAMESTREAM:
			case CEH2_ACID_MUZZFL:
			case CEH2_FLAMEWALL:
			case CEH2_FLAMEWALL2:
			case CEH2_ONFIRE:
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
			case CEH2_SM_WHITE_FLASH:
			case CEH2_YELLOWRED_FLASH:
			case CEH2_BLUESPARK:
			case CEH2_YELLOWSPARK:
			case CEH2_SM_CIRCLE_EXP:
			case CEH2_BG_CIRCLE_EXP:
			case CEH2_SM_EXPLOSION:
			case CEH2_LG_EXPLOSION:
			case CEH2_FLOOR_EXPLOSION:
			case CEH2_FLOOR_EXPLOSION3:
			case CEH2_BLUE_EXPLOSION:
			case CEH2_REDSPARK:
			case CEH2_GREENSPARK:
			case CEH2_ICEHIT:
			case CEH2_MEDUSA_HIT:
			case CEH2_MEZZO_REFLECT:
			case CEH2_FLOOR_EXPLOSION2:
			case CEH2_XBOW_EXPLOSION:
			case CEH2_NEW_EXPLOSION:
			case CEH2_MAGIC_MISSILE_EXPLOSION:
			case CEH2_BONE_EXPLOSION:
			case CEH2_BLDRN_EXPL:
			case CEH2_BRN_BOUNCE:
			case CEH2_ACID_HIT:
			case CEH2_ACID_SPLAT:
			case CEH2_ACID_EXPL:
			case CEH2_LBALL_EXPL:
			case CEH2_FBOOM:
			case CEH2_BOMB:
			case CEH2_FIREWALL_SMALL:
			case CEH2_FIREWALL_MEDIUM:
			case CEH2_FIREWALL_LARGE:

				cl.h2_Effects[index].Smoke.time_amount += frametime;
				ent = &EffectEntities[cl.h2_Effects[index].Smoke.entity_index];

				if (cl.h2_Effects[index].type != CEH2_BG_CIRCLE_EXP)
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


			case CEH2_LSHOCK:
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
			case CEH2_WHITE_FLASH:
			case CEH2_BLUE_FLASH:
			case CEH2_SM_BLUE_FLASH:
			case CEH2_RED_FLASH:
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

			case CEH2_RIDER_DEATH:
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

			case CEH2_GRAVITYWELL:
			
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

			case CEH2_TELEPORTERPUFFS:
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

			case CEH2_TELEPORTERBODY:
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

			case CEH2_BONESHARD:
			case CEH2_BONESHRAPNEL:
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

			case CEH2_CHUNK:
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
						case THINGTYPEH2_GREYSTONE:
							break;
						case THINGTYPEH2_WOOD:
							break;
						case THINGTYPEH2_METAL:
							break;
						case THINGTYPEH2_FLESH:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_bloodshot);
							break;
						case THINGTYPEH2_FIRE:
							break;
						case THINGTYPEH2_CLAY:
						case THINGTYPEH2_BONE:
							break;
						case THINGTYPEH2_LEAVES:
							break;
						case THINGTYPEH2_HAY:
							break;
						case THINGTYPEH2_BROWNSTONE:
							break;
						case THINGTYPEH2_CLOTH:
							break;
						case THINGTYPEH2_WOOD_LEAF:
							break;
						case THINGTYPEH2_WOOD_METAL:
							break;
						case THINGTYPEH2_WOOD_STONE:
							break;
						case THINGTYPEH2_METAL_STONE:
							break;
						case THINGTYPEH2_METAL_CLOTH:
							break;
						case THINGTYPEH2_WEBS:
							break;
						case THINGTYPEH2_GLASS:
							break;
						case THINGTYPEH2_ICE:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_ice);
							break;
						case THINGTYPEH2_CLEARGLASS:
							break;
						case THINGTYPEH2_REDGLASS:
							break;
						case THINGTYPEH2_ACID:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_acidball);
							break;
						case THINGTYPEH2_METEOR:
							CLH2_TrailParticles (oldorg, ent->state.origin, rt_smoke);
							break;
						case THINGTYPEH2_GREENFLESH:
							if(moving)CLH2_TrailParticles (oldorg, ent->state.origin, rt_acidball);
							break;

						}
					}
				}
				break;
		}
	}
}
