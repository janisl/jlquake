//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../progsvm/progsvm.h"
#include "local.h"
#include "../../common/Common.h"
#include "../../common/strings.h"
#include "../../common/file_formats/bsp29.h"
#include "../../common/message_utils.h"

/*
=============================================================================

The PVS must include a small area around the client to allow head bobbing
or other small motion on the client side.  Otherwise, a bob might cause an
entity that should be visible to not show up, especially when the bob
crosses a waterline.

=============================================================================
*/

static byte qh_fatpvs[BSP29_MAX_MAP_LEAFS / 8];

// because there can be a lot of nails, there is a special
// network protocol for them
enum { MAX_NAILS = 32 };
static qhedict_t* nails[MAX_NAILS];
static int numnails;

enum { MAX_MISSILES_H2 = 32 };
static qhedict_t* missiles[MAX_MISSILES_H2];
static qhedict_t* ravens[MAX_MISSILES_H2];
static qhedict_t* raven2s[MAX_MISSILES_H2];
static int nummissiles;
static int numravens;
static int numraven2s;

//	Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
// given point.
static byte* SVQH_FatPVS(const vec3_t org)
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
		common->FatalError("SVQH_FatPVS: count < 1");
	}

	// convert leafs to clusters
	for (int i = 0; i < count; i++)
	{
		leafs[i] = CM_LeafCluster(leafs[i]);
	}

	int fatbytes = (CM_NumClusters() + 31) >> 3;
	Com_Memcpy(qh_fatpvs, CM_ClusterPVS(leafs[0]), fatbytes);
	// or in all the other leaf bits
	for (int i = 1; i < count; i++)
	{
		byte* pvs = CM_ClusterPVS(leafs[i]);
		for (int j = 0; j < fatbytes; j++)
		{
			qh_fatpvs[j] |= pvs[j];
		}
	}
	return qh_fatpvs;
}

static void SVQ1_WriteEntity(qhedict_t* ent, int e, QMsg* msg)
{
	int bits = 0;

	for (int i = 0; i < 3; i++)
	{
		float miss = ent->GetOrigin()[i] - ent->q1_baseline.origin[i];
		if (miss < -0.1 || miss > 0.1)
		{
			bits |= Q1U_ORIGIN1 << i;
		}
	}

	if (ent->GetAngles()[0] != ent->q1_baseline.angles[0])
	{
		bits |= Q1U_ANGLE1;
	}

	if (ent->GetAngles()[1] != ent->q1_baseline.angles[1])
	{
		bits |= Q1U_ANGLE2;
	}

	if (ent->GetAngles()[2] != ent->q1_baseline.angles[2])
	{
		bits |= Q1U_ANGLE3;
	}

	if (ent->GetMoveType() == QHMOVETYPE_STEP)
	{
		bits |= Q1U_NOLERP;	// don't mess up the step animation

	}
	if (ent->q1_baseline.colormap != ent->GetColorMap())
	{
		bits |= Q1U_COLORMAP;
	}

	if (ent->q1_baseline.skinnum != ent->GetSkin())
	{
		bits |= Q1U_SKIN;
	}

	if (ent->q1_baseline.frame != ent->GetFrame())
	{
		bits |= Q1U_FRAME;
	}

	if (ent->q1_baseline.effects != ent->GetEffects())
	{
		bits |= Q1U_EFFECTS;
	}

	if (ent->q1_baseline.modelindex != ent->v.modelindex)
	{
		bits |= Q1U_MODEL;
	}

	if (e >= 256)
	{
		bits |= Q1U_LONGENTITY;
	}

	if (bits >= 256)
	{
		bits |= Q1U_MOREBITS;
	}

	//
	// write the message
	//
	msg->WriteByte(bits | Q1U_SIGNAL);

	if (bits & Q1U_MOREBITS)
	{
		msg->WriteByte(bits >> 8);
	}
	if (bits & Q1U_LONGENTITY)
	{
		msg->WriteShort(e);
	}
	else
	{
		msg->WriteByte(e);
	}

	if (bits & Q1U_MODEL)
	{
		msg->WriteByte(ent->v.modelindex);
	}
	if (bits & Q1U_FRAME)
	{
		msg->WriteByte(ent->GetFrame());
	}
	if (bits & Q1U_COLORMAP)
	{
		msg->WriteByte(ent->GetColorMap());
	}
	if (bits & Q1U_SKIN)
	{
		msg->WriteByte(ent->GetSkin());
	}
	if (bits & Q1U_EFFECTS)
	{
		msg->WriteByte(ent->GetEffects());
	}
	if (bits & Q1U_ORIGIN1)
	{
		msg->WriteCoord(ent->GetOrigin()[0]);
	}
	if (bits & Q1U_ANGLE1)
	{
		msg->WriteAngle(ent->GetAngles()[0]);
	}
	if (bits & Q1U_ORIGIN2)
	{
		msg->WriteCoord(ent->GetOrigin()[1]);
	}
	if (bits & Q1U_ANGLE2)
	{
		msg->WriteAngle(ent->GetAngles()[1]);
	}
	if (bits & Q1U_ORIGIN3)
	{
		msg->WriteCoord(ent->GetOrigin()[2]);
	}
	if (bits & Q1U_ANGLE3)
	{
		msg->WriteAngle(ent->GetAngles()[2]);
	}
}

void SVQ1_WriteEntitiesToClient(qhedict_t* clent, QMsg* msg)
{
	// find the client's PVS
	vec3_t org;
	VectorAdd(clent->GetOrigin(), clent->GetViewOfs(), org);
	byte* pvs = SVQH_FatPVS(org);

	// send over all entities (excpet the client) that touch the pvs
	qhedict_t* ent = NEXT_EDICT(sv.qh_edicts);
	for (int e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALLWAYS sent
		{
			// ignore ents without visible models
			if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
			{
				continue;
			}

			int i;
			for (i = 0; i < ent->num_leafs; i++)
			{
				int l = CM_LeafCluster(ent->LeafNums[i]);
				if (pvs[l >> 3] & (1 << (l & 7)))
				{
					break;
				}
			}

			if (i == ent->num_leafs)
			{
				continue;		// not visible
			}
		}

		if (msg->maxsize - msg->cursize < 16)
		{
			common->Printf("packet overflow\n");
			return;
		}

		// send an update
		SVQ1_WriteEntity(ent, e, msg);
	}
}

