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




#define	MAX_BEAMS	32
typedef struct
{
	int		entity;
	int		dest_entity;
	qhandle_t	model;
	int		endtime;
	vec3_t	offset;
	vec3_t	start, end;
} beam_t;
beam_t		cl_beams[MAX_BEAMS];
//PMM - added this for player-linked beams.  Currently only used by the plasma beam
beam_t		cl_playerbeams[MAX_BEAMS];


//ROGUE
q2cl_sustain_t	cl_sustains[MAX_SUSTAINS];
//ROGUE

// RAFAEL
void CL_BlueBlasterParticles (vec3_t org, vec3_t dir);

static sfxHandle_t	cl_sfx_ric1;
static sfxHandle_t	cl_sfx_ric2;
static sfxHandle_t	cl_sfx_ric3;
static sfxHandle_t	cl_sfx_lashit;
static sfxHandle_t	cl_sfx_spark5;
static sfxHandle_t	cl_sfx_spark6;
static sfxHandle_t	cl_sfx_spark7;
static sfxHandle_t	cl_sfx_railg;
static sfxHandle_t	cl_sfx_rockexp;
static sfxHandle_t	cl_sfx_grenexp;
static sfxHandle_t	cl_sfx_watrexp;
sfxHandle_t			cl_sfx_footsteps[4];

static qhandle_t	cl_mod_parasite_segment;
static qhandle_t	cl_mod_grapple_cable;
static qhandle_t	cl_mod_parasite_tip;
qhandle_t			cl_mod_powerscreen;

//ROGUE
static sfxHandle_t	cl_sfx_lightning;
static sfxHandle_t	cl_sfx_disrexp;
static qhandle_t	cl_mod_lightning;
static qhandle_t	cl_mod_heatbeam;
static qhandle_t	cl_mod_monster_heatbeam;

//ROGUE
/*
=================
CL_RegisterTEntSounds
=================
*/
void CL_RegisterTEntSounds (void)
{
	int		i;
	char	name[MAX_QPATH];

	// PMM - version stuff
//	Com_Printf ("%s\n", ROGUE_VERSION_STRING);
	// PMM
	cl_sfx_ric1 = S_RegisterSound ("world/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound ("world/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound ("world/ric3.wav");
	cl_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
	cl_sfx_spark5 = S_RegisterSound ("world/spark5.wav");
	cl_sfx_spark6 = S_RegisterSound ("world/spark6.wav");
	cl_sfx_spark7 = S_RegisterSound ("world/spark7.wav");
	cl_sfx_railg = S_RegisterSound ("weapons/railgf1a.wav");
	cl_sfx_rockexp = S_RegisterSound ("weapons/rocklx1a.wav");
	cl_sfx_grenexp = S_RegisterSound ("weapons/grenlx1a.wav");
	cl_sfx_watrexp = S_RegisterSound ("weapons/xpld_wat.wav");
	// RAFAEL
	// cl_sfx_plasexp = S_RegisterSound ("weapons/plasexpl.wav");
	S_RegisterSound ("player/land1.wav");

	S_RegisterSound ("player/fall2.wav");
	S_RegisterSound ("player/fall1.wav");

	for (i=0 ; i<4 ; i++)
	{
		String::Sprintf (name, sizeof(name), "player/step%i.wav", i+1);
		cl_sfx_footsteps[i] = S_RegisterSound (name);
	}

//PGM
	cl_sfx_lightning = S_RegisterSound ("weapons/tesla.wav");
	cl_sfx_disrexp = S_RegisterSound ("weapons/disrupthit.wav");
	// version stuff
	sprintf (name, "weapons/sound%d.wav", ROGUE_VERSION_ID);
	if (name[0] == 'w')
		name[0] = 'W';
//PGM
}	

/*
=================
CL_RegisterTEntModels
=================
*/
void CL_RegisterTEntModels (void)
{
	CLQ2_RegisterExplosionModels();
	cl_mod_parasite_segment = R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	cl_mod_grapple_cable = R_RegisterModel("models/ctf/segment/tris.md2");
	cl_mod_parasite_tip = R_RegisterModel("models/monsters/parasite/tip/tris.md2");
	cl_mod_powerscreen = R_RegisterModel("models/items/armor/effect/tris.md2");

R_RegisterModel("models/objects/laser/tris.md2");
R_RegisterModel("models/objects/grenade2/tris.md2");
R_RegisterModel("models/weapons/v_machn/tris.md2");
R_RegisterModel("models/weapons/v_handgr/tris.md2");
R_RegisterModel("models/weapons/v_shotg2/tris.md2");
R_RegisterModel("models/objects/gibs/bone/tris.md2");
R_RegisterModel("models/objects/gibs/sm_meat/tris.md2");
R_RegisterModel("models/objects/gibs/bone2/tris.md2");
// RAFAEL
// R_RegisterModel("models/objects/blaser/tris.md2");

R_RegisterPic ("w_machinegun");
R_RegisterPic ("a_bullets");
R_RegisterPic ("i_health");
R_RegisterPic ("a_grenades");

//ROGUE
	cl_mod_lightning = R_RegisterModel("models/proj/lightning/tris.md2");
	cl_mod_heatbeam = R_RegisterModel("models/proj/beam/tris.md2");
	cl_mod_monster_heatbeam = R_RegisterModel("models/proj/widowbeam/tris.md2");
//ROGUE
}	

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	Com_Memset(cl_beams, 0, sizeof(cl_beams));
	CLQ2_ClearExplosions();
	CLQ2_ClearLasers();

//ROGUE
	Com_Memset(cl_playerbeams, 0, sizeof(cl_playerbeams));
	Com_Memset(cl_sustains, 0, sizeof(cl_sustains));
//ROGUE
}

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
static int CL_ParseBeam (qhandle_t model)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);

