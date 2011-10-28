/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_tent.c -- client side temporary entities

#include "client.h"

/*
=================
CL_ParseParticles
=================
*/
void CL_ParseParticles (void)
{
	int		color, count;
	vec3_t	pos, dir;

	net_message.ReadPos(pos);
	MSG_ReadDir (&net_message, dir);

	color = net_message.ReadByte ();

	count = net_message.ReadByte ();

	CLQ2_ParticleEffect (pos, dir, color, count);
}


/*
=================
CL_ParseBeam
=================
*/
static int CL_ParseBeam ()
{
	int		ent;
	vec3_t	start, end;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);

	CLQ2_ParasiteBeam(ent, start, end);
	return ent;
}

/*
=================
CL_ParseBeam2
=================
*/
static int CL_ParseBeam2 ()
{
	int		ent;
	vec3_t	start, end, offset;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);
	net_message.ReadPos(offset);

	CLQ2_GrappleCableBeam(ent, start, end, offset);
	return ent;
}

// ROGUE
/*
=================
CL_ParsePlayerBeam
  - adds to the cl_playerbeam array instead of the clq2_beams array
=================
*/
static int CL_ParsePlayerBeam ()
{
	int		ent;
	vec3_t	start, end;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);

	CLQ2_HeatBeam(ent, start, end);
	return ent;
}

static int CL_ParsePlayerBeam2 ()
{
	int		ent;
	vec3_t	start, end;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);

	CLQ2_MonsterHeatBeam(ent, start, end);
	return ent;
}
//rogue

/*
=================
CL_ParseLightning
=================
*/
static int CL_ParseLightning ()
{
	int		srcEnt, destEnt;
	vec3_t	start, end;
	
	srcEnt = net_message.ReadShort();
	destEnt = net_message.ReadShort();

	net_message.ReadPos(start);
	net_message.ReadPos(end);

	CLQ2_LightningBeam(srcEnt, destEnt, start, end);
	return srcEnt;
}

/*
=================
CL_ParseLaser
=================
*/
void CL_ParseLaser(int colors)
{
	vec3_t start;
	net_message.ReadPos(start);
	vec3_t end;
	net_message.ReadPos(end);

	CLQ2_NewLaser(start, end, colors);
}

//=============
//ROGUE
void CL_ParseSteam (void)
{
	int id = net_message.ReadShort();		// an id of -1 is an instant effect
	int cnt = net_message.ReadByte();
	vec3_t pos;
	net_message.ReadPos(pos);
	vec3_t dir;
	MSG_ReadDir(&net_message, dir);
	int r = net_message.ReadByte();
	int magnitude = net_message.ReadShort();

	if (id != -1) // sustains
	{
		int interval = net_message.ReadLong();
		CLQ2_SustainParticleStream(id, cnt, pos, dir, r, magnitude, interval);
	}
	else // instant
	{
		int color = r & 0xff;
		CLQ2_ParticleSteamEffect(pos, dir, color, cnt, magnitude);
	}
}

void CL_ParseWidow (void)
{
	int id = net_message.ReadShort();
	vec3_t pos;
	net_message.ReadPos(pos);

	CLQ2_SustainWindow(id, pos);
}

void CL_ParseNuke (void)
{
	vec3_t pos;
	net_message.ReadPos(pos);

	CLQ2_SustainNuke(pos);
}

//ROGUE
//=============

/*
=================
CL_ParseTEnt
=================
*/
static byte splash_color[] = {0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8};