void SVH2_PrepareClientEntities(client_t* client, qhedict_t* clent, QMsg* msg)
{
	enum
	{
		CLIENT_FRAME_INIT = 255,
		CLIENT_FRAME_RESET = 254,

		ENT_CLEARED = 2,

		CLEAR_LIMIT = 2
	};

	int client_num = client - svs.clients;
	h2client_state2_t* state = &sv.h2_states[client_num];
	h2client_frames_t* reference = &state->frames[0];

	if (client->h2_last_sequence != client->h2_current_sequence)
	{
		// Old sequence
		client->h2_current_frame++;
		if (client->h2_current_frame > H2MAX_FRAMES + 1)
		{
			client->h2_current_frame = H2MAX_FRAMES + 1;
		}
	}
	else if (client->h2_last_frame == CLIENT_FRAME_INIT ||
			 client->h2_last_frame == 0 ||
			 client->h2_last_frame == H2MAX_FRAMES + 1)
	{
		// Reference expired in current sequence
		client->h2_current_frame = 1;
		client->h2_current_sequence++;
	}
	else if (client->h2_last_frame >= 1 && client->h2_last_frame <= client->h2_current_frame)
	{
		// Got a valid frame
		*reference = state->frames[client->h2_last_frame];

		for (int i = 0; i < reference->count; i++)
		{
			if (reference->states[i].flags & ENT_CLEARED)
			{
				int e = reference->states[i].number;
				qhedict_t* ent = QH_EDICT_NUM(e);
				if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
				{
					ent->h2_baseline.ClearCount[client_num]++;
				}
				else if (ent->h2_baseline.ClearCount[client_num] == CLEAR_LIMIT)
				{
					ent->h2_baseline.ClearCount[client_num] = 3;
					reference->states[i].flags &= ~ENT_CLEARED;
				}
			}
		}
		client->h2_current_frame = 1;
		client->h2_current_sequence++;
	}
	else
	{
		// Normal frame advance
		client->h2_current_frame++;
		if (client->h2_current_frame > H2MAX_FRAMES + 1)
		{
			client->h2_current_frame = H2MAX_FRAMES + 1;
		}
	}

	bool DoPlayer = false;
	bool DoMonsters = false;
	bool DoMissiles = false;
	bool DoMisc = false;

	if (svh2_update_player->integer)
	{
		DoPlayer = (client->h2_current_sequence % (svh2_update_player->integer)) == 0;
	}
	if (svh2_update_monsters->integer)
	{
		DoMonsters = (client->h2_current_sequence % (svh2_update_monsters->integer)) == 0;
	}
	if (svh2_update_missiles->integer)
	{
		DoMissiles = (client->h2_current_sequence % (svh2_update_missiles->integer)) == 0;
	}
	if (svh2_update_misc->integer)
	{
		DoMisc = (client->h2_current_sequence % (svh2_update_misc->integer)) == 0;
	}

	h2client_frames_t* build = &state->frames[client->h2_current_frame];
	Com_Memset(build, 0, sizeof(*build));
	client->h2_last_frame = CLIENT_FRAME_RESET;

	short NumToRemove = 0;
	msg->WriteByte(h2svc_reference);
	msg->WriteByte(client->h2_current_frame);
	msg->WriteByte(client->h2_current_sequence);

	// find the client's PVS
	vec3_t org;
	if (clent->GetCameraMode())
	{
		qhedict_t* ent = PROG_TO_EDICT(clent->GetCameraMode());
		VectorCopy(ent->GetOrigin(), org);
	}
	else
	{
		VectorAdd(clent->GetOrigin(), clent->GetViewOfs(), org);
	}

	byte* pvs = SVQH_FatPVS(org);

	// send over all entities (excpet the client) that touch the pvs
	int position = 0;
	short RemoveList[MAX_CLIENT_STATES_H2];
	qhedict_t* ent = NEXT_EDICT(sv.qh_edicts);
	for (int e = 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		bool DoRemove = false;
		// don't send if flagged for NODRAW and there are no lighting effects
		if (ent->GetEffects() == H2EF_NODRAW)
		{
			DoRemove = true;
			goto skipA;
		}

		// ignore if not touching a PV leaf
		if (ent != clent)	// clent is ALWAYS sent
		{	// ignore ents without visible models
			if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
			{
				DoRemove = true;
				goto skipA;
			}

			int i;
			for (i = 0; i < ent->num_leafs; i++)
			{
				int l = CM_LeafCluster(ent->LeafNums[i]);
				if (pvs[l >> 3] & (1 << (l & 7)))
				{
					break;
				}
			}

			if (i == ent->num_leafs)
			{
				DoRemove = true;
				goto skipA;
			}
		}

skipA:
		bool IgnoreEnt = false;
		int flagtest = (int)ent->GetFlags();
		if (!DoRemove)
		{
			if (flagtest & QHFL_CLIENT)
			{
				if (!DoPlayer)
				{
					IgnoreEnt = true;
				}
			}
			else if (flagtest & QHFL_MONSTER)
			{
				if (!DoMonsters)
				{
					IgnoreEnt = true;
				}
			}
			else if (ent->GetMoveType() == QHMOVETYPE_FLYMISSILE ||
					 ent->GetMoveType() == H2MOVETYPE_BOUNCEMISSILE ||
					 ent->GetMoveType() == QHMOVETYPE_BOUNCE)
			{
				if (!DoMissiles)
				{
					IgnoreEnt = true;
				}
			}
			else
			{
				if (!DoMisc)
				{
					IgnoreEnt = true;
				}
			}
		}

		int bits = 0;

		while (position < reference->count &&
			e > reference->states[position].number)
		{
			position++;
		}

		bool FoundInList;
		h2entity_state_t* ref_ent;
		h2entity_state_t build_ent;
		if (position < reference->count && reference->states[position].number == e)
		{
			FoundInList = true;
			if (DoRemove)
			{
				RemoveList[NumToRemove] = e;
				NumToRemove++;
				continue;
			}
			else
			{
				ref_ent = &reference->states[position];
			}
		}
		else
		{
			if (DoRemove || IgnoreEnt)
			{
				continue;
			}

			ref_ent = &build_ent;

			build_ent.number = e;
			build_ent.origin[0] = ent->h2_baseline.origin[0];
			build_ent.origin[1] = ent->h2_baseline.origin[1];
			build_ent.origin[2] = ent->h2_baseline.origin[2];
			build_ent.angles[0] = ent->h2_baseline.angles[0];
			build_ent.angles[1] = ent->h2_baseline.angles[1];
			build_ent.angles[2] = ent->h2_baseline.angles[2];
			build_ent.modelindex = ent->h2_baseline.modelindex;
			build_ent.frame = ent->h2_baseline.frame;
			build_ent.colormap = ent->h2_baseline.colormap;
			build_ent.skinnum = ent->h2_baseline.skinnum;
			build_ent.effects = ent->h2_baseline.effects;
			build_ent.scale = ent->h2_baseline.scale;
			build_ent.drawflags = ent->h2_baseline.drawflags;
			build_ent.abslight = ent->h2_baseline.abslight;
			build_ent.flags = 0;

			FoundInList = false;
		}

		h2entity_state_t* set_ent = &build->states[build->count];
		build->count++;
		if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
		{
			Com_Memset(ref_ent,0,sizeof(*ref_ent));
			ref_ent->number = e;
		}
		*set_ent = *ref_ent;

		if (IgnoreEnt)
		{
			continue;
		}

		// send an update
		for (int i = 0; i < 3; i++)
		{
			float miss = ent->GetOrigin()[i] - ref_ent->origin[i];
			if (miss < -0.1 || miss > 0.1)
			{
				bits |= H2U_ORIGIN1 << i;
				set_ent->origin[i] = ent->GetOrigin()[i];
			}
		}

		if (ent->GetAngles()[0] != ref_ent->angles[0])
		{
			bits |= H2U_ANGLE1;
			set_ent->angles[0] = ent->GetAngles()[0];
		}

		if (ent->GetAngles()[1] != ref_ent->angles[1])
		{
			bits |= H2U_ANGLE2;
			set_ent->angles[1] = ent->GetAngles()[1];
		}

		if (ent->GetAngles()[2] != ref_ent->angles[2])
		{
			bits |= H2U_ANGLE3;
			set_ent->angles[2] = ent->GetAngles()[2];
		}

		if (ent->GetMoveType() == QHMOVETYPE_STEP)
		{
			bits |= H2U_NOLERP;	// don't mess up the step animation

		}
		if (ref_ent->colormap != ent->GetColorMap())
		{
			bits |= H2U_COLORMAP;
			set_ent->colormap = ent->GetColorMap();
		}

		if (ref_ent->skinnum != ent->GetSkin() ||
			ref_ent->drawflags != ent->GetDrawFlags())
		{
			bits |= H2U_SKIN;
			set_ent->skinnum = ent->GetSkin();
			set_ent->drawflags = ent->GetDrawFlags();
		}

		if (ref_ent->frame != ent->GetFrame())
		{
			bits |= H2U_FRAME;
			set_ent->frame = ent->GetFrame();
		}

		if (ref_ent->effects != ent->GetEffects())
		{
			bits |= H2U_EFFECTS;
			set_ent->effects = ent->GetEffects();
		}

		if (flagtest & 0xff000000)
		{
			common->Error("Invalid flags setting for class %s", PR_GetString(ent->GetClassName()));
			return;
		}

		int temp_index = ent->v.modelindex;
		if (((int)ent->GetFlags() & H2FL_CLASS_DEPENDENT) && ent->GetModel())
		{
			char NewName[MAX_QPATH];
			String::Cpy(NewName, PR_GetString(ent->GetModel()));
			NewName[String::Length(NewName) - 5] = client->h2_playerclass + 48;
			temp_index = SVQH_ModelIndex(NewName);
		}

		if (ref_ent->modelindex != temp_index)
		{
			bits |= H2U_MODEL;
			set_ent->modelindex = temp_index;
		}

		if (ref_ent->scale != ((int)(ent->GetScale() * 100.0) & 255) ||
			ref_ent->abslight != ((int)(ent->GetAbsLight() * 255.0) & 255))
		{
			bits |= H2U_SCALE;
			set_ent->scale = ((int)(ent->GetScale() * 100.0) & 255);
			set_ent->abslight = (int)(ent->GetAbsLight() * 255.0) & 255;
		}

		if (ent->h2_baseline.ClearCount[client_num] < CLEAR_LIMIT)
		{
			bits |= H2U_CLEAR_ENT;
			set_ent->flags |= ENT_CLEARED;
		}

		if (!bits && FoundInList)
		{
			if (build->count >= MAX_CLIENT_STATES_H2)
			{
				break;
			}

			continue;
		}

		if (e >= 256)
		{
			bits |= H2U_LONGENTITY;
		}

		if (bits >= 256)
		{
			bits |= H2U_MOREBITS;
		}

		if (bits >= 65536)
		{
			bits |= H2U_MOREBITS2;
		}

		//
		// write the message
		//
		msg->WriteByte(bits | H2U_SIGNAL);

		if (bits & H2U_MOREBITS)
		{
			msg->WriteByte(bits >> 8);
		}
		if (bits & H2U_MOREBITS2)
		{
			msg->WriteByte(bits >> 16);
		}

		if (bits & H2U_LONGENTITY)
		{
			msg->WriteShort(e);
		}
		else
		{
			msg->WriteByte(e);
		}

		if (bits & H2U_MODEL)
		{
			msg->WriteShort(temp_index);
		}
		if (bits & H2U_FRAME)
		{
			msg->WriteByte(ent->GetFrame());
		}
		if (bits & H2U_COLORMAP)
		{
			msg->WriteByte(ent->GetColorMap());
		}
		if (bits & H2U_SKIN)
		{	// Used for skin and drawflags
			msg->WriteByte(ent->GetSkin());
			msg->WriteByte(ent->GetDrawFlags());
		}
		if (bits & H2U_EFFECTS)
		{
			msg->WriteByte(ent->GetEffects());
		}
		if (bits & H2U_ORIGIN1)
		{
			msg->WriteCoord(ent->GetOrigin()[0]);
		}
		if (bits & H2U_ANGLE1)
		{
			msg->WriteAngle(ent->GetAngles()[0]);
		}
		if (bits & H2U_ORIGIN2)
		{
			msg->WriteCoord(ent->GetOrigin()[1]);
		}
		if (bits & H2U_ANGLE2)
		{
			msg->WriteAngle(ent->GetAngles()[1]);
		}
		if (bits & H2U_ORIGIN3)
		{
			msg->WriteCoord(ent->GetOrigin()[2]);
		}
		if (bits & H2U_ANGLE3)
		{
			msg->WriteAngle(ent->GetAngles()[2]);
		}
		if (bits & H2U_SCALE)
		{	// Used for scale and abslight
			msg->WriteByte((int)(ent->GetScale() * 100.0) & 255);
			msg->WriteByte((int)(ent->GetAbsLight() * 255.0) & 255);
		}

		if (build->count >= MAX_CLIENT_STATES_H2)
		{
			break;
		}
	}

	msg->WriteByte(h2svc_clear_edicts);
	msg->WriteByte(NumToRemove);
	for (int i = 0; i < NumToRemove; i++)
	{
		msg->WriteShort(RemoveList[i]);
	}
}

