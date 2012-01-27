
#include "qwsvdef.h"
#include "../../core/file_formats/bsp29.h"

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static byte		fatpvs[BSP29_MAX_MAP_LEAFS/8];

/*
=============
SV_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
byte *SV_FatPVS (vec3_t org)
{
	vec3_t mins, maxs;
	for (int i = 0; i < 3; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	int leafs[64];
	int count = CM_BoxLeafnums(mins, maxs, leafs, 64);
	if (count < 1)
	{
		throw Exception("SV_FatPVS: count < 1");
	}

	// convert leafs to clusters
	for (int i = 0; i < count; i++)
	{
		leafs[i] = CM_LeafCluster(leafs[i]);
	}

	int fatbytes = (CM_NumClusters() + 31) >> 3;
	Com_Memcpy(fatpvs, CM_ClusterPVS(leafs[0]), fatbytes);
	// or in all the other leaf bits
	for (int i = 1; i < count; i++)
	{
		byte* pvs = CM_ClusterPVS(leafs[i]);
		for (int j = 0; j < fatbytes; j++)
		{
			fatpvs[j] |= pvs[j];
		}
	}
	return fatpvs;
}

//=============================================================================
/*
// because there can be a lot of nails, there is a special
// network protocol for them

	RDM: changed to use packed missiles, this code left here in case we
	decide to pack missiles which require a velocity as well



#define	MAX_NAILS	32
qhedict_t	*nails[MAX_NAILS];
int		numnails;

extern	int	sv_nailmodel, sv_supernailmodel, sv_playermodel[MAX_PLAYER_CLASS];

qboolean SV_AddNailUpdate (qhedict_t *ent)
{
	if (ent->v.modelindex != sv_nailmodel
		&& ent->v.modelindex != sv_supernailmodel)
		return false;
	if (numnails == MAX_NAILS)
		return true;
	nails[numnails] = ent;
	numnails++;
	return true;
}

void SV_EmitNailUpdate (QMsg *msg)
{
	byte	bits[6];	// [48 bits] xyzpy 12 12 12 4 8 
	int		n, i;
	qhedict_t	*ent;
	int		x, y, z, p, yaw;

	if (!numnails)
		return;

	msg->WriteByte(hwsvc_nails);
	msg->WriteByte(numnails);

	for (n=0 ; n<numnails ; n++)
	{
		ent = nails[n];
		x = (int)(ent->v.origin[0]+4096)>>1;
		y = (int)(ent->v.origin[1]+4096)>>1;
		z = (int)(ent->v.origin[2]+4096)>>1;
		p = (int)(16*ent->v.angles[0]/360)&15;
		yaw = (int)(256*ent->v.angles[1]/360)&255;

		bits[0] = x;
		bits[1] = (x>>8) | (y<<4);
		bits[2] = (y>>4);
		bits[3] = z;
		bits[4] = (z>>8) | (p<<4);
		bits[5] = yaw;

		for (i=0 ; i<6 ; i++)
			msg->WriteByte(bits[i]);
	}
}
*/
  
#define	MAX_MISSILES_H2	32
qhedict_t	*missiles[MAX_MISSILES_H2];
qhedict_t	*ravens[MAX_MISSILES_H2];
qhedict_t	*raven2s[MAX_MISSILES_H2];
int		nummissiles, numravens, numraven2s;

extern	int	sv_magicmissmodel, sv_playermodel[MAX_PLAYER_CLASS], sv_ravenmodel, sv_raven2model;

qboolean SV_AddMissileUpdate (qhedict_t *ent)
{

	if (ent->v.modelindex == sv_magicmissmodel)
	{
		if (nummissiles == MAX_MISSILES_H2)
			return true;
		missiles[nummissiles] = ent;
		nummissiles++;
		return true;
	}
	if (ent->v.modelindex == sv_ravenmodel)
	{
		if (numravens == MAX_MISSILES_H2)
			return true;
		ravens[numravens] = ent;
		numravens++;
		return true;
	}
	if (ent->v.modelindex == sv_raven2model)
	{
		if (numraven2s == MAX_MISSILES_H2)
			return true;
		raven2s[numraven2s] = ent;
		numraven2s++;
		return true;
	}
	return false;
}

void SV_EmitMissileUpdate (QMsg *msg)
{
	byte	bits[5];	// [40 bits] xyz type 12 12 12 4
	int		n, i;
	qhedict_t	*ent;
	int		x, y, z, type;

	if (!nummissiles)
		return;

	msg->WriteByte(hwsvc_packmissile);
	msg->WriteByte(nummissiles);

	for (n=0 ; n<nummissiles ; n++)
	{
		ent = missiles[n];
		x = (int)(ent->GetOrigin()[0]+4096)>>1;
		y = (int)(ent->GetOrigin()[1]+4096)>>1;
		z = (int)(ent->GetOrigin()[2]+4096)>>1;
		if(fabs(ent->GetScale() - 0.1)<0.05)
			type = 1;	//assume ice mace
		else 
			type = 2;	//assume magic missile

		bits[0] = x;
		bits[1] = (x>>8) | (y<<4);
		bits[2] = (y>>4);
		bits[3] = z;
		bits[4] = (z>>8) | (type<<4);

		for (i=0 ; i<5 ; i++)
			msg->WriteByte(bits[i]);
	}
}