// override any beam with the same entity
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.serverTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return ent;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTime)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.serverTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return ent;
}

/*
=================
CL_ParseBeam2
=================
*/
static int CL_ParseBeam2 (qhandle_t model)
{
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);
	net_message.ReadPos(offset);

//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

// override any beam with the same entity

	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.serverTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTime)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.serverTime + 200;	
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return ent;
}

// ROGUE
/*
=================
CL_ParsePlayerBeam
  - adds to the cl_playerbeam array instead of the cl_beams array
=================
*/
static int CL_ParsePlayerBeam (qhandle_t model)
{
	int		ent;
	vec3_t	start, end, offset;
	beam_t	*b;
	int		i;
	
	ent = net_message.ReadShort();
	
	net_message.ReadPos(start);
	net_message.ReadPos(end);
	// PMM - network optimization
	if (model == cl_mod_heatbeam)
		VectorSet(offset, 2, 7, -3);
	else if (model == cl_mod_monster_heatbeam)
	{
		model = cl_mod_heatbeam;
		VectorSet(offset, 0, 0, 0);
	}
	else
		net_message.ReadPos(offset);

//	Com_Printf ("end- %f %f %f\n", end[0], end[1], end[2]);

// override any beam with the same entity
// PMM - For player beams, we only want one per player (entity) so..
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++)
	{
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.serverTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

// find a free beam
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTime)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.serverTime + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");	
	return ent;
}
//rogue