static bool SVQW_AddNailUpdate(qhedict_t* ent)
{
	if (ent->v.modelindex != svqw_nailmodel &&
		ent->v.modelindex != svqw_supernailmodel)
	{
		return false;
	}
	if (numnails == MAX_NAILS)
	{
		return true;
	}
	nails[numnails] = ent;
	numnails++;
	return true;
}

static void SVQW_EmitNailUpdate(QMsg* msg)
{
	if (!numnails)
	{
		return;
	}

	msg->WriteByte(qwsvc_nails);
	msg->WriteByte(numnails);

	for (int n = 0; n < numnails; n++)
	{
		qhedict_t* ent = nails[n];
		int x = (int)(ent->GetOrigin()[0] + 4096) >> 1;
		int y = (int)(ent->GetOrigin()[1] + 4096) >> 1;
		int z = (int)(ent->GetOrigin()[2] + 4096) >> 1;
		int p = (int)(16 * ent->GetAngles()[0] / 360) & 15;
		int yaw = (int)(256 * ent->GetAngles()[1] / 360) & 255;

		byte bits[6];		// [48 bits] xyzpy 12 12 12 4 8
		bits[0] = x;
		bits[1] = (x >> 8) | (y << 4);
		bits[2] = (y >> 4);
		bits[3] = z;
		bits[4] = (z >> 8) | (p << 4);
		bits[5] = yaw;

		for (int i = 0; i < 6; i++)
		{
			msg->WriteByte(bits[i]);
		}
	}
}

static bool SVHW_AddMissileUpdate(qhedict_t* ent)
{

	if (ent->v.modelindex == svhw_magicmissmodel)
	{
		if (nummissiles == MAX_MISSILES_H2)
		{
			return true;
		}
		missiles[nummissiles] = ent;
		nummissiles++;
		return true;
	}
	if (ent->v.modelindex == svhw_ravenmodel)
	{
		if (numravens == MAX_MISSILES_H2)
		{
			return true;
		}
		ravens[numravens] = ent;
		numravens++;
		return true;
	}
	if (ent->v.modelindex == svhw_raven2model)
	{
		if (numraven2s == MAX_MISSILES_H2)
		{
			return true;
		}
		raven2s[numraven2s] = ent;
		numraven2s++;
		return true;
	}
	return false;
}

static void SVHW_EmitMissileUpdate(QMsg* msg)
{
	if (!nummissiles)
	{
		return;
	}

	msg->WriteByte(hwsvc_packmissile);
	msg->WriteByte(nummissiles);

	for (int n = 0; n < nummissiles; n++)
	{
		qhedict_t* ent = missiles[n];
		int x = (int)(ent->GetOrigin()[0] + 4096) >> 1;
		int y = (int)(ent->GetOrigin()[1] + 4096) >> 1;
		int z = (int)(ent->GetOrigin()[2] + 4096) >> 1;
		int type;
		if (fabs(ent->GetScale() - 0.1) < 0.05)
		{
			type = 1;	//assume ice mace
		}
		else
		{
			type = 2;	//assume magic missile

		}
		byte bits[5];		// [40 bits] xyz type 12 12 12 4
		bits[0] = x;
		bits[1] = (x >> 8) | (y << 4);
		bits[2] = (y >> 4);
		bits[3] = z;
		bits[4] = (z >> 8) | (type << 4);

		for (int i = 0; i < 5; i++)
		{
			msg->WriteByte(bits[i]);
		}
	}
}

static void SVHW_EmitRavenUpdate(QMsg* msg)
{
	if ((!numravens) && (!numraven2s))
	{
		return;
	}

	msg->WriteByte(hwsvc_nails);	//svc nails overloaded for ravens
	msg->WriteByte(numravens);

	for (int n = 0; n < numravens; n++)
	{
		qhedict_t* ent = ravens[n];
		int x = (int)(ent->GetOrigin()[0] + 4096) >> 1;
		int y = (int)(ent->GetOrigin()[1] + 4096) >> 1;
		int z = (int)(ent->GetOrigin()[2] + 4096) >> 1;
		int p = (int)(16 * ent->GetAngles()[0] / 360) & 15;
		int frame = (int)(ent->GetFrame()) & 7;
		int yaw = (int)(32 * ent->GetAngles()[1] / 360) & 31;

		byte bits[6];		// [48 bits] xyzpy 12 12 12 4 8
		bits[0] = x;
		bits[1] = (x >> 8) | (y << 4);
		bits[2] = (y >> 4);
		bits[3] = z;
		bits[4] = (z >> 8) | (p << 4);
		bits[5] = yaw | (frame << 5);

		for (int i = 0; i < 6; i++)
		{
			msg->WriteByte(bits[i]);
		}
	}
	msg->WriteByte(numraven2s);

	for (int n = 0; n < numraven2s; n++)
	{
		qhedict_t* ent = raven2s[n];
		int x = (int)(ent->GetOrigin()[0] + 4096) >> 1;
		int y = (int)(ent->GetOrigin()[1] + 4096) >> 1;
		int z = (int)(ent->GetOrigin()[2] + 4096) >> 1;
		int p = (int)(16 * ent->GetAngles()[0] / 360) & 15;
		int yaw = (int)(256 * ent->GetAngles()[1] / 360) & 255;

		byte bits[6];		// [48 bits] xyzpy 12 12 12 4 8
		bits[0] = x;
		bits[1] = (x >> 8) | (y << 4);
		bits[2] = (y >> 4);
		bits[3] = z;
		bits[4] = (z >> 8) | (p << 4);
		bits[5] = yaw;

		for (int i = 0; i < 6; i++)
		{
			msg->WriteByte(bits[i]);
		}
	}
}

static void SVHW_EmitPackedEntities(QMsg* msg)
{
	SVHW_EmitMissileUpdate(msg);
	SVHW_EmitRavenUpdate(msg);
}

//	Writes part of a packetentities message.
// Can delta from either a baseline or a previous packet_entity
static void SVQW_WriteDelta(q1entity_state_t* from, q1entity_state_t* to, QMsg* msg, bool force)
{
	// send an update
	int bits = 0;

	for (int i = 0; i < 3; i++)
	{
		float miss = to->origin[i] - from->origin[i];
		if (miss < -0.1 || miss > 0.1)
		{
			bits |= QWU_ORIGIN1 << i;
		}
	}

	if (to->angles[0] != from->angles[0])
	{
		bits |= QWU_ANGLE1;
	}

	if (to->angles[1] != from->angles[1])
	{
		bits |= QWU_ANGLE2;
	}

	if (to->angles[2] != from->angles[2])
	{
		bits |= QWU_ANGLE3;
	}

	if (to->colormap != from->colormap)
	{
		bits |= QWU_COLORMAP;
	}

	if (to->skinnum != from->skinnum)
	{
		bits |= QWU_SKIN;
	}

	if (to->frame != from->frame)
	{
		bits |= QWU_FRAME;
	}

	if (to->effects != from->effects)
	{
		bits |= QWU_EFFECTS;
	}

	if (to->modelindex != from->modelindex)
	{
		bits |= QWU_MODEL;
	}

	if (bits & 511)
	{
		bits |= QWU_MOREBITS;
	}

	if (to->flags & QWU_SOLID)
	{
		bits |= QWU_SOLID;
	}

	//
	// write the message
	//
	if (!to->number)
	{
		common->Error("Unset entity number");
	}
	if (to->number >= 512)
	{
		common->Error("Entity number >= 512");
	}

	if (!bits && !force)
	{
		return;		// nothing to send!
	}
	int i = to->number | (bits & ~511);
	if (i & QWU_REMOVE)
	{
		common->FatalError("QWU_REMOVE");
	}
	msg->WriteShort(i);

	if (bits & QWU_MOREBITS)
	{
		msg->WriteByte(bits & 255);
	}
	if (bits & QWU_MODEL)
	{
		msg->WriteByte(to->modelindex);
	}
	if (bits & QWU_FRAME)
	{
		msg->WriteByte(to->frame);
	}
	if (bits & QWU_COLORMAP)
	{
		msg->WriteByte(to->colormap);
	}
	if (bits & QWU_SKIN)
	{
		msg->WriteByte(to->skinnum);
	}
	if (bits & QWU_EFFECTS)
	{
		msg->WriteByte(to->effects);
	}
	if (bits & QWU_ORIGIN1)
	{
		msg->WriteCoord(to->origin[0]);
	}
	if (bits & QWU_ANGLE1)
	{
		msg->WriteAngle(to->angles[0]);
	}
	if (bits & QWU_ORIGIN2)
	{
		msg->WriteCoord(to->origin[1]);
	}
	if (bits & QWU_ANGLE2)
	{
		msg->WriteAngle(to->angles[1]);
	}
	if (bits & QWU_ORIGIN3)
	{
		msg->WriteCoord(to->origin[2]);
	}
	if (bits & QWU_ANGLE3)
	{
		msg->WriteAngle(to->angles[2]);
	}
}

