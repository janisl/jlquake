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
	net_message.ReadDir(dir);

	color = net_message.ReadByte ();

	count = net_message.ReadByte ();

	CLQ2_ParticleEffect (pos, dir, color, count);
}

//ROGUE
//=============

/*
=================
CL_ParseTEnt
=================
*/
void CL_ParseTEnt (void)
{
	int type = net_message.ReadByte();
	switch (type)
	{
	case TE_BLOOD:			// bullet hitting flesh
		CLQ2_ParseBlood(net_message);
		break;

	case TE_GUNSHOT:			// bullet hitting wall
		CLQ2_ParseGunShot(net_message);
		break;

	case TE_SPARKS:
		CLQ2_ParseSparks(net_message);
		break;

	case TE_BULLET_SPARKS:
		CLQ2_ParseBulletSparks(net_message);
		break;

	case TE_SCREEN_SPARKS:
		CLQ2_ParseScreenSparks(net_message);
		break;

	case TE_SHIELD_SPARKS:
		CLQ2_ParseShieldSparks(net_message);
		break;

	case TE_SHOTGUN:			// bullet hitting wall
		CLQ2_ParseShotgun(net_message);
		break;

	case TE_SPLASH:			// bullet hitting water
		CLQ2_ParseSplash(net_message);
		break;

	case TE_LASER_SPARKS:
		CLQ2_ParseLaserSparks(net_message);
		break;

	// RAFAEL
	case TE_BLUEHYPERBLASTER:
		CLQ2_ParseBlueHyperBlaster(net_message);
		break;

	case TE_BLASTER:			// blaster hitting wall
		CLQ2_ParseBlaster(net_message);
		break;
		
	case TE_RAILTRAIL:			// railgun effect
		CLQ2_ParseRailTrail(net_message);
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
		CLQ2_ParseGrenadeExplosion(net_message);
		break;

	case TE_GRENADE_EXPLOSION_WATER:
		CLQ2_ParseGrenadeExplosionWater(net_message);
		break;

	// RAFAEL
	case TE_PLASMA_EXPLOSION:
		CLQ2_ParsePlasmaExplosion(net_message);
		break;
	
	case TE_EXPLOSION1:
	case TE_ROCKET_EXPLOSION:
		CLQ2_ParseRocketExplosion(net_message);
		break;

	case TE_EXPLOSION1_BIG:
		CLQ2_ParseExplosion1Big(net_message);
		break;

	case TE_ROCKET_EXPLOSION_WATER:
		CLQ2_ParseRocketExplosionWater(net_message);
		break;

	case TE_EXPLOSION1_NP:
	case TE_PLAIN_EXPLOSION:
		CLQ2_ParsePlainExplosion(net_message);
		break;

	case TE_BFG_EXPLOSION:
		CLQ2_ParseBfgExplosion(net_message);
		break;

	case TE_BFG_BIGEXPLOSION:
		CLQ2_ParseBfgBigExplosion(net_message);
		break;

	case TE_BFG_LASER:
		CLQ2_ParseBfgLaser(net_message);
		break;

	case TE_BUBBLETRAIL:
		CLQ2_ParseBubleTrail(net_message);
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		CLQ2_ParseParasiteAttack(net_message);
		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		CLQ2_ParseBossTeleport(net_message);
		break;

	case TE_GRAPPLE_CABLE:
		CLQ2_ParseGrappleCable(net_message);
		break;

	case TE_WELDING_SPARKS:
		CLQ2_ParseWeldingSparks(net_message);
		break;

	case TE_GREENBLOOD:
		CLQ2_ParseGreenBlood(net_message);
		break;

	case TE_TUNNEL_SPARKS:
		CLQ2_ParseTunnelSparks(net_message);
		break;

	case TE_BLASTER2:			// green blaster hitting wall
		CLQ2_ParseBlaster2(net_message);
		break;

	case TE_FLECHETTE:			// flechette
		CLQ2_ParseFlechette(net_message);
		break;

	case TE_LIGHTNING:
		CLQ2_ParseLightning(net_message);
		break;

	case TE_DEBUGTRAIL:
		CLQ2_ParseDebugTrail(net_message);
		break;

	case TE_FLASHLIGHT:
		CLQ2_ParseFlashLight(net_message);
		break;

	case TE_FORCEWALL:
		CLQ2_ParseForceWall(net_message);
		break;

	case TE_HEATBEAM:
		CLQ2_ParseHeatBeam(net_message);
		break;

	case TE_MONSTER_HEATBEAM:
		CLQ2_ParseMonsterHeatBeam(net_message);
		break;

	case TE_HEATBEAM_SPARKS:
		CLQ2_ParseHeatBeamSparks(net_message);
		break;
	
	case TE_HEATBEAM_STEAM:
		CLQ2_ParseHeatBeamSteam(net_message);
		break;

	case TE_STEAM:
		CLQ2_ParseSteam(net_message);
		break;

	case TE_BUBBLETRAIL2:
		CLQ2_ParseBubleTrail2(net_message);
		break;

	case TE_MOREBLOOD:
		CLQ2_ParseMoreBlood(net_message);
		break;

	case TE_CHAINFIST_SMOKE:
		CLQ2_ParseChainfistSmoke(net_message);
		break;

	case TE_ELECTRIC_SPARKS:
		CLQ2_ParseElectricSparks(net_message);
		break;

	case TE_TRACKER_EXPLOSION:
		CLQ2_ParseTrackerExplosion(net_message);
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		CLQ2_ParseTeleportEffect(net_message);
		break;

	case TE_WIDOWBEAMOUT:
		CLQ2_ParseWidowBeamOut(net_message);
		break;

	case TE_NUKEBLAST:
		CLQ2_ParseNukeBlast(net_message);
		break;

	case TE_WIDOWSPLASH:
		CLQ2_ParseWidowSplash(net_message);
		break;

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