void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos, pos2, dir;
	int		cnt;
	int		color;
	int		r;
	int		ent;
	int		magnitude;

	type = net_message.ReadByte ();

	switch (type)
	{
	case TE_BLOOD:			// bullet hitting flesh
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		CLQ2_ParticleEffect (pos, dir, 0xe8, 60);
		break;

	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_GUNSHOT)
			CLQ2_ParticleEffect (pos, dir, 0, 40);
		else
			CLQ2_ParticleEffect (pos, dir, 0xe0, 6);

		if (type != TE_SPARKS)
		{
			CLQ2_SmokeAndFlash(pos);
			
			// impact sound
			cnt = rand()&15;
			if (cnt == 1)
				S_StartSound (pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				S_StartSound (pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				S_StartSound (pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}

		break;
		
	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_SCREEN_SPARKS)
			CLQ2_ParticleEffect (pos, dir, 0xd0, 40);
		else
			CLQ2_ParticleEffect (pos, dir, 0xb0, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
		
	case TE_SHOTGUN:			// bullet hitting wall
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		CLQ2_ParticleEffect (pos, dir, 0, 20);
		CLQ2_SmokeAndFlash(pos);
		break;

	case TE_SPLASH:			// bullet hitting water
		cnt = net_message.ReadByte ();
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		r = net_message.ReadByte ();
		if (r > 6)
			color = 0x00;
		else
			color = splash_color[r];
		CLQ2_ParticleEffect (pos, dir, color, cnt);

		if (r == SPLASH_SPARKS)
		{
			r = rand() & 3;
			if (r == 0)
				S_StartSound (pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
			else if (r == 1)
				S_StartSound (pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
			else
				S_StartSound (pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
		}
		break;

	case TE_LASER_SPARKS:
		cnt = net_message.ReadByte ();
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		color = net_message.ReadByte ();
		CLQ2_ParticleEffect2 (pos, dir, color, cnt);
		break;

	// RAFAEL
	case TE_BLUEHYPERBLASTER:
		net_message.ReadPos(pos);
		net_message.ReadPos(dir);
		CLQ2_BlasterParticles (pos, dir);
		break;

	case TE_BLASTER:			// blaster hitting wall
		net_message.ReadPos(pos);
		MSG_ReadDir(&net_message, dir);
		CLQ2_BlasterParticles(pos, dir);
		CLQ2_BlasterExplosion(pos, dir);
		S_StartSound(pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
		
	case TE_RAILTRAIL:			// railgun effect
		net_message.ReadPos(pos);
		net_message.ReadPos(pos2);
		CLQ2_RailTrail(pos, pos2);
		S_StartSound(pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_GrenadeExplosion(pos);
		CLQ2_ExplosionParticles(pos);
		S_StartSound(pos, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;

	case TE_GRENADE_EXPLOSION_WATER:
		net_message.ReadPos(pos);
		CLQ2_GrenadeExplosion(pos);
		CLQ2_ExplosionParticles(pos);
		S_StartSound(pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		break;

	// RAFAEL
	case TE_PLASMA_EXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_RocketExplosion(pos);
		CLQ2_ExplosionParticles (pos);
		S_StartSound(pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;
	
	case TE_EXPLOSION1:
	case TE_ROCKET_EXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_RocketExplosion(pos);
		CLQ2_ExplosionParticles(pos);									// PMM
		S_StartSound(pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1_BIG:						// PMM
		net_message.ReadPos(pos);
		CLQ2_BigExplosion(pos);
		S_StartSound(pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_ROCKET_EXPLOSION_WATER:
		net_message.ReadPos(pos);
		CLQ2_RocketExplosion(pos);
		CLQ2_ExplosionParticles (pos);									// PMM
		S_StartSound(pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1_NP:						// PMM
	case TE_PLAIN_EXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_RocketExplosion(pos);
		S_StartSound(pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_BFG_EXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_BfgExplosion(pos);
		break;

	case TE_BFG_BIGEXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_BFGExplosionParticles (pos);
		break;

	case TE_BFG_LASER:
		CL_ParseLaser (0xd0d1d2d3);
		break;

	case TE_BUBBLETRAIL:
		net_message.ReadPos(pos);
		net_message.ReadPos(pos2);
		CLQ2_BubbleTrail (pos, pos2);
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		ent = CL_ParseBeam ();
		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		net_message.ReadPos(pos);
		CLQ2_BigTeleportParticles (pos);
		S_StartSound (pos, 0, 0, S_RegisterSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_GRAPPLE_CABLE:
		ent = CL_ParseBeam2 ();
		break;

	// RAFAEL
	case TE_WELDING_SPARKS:
		cnt = net_message.ReadByte ();
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		color = net_message.ReadByte ();
		CLQ2_ParticleEffect2 (pos, dir, color, cnt);

		CLQ2_WeldingSparks(pos);
		break;

	case TE_GREENBLOOD:
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		CLQ2_ParticleEffect2 (pos, dir, 0xdf, 30);
		break;

	// RAFAEL
	case TE_TUNNEL_SPARKS:
		cnt = net_message.ReadByte ();
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		color = net_message.ReadByte ();
		CLQ2_ParticleEffect3 (pos, dir, color, cnt);
		break;

//=============
//PGM
		// PMM -following code integrated for flechette (different color)
	case TE_BLASTER2:			// green blaster hitting wall
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		
		CLQ2_BlasterParticles2 (pos, dir, 0xd0);
		CLQ2_Blaster2Explosion(pos, dir);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_FLECHETTE:			// flechette
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		
		CLQ2_BlasterParticles2(pos, dir, 0x6f); // 75
		CLQ2_FlechetteExplosion(pos, dir);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_LIGHTNING:
		ent = CL_ParseLightning ();
		S_StartSound (NULL, ent, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
		break;

	case TE_DEBUGTRAIL:
		net_message.ReadPos(pos);
		net_message.ReadPos(pos2);
		CLQ2_DebugTrail (pos, pos2);
		break;

	case TE_FLASHLIGHT:
		net_message.ReadPos(pos);
		ent = net_message.ReadShort();
		CLQ2_Flashlight(ent, pos);
		break;

	case TE_FORCEWALL:
		net_message.ReadPos(pos);
		net_message.ReadPos(pos2);
		color = net_message.ReadByte ();
		CLQ2_ForceWall(pos, pos2, color);
		break;

	case TE_HEATBEAM:
		ent = CL_ParsePlayerBeam ();
		break;

	case TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam2 ();
		break;

	case TE_HEATBEAM_SPARKS:
//		cnt = net_message.ReadByte ();
		cnt = 50;
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
//		r = net_message.ReadByte ();
//		magnitude = net_message.ReadShort();
		r = 8;
		magnitude = 60;
		color = r & 0xff;
		CLQ2_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
	
	case TE_HEATBEAM_STEAM:
//		cnt = net_message.ReadByte ();
		cnt = 20;
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
//		r = net_message.ReadByte ();
//		magnitude = net_message.ReadShort();
//		color = r & 0xff;
		color = 0xe0;
		magnitude = 60;
		CLQ2_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;

	case TE_BUBBLETRAIL2:
//		cnt = net_message.ReadByte ();
		cnt = 8;
		net_message.ReadPos(pos);
		net_message.ReadPos(pos2);
		CLQ2_BubbleTrail2 (pos, pos2, cnt);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_MOREBLOOD:
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		CLQ2_ParticleEffect (pos, dir, 0xe8, 250);
		break;

	case TE_CHAINFIST_SMOKE:
		dir[0]=0; dir[1]=0; dir[2]=1;
		net_message.ReadPos(pos);
		CLQ2_ParticleSmokeEffect (pos, dir, 0, 20, 20);
		break;

	case TE_ELECTRIC_SPARKS:
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
//		CLQ2_ParticleEffect (pos, dir, 109, 40);
		CLQ2_ParticleEffect (pos, dir, 0x75, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_TRACKER_EXPLOSION:
		net_message.ReadPos(pos);
		CLQ2_ColorFlash (0, pos, 150, -1, -1, -1);
		CLQ2_ColorExplosionParticles (pos, 0, 1);
//		CLQ2_Tracker_Explode (pos);
		S_StartSound (pos, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0);
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		net_message.ReadPos(pos);
		CLQ2_TeleportParticles (pos);
		break;

	case TE_WIDOWBEAMOUT:
		CL_ParseWidow ();
		break;

	case TE_NUKEBLAST:
		CL_ParseNuke ();
		break;

	case TE_WIDOWSPLASH:
		net_message.ReadPos(pos);
		CLQ2_WidowSplash (pos);
		break;
//PGM
//==============

	default:
		Com_Error (ERR_DROP, "CL_ParseTEnt: bad type");
	}
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts (void)
{
	CLQ2_AddBeams ();
	// PMM - draw plasma beams
	CLQ2_AddPlayerBeams ();
	CLQ2_AddExplosions ();
	CLQ2_AddLasers ();
	// PMM - set up sustain
	CLQ2_ProcessSustain();
}