//	Writes part of a packetentities message.
// Can delta from either a baseline or a previous packet_entity
static void SVHW_WriteDelta(h2entity_state_t* from, h2entity_state_t* to, QMsg* msg, bool force, qhedict_t* ent, client_t* client)
{
	// send an update
	int bits = 0;

	for (int i = 0; i < 3; i++)
	{
		float miss = to->origin[i] - from->origin[i];
		if (miss < -0.1 || miss > 0.1)
		{
			bits |= HWU_ORIGIN1 << i;
		}
	}

	if (to->angles[0] != from->angles[0])
	{
		bits |= HWU_ANGLE1;
	}

	if (to->angles[1] != from->angles[1])
	{
		bits |= HWU_ANGLE2;
	}

	if (to->angles[2] != from->angles[2])
	{
		bits |= HWU_ANGLE3;
	}

	if (to->colormap != from->colormap)
	{
		bits |= HWU_COLORMAP;
	}

	if (to->skinnum != from->skinnum)
	{
		bits |= HWU_SKIN;
	}

	if (to->drawflags != from->drawflags)
	{
		bits |= HWU_DRAWFLAGS;
	}

	if (to->frame != from->frame)
	{
		bits |= HWU_FRAME;
	}

	if (to->effects != from->effects)
	{
		bits |= HWU_EFFECTS;
	}

	int temp_index = to->modelindex;
	if (((int)ent->GetFlags() & H2FL_CLASS_DEPENDENT) && ent->GetModel())
	{
		char NewName[MAX_QPATH];
		String::Cpy(NewName, PR_GetString(ent->GetModel()));
		if (client->h2_playerclass <= 0 || client->h2_playerclass > MAX_PLAYER_CLASS)
		{
			NewName[String::Length(NewName) - 5] = '1';
		}
		else
		{
			NewName[String::Length(NewName) - 5] = client->h2_playerclass + 48;
		}
		temp_index = SVQH_ModelIndex(NewName);
	}

	if (temp_index != from->modelindex)
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

	if (to->wpn_sound)
	{	//not delta'ed, sound gets cleared after send
		bits |= HWU_SOUND;
	}

	if (bits & 0xff0000)
	{
		bits |= HWU_MOREBITS2;
	}

	if (bits & 511)
	{
		bits |= HWU_MOREBITS;
	}

	//
	// write the message
	//
	if (!to->number)
	{
		common->Error("Unset entity number");
	}
	if (to->number >= 512)
	{
		common->Error("Entity number >= 512");
	}

	if (!bits && !force)
	{
		return;		// nothing to send!
	}
	int i = to->number | (bits & ~511);
	if (i & HWU_REMOVE)
	{
		common->FatalError("HWU_REMOVE");
	}
	msg->WriteShort(i & 0xffff);

	if (bits & HWU_MOREBITS)
	{
		msg->WriteByte(bits & 255);
	}
	if (bits & HWU_MOREBITS2)
	{
		msg->WriteByte((bits >> 16) & 0xff);
	}
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
	{
		msg->WriteByte(to->frame);
	}
	if (bits & HWU_COLORMAP)
	{
		msg->WriteByte(to->colormap);
	}
	if (bits & HWU_SKIN)
	{
		msg->WriteByte(to->skinnum);
	}
	if (bits & HWU_DRAWFLAGS)
	{
		msg->WriteByte(to->drawflags);
	}
	if (bits & HWU_EFFECTS)
	{
		msg->WriteLong(to->effects);
	}
	if (bits & HWU_ORIGIN1)
	{
		msg->WriteCoord(to->origin[0]);
	}
	if (bits & HWU_ANGLE1)
	{
		msg->WriteAngle(to->angles[0]);
	}
	if (bits & HWU_ORIGIN2)
	{
		msg->WriteCoord(to->origin[1]);
	}
	if (bits & HWU_ANGLE2)
	{
		msg->WriteAngle(to->angles[1]);
	}
	if (bits & HWU_ORIGIN3)
	{
		msg->WriteCoord(to->origin[2]);
	}
	if (bits & HWU_ANGLE3)
	{
		msg->WriteAngle(to->angles[2]);
	}
	if (bits & HWU_SCALE)
	{
		msg->WriteByte(to->scale);
	}
	if (bits & HWU_ABSLIGHT)
	{
		msg->WriteByte(to->abslight);
	}
	if (bits & HWU_SOUND)
	{
		msg->WriteShort(to->wpn_sound);
	}
}