void SV_EmitRavenUpdate (QMsg *msg)
{
	byte	bits[6];	// [48 bits] xyzpy 12 12 12 4 8 
	int		n, i;
	qhedict_t	*ent;
	int		x, y, z, p, yaw, frame;

	if ((!numravens) && (!numraven2s))
		return;

	msg->WriteByte(hwsvc_nails);	//svc nails overloaded for ravens
	msg->WriteByte(numravens);

	for (n=0 ; n<numravens ; n++)
	{
		ent = ravens[n];
		x = (int)(ent->GetOrigin()[0]+4096)>>1;
		y = (int)(ent->GetOrigin()[1]+4096)>>1;
		z = (int)(ent->GetOrigin()[2]+4096)>>1;
		p = (int)(16*ent->GetAngles()[0]/360)&15;
		frame = (int)(ent->GetFrame())&7;
		yaw = (int)(32*ent->GetAngles()[1]/360)&31;

		bits[0] = x;
		bits[1] = (x>>8) | (y<<4);
		bits[2] = (y>>4);
		bits[3] = z;
		bits[4] = (z>>8) | (p<<4);
		bits[5] = yaw | (frame<<5);

		for (i=0 ; i<6 ; i++)
			msg->WriteByte(bits[i]);
	}
	msg->WriteByte(numraven2s);

	for (n=0 ; n<numraven2s ; n++)
	{
		ent = raven2s[n];
		x = (int)(ent->GetOrigin()[0]+4096)>>1;
		y = (int)(ent->GetOrigin()[1]+4096)>>1;
		z = (int)(ent->GetOrigin()[2]+4096)>>1;
		p = (int)(16*ent->GetAngles()[0]/360)&15;
		yaw = (int)(256*ent->GetAngles()[1]/360)&255;

		bits[0] = x;
		bits[1] = (x>>8) | (y<<4);
		bits[2] = (y>>4);
		bits[3] = z;
		bits[4] = (z>>8) | (p<<4);
		bits[5] = yaw;

		for (i=0 ; i<6 ; i++)
			msg->WriteByte(bits[i]);
	}
}

void SV_EmitPackedEntities(QMsg *msg)
{
	SV_EmitMissileUpdate(msg);
	SV_EmitRavenUpdate(msg);
}

//=============================================================================