/*
=================
CL_ParseLightning
=================
*/
static int CL_ParseLightning (qhandle_t model)
{
	int		srcEnt, destEnt;
	vec3_t	start, end;
	beam_t	*b;
	int		i;
	
	srcEnt = net_message.ReadShort();
	destEnt = net_message.ReadShort();

	net_message.ReadPos(start);
	net_message.ReadPos(end);

// override any beam with the same source AND destination entities
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
		if (b->entity == srcEnt && b->dest_entity == destEnt)
		{
//			Com_Printf("%d: OVERRIDE  %d -> %d\n", cl.serverTime, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.serverTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return srcEnt;
		}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTime)
		{
//			Com_Printf("%d: NORMAL  %d -> %d\n", cl.serverTime, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.serverTime + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return srcEnt;
		}
	}
	Com_Printf ("beam list overflow!\n");	
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
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		color;
	int		magnitude;
	q2cl_sustain_t	*s, *free_sustain;

	id = net_message.ReadShort();		// an id of -1 is an instant effect
	if (id != -1) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = net_message.ReadByte ();
			net_message.ReadPos(s->org);
			MSG_ReadDir (&net_message, s->dir);
			r = net_message.ReadByte ();
			s->color = r & 0xff;
			s->magnitude = net_message.ReadShort();
			s->endtime = cl.serverTime + net_message.ReadLong();
			s->think = CLQ2_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cl.serverTime;
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = net_message.ReadByte ();
			net_message.ReadPos(pos);
			MSG_ReadDir (&net_message, dir);
			r = net_message.ReadByte ();
			magnitude = net_message.ReadShort();
			magnitude = net_message.ReadLong(); // really interval
		}
	}
	else // instant
	{
		cnt = net_message.ReadByte ();
		net_message.ReadPos(pos);
		MSG_ReadDir (&net_message, dir);
		r = net_message.ReadByte ();
		magnitude = net_message.ReadShort();
		color = r & 0xff;
		CLQ2_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
//		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
	}
}

void CL_ParseWidow (void)
{
	vec3_t	pos;
	int		id, i;
	q2cl_sustain_t	*s, *free_sustain;

	id = net_message.ReadShort();

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		s->id = id;
		net_message.ReadPos(s->org);
		s->endtime = cl.serverTime + 2100;
		s->think = CLQ2_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cl.serverTime;
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		net_message.ReadPos(pos);
	}
}