//	Writes a delta update of a qwpacket_entities_t to the message.
static void SVQW_EmitPacketEntities(client_t* client, qwpacket_entities_t* to, QMsg* msg)
{
	// this is the frame that we are going to delta update from
	qwpacket_entities_t* from;
	int oldmax;
	if (client->qh_delta_sequence != -1)
	{
		qwclient_frame_t* fromframe = &client->qw_frames[client->qh_delta_sequence & UPDATE_MASK_QW];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		msg->WriteByte(qwsvc_deltapacketentities);
		msg->WriteByte(client->qh_delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		msg->WriteByte(qwsvc_packetentities);
	}

	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		int newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		int oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{
			// delta update from old position
			SVQW_WriteDelta(&from->entities[oldindex], &to->entities[newindex], msg, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			qhedict_t* ent = QH_EDICT_NUM(newnum);
			SVQW_WriteDelta(&ent->q1_baseline, &to->entities[newindex], msg, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			msg->WriteShort(oldnum | QWU_REMOVE);
			oldindex++;
			continue;
		}
	}

	msg->WriteShort(0);	// end of packetentities
}

//	Writes a delta update of a hwpacket_entities_t to the message.
static void SVHW_EmitPacketEntities(client_t* client, hwpacket_entities_t* to, QMsg* msg)
{
	// this is the frame that we are going to delta update from
	hwpacket_entities_t* from;
	int oldmax;
	if (client->qh_delta_sequence != -1)
	{
		hwclient_frame_t* fromframe = &client->hw_frames[client->qh_delta_sequence & UPDATE_MASK_HW];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		msg->WriteByte(hwsvc_deltapacketentities);
		msg->WriteByte(client->qh_delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		msg->WriteByte(hwsvc_packetentities);
	}

	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		int newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		int oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{
			// delta update from old position
			SVHW_WriteDelta(&from->entities[oldindex], &to->entities[newindex], msg, false, QH_EDICT_NUM(newnum), client);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			qhedict_t* ent = QH_EDICT_NUM(newnum);
			SVHW_WriteDelta(&ent->h2_baseline, &to->entities[newindex], msg, true, ent, client);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			msg->WriteShort(oldnum | HWU_REMOVE);
			oldindex++;
			continue;
		}
	}

	msg->WriteShort(0);	// end of packetentities
}

static void SVQW_WritePlayersToClient(client_t* client, qhedict_t* clent, byte* pvs, QMsg* msg)
{
	client_t* cl = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++,cl++)
	{
		if (cl->state != CS_ACTIVE)
		{
			continue;
		}

		qhedict_t* ent = cl->qh_edict;

		// ZOID visibility tracking
		if (ent != clent &&
			!(client->qh_spec_track && client->qh_spec_track - 1 == j))
		{
			if (cl->qh_spectator)
			{
				continue;
			}

			// ignore if not touching a PV leaf
			int i;
			for (i = 0; i < ent->num_leafs; i++)
			{
				int l = CM_LeafCluster(ent->LeafNums[i]);
				if (pvs[l >> 3] & (1 << (l & 7)))
				{
					break;
				}
			}
			if (i == ent->num_leafs)
			{
				continue;		// not visible
			}
		}

		int pflags = QWPF_MSEC | QWPF_COMMAND;

		if (ent->v.modelindex != svqw_playermodel)
		{
			pflags |= QWPF_MODEL;
		}
		for (int i = 0; i < 3; i++)
		{
			if (ent->GetVelocity()[i])
			{
				pflags |= QWPF_VELOCITY1ND << i;
			}
		}
		if (ent->GetEffects())
		{
			pflags |= QWPF_EFFECTS;
		}
		if (ent->GetSkin())
		{
			pflags |= QWPF_SKINNUM;
		}
		if (ent->GetHealth() <= 0)
		{
			pflags |= QWPF_DEAD;
		}
		if (ent->GetMins()[2] != -24)
		{
			pflags |= QWPF_GIB;
		}

		if (cl->qh_spectator)
		{	// only sent origin and velocity to spectators
			pflags &= QWPF_VELOCITY1ND | QWPF_VELOCITY2 | QWPF_VELOCITY3;
		}
		else if (ent == clent)
		{	// don't send a lot of data on personal entity
			pflags &= ~(QWPF_MSEC | QWPF_COMMAND);
			if (ent->GetWeaponFrame())
			{
				pflags |= QWPF_WEAPONFRAME;
			}
		}

		if (client->qh_spec_track && client->qh_spec_track - 1 == j &&
			ent->GetWeaponFrame())
		{
			pflags |= QWPF_WEAPONFRAME;
		}

		msg->WriteByte(qwsvc_playerinfo);
		msg->WriteByte(j);
		msg->WriteShort(pflags);

		for (int i = 0; i < 3; i++)
		{
			msg->WriteCoord(ent->GetOrigin()[i]);
		}

		msg->WriteByte(ent->GetFrame());

		if (pflags & QWPF_MSEC)
		{
			int msec = sv.qh_time - 1000 * cl->qh_localtime;
			if (msec > 255)
			{
				msec = 255;
			}
			msg->WriteByte(msec);
		}

		if (pflags & QWPF_COMMAND)
		{
			qwusercmd_t cmd = cl->qw_lastUsercmd;

			if (ent->GetHealth() <= 0)
			{	// don't show the corpse looking around...
				cmd.angles[0] = 0;
				cmd.angles[1] = ent->GetAngles()[1];
				cmd.angles[0] = 0;
			}

			cmd.buttons = 0;	// never send buttons
			cmd.impulse = 0;	// never send impulses

			qwusercmd_t nullcmd;
			Com_Memset(&nullcmd, 0, sizeof(nullcmd));
			MSGQW_WriteDeltaUsercmd(msg, &nullcmd, &cmd);
		}

		for (int i = 0; i < 3; i++)
		{
			if (pflags & (QWPF_VELOCITY1ND << i))
			{
				msg->WriteShort(ent->GetVelocity()[i]);
			}
		}

		if (pflags & QWPF_MODEL)
		{
			msg->WriteByte(ent->v.modelindex);
		}

		if (pflags & QWPF_SKINNUM)
		{
			msg->WriteByte(ent->GetSkin());
		}

		if (pflags & QWPF_EFFECTS)
		{
			msg->WriteByte(ent->GetEffects());
		}

		if (pflags & QWPF_WEAPONFRAME)
		{
			msg->WriteByte(ent->GetWeaponFrame());
		}
	}
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
static float cardioid_rating(qhedict_t* targ, qhedict_t* self)
{
	vec3_t vec,spot1,spot2;
	vec3_t forward,right,up;
	float dot,dist;

	AngleVectors(self->GetVAngle(),forward,right,up);

	VectorAdd(self->GetOrigin(),self->GetViewOfs(),spot1);
	VectorSubtract(targ->v.absmax,targ->v.absmin,spot2);
	VectorMA(targ->v.absmin,0.5,spot2,spot2);

	VectorSubtract(spot2,spot1,vec);
	dist = VectorNormalize(vec);
	dot = DotProduct(vec,forward);	//from 1 to -1

	if (dot < -0.3)	//see only from -125 to 125 degrees
	{
		return false;
	}

	if (dot > 0)//to front of perpendicular plane to forward
	{
		dot *= 31;	//much more distance leniency in front, max dist = 2048 directly in front
	}
	dot = (dot + 1) * 64;	//64 = base distance if along the perpendicular plane, max is 2048 straight ahead
	if (dist >= dot)//too far away for that angle to be important
	{
		return false;
	}

	//from 0.000000? to almost 1
	return 1 - (dist / dot);//The higher this number is, the more important it is to send this ent
}

static int MAX_VISCLIENTS = 2;
#endif

static void SVHW_WritePlayersToClient(client_t* client, qhedict_t* clent, byte* pvs, QMsg* msg)
{
#ifdef MGNET
	int k, l;
	int visclient[MAX_CLIENTS_QHW];
	int forcevisclient[MAX_CLIENTS_QHW];
	int cl_v_priority[MAX_CLIENTS_QHW];
	int cl_v_psort[MAX_CLIENTS_QHW];
	int totalvc,num_eliminated;

	int numvc = 0;
	int forcevc = 0;
#endif

	client_t* cl = svs.clients;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++,cl++)
	{
		if (cl->state != CS_ACTIVE)
		{
			continue;
		}

		qhedict_t* ent = cl->qh_edict;


		// ZOID visibility tracking
		int invis_level = false;
		if (ent != clent &&
			!(client->qh_spec_track && client->qh_spec_track - 1 == j))
		{
			if ((int)ent->GetEffects() & H2EF_NODRAW)
			{
				if (hw_dmMode->value == HWDM_SIEGE && clent->GetPlayerClass() == CLASS_DWARF)
				{
					invis_level = false;
				}
				else
				{
					invis_level = true;	//still can hear
				}
			}
#ifdef MGNET
			//could be invisiblenow and still sent, cull out by other methods as well
			if (cl->qh_spectator)
#else
			else if (cl->qh_spectator)
#endif
			{
				invis_level = 2;//no vis or weaponsound
			}
			else
			{
				// ignore if not touching a PV leaf
				int i;
				for (i = 0; i < ent->num_leafs; i++)
				{
					int l = CM_LeafCluster(ent->LeafNums[i]);
					if (pvs[l >> 3] & (1 << (l & 7)))
					{
						break;
					}
				}
				if (i == ent->num_leafs)
				{
					invis_level = 2;//no vis or weaponsound
				}
			}
		}

		if (invis_level == true)
		{	//ok to send weaponsound
			if (ent->GetWpnSound())
			{
				msg->WriteByte(hwsvc_player_sound);
				msg->WriteByte(j);
				for (int i = 0; i < 3; i++)
				{
					msg->WriteCoord(ent->GetOrigin()[i]);
				}
				msg->WriteShort(ent->GetWpnSound());
			}
		}
		if (invis_level > 0)
		{
			continue;
		}

#ifdef MGNET
		if (!cl->skipsend && ent != clent)
		{	//don't count self
			visclient[numvc] = j;
			numvc++;
		}
		else
		{	//Self, or Wasn't sent last time, must send this frame
			cl->skipsend = false;
			forcevisclient[forcevc] = j;
			forcevc++;
			continue;
		}
	}

	totalvc = numvc + forcevc;
	if (totalvc > MAX_VISCLIENTS)	//You have more than 5 clients in your view, cull some out
	{	//prioritize by
	//line of sight (20%)
	//distance (50%)
	//dot off v_forward (30%)
	//put this in "priority" then sort by priority
	//and send the highest priority
	//number of highest priority sent depends on how
	//many are forced through because they were skipped
	//last send.  Ideally, no more than 5 are sent.
		for (j = 0; j<numvc&& totalvc>MAX_VISCLIENTS; j++)
		{	//priority 1 - if behind, cull out
			for (k = 0, cl = svs.clients; k < visclient[j]; k++, cl++)
				;
//			cl=svs.clients+visclient[j];
			ent = cl->edict;
			cl_v_priority[j] = cardioid_rating(ent,clent);
			if (!cl_v_priority[j])
			{	//% they won't be sent, l represents how many were forced through
				cl->skipsend = true;
				totalvc--;
			}
		}

		if (totalvc > MAX_VISCLIENTS)
		{	//still more than 5 inside cardioid, sort by priority
		//and drop those after 5

			//CHECK this make sure it works
			for (i = 0; i < numvc; i++)	//do this as many times as there are visclients
				for (j = 0; j < numvc - 1 - i; j++)	//go through the list
					if (cl_v_priority[j] < cl_v_priority[j + 1])
					{
						k = cl_v_psort[j];	//store lower one
						cl_v_psort[j] = cl_v_psort[j + 1];	//put next one in it's spot
						cl_v_psort[j + 1] = k;	//put lower one next
					}
			num_eliminated = 0;
			while (totalvc > MAX_VISCLIENTS)
			{	//eliminate all over 5 unless not sent last time
				if (!cl->skipsend)
				{
					cl = svs.clients + cl_v_psort[numvc - num_eliminated];
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

	for (j = 0, l = 0, k = 0, cl = svs.clients; j < MAX_CLIENTS_QHW; j++,cl++)
	{
		//priority 1 - if behind, cull out
		if (forcevisclient[l] == j && l <= forcevc)
		{
			l++;
		}
		else if (visclient[k] == j && k <= numvc)
		{
			k++;//clent is always forced
		}
		else
		{
			continue;	//not in PVS or forced

		}
		if (cl->skipsend)
		{	//still 2 bytes, but what ya gonna do?
			msg->WriteByte(hwsvc_playerskipped);
			msg->WriteByte(j);
			continue;
		}

		qhedict_t* ent = cl->edict;
#endif

		int pflags = HWPF_MSEC | HWPF_COMMAND;

		bool playermodel = false;
		if (ent->v.modelindex != svhw_playermodel[0] &&	//paladin
			ent->v.modelindex != svhw_playermodel[1] &&	//crusader
			ent->v.modelindex != svhw_playermodel[2] &&	//necro
			ent->v.modelindex != svhw_playermodel[3] &&	//assassin
			ent->v.modelindex != svhw_playermodel[4] &&	//succ
			ent->v.modelindex != svhw_playermodel[5])	//dwarf
		{
			pflags |= HWPF_MODEL;
		}
		else
		{
			playermodel = true;
		}

		for (int i = 0; i < 3; i++)
		{
			if (ent->GetVelocity()[i])
			{
				pflags |= HWPF_VELOCITY1 << i;
			}
		}
		if (((long)ent->GetEffects() & 0xff))
		{
			pflags |= HWPF_EFFECTS;
		}
		if (((long)ent->GetEffects() & 0xff00))
		{
			pflags |= HWPF_EFFECTS2;
		}
		if (ent->GetSkin())
		{
			if (hw_dmMode->value == HWDM_SIEGE && playermodel && ent->GetSkin() == 1)
			{
				;
			}
			//in siege, don't send skin if 2nd skin and using
			//playermodel, it will know on other side- saves
			//us 1 byte per client per frame!
			else
			{
				pflags |= HWPF_SKINNUM;
			}
		}
		if (ent->GetHealth() <= 0)
		{
			pflags |= HWPF_DEAD;
		}
		if (ent->GetHull() == HWHULL_CROUCH)
		{
			pflags |= HWPF_CROUCH;
		}

		if (cl->qh_spectator)
		{	// only sent origin and velocity to spectators
			pflags &= HWPF_VELOCITY1 | HWPF_VELOCITY2 | HWPF_VELOCITY3;
		}
		else if (ent == clent)
		{	// don't send a lot of data on personal entity
			pflags &= ~(HWPF_MSEC | HWPF_COMMAND);
			if (ent->GetWeaponFrame())
			{
				pflags |= HWPF_WEAPONFRAME;
			}
		}
		if (ent->GetDrawFlags())
		{
			pflags |= HWPF_DRAWFLAGS;
		}
		if (ent->GetScale() != 0 && ent->GetScale() != 1.0)
		{
			pflags |= HWPF_SCALE;
		}
		if (ent->GetAbsLight() != 0)
		{
			pflags |= HWPF_ABSLIGHT;
		}
		if (ent->GetWpnSound())
		{
			pflags |= HWPF_SOUND;
		}

		msg->WriteByte(hwsvc_playerinfo);
		msg->WriteByte(j);
		msg->WriteShort(pflags);

		for (int i = 0; i < 3; i++)
		{
			msg->WriteCoord(ent->GetOrigin()[i]);
		}

		msg->WriteByte(ent->GetFrame());

		if (pflags & HWPF_MSEC)
		{
			int msec = sv.qh_time - 1000 * cl->qh_localtime;
			if (msec > 255)
			{
				msec = 255;
			}
			msg->WriteByte(msec);
		}

		if (pflags & HWPF_COMMAND)
		{
			hwusercmd_t cmd = cl->hw_lastUsercmd;

			if (ent->GetHealth() <= 0)
			{	// don't show the corpse looking around...
				cmd.angles[0] = 0;
				cmd.angles[1] = ent->GetAngles()[1];
				cmd.angles[0] = 0;
			}

			cmd.buttons = 0;	// never send buttons
			cmd.impulse = 0;	// never send impulses
			MSGHW_WriteUsercmd(msg, &cmd, false);
		}

		for (int i = 0; i < 3; i++)
		{
			if (pflags & (HWPF_VELOCITY1 << i))
			{
				msg->WriteShort(ent->GetVelocity()[i]);
			}
		}

		//rjr
		if (pflags & HWPF_MODEL)
		{
			msg->WriteShort(ent->v.modelindex);
		}

		if (pflags & HWPF_SKINNUM)
		{
			msg->WriteByte(ent->GetSkin());
		}

		if (pflags & HWPF_EFFECTS)
		{
			msg->WriteByte(((long)ent->GetEffects() & 0xff));
		}

		if (pflags & HWPF_EFFECTS2)
		{
			msg->WriteByte(((long)ent->GetEffects() & 0xff00) >> 8);
		}

		if (pflags & HWPF_WEAPONFRAME)
		{
			msg->WriteByte(ent->GetWeaponFrame());
		}

		if (pflags & HWPF_DRAWFLAGS)
		{
			msg->WriteByte(ent->GetDrawFlags());
		}
		if (pflags & HWPF_SCALE)
		{
			msg->WriteByte((int)(ent->GetScale() * 100.0) & 255);
		}
		if (pflags & HWPF_ABSLIGHT)
		{
			msg->WriteByte((int)(ent->GetAbsLight() * 100.0) & 255);
		}
		if (pflags & HWPF_SOUND)
		{
			msg->WriteShort(ent->GetWpnSound());
		}
	}
}

//	Encodes the current state of the world as
// a qwsvc_packetentities messages and possibly
// a qwsvc_nails message and
// qwsvc_playerinfo messages
void SVQW_WriteEntitiesToClient(client_t* client, QMsg* msg)
{
	// this is the frame we are creating
	qwclient_frame_t* frame = &client->qw_frames[client->netchan.incomingSequence & UPDATE_MASK_QW];

	// find the client's PVS
	qhedict_t* clent = client->qh_edict;
	vec3_t org;
	VectorAdd(clent->GetOrigin(), clent->GetViewOfs(), org);
	byte* pvs = SVQH_FatPVS(org);

	// send over the players in the PVS
	SVQW_WritePlayersToClient(client, clent, pvs, msg);

	// put other visible entities into either a packet_entities or a nails message
	qwpacket_entities_t* pack = &frame->entities;
	pack->num_entities = 0;

	numnails = 0;

	qhedict_t* ent = QH_EDICT_NUM(MAX_CLIENTS_QHW + 1);
	for (int e = MAX_CLIENTS_QHW + 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		// ignore ents without visible models
		if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
		{
			continue;
		}

		// ignore if not touching a PV leaf
		int i;
		for (i = 0; i < ent->num_leafs; i++)
		{
			int l = CM_LeafCluster(ent->LeafNums[i]);
			if (pvs[l >> 3] & (1 << (l & 7)))
			{
				break;
			}
		}

		if (i == ent->num_leafs)
		{
			continue;		// not visible

		}
		if (SVQW_AddNailUpdate(ent))
		{
			continue;	// added to the special update list

		}
		// add to the packetentities
		if (pack->num_entities == QWMAX_PACKET_ENTITIES)
		{
			continue;	// all full

		}
		q1entity_state_t* state = &pack->entities[pack->num_entities];
		pack->num_entities++;

		state->number = e;
		state->flags = 0;
		VectorCopy(ent->GetOrigin(), state->origin);
		VectorCopy(ent->GetAngles(), state->angles);
		state->modelindex = ent->v.modelindex;
		state->frame = ent->GetFrame();
		state->colormap = ent->GetColorMap();
		state->skinnum = ent->GetSkin();
		state->effects = ent->GetEffects();
	}

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SVQW_EmitPacketEntities(client, pack, msg);

	// now add the specialized nail update
	SVQW_EmitNailUpdate(msg);
}

//	Encodes the current state of the world as
// a hwsvc_packetentities messages and possibly
// a hwsvc_nails message and
// hwsvc_playerinfo messages
void SVHW_WriteEntitiesToClient(client_t* client, QMsg* msg)
{
	// this is the frame we are creating
	hwclient_frame_t* frame = &client->hw_frames[client->netchan.incomingSequence & UPDATE_MASK_HW];

	// find the client's PVS
	qhedict_t* clent = client->qh_edict;
	vec3_t org;
	VectorAdd(clent->GetOrigin(), clent->GetViewOfs(), org);
	byte* pvs = SVQH_FatPVS(org);

	// send over the players in the PVS
	SVHW_WritePlayersToClient(client, clent, pvs, msg);

	// put other visible entities into either a packet_entities or a nails message
	hwpacket_entities_t* pack = &frame->entities;
	pack->num_entities = 0;

	nummissiles = 0;
	numravens = 0;
	numraven2s = 0;

	qhedict_t* ent = QH_EDICT_NUM(MAX_CLIENTS_QHW + 1);
	for (int e = MAX_CLIENTS_QHW + 1; e < sv.qh_num_edicts; e++, ent = NEXT_EDICT(ent))
	{
		// ignore ents without visible models
		if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
		{
			continue;
		}

		if ((int)ent->GetEffects() & H2EF_NODRAW)
		{
			continue;
		}

		// ignore if not touching a PV leaf
		int i;
		for (i = 0; i < ent->num_leafs; i++)
		{
			int l = CM_LeafCluster(ent->LeafNums[i]);
			if (pvs[l >> 3] & (1 << (l & 7)))
			{
				break;
			}
		}

		if (i == ent->num_leafs)
		{
			continue;		// not visible

		}
		if (SVHW_AddMissileUpdate(ent))
		{
			continue;	// added to the special update list

		}
		// add to the packetentities
		if (pack->num_entities == HWMAX_PACKET_ENTITIES)
		{
			continue;	// all full

		}
		h2entity_state_t* state = &pack->entities[pack->num_entities];
		pack->num_entities++;

		state->number = e;
		state->flags = 0;
		VectorCopy(ent->GetOrigin(), state->origin);
		VectorCopy(ent->GetAngles(), state->angles);
		state->modelindex = ent->v.modelindex;
		state->frame = ent->GetFrame();
		state->colormap = ent->GetColorMap();
		state->skinnum = ent->GetSkin();
		state->effects = ent->GetEffects();
		state->scale = (int)(ent->GetScale() * 100.0) & 255;
		state->drawflags = ent->GetDrawFlags();
		state->abslight = (int)(ent->GetAbsLight() * 255.0) & 255;
		//clear sound so it doesn't send twice
		state->wpn_sound = ent->GetWpnSound();
	}

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SVHW_EmitPacketEntities(client, pack, msg);

	// now add the specialized nail update
	SVHW_EmitPackedEntities(msg);
}

void SVHW_WriteInventory(client_t* host_client, qhedict_t* ent, QMsg* msg)
{
	int sc1, sc2;
	if (host_client->h2_send_all_v)
	{
		sc1 = sc2 = 0xffffffff;
		host_client->h2_send_all_v = false;
	}
	else
	{
		sc1 = sc2 = 0;

		if (ent->GetHealth() != host_client->h2_old_v.health)
		{
			sc1 |= SC1_HEALTH;
		}
		if (ent->GetLevel() != host_client->h2_old_v.level)
		{
			sc1 |= SC1_LEVEL;
		}
		if (ent->GetIntelligence() != host_client->h2_old_v.intelligence)
		{
			sc1 |= SC1_INTELLIGENCE;
		}
		if (ent->GetWisdom() != host_client->h2_old_v.wisdom)
		{
			sc1 |= SC1_WISDOM;
		}
		if (ent->GetStrength() != host_client->h2_old_v.strength)
		{
			sc1 |= SC1_STRENGTH;
		}
		if (ent->GetDexterity() != host_client->h2_old_v.dexterity)
		{
			sc1 |= SC1_DEXTERITY;
		}
		if (ent->GetTeleportTime() > sv.qh_time * 0.001f)
		{
			sc1 |= SC1_TELEPORT_TIME;
		}
		if (ent->GetBlueMana() != host_client->h2_old_v.bluemana)
		{
			sc1 |= SC1_BLUEMANA;
		}
		if (ent->GetGreenMana() != host_client->h2_old_v.greenmana)
		{
			sc1 |= SC1_GREENMANA;
		}
		if (ent->GetExperience() != host_client->h2_old_v.experience)
		{
			sc1 |= SC1_EXPERIENCE;
		}
		if (ent->GetCntTorch() != host_client->h2_old_v.cnt_torch)
		{
			sc1 |= SC1_CNT_TORCH;
		}
		if (ent->GetCntHBoost() != host_client->h2_old_v.cnt_h_boost)
		{
			sc1 |= SC1_CNT_H_BOOST;
		}
		if (ent->GetCntSHBoost() != host_client->h2_old_v.cnt_sh_boost)
		{
			sc1 |= SC1_CNT_SH_BOOST;
		}
		if (ent->GetCntManaBoost() != host_client->h2_old_v.cnt_mana_boost)
		{
			sc1 |= SC1_CNT_MANA_BOOST;
		}
		if (ent->GetCntTeleport() != host_client->h2_old_v.cnt_teleport)
		{
			sc1 |= SC1_CNT_TELEPORT;
		}
		if (ent->GetCntTome() != host_client->h2_old_v.cnt_tome)
		{
			sc1 |= SC1_CNT_TOME;
		}
		if (ent->GetCntSummon() != host_client->h2_old_v.cnt_summon)
		{
			sc1 |= SC1_CNT_SUMMON;
		}
		if (ent->GetCntInvisibility() != host_client->h2_old_v.cnt_invisibility)
		{
			sc1 |= SC1_CNT_INVISIBILITY;
		}
		if (ent->GetCntGlyph() != host_client->h2_old_v.cnt_glyph)
		{
			sc1 |= SC1_CNT_GLYPH;
		}
		if (ent->GetCntHaste() != host_client->h2_old_v.cnt_haste)
		{
			sc1 |= SC1_CNT_HASTE;
		}
		if (ent->GetCntBlast() != host_client->h2_old_v.cnt_blast)
		{
			sc1 |= SC1_CNT_BLAST;
		}
		if (ent->GetCntPolyMorph() != host_client->h2_old_v.cnt_polymorph)
		{
			sc1 |= SC1_CNT_POLYMORPH;
		}
		if (ent->GetCntFlight() != host_client->h2_old_v.cnt_flight)
		{
			sc1 |= SC1_CNT_FLIGHT;
		}
		if (ent->GetCntCubeOfForce() != host_client->h2_old_v.cnt_cubeofforce)
		{
			sc1 |= SC1_CNT_CUBEOFFORCE;
		}
		if (ent->GetCntInvincibility() != host_client->h2_old_v.cnt_invincibility)
		{
			sc1 |= SC1_CNT_INVINCIBILITY;
		}
		if (ent->GetArtifactActive() != host_client->h2_old_v.artifact_active)
		{
			sc1 |= SC1_ARTIFACT_ACTIVE;
		}
		if (ent->GetArtifactLow() != host_client->h2_old_v.artifact_low)
		{
			sc1 |= SC1_ARTIFACT_LOW;
		}
		if (ent->GetMoveType() != host_client->h2_old_v.movetype)
		{
			sc1 |= SC1_MOVETYPE;
		}
		if (ent->GetCameraMode() != host_client->h2_old_v.cameramode)
		{
			sc1 |= SC1_CAMERAMODE;
		}
		if (ent->GetHasted() != host_client->h2_old_v.hasted)
		{
			sc1 |= SC1_HASTED;
		}
		if (ent->GetInventory() != host_client->h2_old_v.inventory)
		{
			sc1 |= SC1_INVENTORY;
		}
		if (ent->GetRingsActive() != host_client->h2_old_v.rings_active)
		{
			sc1 |= SC1_RINGS_ACTIVE;
		}

		if (ent->GetRingsLow() != host_client->h2_old_v.rings_low)
		{
			sc2 |= SC2_RINGS_LOW;
		}
		if (ent->GetArmorAmulet() != host_client->h2_old_v.armor_amulet)
		{
			sc2 |= SC2_AMULET;
		}
		if (ent->GetArmorBracer() != host_client->h2_old_v.armor_bracer)
		{
			sc2 |= SC2_BRACER;
		}
		if (ent->GetArmorBreastPlate() != host_client->h2_old_v.armor_breastplate)
		{
			sc2 |= SC2_BREASTPLATE;
		}
		if (ent->GetArmorHelmet() != host_client->h2_old_v.armor_helmet)
		{
			sc2 |= SC2_HELMET;
		}
		if (ent->GetRingFlight() != host_client->h2_old_v.ring_flight)
		{
			sc2 |= SC2_FLIGHT_T;
		}
		if (ent->GetRingWater() != host_client->h2_old_v.ring_water)
		{
			sc2 |= SC2_WATER_T;
		}
		if (ent->GetRingTurning() != host_client->h2_old_v.ring_turning)
		{
			sc2 |= SC2_TURNING_T;
		}
		if (ent->GetRingRegeneration() != host_client->h2_old_v.ring_regeneration)
		{
			sc2 |= SC2_REGEN_T;
		}
		if (ent->GetPuzzleInv1() != host_client->h2_old_v.puzzle_inv1)
		{
			sc2 |= SC2_PUZZLE1;
		}
		if (ent->GetPuzzleInv2() != host_client->h2_old_v.puzzle_inv2)
		{
			sc2 |= SC2_PUZZLE2;
		}
		if (ent->GetPuzzleInv3() != host_client->h2_old_v.puzzle_inv3)
		{
			sc2 |= SC2_PUZZLE3;
		}
		if (ent->GetPuzzleInv4() != host_client->h2_old_v.puzzle_inv4)
		{
			sc2 |= SC2_PUZZLE4;
		}
		if (ent->GetPuzzleInv5() != host_client->h2_old_v.puzzle_inv5)
		{
			sc2 |= SC2_PUZZLE5;
		}
		if (ent->GetPuzzleInv6() != host_client->h2_old_v.puzzle_inv6)
		{
			sc2 |= SC2_PUZZLE6;
		}
		if (ent->GetPuzzleInv7() != host_client->h2_old_v.puzzle_inv7)
		{
			sc2 |= SC2_PUZZLE7;
		}
		if (ent->GetPuzzleInv8() != host_client->h2_old_v.puzzle_inv8)
		{
			sc2 |= SC2_PUZZLE8;
		}
		if (ent->GetMaxHealth() != host_client->h2_old_v.max_health)
		{
			sc2 |= SC2_MAXHEALTH;
		}
		if (ent->GetMaxMana() != host_client->h2_old_v.max_mana)
		{
			sc2 |= SC2_MAXMANA;
		}
		if (ent->GetFlags() != host_client->h2_old_v.flags)
		{
			sc2 |= SC2_FLAGS;
		}
	}

	byte test;
	if (!sc1 && !sc2)
	{
		goto end;
	}

	msg->WriteByte(hwsvc_update_inv);
	test = 0;
	if (sc1 & 0x000000ff)
	{
		test |= 1;
	}
	if (sc1 & 0x0000ff00)
	{
		test |= 2;
	}
	if (sc1 & 0x00ff0000)
	{
		test |= 4;
	}
	if (sc1 & 0xff000000)
	{
		test |= 8;
	}
	if (sc2 & 0x000000ff)
	{
		test |= 16;
	}
	if (sc2 & 0x0000ff00)
	{
		test |= 32;
	}
	if (sc2 & 0x00ff0000)
	{
		test |= 64;
	}
	if (sc2 & 0xff000000)
	{
		test |= 128;
	}

	msg->WriteByte(test);

	if (test & 1)
	{
		msg->WriteByte(sc1 & 0xff);
	}
	if (test & 2)
	{
		msg->WriteByte((sc1 >> 8) & 0xff);
	}
	if (test & 4)
	{
		msg->WriteByte((sc1 >> 16) & 0xff);
	}
	if (test & 8)
	{
		msg->WriteByte((sc1 >> 24) & 0xff);
	}
	if (test & 16)
	{
		msg->WriteByte(sc2 & 0xff);
	}
	if (test & 32)
	{
		msg->WriteByte((sc2 >> 8) & 0xff);
	}
	if (test & 64)
	{
		msg->WriteByte((sc2 >> 16) & 0xff);
	}
	if (test & 128)
	{
		msg->WriteByte((sc2 >> 24) & 0xff);
	}

	if (sc1 & SC1_HEALTH)
	{
		msg->WriteShort(ent->GetHealth());
	}
	if (sc1 & SC1_LEVEL)
	{
		msg->WriteByte(ent->GetLevel());
	}
	if (sc1 & SC1_INTELLIGENCE)
	{
		msg->WriteByte(ent->GetIntelligence());
	}
	if (sc1 & SC1_WISDOM)
	{
		msg->WriteByte(ent->GetWisdom());
	}
	if (sc1 & SC1_STRENGTH)
	{
		msg->WriteByte(ent->GetStrength());
	}
	if (sc1 & SC1_DEXTERITY)
	{
		msg->WriteByte(ent->GetDexterity());
	}
	if (sc1 & SC1_BLUEMANA)
	{
		msg->WriteByte(ent->GetBlueMana());
	}
	if (sc1 & SC1_GREENMANA)
	{
		msg->WriteByte(ent->GetGreenMana());
	}
	if (sc1 & SC1_EXPERIENCE)
	{
		msg->WriteLong(ent->GetExperience());
	}
	if (sc1 & SC1_CNT_TORCH)
	{
		msg->WriteByte(ent->GetCntTorch());
	}
	if (sc1 & SC1_CNT_H_BOOST)
	{
		msg->WriteByte(ent->GetCntHBoost());
	}
	if (sc1 & SC1_CNT_SH_BOOST)
	{
		msg->WriteByte(ent->GetCntSHBoost());
	}
	if (sc1 & SC1_CNT_MANA_BOOST)
	{
		msg->WriteByte(ent->GetCntManaBoost());
	}
	if (sc1 & SC1_CNT_TELEPORT)
	{
		msg->WriteByte(ent->GetCntTeleport());
	}
	if (sc1 & SC1_CNT_TOME)
	{
		msg->WriteByte(ent->GetCntTome());
	}
	if (sc1 & SC1_CNT_SUMMON)
	{
		msg->WriteByte(ent->GetCntSummon());
	}
	if (sc1 & SC1_CNT_INVISIBILITY)
	{
		msg->WriteByte(ent->GetCntInvisibility());
	}
	if (sc1 & SC1_CNT_GLYPH)
	{
		msg->WriteByte(ent->GetCntGlyph());
	}
	if (sc1 & SC1_CNT_HASTE)
	{
		msg->WriteByte(ent->GetCntHaste());
	}
	if (sc1 & SC1_CNT_BLAST)
	{
		msg->WriteByte(ent->GetCntBlast());
	}
	if (sc1 & SC1_CNT_POLYMORPH)
	{
		msg->WriteByte(ent->GetCntPolyMorph());
	}
	if (sc1 & SC1_CNT_FLIGHT)
	{
		msg->WriteByte(ent->GetCntFlight());
	}
	if (sc1 & SC1_CNT_CUBEOFFORCE)
	{
		msg->WriteByte(ent->GetCntCubeOfForce());
	}
	if (sc1 & SC1_CNT_INVINCIBILITY)
	{
		msg->WriteByte(ent->GetCntInvincibility());
	}
	if (sc1 & SC1_ARTIFACT_ACTIVE)
	{
		msg->WriteByte(ent->GetArtifactActive());
	}
	if (sc1 & SC1_ARTIFACT_LOW)
	{
		msg->WriteByte(ent->GetArtifactLow());
	}
	if (sc1 & SC1_MOVETYPE)
	{
		msg->WriteByte(ent->GetMoveType());
	}
	if (sc1 & SC1_CAMERAMODE)
	{
		msg->WriteByte(ent->GetCameraMode());
	}
	if (sc1 & SC1_HASTED)
	{
		msg->WriteFloat(ent->GetHasted());
	}
	if (sc1 & SC1_INVENTORY)
	{
		msg->WriteByte(ent->GetInventory());
	}
	if (sc1 & SC1_RINGS_ACTIVE)
	{
		msg->WriteByte(ent->GetRingsActive());
	}

	if (sc2 & SC2_RINGS_LOW)
	{
		msg->WriteByte(ent->GetRingsLow());
	}
	if (sc2 & SC2_AMULET)
	{
		msg->WriteByte(ent->GetArmorAmulet());
	}
	if (sc2 & SC2_BRACER)
	{
		msg->WriteByte(ent->GetArmorBracer());
	}
	if (sc2 & SC2_BREASTPLATE)
	{
		msg->WriteByte(ent->GetArmorBreastPlate());
	}
	if (sc2 & SC2_HELMET)
	{
		msg->WriteByte(ent->GetArmorHelmet());
	}
	if (sc2 & SC2_FLIGHT_T)
	{
		msg->WriteByte(ent->GetRingFlight());
	}
	if (sc2 & SC2_WATER_T)
	{
		msg->WriteByte(ent->GetRingWater());
	}
	if (sc2 & SC2_TURNING_T)
	{
		msg->WriteByte(ent->GetRingTurning());
	}
	if (sc2 & SC2_REGEN_T)
	{
		msg->WriteByte(ent->GetRingRegeneration());
	}
	if (sc2 & SC2_PUZZLE1)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv1()));
	}
	if (sc2 & SC2_PUZZLE2)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv2()));
	}
	if (sc2 & SC2_PUZZLE3)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv3()));
	}
	if (sc2 & SC2_PUZZLE4)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv4()));
	}
	if (sc2 & SC2_PUZZLE5)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv5()));
	}
	if (sc2 & SC2_PUZZLE6)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv6()));
	}
	if (sc2 & SC2_PUZZLE7)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv7()));
	}
	if (sc2 & SC2_PUZZLE8)
	{
		msg->WriteString2(PR_GetString(ent->GetPuzzleInv8()));
	}
	if (sc2 & SC2_MAXHEALTH)
	{
		msg->WriteShort(ent->GetMaxHealth());
	}
	if (sc2 & SC2_MAXMANA)
	{
		msg->WriteByte(ent->GetMaxMana());
	}
	if (sc2 & SC2_FLAGS)
	{
		msg->WriteFloat(ent->GetFlags());
	}