/*
==================
SV_WriteDelta

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void SV_WriteDelta (h2entity_state_t *from, h2entity_state_t *to, QMsg *msg, qboolean force, qhedict_t *ent, client_t *client)
{
	int		bits;
	int		i;
	float	miss;
	int		temp_index;
	char	NewName[MAX_QPATH];

// send an update
	bits = 0;
	
	for (i=0 ; i<3 ; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if ( miss < -0.1 || miss > 0.1 )
			bits |= HWU_ORIGIN1<<i;
	}

	if ( to->angles[0] != from->angles[0] )
		bits |= HWU_ANGLE1;
		
	if ( to->angles[1] != from->angles[1] )
		bits |= HWU_ANGLE2;
		
	if ( to->angles[2] != from->angles[2] )
		bits |= HWU_ANGLE3;
		
	if ( to->colormap != from->colormap )
		bits |= HWU_COLORMAP;
		
	if ( to->skinnum != from->skinnum)
	{
		bits |= HWU_SKIN;
	}

	if (to->drawflags != from->drawflags)
		bits |= HWU_DRAWFLAGS;

	if ( to->frame != from->frame )
		bits |= HWU_FRAME;
	
	if ( to->effects != from->effects )
		bits |= HWU_EFFECTS;

	temp_index = to->modelindex;
	if (((int)ent->GetFlags() & FL_CLASS_DEPENDENT) && ent->GetModel())
	{
		String::Cpy(NewName, PR_GetString(ent->GetModel()));
		if (client->playerclass <= 0 || client->playerclass > MAX_PLAYER_CLASS)
		{
			NewName[String::Length(NewName)-5] = '1';
		}
		else
		{
			NewName[String::Length(NewName)-5] = client->playerclass + 48;
		}
		temp_index = SV_ModelIndex (NewName);
	}

	if (temp_index != from->modelindex )
	{
		bits |= HWU_MODEL;
		if (temp_index > 255)
		{
			bits |= HWU_MODEL16;
		}
	}

	if (to->scale != from->scale)
	{
		bits |= HWU_SCALE;
	}

	if (to->abslight != from->abslight)
	{
		bits |= HWU_ABSLIGHT;
	}

	if(to->wpn_sound)
	{	//not delta'ed, sound gets cleared after send 
		bits |= HWU_SOUND;
	}

	if (bits & 0xff0000)
		bits |= HWU_MOREBITS2;

	if (bits & 511)
		bits |= HWU_MOREBITS;

	//
	// write the message
	//
	if (!to->number)
		SV_Error ("Unset entity number");
	if (to->number >= 512)
		SV_Error ("Entity number >= 512");

	if (!bits && !force)
		return;		// nothing to send!
	i = to->number | (bits&~511);
	if (i & HWU_REMOVE)
		Sys_Error ("HWU_REMOVE");
	msg->WriteShort(i & 0xffff);
	
	if (bits & HWU_MOREBITS)
		msg->WriteByte(bits&255);
	if (bits & HWU_MOREBITS2)
		msg->WriteByte((bits >> 16) & 0xff);
	if (bits & HWU_MODEL)
	{
		if (bits & HWU_MODEL16)
		{
			msg->WriteShort(temp_index);
		}
		else
		{
			msg->WriteByte(temp_index);
		}
	}
	if (bits & HWU_FRAME)
		msg->WriteByte(to->frame);
	if (bits & HWU_COLORMAP)
		msg->WriteByte(to->colormap);
	if (bits & HWU_SKIN)
		msg->WriteByte(to->skinnum);
	if (bits & HWU_DRAWFLAGS)
		msg->WriteByte(to->drawflags);
	if (bits & HWU_EFFECTS)
		msg->WriteLong(to->effects);
	if (bits & HWU_ORIGIN1)
		msg->WriteCoord(to->origin[0]);		
	if (bits & HWU_ANGLE1)
		msg->WriteAngle(to->angles[0]);
	if (bits & HWU_ORIGIN2)
		msg->WriteCoord(to->origin[1]);
	if (bits & HWU_ANGLE2)
		msg->WriteAngle(to->angles[1]);
	if (bits & HWU_ORIGIN3)
		msg->WriteCoord(to->origin[2]);
	if (bits & HWU_ANGLE3)
		msg->WriteAngle(to->angles[2]);
	if (bits & HWU_SCALE)
		msg->WriteByte(to->scale);
	if (bits & HWU_ABSLIGHT)
		msg->WriteByte(to->abslight);
	if (bits & HWU_SOUND)
		msg->WriteShort(to->wpn_sound);
}

/*
=============
SV_EmitPacketEntities

Writes a delta update of a hwpacket_entities_t to the message.

=============
*/
void SV_EmitPacketEntities (client_t *client, hwpacket_entities_t *to, QMsg *msg)
{
	qhedict_t	*ent;
	client_frame_t	*fromframe;
	hwpacket_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;

	// this is the frame that we are going to delta update from
	if (client->delta_sequence != -1)
	{
		fromframe = &client->frames[client->delta_sequence & UPDATE_MASK_HW];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		msg->WriteByte(hwsvc_deltapacketentities);
		msg->WriteByte(client->delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		msg->WriteByte(hwsvc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK_HW
//			, client->netchan.outgoing_sequence & UPDATE_MASK_HW);
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (&from->entities[oldindex], &to->entities[newindex], msg, false, EDICT_NUM(newnum), client);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			ent = EDICT_NUM(newnum);
//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (&ent->baseline, &to->entities[newindex], msg, true, ent, client);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			msg->WriteShort(oldnum | HWU_REMOVE);
			oldindex++;
			continue;
		}
	}

	msg->WriteShort(0);	// end of packetentities
}







void SV_WriteInventory (client_t *host_client, qhedict_t *ent, QMsg *msg)
{
	int		sc1,sc2;
	byte	test;

	if (host_client->send_all_v) 
	{
		sc1 = sc2 = 0xffffffff;
		host_client->send_all_v = false;
	}
	else
	{
		sc1 = sc2 = 0;

		if (ent->GetHealth() != host_client->old_v.health)
			sc1 |= SC1_HEALTH;
		if(ent->GetLevel() != host_client->old_v.level)
			sc1 |= SC1_LEVEL;
		if(ent->GetIntelligence() != host_client->old_v.intelligence)
			sc1 |= SC1_INTELLIGENCE;
		if(ent->GetWisdom() != host_client->old_v.wisdom)
			sc1 |= SC1_WISDOM;
		if(ent->GetStrength() != host_client->old_v.strength)
			sc1 |= SC1_STRENGTH;
		if(ent->GetDexterity() != host_client->old_v.dexterity)
			sc1 |= SC1_DEXTERITY;
		if (ent->GetTeleportTime() > sv.time)
		{
//			Con_Printf ("Teleport_time>time, sending bit\n");
			sc1 |= SC1_TELEPORT_TIME;
//			ent->v.teleport_time = 0;
		}

//		if (ent->v.weapon != host_client->old_v.weapon)
//			sc1 |= SC1_WEAPON;
		if (ent->GetBlueMana() != host_client->old_v.bluemana)
			sc1 |= SC1_BLUEMANA;
		if (ent->GetGreenMana() != host_client->old_v.greenmana)
			sc1 |= SC1_GREENMANA;
		if (ent->GetExperience() != host_client->old_v.experience)
			sc1 |= SC1_EXPERIENCE;
		if (ent->GetCntTorch() != host_client->old_v.cnt_torch)
			sc1 |= SC1_CNT_TORCH;
		if (ent->GetCntHBoost() != host_client->old_v.cnt_h_boost)
			sc1 |= SC1_CNT_H_BOOST;
		if (ent->GetCntSHBoost() != host_client->old_v.cnt_sh_boost)
			sc1 |= SC1_CNT_SH_BOOST;
		if (ent->GetCntManaBoost() != host_client->old_v.cnt_mana_boost)
			sc1 |= SC1_CNT_MANA_BOOST;
		if (ent->GetCntTeleport() != host_client->old_v.cnt_teleport)
			sc1 |= SC1_CNT_TELEPORT;
		if (ent->GetCntTome() != host_client->old_v.cnt_tome)
			sc1 |= SC1_CNT_TOME;
		if (ent->GetCntSummon() != host_client->old_v.cnt_summon)
			sc1 |= SC1_CNT_SUMMON;
		if (ent->GetCntInvisibility() != host_client->old_v.cnt_invisibility)
			sc1 |= SC1_CNT_INVISIBILITY;
		if (ent->GetCntGlyph() != host_client->old_v.cnt_glyph)
			sc1 |= SC1_CNT_GLYPH;
		if (ent->GetCntHaste() != host_client->old_v.cnt_haste)
			sc1 |= SC1_CNT_HASTE;
		if (ent->GetCntBlast() != host_client->old_v.cnt_blast)
			sc1 |= SC1_CNT_BLAST;
		if (ent->GetCntPolyMorph() != host_client->old_v.cnt_polymorph)
			sc1 |= SC1_CNT_POLYMORPH;
		if (ent->GetCntFlight() != host_client->old_v.cnt_flight)
			sc1 |= SC1_CNT_FLIGHT;
		if (ent->GetCntCubeOfForce() != host_client->old_v.cnt_cubeofforce)
			sc1 |= SC1_CNT_CUBEOFFORCE;
		if (ent->GetCntInvincibility() != host_client->old_v.cnt_invincibility)
			sc1 |= SC1_CNT_INVINCIBILITY;
		if (ent->GetArtifactActive() != host_client->old_v.artifact_active)
			sc1 |= SC1_ARTIFACT_ACTIVE;
		if (ent->GetArtifactLow() != host_client->old_v.artifact_low)
			sc1 |= SC1_ARTIFACT_LOW;
		if (ent->GetMoveType() != host_client->old_v.movetype)
			sc1 |= SC1_MOVETYPE;
		if (ent->GetCameraMode() != host_client->old_v.cameramode)
			sc1 |= SC1_CAMERAMODE;
		if (ent->GetHasted() != host_client->old_v.hasted)
			sc1 |= SC1_HASTED;
		if (ent->GetInventory() != host_client->old_v.inventory)
			sc1 |= SC1_INVENTORY;
		if (ent->GetRingsActive() != host_client->old_v.rings_active)
			sc1 |= SC1_RINGS_ACTIVE;

		if (ent->GetRingsLow() != host_client->old_v.rings_low)
			sc2 |= SC2_RINGS_LOW;
		if (ent->GetArmorAmulet() != host_client->old_v.armor_amulet)
			sc2 |= SC2_AMULET;
		if (ent->GetArmorBracer() != host_client->old_v.armor_bracer)
			sc2 |= SC2_BRACER;
		if (ent->GetArmorBreastPlate() != host_client->old_v.armor_breastplate)
			sc2 |= SC2_BREASTPLATE;
		if (ent->GetArmorHelmet() != host_client->old_v.armor_helmet)
			sc2 |= SC2_HELMET;
		if (ent->GetRingFlight() != host_client->old_v.ring_flight)
			sc2 |= SC2_FLIGHT_T;
		if (ent->GetRingWater() != host_client->old_v.ring_water)
			sc2 |= SC2_WATER_T;
		if (ent->GetRingTurning() != host_client->old_v.ring_turning)
			sc2 |= SC2_TURNING_T;
		if (ent->GetRingRegeneration() != host_client->old_v.ring_regeneration)
			sc2 |= SC2_REGEN_T;
//		if (ent->v.haste_time != host_client->old_v.haste_time)
//			sc2 |= SC2_HASTE_T;
//		if (ent->v.tome_time != host_client->old_v.tome_time)
//			sc2 |= SC2_TOME_T;
		if (ent->GetPuzzleInv1() != host_client->old_v.puzzle_inv1)
			sc2 |= SC2_PUZZLE1;
		if (ent->GetPuzzleInv2() != host_client->old_v.puzzle_inv2)
			sc2 |= SC2_PUZZLE2;
		if (ent->GetPuzzleInv3() != host_client->old_v.puzzle_inv3)
			sc2 |= SC2_PUZZLE3;
		if (ent->GetPuzzleInv4() != host_client->old_v.puzzle_inv4)
			sc2 |= SC2_PUZZLE4;
		if (ent->GetPuzzleInv5() != host_client->old_v.puzzle_inv5)
			sc2 |= SC2_PUZZLE5;
		if (ent->GetPuzzleInv6() != host_client->old_v.puzzle_inv6)
			sc2 |= SC2_PUZZLE6;
		if (ent->GetPuzzleInv7() != host_client->old_v.puzzle_inv7)
			sc2 |= SC2_PUZZLE7;
		if (ent->GetPuzzleInv8() != host_client->old_v.puzzle_inv8)
			sc2 |= SC2_PUZZLE8;
		if (ent->GetMaxHealth() != host_client->old_v.max_health)
			sc2 |= SC2_MAXHEALTH;
		if (ent->GetMaxMana() != host_client->old_v.max_mana)
			sc2 |= SC2_MAXMANA;
		if (ent->GetFlags() != host_client->old_v.flags)
			sc2 |= SC2_FLAGS;
	}

	if (!sc1 && !sc2)
		goto end;

	msg->WriteByte(hwsvc_update_inv);
	test = 0;
	if (sc1 & 0x000000ff)
		test |= 1;
	if (sc1 & 0x0000ff00)
		test |= 2;
	if (sc1 & 0x00ff0000)
		test |= 4;
	if (sc1 & 0xff000000)
		test |= 8;
	if (sc2 & 0x000000ff)
		test |= 16;
	if (sc2 & 0x0000ff00)
		test |= 32;
	if (sc2 & 0x00ff0000)
		test |= 64;
	if (sc2 & 0xff000000)
		test |= 128;

	msg->WriteByte(test);

	if (test & 1)
		msg->WriteByte(sc1 & 0xff);
	if (test & 2)
		msg->WriteByte((sc1 >> 8) & 0xff);
	if (test & 4)
		msg->WriteByte((sc1 >> 16) & 0xff);
	if (test & 8)
		msg->WriteByte((sc1 >> 24) & 0xff);
	if (test & 16)
		msg->WriteByte(sc2 & 0xff);
	if (test & 32)
		msg->WriteByte((sc2 >> 8) & 0xff);
	if (test & 64)
		msg->WriteByte((sc2 >> 16) & 0xff);
	if (test & 128)
		msg->WriteByte((sc2 >> 24) & 0xff);

	if (sc1 & SC1_HEALTH)
		msg->WriteShort(ent->GetHealth());
	if (sc1 & SC1_LEVEL)
		msg->WriteByte(ent->GetLevel());
	if (sc1 & SC1_INTELLIGENCE)
		msg->WriteByte(ent->GetIntelligence());
	if (sc1 & SC1_WISDOM)
		msg->WriteByte(ent->GetWisdom());
	if (sc1 & SC1_STRENGTH)
		msg->WriteByte(ent->GetStrength());
	if (sc1 & SC1_DEXTERITY)
		msg->WriteByte(ent->GetDexterity());
//	if (sc1 & SC1_WEAPON)
//		msg->WriteByte(ent->v.weapon);
	if (sc1 & SC1_BLUEMANA)
		msg->WriteByte(ent->GetBlueMana());
	if (sc1 & SC1_GREENMANA)
		msg->WriteByte(ent->GetGreenMana());
	if (sc1 & SC1_EXPERIENCE)
		msg->WriteLong(ent->GetExperience());
	if (sc1 & SC1_CNT_TORCH)
		msg->WriteByte(ent->GetCntTorch());
	if (sc1 & SC1_CNT_H_BOOST)
		msg->WriteByte(ent->GetCntHBoost());
	if (sc1 & SC1_CNT_SH_BOOST)
		msg->WriteByte(ent->GetCntSHBoost());
	if (sc1 & SC1_CNT_MANA_BOOST)
		msg->WriteByte(ent->GetCntManaBoost());
	if (sc1 & SC1_CNT_TELEPORT)
		msg->WriteByte(ent->GetCntTeleport());
	if (sc1 & SC1_CNT_TOME)
		msg->WriteByte(ent->GetCntTome());
	if (sc1 & SC1_CNT_SUMMON)
		msg->WriteByte(ent->GetCntSummon());
	if (sc1 & SC1_CNT_INVISIBILITY)
		msg->WriteByte(ent->GetCntInvisibility());
	if (sc1 & SC1_CNT_GLYPH)
		msg->WriteByte(ent->GetCntGlyph());
	if (sc1 & SC1_CNT_HASTE)
		msg->WriteByte(ent->GetCntHaste());
	if (sc1 & SC1_CNT_BLAST)
		msg->WriteByte(ent->GetCntBlast());
	if (sc1 & SC1_CNT_POLYMORPH)
		msg->WriteByte(ent->GetCntPolyMorph());
	if (sc1 & SC1_CNT_FLIGHT)
		msg->WriteByte(ent->GetCntFlight());
	if (sc1 & SC1_CNT_CUBEOFFORCE)
		msg->WriteByte(ent->GetCntCubeOfForce());
	if (sc1 & SC1_CNT_INVINCIBILITY)
		msg->WriteByte(ent->GetCntInvincibility());
	if (sc1 & SC1_ARTIFACT_ACTIVE)
		msg->WriteByte(ent->GetArtifactActive());
	if (sc1 & SC1_ARTIFACT_LOW)
		msg->WriteByte(ent->GetArtifactLow());
	if (sc1 & SC1_MOVETYPE)
		msg->WriteByte(ent->GetMoveType());
	if (sc1 & SC1_CAMERAMODE)
		msg->WriteByte(ent->GetCameraMode());
	if (sc1 & SC1_HASTED)
		msg->WriteFloat(ent->GetHasted());
	if (sc1 & SC1_INVENTORY)
		msg->WriteByte(ent->GetInventory());
	if (sc1 & SC1_RINGS_ACTIVE)
		msg->WriteByte(ent->GetRingsActive());

	if (sc2 & SC2_RINGS_LOW)
		msg->WriteByte(ent->GetRingsLow());
	if (sc2 & SC2_AMULET)
		msg->WriteByte(ent->GetArmorAmulet());
	if (sc2 & SC2_BRACER)
		msg->WriteByte(ent->GetArmorBracer());
	if (sc2 & SC2_BREASTPLATE)
		msg->WriteByte(ent->GetArmorBreastPlate());
	if (sc2 & SC2_HELMET)
		msg->WriteByte(ent->GetArmorHelmet());
	if (sc2 & SC2_FLIGHT_T)
		msg->WriteByte(ent->GetRingFlight());
	if (sc2 & SC2_WATER_T)
		msg->WriteByte(ent->GetRingWater());
	if (sc2 & SC2_TURNING_T)
		msg->WriteByte(ent->GetRingTurning());
	if (sc2 & SC2_REGEN_T)
		msg->WriteByte(ent->GetRingRegeneration());
//	if (sc2 & SC2_HASTE_T)
//		msg->WriteFloat(ent->v.haste_time);
//	if (sc2 & SC2_TOME_T)
//		msg->WriteFloat(ent->v.tome_time);
	if (sc2 & SC2_PUZZLE1)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv1()));
	if (sc2 & SC2_PUZZLE2)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv2()));
	if (sc2 & SC2_PUZZLE3)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv3()));
	if (sc2 & SC2_PUZZLE4)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv4()));
	if (sc2 & SC2_PUZZLE5)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv5()));
	if (sc2 & SC2_PUZZLE6)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv6()));
	if (sc2 & SC2_PUZZLE7)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv7()));
	if (sc2 & SC2_PUZZLE8)
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv8()));
	if (sc2 & SC2_MAXHEALTH)
		msg->WriteShort(ent->GetMaxHealth());
	if (sc2 & SC2_MAXMANA)
		msg->WriteByte(ent->GetMaxMana());
	if (sc2 & SC2_FLAGS)
		msg->WriteFloat(ent->GetFlags());