void CL_ParseNuke (void)
{
	vec3_t	pos;
	int		i;
	q2cl_sustain_t	*s, *free_sustain;

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		s->id = 21000;
		net_message.ReadPos(s->org);
		s->endtime = cl.serverTime + 1000;
		s->think = CLQ2_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cl.serverTime;
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		net_message.ReadPos(pos);
	}
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
		ent = CL_ParseBeam (cl_mod_parasite_segment);
		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		net_message.ReadPos(pos);
		CLQ2_BigTeleportParticles (pos);
		S_StartSound (pos, 0, 0, S_RegisterSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_GRAPPLE_CABLE:
		ent = CL_ParseBeam2 (cl_mod_grapple_cable);
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
		ent = CL_ParseLightning (cl_mod_lightning);
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
		ent = CL_ParsePlayerBeam (cl_mod_heatbeam);
		break;

	case TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_monster_heatbeam);
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
CL_AddBeams
=================
*/
void CL_AddBeams (void)
{
	int			i,j;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	refEntity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	float		model_length;
	
// update beams
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTime)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl.playernum+1)	// entity 0 is the world
		{
			VectorCopy (cl.refdef.vieworg, b->start);
			b->start[2] -= 22;	// adjust for view height
		}
		VectorAdd (b->start, b->offset, org);

	// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
	// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2(dist[2], forward) * -180.0 / M_PI);
			if (pitch < 0)
				pitch += 360.0;
		}

	// add new entities for the beams
		d = VectorNormalize(dist);

		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		if (b->model == cl_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		steps = ceil(d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
//			Com_Printf ("special case\n");
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.hModel = b->model;
			ent.renderfx = RF_ABSOLUTE_LIGHT;
			ent.radius = 1;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand()%360;
			AnglesToAxis(angles, ent.axis);
			R_AddRefEntityToScene (&ent);			
			return;
		}
		while (d > 0)
		{
			VectorCopy (org, ent.origin);
			ent.hModel = b->model;
			vec3_t angles;
			if (b->model == cl_mod_lightning)
			{
				ent.renderfx = RF_ABSOLUTE_LIGHT;
				ent.radius = 1;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = rand()%360;
			}
			else
			{
				angles[0] = pitch;
				angles[1] = yaw;
				angles[2] = rand()%360;
			}
			AnglesToAxis(angles, ent.axis);
			
//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			R_AddRefEntityToScene (&ent);

			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}


/*
//				Com_Printf ("Endpoint:  %f %f %f\n", b->end[0], b->end[1], b->end[2]);
//				Com_Printf ("Pred View Angles:  %f %f %f\n", cl.predicted_angles[0], cl.predicted_angles[1], cl.predicted_angles[2]);
//				Com_Printf ("Act View Angles: %f %f %f\n", cl.refdef.viewangles[0], cl.refdef.viewangles[1], cl.refdef.viewangles[2]);
//				VectorCopy (cl.predicted_origin, b->start);
//				b->start[2] += 22;	// adjust for view height
//				if (fabs(cl.refdef.vieworg[2] - b->start[2]) >= 10) {
//					b->start[2] = cl.refdef.vieworg[2];
//				}

//				Com_Printf ("Time:  %d %d %f\n", cl.serverTime, cls.realtime, cls.frametime);
*/

extern Cvar *hand;

/*
=================
ROGUE - draw player locked beams
CL_AddPlayerBeams
=================
*/
void CL_AddPlayerBeams (void)
{
	int			i,j;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	refEntity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	int			framenum;
	float		model_length;
	
	float		hand_multiplier;
	q2frame_t		*oldframe;
	q2player_state_t	*ps, *ops;

//PMM
	if (hand)
	{
		if (hand->value == 2)
			hand_multiplier = 0;
		else if (hand->value == 1)
			hand_multiplier = -1;
		else
			hand_multiplier = 1;
	}
	else 
	{
		hand_multiplier = 1;
	}
//PMM

// update beams
	for (i=0, b=cl_playerbeams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.serverTime)
			continue;

		if(cl_mod_heatbeam && (b->model == cl_mod_heatbeam))
		{

			// if coming from the player, update the start position
			if (b->entity == cl.playernum+1)	// entity 0 is the world
			{	
				// set up gun position
				// code straight out of CL_AddViewWeapon
				ps = &cl.q2_frame.playerstate;
				j = (cl.q2_frame.serverframe - 1) & UPDATE_MASK;
				oldframe = &cl.frames[j];
				if (oldframe->serverframe != cl.q2_frame.serverframe-1 || !oldframe->valid)
					oldframe = &cl.q2_frame;		// previous frame was dropped or involid
				ops = &oldframe->playerstate;
				for (j=0 ; j<3 ; j++)
				{
					b->start[j] = cl.refdef.vieworg[j] + ops->gunoffset[j]
						+ cl.q2_lerpfrac * (ps->gunoffset[j] - ops->gunoffset[j]);
				}
				VectorMA (b->start, -(hand_multiplier * b->offset[0]), cl.refdef.viewaxis[1], org);
				VectorMA (     org, b->offset[1], cl.refdef.viewaxis[0], org);
				VectorMA (     org, b->offset[2], cl.refdef.viewaxis[2], org);
				if ((hand) && (hand->value == 2)) {
					VectorMA (org, -1, cl.refdef.viewaxis[2], org);
				}
			}
			else
				VectorCopy (b->start, org);
		}
		else
		{
			// if coming from the player, update the start position
			if (b->entity == cl.playernum+1)	// entity 0 is the world
			{
				VectorCopy (cl.refdef.vieworg, b->start);
				b->start[2] -= 22;	// adjust for view height
			}
			VectorAdd (b->start, b->offset, org);
		}

	// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

//PMM
		if(cl_mod_heatbeam && (b->model == cl_mod_heatbeam) && (b->entity == cl.playernum+1))
		{
			vec_t len;

			len = VectorLength (dist);
			VectorScale (cl.refdef.viewaxis[0], len, dist);
			VectorMA (dist, -(hand_multiplier * b->offset[0]), cl.refdef.viewaxis[1], dist);
			VectorMA (dist, b->offset[1], cl.refdef.viewaxis[0], dist);
			VectorMA (dist, b->offset[2], cl.refdef.viewaxis[2], dist);
			if ((hand) && (hand->value == 2)) {
				VectorMA (org, -1, cl.refdef.viewaxis[2], org);
			}
		}
//PMM

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
	// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (atan2(dist[2], forward) * -180.0 / M_PI);
			if (pitch < 0)
				pitch += 360.0;
		}
		
		if (cl_mod_heatbeam && (b->model == cl_mod_heatbeam))
		{
			if (b->entity != cl.playernum+1)
			{
				framenum = 2;
//				Com_Printf ("Third person\n");
				vec3_t angles;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = 0;
				AnglesToAxis(angles, ent.axis);
//				Com_Printf ("%f %f - %f %f %f\n", -pitch, yaw+180.0, b->offset[0], b->offset[1], b->offset[2]);
					
				// if it's a non-origin offset, it's a player, so use the hardcoded player offset
				if (!VectorCompare (b->offset, vec3_origin))
				{
					VectorMA (org, (b->offset[0])-1, ent.axis[1], org);
					VectorMA (org, -(b->offset[1]), ent.axis[0], org);
					VectorMA (org, -(b->offset[2])-10, ent.axis[2], org);
				}
				else
				{
					// if it's a monster, do the particle effect
					CLQ2_MonsterPlasma_Shell(b->start);
				}
			}
			else
			{
				framenum = 1;
			}
		}

		// if it's the heatbeam, draw the particle effect
		if ((cl_mod_heatbeam && (b->model == cl_mod_heatbeam) && (b->entity == cl.playernum+1)))
		{
			CLQ2_HeatbeamPaticles (org, dist);
		}

	// add new entities for the beams
		d = VectorNormalize(dist);

		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;
		if (b->model == cl_mod_heatbeam)
		{
			model_length = 32.0;
		}
		else if (b->model == cl_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		steps = ceil(d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
//			Com_Printf ("special case\n");
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.hModel = b->model;
			ent.renderfx = RF_ABSOLUTE_LIGHT;
			ent.radius = 1;
			vec3_t angles;
			angles[0] = pitch;
			angles[1] = yaw;
			angles[2] = rand()%360;
			AnglesToAxis(angles, ent.axis);
			R_AddRefEntityToScene (&ent);			
			return;
		}
		while (d > 0)
		{
			VectorCopy (org, ent.origin);
			ent.hModel = b->model;
			vec3_t angles;
			if(cl_mod_heatbeam && (b->model == cl_mod_heatbeam))
			{
				ent.renderfx = RF_ABSOLUTE_LIGHT;
				ent.radius = 1;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = (cl.serverTime) % 360;
				ent.frame = framenum;
			}
			else if (b->model == cl_mod_lightning)
			{
				ent.renderfx = RF_ABSOLUTE_LIGHT;
				ent.radius = 1;
				angles[0] = -pitch;
				angles[1] = yaw + 180.0;
				angles[2] = rand()%360;
			}
			else
			{
				angles[0] = pitch;
				angles[1] = yaw;
				angles[2] = rand()%360;
			}
			AnglesToAxis(angles, ent.axis);
			
//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			R_AddRefEntityToScene (&ent);

			for (j=0 ; j<3 ; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}

/* PMM - CL_Sustains */
void CL_ProcessSustain ()
{
	q2cl_sustain_t	*s;
	int				i;

	for (i=0, s=cl_sustains; i< MAX_SUSTAINS; i++, s++)
	{
		if (s->id)
		{
			if ((s->endtime >= cl.serverTime) && (cl.serverTime >= s->nextthink))
			{
//				Com_Printf ("think %d %d %d\n", cl.serverTime, s->nextthink, s->thinkinterval);
				s->think (s);
			}
			else if (s->endtime < cl.serverTime)
				s->id = 0;
		}
	}
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts (void)
{
	CL_AddBeams ();
	// PMM - draw plasma beams
	CL_AddPlayerBeams ();
	CLQ2_AddExplosions ();
	CLQ2_AddLasers ();
	// PMM - set up sustain
	CL_ProcessSustain();
}