end:
	host_client->h2_old_v.movetype = ent->GetMoveType();
	host_client->h2_old_v.health = ent->GetHealth();
	host_client->h2_old_v.max_health = ent->GetMaxHealth();
	host_client->h2_old_v.bluemana = ent->GetBlueMana();
	host_client->h2_old_v.greenmana = ent->GetGreenMana();
	host_client->h2_old_v.max_mana = ent->GetMaxMana();
	host_client->h2_old_v.armor_amulet = ent->GetArmorAmulet();
	host_client->h2_old_v.armor_bracer = ent->GetArmorBracer();
	host_client->h2_old_v.armor_breastplate = ent->GetArmorBreastPlate();
	host_client->h2_old_v.armor_helmet = ent->GetArmorHelmet();
	host_client->h2_old_v.level = ent->GetLevel();
	host_client->h2_old_v.intelligence = ent->GetIntelligence();
	host_client->h2_old_v.wisdom = ent->GetWisdom();
	host_client->h2_old_v.dexterity = ent->GetDexterity();
	host_client->h2_old_v.strength = ent->GetStrength();
	host_client->h2_old_v.experience = ent->GetExperience();
	host_client->h2_old_v.ring_flight = ent->GetRingFlight();
	host_client->h2_old_v.ring_water = ent->GetRingWater();
	host_client->h2_old_v.ring_turning = ent->GetRingTurning();
	host_client->h2_old_v.ring_regeneration = ent->GetRingRegeneration();
	host_client->h2_old_v.puzzle_inv1 = ent->GetPuzzleInv1();
	host_client->h2_old_v.puzzle_inv2 = ent->GetPuzzleInv2();
	host_client->h2_old_v.puzzle_inv3 = ent->GetPuzzleInv3();
	host_client->h2_old_v.puzzle_inv4 = ent->GetPuzzleInv4();
	host_client->h2_old_v.puzzle_inv5 = ent->GetPuzzleInv5();
	host_client->h2_old_v.puzzle_inv6 = ent->GetPuzzleInv6();
	host_client->h2_old_v.puzzle_inv7 = ent->GetPuzzleInv7();
	host_client->h2_old_v.puzzle_inv8 = ent->GetPuzzleInv8();
	host_client->h2_old_v.flags = ent->GetFlags();
	host_client->h2_old_v.flags2 = ent->GetFlags2();
	host_client->h2_old_v.rings_active = ent->GetRingsActive();
	host_client->h2_old_v.rings_low = ent->GetRingsLow();
	host_client->h2_old_v.artifact_active = ent->GetArtifactActive();
	host_client->h2_old_v.artifact_low = ent->GetArtifactLow();
	host_client->h2_old_v.hasted = ent->GetHasted();
	host_client->h2_old_v.inventory = ent->GetInventory();
	host_client->h2_old_v.cnt_torch = ent->GetCntTorch();
	host_client->h2_old_v.cnt_h_boost = ent->GetCntHBoost();
	host_client->h2_old_v.cnt_sh_boost = ent->GetCntSHBoost();
	host_client->h2_old_v.cnt_mana_boost = ent->GetCntManaBoost();
	host_client->h2_old_v.cnt_teleport = ent->GetCntTeleport();
	host_client->h2_old_v.cnt_tome = ent->GetCntTome();
	host_client->h2_old_v.cnt_summon = ent->GetCntSummon();
	host_client->h2_old_v.cnt_invisibility = ent->GetCntInvisibility();
	host_client->h2_old_v.cnt_glyph = ent->GetCntGlyph();
	host_client->h2_old_v.cnt_haste = ent->GetCntHaste();
	host_client->h2_old_v.cnt_blast = ent->GetCntBlast();
	host_client->h2_old_v.cnt_polymorph = ent->GetCntPolyMorph();
	host_client->h2_old_v.cnt_flight = ent->GetCntFlight();
	host_client->h2_old_v.cnt_cubeofforce = ent->GetCntCubeOfForce();
	host_client->h2_old_v.cnt_invincibility = ent->GetCntInvincibility();
	host_client->h2_old_v.cameramode = ent->GetCameraMode();
}