end:
	host_client->old_v.movetype = ent->GetMoveType();
	host_client->old_v.health = ent->GetHealth();
	host_client->old_v.max_health = ent->GetMaxHealth();
	host_client->old_v.bluemana = ent->GetBlueMana();
	host_client->old_v.greenmana = ent->GetGreenMana();
	host_client->old_v.max_mana = ent->GetMaxMana();
	host_client->old_v.armor_amulet = ent->GetArmorAmulet();
	host_client->old_v.armor_bracer = ent->GetArmorBracer();
	host_client->old_v.armor_breastplate = ent->GetArmorBreastPlate();
	host_client->old_v.armor_helmet = ent->GetArmorHelmet();
	host_client->old_v.level = ent->GetLevel();
	host_client->old_v.intelligence = ent->GetIntelligence();
	host_client->old_v.wisdom = ent->GetWisdom();
	host_client->old_v.dexterity = ent->GetDexterity();
	host_client->old_v.strength = ent->GetStrength();
	host_client->old_v.experience = ent->GetExperience();
	host_client->old_v.ring_flight = ent->GetRingFlight();
	host_client->old_v.ring_water = ent->GetRingWater();
	host_client->old_v.ring_turning = ent->GetRingTurning();
	host_client->old_v.ring_regeneration = ent->GetRingRegeneration();
	host_client->old_v.puzzle_inv1 = ent->GetPuzzleInv1();
	host_client->old_v.puzzle_inv2 = ent->GetPuzzleInv2();
	host_client->old_v.puzzle_inv3 = ent->GetPuzzleInv3();
	host_client->old_v.puzzle_inv4 = ent->GetPuzzleInv4();
	host_client->old_v.puzzle_inv5 = ent->GetPuzzleInv5();
	host_client->old_v.puzzle_inv6 = ent->GetPuzzleInv6();
	host_client->old_v.puzzle_inv7 = ent->GetPuzzleInv7();
	host_client->old_v.puzzle_inv8 = ent->GetPuzzleInv8();
	host_client->old_v.flags = ent->GetFlags();
	host_client->old_v.flags2 = ent->GetFlags2();
	host_client->old_v.rings_active = ent->GetRingsActive();
	host_client->old_v.rings_low = ent->GetRingsLow();
	host_client->old_v.artifact_active = ent->GetArtifactActive();
	host_client->old_v.artifact_low = ent->GetArtifactLow();
	host_client->old_v.hasted = ent->GetHasted();
	host_client->old_v.inventory = ent->GetInventory();
	host_client->old_v.cnt_torch = ent->GetCntTorch();
	host_client->old_v.cnt_h_boost = ent->GetCntHBoost();
	host_client->old_v.cnt_sh_boost = ent->GetCntSHBoost();
	host_client->old_v.cnt_mana_boost = ent->GetCntManaBoost();
	host_client->old_v.cnt_teleport = ent->GetCntTeleport();
	host_client->old_v.cnt_tome = ent->GetCntTome();
	host_client->old_v.cnt_summon = ent->GetCntSummon();
	host_client->old_v.cnt_invisibility = ent->GetCntInvisibility();
	host_client->old_v.cnt_glyph = ent->GetCntGlyph();
	host_client->old_v.cnt_haste = ent->GetCntHaste();
	host_client->old_v.cnt_blast = ent->GetCntBlast();
	host_client->old_v.cnt_polymorph = ent->GetCntPolyMorph();
	host_client->old_v.cnt_flight = ent->GetCntFlight();
	host_client->old_v.cnt_cubeofforce = ent->GetCntCubeOfForce();
	host_client->old_v.cnt_invincibility = ent->GetCntInvincibility();
	host_client->old_v.cameramode = ent->GetCameraMode();
}


