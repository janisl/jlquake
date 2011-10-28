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
// cl_fx.c -- entity effects parsing and management

#include "client.h"

/*
==============================================================

DLIGHT MANAGEMENT

==============================================================
*/

/*
==============
CL_ParseMuzzleFlash
==============
*/
void CL_ParseMuzzleFlash (void)
{
	int			i, weapon;
	q2centity_t	*pl;
	int			silenced;
	float		volume;
	char		soundname[64];

	i = net_message.ReadShort();
	if (i < 1 || i >= MAX_EDICTS_Q2)
		throw DropException("CL_ParseMuzzleFlash: bad entity");

	weapon = net_message.ReadByte ();
	silenced = weapon & Q2MZ_SILENCED;
	weapon &= ~Q2MZ_SILENCED;

	pl = &clq2_entities[i];

	float radius;
	if (silenced)
		radius = 100 + (rand()&31);
	else
		radius = 200 + (rand()&31);

	if (silenced)
		volume = 0.2;
	else
		volume = 1;

	int delay = 0;
	vec3_t color;
	switch (weapon)
	{
	case Q2MZ_BLASTER:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_BLUEHYPERBLASTER:
		color[0] = 0;color[1] = 0;color[2] = 1;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_HYPERBLASTER:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_MACHINEGUN:
		color[0] = 1;color[1] = 1;color[2] = 0;
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_SHOTGUN:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		S_StartSound (NULL, i, Q2CHAN_AUTO,   S_RegisterSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_SSHOTGUN:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_CHAINGUN1:
		radius = 200 + (rand()&31);
		color[0] = 1;color[1] = 0.25;color[2] = 0;
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_CHAINGUN2:
		radius = 225 + (rand()&31);
		color[0] = 1;color[1] = 0.5;color[2] = 0;
		delay = 0.1;	// long delay
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.05);
		break;
	case Q2MZ_CHAINGUN3:
		radius = 250 + (rand()&31);
		color[0] = 1;color[1] = 1;color[2] = 0;
		delay = 0.1;	// long delay
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.033);
		String::Sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.066);
		break;
	case Q2MZ_RAILGUN:
		color[0] = 0.5;color[1] = 0.5;color[2] = 1.0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_ROCKET:
		color[0] = 1;color[1] = 0.5;color[2] = 0.2;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound (NULL, i, Q2CHAN_AUTO,   S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_GRENADE:
		color[0] = 1;color[1] = 0.5;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound (NULL, i, Q2CHAN_AUTO,   S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_BFG:
		color[0] = 0;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	case Q2MZ_LOGIN:
		color[0] = 0;color[1] = 1; color[2] = 0;
		delay = 1.0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CLQ2_LogoutEffect (pl->current.origin, weapon);
		break;
	case Q2MZ_LOGOUT:
		color[0] = 1;color[1] = 0; color[2] = 0;
		delay = 1.0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CLQ2_LogoutEffect (pl->current.origin, weapon);
		break;
	case Q2MZ_RESPAWN:
		color[0] = 1;color[1] = 1; color[2] = 0;
		delay = 1.0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
		CLQ2_LogoutEffect (pl->current.origin, weapon);
		break;
	// RAFAEL
	case Q2MZ_PHALANX:
		color[0] = 1;color[1] = 0.5; color[2] = 0.5;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
		break;
	// RAFAEL
	case Q2MZ_IONRIPPER:	
		color[0] = 1;color[1] = 0.5; color[2] = 0.5;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
		break;

// ======================
// PGM
	case Q2MZ_ETF_RIFLE:
		color[0] = 0.9;color[1] = 0.7;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_SHOTGUN2:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_HEATBEAM:
		color[0] = 1;color[1] = 1;color[2] = 0;
		delay = 100;
		break;
	case Q2MZ_BLASTER2:
		color[0] = 0;color[1] = 1;color[2] = 0;
		// FIXME - different sound for blaster2 ??
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_TRACKER:
		// negative flashes handled the same in gl/soft until CL_AddDLights
		color[0] = -1;color[1] = -1;color[2] = -1;
		S_StartSound (NULL, i, Q2CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
		break;		
	case Q2MZ_NUKE1:
		color[0] = 1;color[1] = 0;color[2] = 0;
		delay = 100;
		break;
	case Q2MZ_NUKE2:
		color[0] = 1;color[1] = 1;color[2] = 0;
		delay = 100;
		break;
	case Q2MZ_NUKE4:
		color[0] = 0;color[1] = 0;color[2] = 1;
		delay = 100;
		break;
	case Q2MZ_NUKE8:
		color[0] = 0;color[1] = 1;color[2] = 1;
		delay = 100;
		break;
// PGM
// ======================
	default:
		return;
	}
	CLQ2_MuzzleFlashLight(i, pl->current.origin, pl->current.angles, radius, delay, color);
}

/*
==============
CL_ParseMuzzleFlash2
==============
*/
void CL_ParseMuzzleFlash2 (void) 
{
	int			ent;
	vec3_t		origin;
	int			flash_number;
	vec3_t		forward, right;
	char		soundname[64];

	ent = net_message.ReadShort();
	if (ent < 1 || ent >= MAX_EDICTS_Q2)
		throw DropException("CL_ParseMuzzleFlash2: bad entity");

	flash_number = net_message.ReadByte ();

	// locate the origin
	AngleVectors (clq2_entities[ent].current.angles, forward, right, NULL);
	origin[0] = clq2_entities[ent].current.origin[0] + forward[0] * monster_flash_offset[flash_number][0] + right[0] * monster_flash_offset[flash_number][1];
	origin[1] = clq2_entities[ent].current.origin[1] + forward[1] * monster_flash_offset[flash_number][0] + right[1] * monster_flash_offset[flash_number][1];
	origin[2] = clq2_entities[ent].current.origin[2] + forward[2] * monster_flash_offset[flash_number][0] + right[2] * monster_flash_offset[flash_number][1] + monster_flash_offset[flash_number][2];

	float radius = 200 + (rand()&31);
	int delay = 0;
	vec3_t color;
	switch (flash_number)
	{
	case Q2MZ2_INFANTRY_MACHINEGUN_1:
	case Q2MZ2_INFANTRY_MACHINEGUN_2:
	case Q2MZ2_INFANTRY_MACHINEGUN_3:
	case Q2MZ2_INFANTRY_MACHINEGUN_4:
	case Q2MZ2_INFANTRY_MACHINEGUN_5:
	case Q2MZ2_INFANTRY_MACHINEGUN_6:
	case Q2MZ2_INFANTRY_MACHINEGUN_7:
	case Q2MZ2_INFANTRY_MACHINEGUN_8:
	case Q2MZ2_INFANTRY_MACHINEGUN_9:
	case Q2MZ2_INFANTRY_MACHINEGUN_10:
	case Q2MZ2_INFANTRY_MACHINEGUN_11:
	case Q2MZ2_INFANTRY_MACHINEGUN_12:
	case Q2MZ2_INFANTRY_MACHINEGUN_13:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SOLDIER_MACHINEGUN_1:
	case Q2MZ2_SOLDIER_MACHINEGUN_2:
	case Q2MZ2_SOLDIER_MACHINEGUN_3:
	case Q2MZ2_SOLDIER_MACHINEGUN_4:
	case Q2MZ2_SOLDIER_MACHINEGUN_5:
	case Q2MZ2_SOLDIER_MACHINEGUN_6:
	case Q2MZ2_SOLDIER_MACHINEGUN_7:
	case Q2MZ2_SOLDIER_MACHINEGUN_8:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GUNNER_MACHINEGUN_1:
	case Q2MZ2_GUNNER_MACHINEGUN_2:
	case Q2MZ2_GUNNER_MACHINEGUN_3:
	case Q2MZ2_GUNNER_MACHINEGUN_4:
	case Q2MZ2_GUNNER_MACHINEGUN_5:
	case Q2MZ2_GUNNER_MACHINEGUN_6:
	case Q2MZ2_GUNNER_MACHINEGUN_7:
	case Q2MZ2_GUNNER_MACHINEGUN_8:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_ACTOR_MACHINEGUN_1:
	case Q2MZ2_SUPERTANK_MACHINEGUN_1:
	case Q2MZ2_SUPERTANK_MACHINEGUN_2:
	case Q2MZ2_SUPERTANK_MACHINEGUN_3:
	case Q2MZ2_SUPERTANK_MACHINEGUN_4:
	case Q2MZ2_SUPERTANK_MACHINEGUN_5:
	case Q2MZ2_SUPERTANK_MACHINEGUN_6:
	case Q2MZ2_TURRET_MACHINEGUN:			// PGM
		color[0] = 1;color[1] = 1;color[2] = 0;

		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_BOSS2_MACHINEGUN_L1:
	case Q2MZ2_BOSS2_MACHINEGUN_L2:
	case Q2MZ2_BOSS2_MACHINEGUN_L3:
	case Q2MZ2_BOSS2_MACHINEGUN_L4:
	case Q2MZ2_BOSS2_MACHINEGUN_L5:
	case Q2MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case Q2MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		color[0] = 1;color[1] = 1;color[2] = 0;

		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
		break;

	case Q2MZ2_SOLDIER_BLASTER_1:
	case Q2MZ2_SOLDIER_BLASTER_2:
	case Q2MZ2_SOLDIER_BLASTER_3:
	case Q2MZ2_SOLDIER_BLASTER_4:
	case Q2MZ2_SOLDIER_BLASTER_5:
	case Q2MZ2_SOLDIER_BLASTER_6:
	case Q2MZ2_SOLDIER_BLASTER_7:
	case Q2MZ2_SOLDIER_BLASTER_8:
	case Q2MZ2_TURRET_BLASTER:			// PGM
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_FLYER_BLASTER_1:
	case Q2MZ2_FLYER_BLASTER_2:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_MEDIC_BLASTER_1:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_HOVER_BLASTER_1:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_FLOAT_BLASTER_1:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SOLDIER_SHOTGUN_1:
	case Q2MZ2_SOLDIER_SHOTGUN_2:
	case Q2MZ2_SOLDIER_SHOTGUN_3:
	case Q2MZ2_SOLDIER_SHOTGUN_4:
	case Q2MZ2_SOLDIER_SHOTGUN_5:
	case Q2MZ2_SOLDIER_SHOTGUN_6:
	case Q2MZ2_SOLDIER_SHOTGUN_7:
	case Q2MZ2_SOLDIER_SHOTGUN_8:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_BLASTER_1:
	case Q2MZ2_TANK_BLASTER_2:
	case Q2MZ2_TANK_BLASTER_3:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_MACHINEGUN_1:
	case Q2MZ2_TANK_MACHINEGUN_2:
	case Q2MZ2_TANK_MACHINEGUN_3:
	case Q2MZ2_TANK_MACHINEGUN_4:
	case Q2MZ2_TANK_MACHINEGUN_5:
	case Q2MZ2_TANK_MACHINEGUN_6:
	case Q2MZ2_TANK_MACHINEGUN_7:
	case Q2MZ2_TANK_MACHINEGUN_8:
	case Q2MZ2_TANK_MACHINEGUN_9:
	case Q2MZ2_TANK_MACHINEGUN_10:
	case Q2MZ2_TANK_MACHINEGUN_11:
	case Q2MZ2_TANK_MACHINEGUN_12:
	case Q2MZ2_TANK_MACHINEGUN_13:
	case Q2MZ2_TANK_MACHINEGUN_14:
	case Q2MZ2_TANK_MACHINEGUN_15:
	case Q2MZ2_TANK_MACHINEGUN_16:
	case Q2MZ2_TANK_MACHINEGUN_17:
	case Q2MZ2_TANK_MACHINEGUN_18:
	case Q2MZ2_TANK_MACHINEGUN_19:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		String::Sprintf(soundname, sizeof(soundname), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound(soundname), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_CHICK_ROCKET_1:
	case Q2MZ2_TURRET_ROCKET:			// PGM
		color[0] = 1;color[1] = 0.5;color[2] = 0.2;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_ROCKET_1:
	case Q2MZ2_TANK_ROCKET_2:
	case Q2MZ2_TANK_ROCKET_3:
		color[0] = 1;color[1] = 0.5;color[2] = 0.2;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SUPERTANK_ROCKET_1:
	case Q2MZ2_SUPERTANK_ROCKET_2:
	case Q2MZ2_SUPERTANK_ROCKET_3:
	case Q2MZ2_BOSS2_ROCKET_1:
	case Q2MZ2_BOSS2_ROCKET_2:
	case Q2MZ2_BOSS2_ROCKET_3:
	case Q2MZ2_BOSS2_ROCKET_4:
	case Q2MZ2_CARRIER_ROCKET_1:
//	case Q2MZ2_CARRIER_ROCKET_2:
//	case Q2MZ2_CARRIER_ROCKET_3:
//	case Q2MZ2_CARRIER_ROCKET_4:
		color[0] = 1;color[1] = 0.5;color[2] = 0.2;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GUNNER_GRENADE_1:
	case Q2MZ2_GUNNER_GRENADE_2:
	case Q2MZ2_GUNNER_GRENADE_3:
	case Q2MZ2_GUNNER_GRENADE_4:
		color[0] = 1;color[1] = 0.5;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GLADIATOR_RAILGUN_1:
	// PMM
	case Q2MZ2_CARRIER_RAILGUN:
	case Q2MZ2_WIDOW_RAIL:
	// pmm
		color[0] = 0.5;color[1] = 0.5;color[2] = 1.0;
		break;

// --- Xian's shit starts ---
	case Q2MZ2_MAKRON_BFG:
		color[0] = 0.5;color[1] = 1 ;color[2] = 0.5;
		//S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_MAKRON_BLASTER_1:
	case Q2MZ2_MAKRON_BLASTER_2:
	case Q2MZ2_MAKRON_BLASTER_3:
	case Q2MZ2_MAKRON_BLASTER_4:
	case Q2MZ2_MAKRON_BLASTER_5:
	case Q2MZ2_MAKRON_BLASTER_6:
	case Q2MZ2_MAKRON_BLASTER_7:
	case Q2MZ2_MAKRON_BLASTER_8:
	case Q2MZ2_MAKRON_BLASTER_9:
	case Q2MZ2_MAKRON_BLASTER_10:
	case Q2MZ2_MAKRON_BLASTER_11:
	case Q2MZ2_MAKRON_BLASTER_12:
	case Q2MZ2_MAKRON_BLASTER_13:
	case Q2MZ2_MAKRON_BLASTER_14:
	case Q2MZ2_MAKRON_BLASTER_15:
	case Q2MZ2_MAKRON_BLASTER_16:
	case Q2MZ2_MAKRON_BLASTER_17:
		color[0] = 1;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
		break;
	
	case Q2MZ2_JORG_MACHINEGUN_L1:
	case Q2MZ2_JORG_MACHINEGUN_L2:
	case Q2MZ2_JORG_MACHINEGUN_L3:
	case Q2MZ2_JORG_MACHINEGUN_L4:
	case Q2MZ2_JORG_MACHINEGUN_L5:
	case Q2MZ2_JORG_MACHINEGUN_L6:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_JORG_MACHINEGUN_R1:
	case Q2MZ2_JORG_MACHINEGUN_R2:
	case Q2MZ2_JORG_MACHINEGUN_R3:
	case Q2MZ2_JORG_MACHINEGUN_R4:
	case Q2MZ2_JORG_MACHINEGUN_R5:
	case Q2MZ2_JORG_MACHINEGUN_R6:
		color[0] = 1;color[1] = 1;color[2] = 0;
		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		break;

	case Q2MZ2_JORG_BFG_1:
		color[0] = 0.5;color[1] = 1 ;color[2] = 0.5;
		break;

	case Q2MZ2_BOSS2_MACHINEGUN_R1:
	case Q2MZ2_BOSS2_MACHINEGUN_R2:
	case Q2MZ2_BOSS2_MACHINEGUN_R3:
	case Q2MZ2_BOSS2_MACHINEGUN_R4:
	case Q2MZ2_BOSS2_MACHINEGUN_R5:
	case Q2MZ2_CARRIER_MACHINEGUN_R1:			// PMM
	case Q2MZ2_CARRIER_MACHINEGUN_R2:			// PMM

		color[0] = 1;color[1] = 1;color[2] = 0;

		CLQ2_ParticleEffect (origin, vec3_origin, 0, 40);
		CLQ2_SmokeAndFlash(origin);
		break;

// ======
// ROGUE
	case Q2MZ2_STALKER_BLASTER:
	case Q2MZ2_DAEDALUS_BLASTER:
	case Q2MZ2_MEDIC_BLASTER_2:
	case Q2MZ2_WIDOW_BLASTER:
	case Q2MZ2_WIDOW_BLASTER_SWEEP1:
	case Q2MZ2_WIDOW_BLASTER_SWEEP2:
	case Q2MZ2_WIDOW_BLASTER_SWEEP3:
	case Q2MZ2_WIDOW_BLASTER_SWEEP4:
	case Q2MZ2_WIDOW_BLASTER_SWEEP5:
	case Q2MZ2_WIDOW_BLASTER_SWEEP6:
	case Q2MZ2_WIDOW_BLASTER_SWEEP7:
	case Q2MZ2_WIDOW_BLASTER_SWEEP8:
	case Q2MZ2_WIDOW_BLASTER_SWEEP9:
	case Q2MZ2_WIDOW_BLASTER_100:
	case Q2MZ2_WIDOW_BLASTER_90:
	case Q2MZ2_WIDOW_BLASTER_80:
	case Q2MZ2_WIDOW_BLASTER_70:
	case Q2MZ2_WIDOW_BLASTER_60:
	case Q2MZ2_WIDOW_BLASTER_50:
	case Q2MZ2_WIDOW_BLASTER_40:
	case Q2MZ2_WIDOW_BLASTER_30:
	case Q2MZ2_WIDOW_BLASTER_20:
	case Q2MZ2_WIDOW_BLASTER_10:
	case Q2MZ2_WIDOW_BLASTER_0:
	case Q2MZ2_WIDOW_BLASTER_10L:
	case Q2MZ2_WIDOW_BLASTER_20L:
	case Q2MZ2_WIDOW_BLASTER_30L:
	case Q2MZ2_WIDOW_BLASTER_40L:
	case Q2MZ2_WIDOW_BLASTER_50L:
	case Q2MZ2_WIDOW_BLASTER_60L:
	case Q2MZ2_WIDOW_BLASTER_70L:
	case Q2MZ2_WIDOW_RUN_1:
	case Q2MZ2_WIDOW_RUN_2:
	case Q2MZ2_WIDOW_RUN_3:
	case Q2MZ2_WIDOW_RUN_4:
	case Q2MZ2_WIDOW_RUN_5:
	case Q2MZ2_WIDOW_RUN_6:
	case Q2MZ2_WIDOW_RUN_7:
	case Q2MZ2_WIDOW_RUN_8:
		color[0] = 0;color[1] = 1;color[2] = 0;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_WIDOW_DISRUPTOR:
		color[0] = -1;color[1] = -1;color[2] = -1;
		S_StartSound (NULL, ent, Q2CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_WIDOW_PLASMABEAM:
	case Q2MZ2_WIDOW2_BEAMER_1:
	case Q2MZ2_WIDOW2_BEAMER_2:
	case Q2MZ2_WIDOW2_BEAMER_3:
	case Q2MZ2_WIDOW2_BEAMER_4:
	case Q2MZ2_WIDOW2_BEAMER_5:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_1:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_2:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_3:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_4:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_5:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_6:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_7:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_8:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_9:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_10:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_11:
		radius = 300 + (rand()&100);
		color[0] = 1;color[1] = 1;color[2] = 0;
		delay = 200;
		break;
// ROGUE
// ======

// --- Xian's shit ends ---

	default:
		return;
	}
	CLQ2_MuzzleFlash2Light(ent, origin, radius, delay, color);
}

/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

void CL_FlyEffect (q2centity_t *ent, vec3_t origin)
{
	int		n;
	int		count;
	int		starttime;

	if (ent->fly_stoptime < cl.serverTime)
	{
		starttime = cl.serverTime;
		ent->fly_stoptime = cl.serverTime + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = cl.serverTime - starttime;
	if (n < 20000)
		count = n * 162 / 20000.0;
	else
	{
		n = ent->fly_stoptime - cl.serverTime;
		if (n < 20000)
			count = n * 162 / 20000.0;
		else
			count = 162;
	}

	CLQ2_FlyParticles (origin, count);
}

/*
==============
CL_EntityEvent

An entity has just been parsed that has an event value

the female events are there for backwards compatability
==============
*/
void CL_EntityEvent (q2entity_state_t *ent)
{
	switch (ent->event)
	{
	case EV_ITEM_RESPAWN:
		S_StartSound (NULL, ent->number, Q2CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CLQ2_ItemRespawnParticles (ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		S_StartSound (NULL, ent->number, Q2CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CLQ2_TeleportParticles (ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			S_StartSound (NULL, ent->number, Q2CHAN_BODY, clq2_sfx_footsteps[rand()&3], 1, ATTN_NORM, 0);
		break;
	case EV_FALLSHORT:
		S_StartSound (NULL, ent->number, Q2CHAN_AUTO, S_RegisterSound ("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound (NULL, ent->number, Q2CHAN_AUTO, S_RegisterSound ("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound (NULL, ent->number, Q2CHAN_AUTO, S_RegisterSound ("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	}
}


/*
==============
CL_ClearEffects

==============
*/
void CL_ClearEffects (void)
{
	CL_ClearParticles ();
	CL_ClearDlights ();
	CL_ClearLightStyles ();
}