#ifdef MGNET
/*
=============
float cardioid_rating (qhedict_t *targ , qhedict_t *self)

Determines how important a visclient is- based on offset from
forward angle and distance.  Resultant pattern is a somewhat
extended 3-dimensional cleaved cardioid with each point on
the surface being equal in priority(0) and increasing linearly
towards equal priority(1) along a straight line to the center.
=============
*/
static float cardioid_rating (qhedict_t *targ , qhedict_t *self)
{
	vec3_t	vec,spot1,spot2;
	vec3_t	forward,right,up;
	float	dot,dist;

    AngleVectors (self->GetVAngle(),forward,right,up);

	VectorAdd(self->GetOrigin(),self->GetViewOfs(),spot1);
	VectorSubtract(targ->v.absmax,targ->v.absmin,spot2);
	VectorMA(targ->v.absmin,0.5,spot2,spot2);

	VectorSubtract(spot2,spot1,vec);
	dist = VectorNormalize(vec);
	dot = DotProduct(vec,forward);//from 1 to -1
	
    if (dot < -0.3)//see only from -125 to 125 degrees
		return false;
	
	if(dot>0)//to front of perpendicular plane to forward
		dot*=31;//much more distance leniency in front, max dist = 2048 directly in front
	dot = (dot + 1) * 64;//64 = base distance if along the perpendicular plane, max is 2048 straight ahead
	if(dist>=dot)//too far away for that angle to be important
		return false;

	//from 0.000000? to almost 1
	return 1 - (dist/dot);//The higher this number is, the more important it is to send this ent
}

static int MAX_VISCLIENTS = 2;
#endif

/*
=============
SV_WritePlayersToClient

=============
*/
void SV_WritePlayersToClient (client_t *client, qhedict_t *clent, byte *pvs, QMsg *msg)
{
	int			i, j;
	client_t	*cl;
	qhedict_t		*ent;
	int			msec;
	hwusercmd_t	cmd;
	int			pflags;
	int			invis_level;
	qboolean	playermodel = false;

#ifdef MGNET
	int			k, l;
	int			visclient[HWMAX_CLIENTS];
	int			forcevisclient[HWMAX_CLIENTS];
	int			cl_v_priority[HWMAX_CLIENTS];
	int			cl_v_psort[HWMAX_CLIENTS];
	int			totalvc,num_eliminated;

	int numvc = 0;
	int forcevc = 0;
#endif

	for (j=0,cl=svs.clients ; j<HWMAX_CLIENTS ; j++,cl++)
	{
		if (cl->state != cs_spawned)
			continue;

		ent = cl->edict;


		// ZOID visibility tracking
		invis_level = false;
		if (ent != clent &&
			!(client->spec_track && client->spec_track - 1 == j)) 
		{
			if ((int)ent->GetEffects() & EF_NODRAW)
			{
				if(dmMode->value==DM_SIEGE&&clent->GetPlayerClass()==CLASS_DWARF)
					invis_level = false;
				else
					invis_level = true;//still can hear
			}
#ifdef MGNET
			//could be invisiblenow and still sent, cull out by other methods as well
			if (cl->spectator)
#else
			else if (cl->spectator)
#endif
			{
				invis_level = 2;//no vis or weaponsound
			}
			else
			{
				// ignore if not touching a PV leaf
				for (i=0 ; i < ent->num_leafs ; i++)
				{
					int l = CM_LeafCluster(ent->LeafNums[i]);
					if (pvs[l >> 3] & (1 << (l & 7)))
						break;
				}
				if (i == ent->num_leafs)
					invis_level = 2;//no vis or weaponsound
			}
		}
		
		if(invis_level==true)
		{//ok to send weaponsound
			if(ent->GetWpnSound())
			{
				msg->WriteByte(hwsvc_player_sound);
				msg->WriteByte(j);
				for (i=0 ; i<3 ; i++)
					msg->WriteCoord(ent->GetOrigin()[i]);
				msg->WriteShort(ent->GetWpnSound());
			}
		}
		if(invis_level>0)
			continue;

#ifdef MGNET
		if(!cl->skipsend&&ent != clent)
		{//don't count self
			visclient[numvc]=j;
			numvc++;
		}
		else
		{//Self, or Wasn't sent last time, must send this frame
			cl->skipsend = false;
			forcevisclient[forcevc]=j;
			forcevc++;
			continue;
		}
	}

	totalvc=numvc+forcevc;
	if(totalvc>MAX_VISCLIENTS)//You have more than 5 clients in your view, cull some out
	{//prioritize by 
		//line of sight (20%)
		//distance (50%)
		//dot off v_forward (30%)
		//put this in "priority" then sort by priority
		//and send the highest priority
		//number of highest priority sent depends on how
		//many are forced through because they were skipped
		//last send.  Ideally, no more than 5 are sent.
		for (j=0; j<numvc && totalvc>MAX_VISCLIENTS ; j++)
		{//priority 1 - if behind, cull out
			for(k=0, cl = svs.clients; k<visclient[j]; k++, cl++);
//			cl=svs.clients+visclient[j];
			ent = cl->edict;
			cl_v_priority[j] = cardioid_rating(ent,clent);
			if(!cl_v_priority[j])
			{//% they won't be sent, l represents how many were forced through
				cl->skipsend = true;
				totalvc--;
			}
		}

		if(totalvc>MAX_VISCLIENTS)
		{//still more than 5 inside cardioid, sort by priority
		//and drop those after 5

			//CHECK this make sure it works
			for (i=0 ; i<numvc ; i++)//do this as many times as there are visclients
				for (j=0 ; j<numvc-1-i ; j++)//go through the list
					if (cl_v_priority[j] < cl_v_priority[j+1])
					{
						k = cl_v_psort[j];//store lower one
						cl_v_psort[j] = cl_v_psort[j+1];//put next one in it's spot
						cl_v_psort[j+1] = k;//put lower one next
					}
			num_eliminated = 0;
			while(totalvc>MAX_VISCLIENTS)
			{//eliminate all over 5 unless not sent last time
				if(!cl->skipsend)
				{
					cl=svs.clients+cl_v_psort[numvc - num_eliminated];
					cl->skipsend = true;
					num_eliminated++;
					totalvc--;
				}
			}
		}
		//Alternate Possibilities: ...?
		//priority 2 - if too many numleafs away, cull out
		//priority 3 - don't send those farthest away, flag for re-send next time
		//priority 4 - flat percentage based on how many over 5
/*			if(rand()%10<(numvc + l - 5))
			{//% they won't be sent, l represents how many were forced through
				cl->skipsend = true;
				numvc--;
			}*/
		//priority 5 - send less info on clients
	}

	for (j=0, l=0, k=0, cl=svs.clients; j<HWMAX_CLIENTS ; j++,cl++)
	{//priority 1 - if behind, cull out
		if(forcevisclient[l]==j&&l<=forcevc)
			l++;
		else if(visclient[k]==j&&k<=numvc)
			k++;//clent is always forced
		else
			continue;//not in PVS or forced

		if(cl->skipsend)
		{//still 2 bytes, but what ya gonna do?
			msg->WriteByte(hwsvc_playerskipped);
			msg->WriteByte(j);
			continue;
		}

		ent = cl->edict;
#endif

		pflags = PF_MSEC | PF_COMMAND;

		if (ent->v.modelindex != sv_playermodel[0] &&//paladin
			ent->v.modelindex != sv_playermodel[1] &&//crusader
			ent->v.modelindex != sv_playermodel[2] &&//necro
			ent->v.modelindex != sv_playermodel[3] &&//assassin
			ent->v.modelindex != sv_playermodel[4] &&//succ
			ent->v.modelindex != sv_playermodel[5])//dwarf
			pflags |= PF_MODEL;
		else
			playermodel = true;

		for (i=0 ; i<3 ; i++)
			if (ent->GetVelocity()[i])
				pflags |= PF_VELOCITY1<<i;
		if (((long)ent->GetEffects() & 0xff))
			pflags |= PF_EFFECTS;
		if (((long)ent->GetEffects() & 0xff00))
			pflags |= PF_EFFECTS2;
		if (ent->GetSkin())
		{
			if(dmMode->value==DM_SIEGE&&playermodel&&ent->GetSkin()==1);
			//in siege, don't send skin if 2nd skin and using
			//playermodel, it will know on other side- saves
			//us 1 byte per client per frame!
			else
				pflags |= PF_SKINNUM;
		}
		if (ent->GetHealth() <= 0)
			pflags |= PF_DEAD;
		if (ent->GetHull() == HULL_CROUCH)
			pflags |= PF_CROUCH;

		if (cl->spectator)
		{	// only sent origin and velocity to spectators
			pflags &= PF_VELOCITY1 | PF_VELOCITY2 | PF_VELOCITY3;
		}
		else if (ent == clent)
		{	// don't send a lot of data on personal entity
			pflags &= ~(PF_MSEC|PF_COMMAND);
			if (ent->GetWeaponFrame())
				pflags |= PF_WEAPONFRAME;
		}
		if (ent->GetDrawFlags())
		{
			pflags |= PF_DRAWFLAGS;
		}
		if (ent->GetScale() != 0 && ent->GetScale() != 1.0)
		{
			pflags |= PF_SCALE;
		}
		if (ent->GetAbsLight() != 0)
		{
			pflags |= PF_ABSLIGHT;
		}
		if (ent->GetWpnSound())
		{
			pflags |= PF_SOUND;
		}

		msg->WriteByte(hwsvc_playerinfo);
		msg->WriteByte(j);
		msg->WriteShort(pflags);

		for (i=0 ; i<3 ; i++)
			msg->WriteCoord(ent->GetOrigin()[i]);

		msg->WriteByte(ent->GetFrame());

		if (pflags & PF_MSEC)
		{
			msec = 1000*(sv.time - cl->localtime);
			if (msec > 255)
				msec = 255;
			msg->WriteByte(msec);
		}

		if (pflags & PF_COMMAND)
		{
			cmd = cl->lastcmd;

			if (ent->GetHealth() <= 0)
			{	// don't show the corpse looking around...
				cmd.angles[0] = 0;
				cmd.angles[1] = ent->GetAngles()[1];
				cmd.angles[0] = 0;
			}

			cmd.buttons = 0;	// never send buttons
			cmd.impulse = 0;	// never send impulses
			MSG_WriteUsercmd (msg, &cmd, false);
		}

		for (i=0 ; i<3 ; i++)
			if (pflags & (PF_VELOCITY1<<i) )
				msg->WriteShort(ent->GetVelocity()[i]);

//rjr
		if (pflags & PF_MODEL)
			msg->WriteShort(ent->v.modelindex);

		if (pflags & PF_SKINNUM)
			msg->WriteByte(ent->GetSkin());

		if (pflags & PF_EFFECTS)
			msg->WriteByte(((long)ent->GetEffects() & 0xff));

		if (pflags & PF_EFFECTS2)
			msg->WriteByte(((long)ent->GetEffects() & 0xff00)>>8);

		if (pflags & PF_WEAPONFRAME)
			msg->WriteByte(ent->GetWeaponFrame());

		if (pflags & PF_DRAWFLAGS)
		{
			msg->WriteByte(ent->GetDrawFlags());
		}
		if (pflags & PF_SCALE)
		{
			msg->WriteByte((int)(ent->GetScale()*100.0)&255);
		}
		if (pflags & PF_ABSLIGHT)
		{
			msg->WriteByte((int)(ent->GetAbsLight()*100.0)&255);
		}
		if (pflags & PF_SOUND)
		{
			msg->WriteShort(ent->GetWpnSound());
		}
	}
}

/*
=============
SV_WriteEntitiesToClient

Encodes the current state of the world as
a hwsvc_packetentities messages and possibly
a hwsvc_nails message and
hwsvc_playerinfo messages
=============
*/
void SV_WriteEntitiesToClient (client_t *client, QMsg *msg)
{
	int		e, i;
	byte	*pvs;
	vec3_t	org;
	qhedict_t	*ent;
	hwpacket_entities_t	*pack;
	qhedict_t	*clent;
	client_frame_t	*frame;
	h2entity_state_t	*state;

	// this is the frame we are creating
	frame = &client->frames[client->netchan.incomingSequence & UPDATE_MASK_HW];

	// find the client's PVS
	clent = client->edict;
	VectorAdd (clent->GetOrigin(), clent->GetViewOfs(), org);
	pvs = SV_FatPVS (org);

	// send over the players in the PVS
	SV_WritePlayersToClient (client, clent, pvs, msg);
	
	// put other visible entities into either a packet_entities or a nails message
	pack = &frame->entities;
	pack->num_entities = 0;

//	numnails = 0;
	nummissiles = 0;
	numravens = 0;
	numraven2s = 0;

	for (e=HWMAX_CLIENTS+1, ent=EDICT_NUM(e) ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		// ignore ents without visible models
		if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
			continue;
	
		if ((int)ent->GetEffects() & EF_NODRAW)
		{
			continue;
		}

		// ignore if not touching a PV leaf
		for (i=0 ; i < ent->num_leafs ; i++)
		{
			int l = CM_LeafCluster(ent->LeafNums[i]);
			if (pvs[l >> 3] & (1 << (l & 7)))
			{
				break;
			}
		}

		if (i == ent->num_leafs)
			continue;		// not visible

//		if (SV_AddNailUpdate (ent))
//			continue;
		if (SV_AddMissileUpdate (ent))
			continue;	// added to the special update list

		// add to the packetentities
		if (pack->num_entities == HWMAX_PACKET_ENTITIES)
			continue;	// all full

		state = &pack->entities[pack->num_entities];
		pack->num_entities++;

		state->number = e;
		state->flags = 0;
		VectorCopy (ent->GetOrigin(), state->origin);
		VectorCopy (ent->GetAngles(), state->angles);
		state->modelindex = ent->v.modelindex;
		state->frame = ent->GetFrame();
		state->colormap = ent->GetColorMap();
		state->skinnum = ent->GetSkin();
		state->effects = ent->GetEffects();
		state->scale = (int)(ent->GetScale()*100.0)&255;
		state->drawflags = ent->GetDrawFlags();
		state->abslight = (int)(ent->GetAbsLight()*255.0)&255;
		//clear sound so it doesn't send twice
		state->wpn_sound = ent->GetWpnSound();
	}

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SV_EmitPacketEntities (client, pack, msg);

	// now add the specialized nail update
//	SV_EmitNailUpdate (msg);
	SV_EmitPackedEntities (msg);
}
