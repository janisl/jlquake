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

#include "../server.h"
#include "local.h"
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

#define MAX_SNAPSHOT_ENTITIES   1024

struct snapshotEntityNumbers_t
{
	int numSnapshotEntities;
	int snapshotEntities[MAX_SNAPSHOT_ENTITIES];
};

/*
=============================================================================

Delta encode a client frame onto the network channel

A normal server packet will look like:

4	sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1	q3svc_snapshot
4	last client reliable command
4	serverTime
1	lastframe for delta compression
1	snapFlags
1	areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

//	Writes a delta update of an q3entityState_t list to the message.
static void SVQ3_EmitPacketEntities(q3clientSnapshot_t* from, q3clientSnapshot_t* to, QMsg* msg)
{
	// generate the delta update
	int from_num_entities;
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	q3entityState_t* newent = NULL;
	q3entityState_t* oldent = NULL;
	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		int newnum;
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.q3_snapshotEntities[(to->first_entity + newindex) % svs.q3_numSnapshotEntities];
			newnum = newent->number;
		}

		int oldnum;
		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.q3_snapshotEntities[(from->first_entity + oldindex) % svs.q3_numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSGQ3_WriteDeltaEntity(msg, oldent, newent, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSGQ3_WriteDeltaEntity(msg, &sv.q3_svEntities[newnum].q3_baseline, newent, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSGQ3_WriteDeltaEntity(msg, oldent, NULL, true);
			oldindex++;
			continue;
		}
	}

	msg->WriteBits((MAX_GENTITIES_Q3 - 1), GENTITYNUM_BITS_Q3);	// end of packetentities
}

//	Writes a delta update of an wsentityState_t list to the message.
static void SVWS_EmitPacketEntities(q3clientSnapshot_t* from, q3clientSnapshot_t* to, QMsg* msg)
{
	// generate the delta update
	int from_num_entities;
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	wsentityState_t* newent = NULL;
	wsentityState_t* oldent = NULL;
	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		int newnum;
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.ws_snapshotEntities[(to->first_entity + newindex) % svs.q3_numSnapshotEntities];
			newnum = newent->number;
		}

		int oldnum;
		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.ws_snapshotEntities[(from->first_entity + oldindex) % svs.q3_numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSGWS_WriteDeltaEntity(msg, oldent, newent, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSGWS_WriteDeltaEntity(msg, &sv.q3_svEntities[newnum].ws_baseline, newent, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSGWS_WriteDeltaEntity(msg, oldent, NULL, true);
			oldindex++;
			continue;
		}
	}

	msg->WriteBits((MAX_GENTITIES_Q3 - 1), GENTITYNUM_BITS_Q3);			// end of packetentities
}

//	Writes a delta update of an wmentityState_t list to the message.
static void SVWM_EmitPacketEntities(q3clientSnapshot_t* from, q3clientSnapshot_t* to, QMsg* msg)
{
	// generate the delta update
	int from_num_entities;
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	wmentityState_t* newent = NULL;
	wmentityState_t* oldent = NULL;
	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		int newnum;
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.wm_snapshotEntities[(to->first_entity + newindex) % svs.q3_numSnapshotEntities];
			newnum = newent->number;
		}

		int oldnum;
		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.wm_snapshotEntities[(from->first_entity + oldindex) % svs.q3_numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSGWM_WriteDeltaEntity(msg, oldent, newent, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSGWM_WriteDeltaEntity(msg, &sv.q3_svEntities[newnum].wm_baseline, newent, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSGWM_WriteDeltaEntity(msg, oldent, NULL, true);
			oldindex++;
			continue;
		}
	}

	msg->WriteBits((MAX_GENTITIES_Q3 - 1), GENTITYNUM_BITS_Q3);			// end of packetentities
}

//	Writes a delta update of an etentityState_t list to the message.
static void SVET_EmitPacketEntities(q3clientSnapshot_t* from, q3clientSnapshot_t* to, QMsg* msg)
{
	// generate the delta update
	int from_num_entities;
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	etentityState_t* newent = NULL;
	etentityState_t* oldent = NULL;
	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		int newnum;
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.et_snapshotEntities[(to->first_entity + newindex) % svs.q3_numSnapshotEntities];
			newnum = newent->number;
		}

		int oldnum;
		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.et_snapshotEntities[(from->first_entity + oldindex) % svs.q3_numSnapshotEntities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{
			// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSGET_WriteDeltaEntity(msg, oldent, newent, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSGET_WriteDeltaEntity(msg, &sv.q3_svEntities[newnum].et_baseline, newent, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			MSGET_WriteDeltaEntity(msg, oldent, NULL, true);
			oldindex++;
			continue;
		}
	}

	msg->WriteBits((MAX_GENTITIES_Q3 - 1), GENTITYNUM_BITS_Q3);			// end of packetentities
}

static void SVT3_WriteSnapshotToClient(client_t* client, QMsg* msg)
{
	q3clientSnapshot_t* frame, * oldframe;
	int lastframe;
	int i;
	int snapFlags;

	// this is the snapshot we are creating
	frame = &client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3];

	// try to use a previous frame as the source for delta compressing the snapshot
	if (client->q3_deltaMessage <= 0 || client->state != CS_ACTIVE)
	{
		// client is asking for a retransmit
		oldframe = NULL;
		lastframe = 0;
	}
	else if (client->netchan.outgoingSequence - client->q3_deltaMessage
			 >= (PACKET_BACKUP_Q3 - 3))
	{
		// client hasn't gotten a good message through in a long time
		common->DPrintf("%s: Delta request from out of date packet.\n", client->name);
		oldframe = NULL;
		lastframe = 0;
	}
	else
	{
		// we have a valid snapshot to delta from
		oldframe = &client->q3_frames[client->q3_deltaMessage & PACKET_MASK_Q3];
		lastframe = client->netchan.outgoingSequence - client->q3_deltaMessage;

		// the snapshot's entities may still have rolled off the buffer, though
		if (oldframe->first_entity <= svs.q3_nextSnapshotEntities - svs.q3_numSnapshotEntities)
		{
			common->DPrintf("%s: Delta request from out of date entities.\n", client->name);
			oldframe = NULL;
			lastframe = 0;
		}
	}

	msg->WriteByte(q3svc_snapshot);

	// NOTE, MRE: now sent at the start of every message from server to client
	// let the client know which reliable clientCommands we have received
	//msg->WriteLong(client->lastClientCommand );

	// send over the current server time so the client can drift
	// its view of time to try to match
	msg->WriteLong(svs.q3_time);

	// what we are delta'ing from
	msg->WriteByte(lastframe);

	snapFlags = svs.q3_snapFlagServerBit;
	if (client->q3_rateDelayed)
	{
		snapFlags |= Q3SNAPFLAG_RATE_DELAYED;
	}
	if (client->state != CS_ACTIVE)
	{
		snapFlags |= Q3SNAPFLAG_NOT_ACTIVE;
	}

	msg->WriteByte(snapFlags);

	// send over the areabits
	msg->WriteByte(frame->areabytes);
	msg->WriteData(frame->areabits, frame->areabytes);

	if (GGameType & GAME_WolfSP)
	{
		// delta encode the playerstate
		if (oldframe)
		{
			MSGWS_WriteDeltaPlayerstate(msg, &oldframe->ws_ps, &frame->ws_ps);
		}
		else
		{
			MSGWS_WriteDeltaPlayerstate(msg, NULL, &frame->ws_ps);
		}

		// delta encode the entities
		SVWS_EmitPacketEntities(oldframe, frame, msg);
	}
	else if (GGameType & GAME_WolfMP)
	{
		// delta encode the playerstate
		if (oldframe)
		{
			MSGWM_WriteDeltaPlayerstate(msg, &oldframe->wm_ps, &frame->wm_ps);
		}
		else
		{
			MSGWM_WriteDeltaPlayerstate(msg, NULL, &frame->wm_ps);
		}

		// delta encode the entities
		SVWM_EmitPacketEntities(oldframe, frame, msg);
	}
	else if (GGameType & GAME_ET)
	{
		// delta encode the playerstate
		if (oldframe)
		{
			MSGET_WriteDeltaPlayerstate(msg, &oldframe->et_ps, &frame->et_ps);
		}
		else
		{
			MSGET_WriteDeltaPlayerstate(msg, NULL, &frame->et_ps);
		}

		// delta encode the entities
		SVET_EmitPacketEntities(oldframe, frame, msg);
	}
	else
	{
		// delta encode the playerstate
		if (oldframe)
		{
			MSGQ3_WriteDeltaPlayerstate(msg, &oldframe->q3_ps, &frame->q3_ps);
		}
		else
		{
			MSGQ3_WriteDeltaPlayerstate(msg, NULL, &frame->q3_ps);
		}

		// delta encode the entities
		SVQ3_EmitPacketEntities(oldframe, frame, msg);
	}

	// padding for rate debugging
	if (svt3_padPackets->integer)
	{
		for (i = 0; i < svt3_padPackets->integer; i++)
		{
			msg->WriteByte(q3svc_nop);
		}
	}
}

//	(re)send all server commands the client hasn't acknowledged yet
void SVT3_UpdateServerCommandsToClient(client_t* client, QMsg* msg)
{
	// write any unacknowledged serverCommands
	for (int i = client->q3_reliableAcknowledge + 1; i <= client->q3_reliableSequence; i++)
	{
		msg->WriteByte(q3svc_serverCommand);
		msg->WriteLong(i);
		msg->WriteString(SVT3_GetReliableCommand(client, i & ((GGameType & GAME_Quake3 ?
			MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF) - 1)));
	}
	client->q3_reliableSent = client->q3_reliableSequence;
}

static int SVT3_QsortEntityNumbers(const void* a, const void* b)
{
	const int* ea = reinterpret_cast<const int*>(a);
	const int* eb = reinterpret_cast<const int*>(b);

	if (*ea == *eb)
	{
		common->Error("SV_QsortEntityStates: duplicated entity");
	}

	if (*ea < *eb)
	{
		return -1;
	}

	return 1;
}

static void SVT3_AddEntToSnapshot(int clientNum, q3svEntity_t* svEnt, idEntity3* ent, snapshotEntityNumbers_t* eNums)
{
	// if we have already added this entity to this snapshot, don't add again
	if (svEnt->snapshotCounter == sv.q3_snapshotCounter)
	{
		return;
	}
	svEnt->snapshotCounter = sv.q3_snapshotCounter;

	// if we are full, silently discard entities
	if (eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES)
	{
		return;
	}

	if (ent->GetSnapshotCallback())
	{
		if (!SVET_GameSnapshotCallback(ent->GetNumber(), clientNum))
		{
			return;
		}
	}

	eNums->snapshotEntities[eNums->numSnapshotEntities] = ent->GetNumber();
	eNums->numSnapshotEntities++;
}

static void SVT3_AddEntitiesVisibleFromPoint(int clientNum, const vec3_t origin, q3clientSnapshot_t* frame,
	snapshotEntityNumbers_t* eNums, bool portal, bool localClient)
{
	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specfically check for it
	if (!sv.state)
	{
		return;
	}

	int leafnum = CM_PointLeafnum(origin);
	int clientarea = CM_LeafArea(leafnum);
	int clientcluster = CM_LeafCluster(leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits(frame->areabits, clientarea);

	byte* clientpvs = CM_ClusterPVS(clientcluster);

	idEntity3* playerEnt = SVT3_EntityNum(clientNum);
	if (playerEnt->GetSvFlagSelfPortal())
	{
		SVT3_AddEntitiesVisibleFromPoint(clientNum, playerEnt->GetOrigin2(), frame, eNums, portal, localClient);
	}

	int l;
	for (int e = 0; e < sv.q3_num_entities; e++)
	{
		idEntity3* ent = SVT3_EntityNum(e);

		// never send entities that aren't linked in
		if (!ent->GetLinked())
		{
			continue;
		}

		if (ent->GetNumber() != e)
		{
			common->DPrintf("FIXING ENT->S.NUMBER!!!\n");
			ent->SetNumber(e);
		}

		// entities can be flagged to explicitly not be sent to the client
		if (ent->GetSvFlags() & Q3SVF_NOCLIENT)
		{
			continue;
		}

		// entities can be flagged to be sent to only one client
		if (ent->GetSvFlagSingleClient())
		{
			if (ent->GetSingleClient() != clientNum)
			{
				continue;
			}
		}
		// entities can be flagged to be sent to everyone but one client
		if (ent->GetSvFlagNotSingleClient())
		{
			if (ent->GetSingleClient() == clientNum)
			{
				continue;
			}
		}
		// entities can be flagged to be sent to a given mask of clients
		if (ent->GetSvFlagClientMask())
		{
			if (clientNum >= 32)
			{
				common->Error("Q3SVF_CLIENTMASK: cientNum > 32\n");
			}
			if (~ent->GetSingleClient() & (1 << clientNum))
			{
				continue;
			}
		}

		q3svEntity_t* svEnt = &sv.q3_svEntities[e];

		// don't double add an entity through portals
		if (svEnt->snapshotCounter == sv.q3_snapshotCounter)
		{
			continue;
		}

		// if this client is viewing from a camera, only add ents visible from portal ents
		if (playerEnt->GetEFlagViewingCamera() && !portal)
		{
			if (ent->GetSvFlags() & Q3SVF_PORTAL)
			{
				SVT3_AddEntToSnapshot(clientNum, svEnt, ent, eNums);
				SVT3_AddEntitiesVisibleFromPoint(clientNum, ent->GetOrigin2(), frame, eNums, true, localClient);
			}
			continue;
		}

		// broadcast entities are always sent
		if (ent->GetSvFlags() & Q3SVF_BROADCAST)
		{
			SVT3_AddEntToSnapshot(clientNum, svEnt, ent, eNums);
			continue;
		}

		byte* bitvector = clientpvs;

		// Gordon: just check origin for being in pvs, ignore bmodel extents
		if (ent->GetSvFlagIgnoreBModelExtents())
		{
			if (bitvector[svEnt->originCluster >> 3] & (1 << (svEnt->originCluster & 7)))
			{
				SVT3_AddEntToSnapshot(clientNum, svEnt, ent, eNums);
			}
			continue;
		}

		// ignore if not touching a PV leaf
		// check area
		if (!CM_AreasConnected(clientarea, svEnt->areanum))
		{
			// doors can legally straddle two areas, so
			// we may need to check another one
			if (!CM_AreasConnected(clientarea, svEnt->areanum2))
			{
				goto notVisible;	// blocked by a door
			}
		}

		// check individual leafs
		if (!svEnt->numClusters)
		{
			goto notVisible;
		}
		l = 0;
		int i;
		for (i = 0; i < svEnt->numClusters; i++)
		{
			l = svEnt->clusternums[i];
			if (bitvector[l >> 3] & (1 << (l & 7)))
			{
				break;
			}
		}

		// if we haven't found it to be visible,
		// check overflow clusters that coudln't be stored
		if (i == svEnt->numClusters)
		{
			if (svEnt->lastCluster)
			{
				for (; l <= svEnt->lastCluster; l++)
				{
					if (bitvector[l >> 3] & (1 << (l & 7)))
					{
						break;
					}
				}
				if (l == svEnt->lastCluster)
				{
					goto notVisible;	// not visible
				}
			}
			else
			{
				goto notVisible;
			}
		}

		//----(SA) added "visibility dummies"
		if (ent->GetSvFlags() & WOLFSVF_VISDUMMY)
		{
			//find master;
			idEntity3* ment = SVT3_EntityNum(ent->GetOtherEntityNum());

			q3svEntity_t* master = &sv.q3_svEntities[ent->GetOtherEntityNum()];

			if (master->snapshotCounter == sv.q3_snapshotCounter || !ment->GetLinked())
			{
				goto notVisible;
			}

			SVT3_AddEntToSnapshot(clientNum, master, ment, eNums);
			// master needs to be added, but not this dummy ent
			goto notVisible;
		}
		else if (ent->GetSvFlags() & WOLFSVF_VISDUMMY_MULTIPLE)
		{
			{
				for (int h = 0; h < sv.q3_num_entities; h++)
				{
					idEntity3* ment = SVT3_EntityNum(h);

					if (ment == ent)
					{
						continue;
					}

					q3svEntity_t* master = &sv.q3_svEntities[h];

					if (!ment->GetLinked())
					{
						continue;
					}

					if (ment->GetNumber() != h)
					{
						common->DPrintf("FIXING vis dummy multiple ment->S.NUMBER!!!\n");
						ment->SetNumber(h);
					}

					if (ment->GetSvFlags() & Q3SVF_NOCLIENT)
					{
						continue;
					}

					if (master->snapshotCounter == sv.q3_snapshotCounter)
					{
						continue;
					}

					if (ment->GetOtherEntityNum() == ent->GetNumber())
					{
						SVT3_AddEntToSnapshot(clientNum, master, ment, eNums);
					}
				}
				goto notVisible;
			}
		}

		// add it
		SVT3_AddEntToSnapshot(clientNum, svEnt, ent, eNums);

		// if its a portal entity, add everything visible from its camera position
		if (ent->GetSvFlags() & Q3SVF_PORTAL)
		{
			if (ent->GetGeneric1())
			{
				vec3_t dir;
				VectorSubtract(ent->GetOrigin(), origin, dir);
				if (VectorLengthSquared(dir) > (float)ent->GetGeneric1() * ent->GetGeneric1())
				{
					continue;
				}
			}
			SVT3_AddEntitiesVisibleFromPoint(clientNum, ent->GetOrigin2(), frame, eNums, true, localClient);
		}

		continue;

notVisible:

		// Ridah, if this entity has changed events, then send it regardless of whether we can see it or not
		// DHM - Nerve :: not in multiplayer please
		if (GGameType & (GAME_WolfSP | GAME_WolfMP) && svt3_gametype->integer == Q3GT_SINGLE_PLAYER && localClient)
		{
			if (ent->GetEventTime() == svs.q3_time)
			{
				ent->SetEFlagNoDraw();		// don't draw, just process event
				SVT3_AddEntToSnapshot(clientNum, svEnt, ent, eNums);
			}
			else if (ent->GetEType() == Q3ET_PLAYER)
			{
				// keep players around if they are alive and active (so sounds dont get messed up)
				if (!ent->GetEFlagDead())
				{
					ent->SetEFlagNoDraw();		// don't draw, just process events and sounds
					SVT3_AddEntToSnapshot(clientNum, svEnt, ent, eNums);
				}
			}
		}
	}
}

//	Decides which entities are going to be visible to the client, and
// copies off the playerstate and areabits.
//	This properly handles multiple recursive portals, but the render
// currently doesn't.
//	For viewing through other player's eyes, clent can be something other than client->gentity
static void SVT3_BuildClientSnapshot(client_t* client)
{
	// bump the counter used to prevent double adding
	sv.q3_snapshotCounter++;

	// this is the frame we are creating
	q3clientSnapshot_t* frame = &client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3];

	// clear everything in this snapshot
	snapshotEntityNumbers_t entityNumbers;
	entityNumbers.numSnapshotEntities = 0;
	Com_Memset(frame->areabits, 0, sizeof(frame->areabits));

	frame->num_entities = 0;

	idEntity3* clent = client->q3_entity;
	if (!clent || client->state == CS_ZOMBIE)
	{
		return;
	}

	// grab the current player state
	idPlayerState3* ps = SVT3_GameClientNum(client - svs.clients);
	if (GGameType & GAME_WolfSP)
	{
		// grab the current wsplayerState_t
		wsplayerState_t* gps = SVWS_GameClientNum(client - svs.clients);
		frame->ws_ps = *gps;
	}
	else if (GGameType & GAME_WolfMP)
	{
		// grab the current wmplayerState_t
		wmplayerState_t* gps = SVWM_GameClientNum(client - svs.clients);
		frame->wm_ps = *gps;
	}
	else if (GGameType & GAME_ET)
	{
		// grab the current etplayerState_t
		etplayerState_t* gps = SVET_GameClientNum(client - svs.clients);
		frame->et_ps = *gps;
	}
	else
	{
		// grab the current q3playerState_t
		q3playerState_t* gps = SVQ3_GameClientNum(client - svs.clients);
		frame->q3_ps = *gps;
	}

	// never send client's own entity, because it can
	// be regenerated from the playerstate
	int clientNum = ps->GetClientNum();
	if (clientNum < 0 || clientNum >= MAX_GENTITIES_Q3)
	{
		common->Error("SVT3_SvEntityForGentity: bad gEnt");
	}
	q3svEntity_t* svEnt = &sv.q3_svEntities[clientNum];

	svEnt->snapshotCounter = sv.q3_snapshotCounter;

	// find the client's viewpoint
	vec3_t org;
	if (clent->GetSvFlagSelfPortalExclusive())
	{
		VectorCopy(clent->GetOrigin2(), org);
	}
	else
	{
		VectorCopy(ps->GetOrigin(), org);
	}
	org[2] += ps->GetViewHeight();

	// need to account for lean, so areaportal doors draw properly
	if (ps->GetLeanf() != 0)
	{
		vec3_t right, v3ViewAngles;
		VectorCopy(ps->GetViewAngles(), v3ViewAngles);
		v3ViewAngles[2] += ps->GetLeanf() / 2.0f;
		AngleVectors(v3ViewAngles, NULL, right, NULL);
		VectorMA(org, ps->GetLeanf(), right, org);
	}

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
	SVT3_AddEntitiesVisibleFromPoint(clientNum, org, frame, &entityNumbers,
		false, client->netchan.remoteAddress.type == NA_LOOPBACK);

	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort(entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
		sizeof(entityNumbers.snapshotEntities[0]), SVT3_QsortEntityNumbers);

	// now that all viewpoint's areabits have been OR'd together, invert
	// all of them to make it a mask vector, which is what the renderer wants
	for (int i = 0; i < MAX_MAP_AREA_BYTES / 4; i++)
	{
		((int*)frame->areabits)[i] = ((int*)frame->areabits)[i] ^ -1;
	}

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.q3_nextSnapshotEntities;
	for (int i = 0; i < entityNumbers.numSnapshotEntities; i++)
	{
		if (GGameType & GAME_WolfSP)
		{
			wssharedEntity_t* ent = SVWS_GentityNum(entityNumbers.snapshotEntities[i]);
			wsentityState_t* state = &svs.ws_snapshotEntities[svs.q3_nextSnapshotEntities % svs.q3_numSnapshotEntities];
			*state = ent->s;
		}
		else if (GGameType & GAME_WolfMP)
		{
			wmsharedEntity_t* ent = SVWM_GentityNum(entityNumbers.snapshotEntities[i]);
			wmentityState_t* state = &svs.wm_snapshotEntities[svs.q3_nextSnapshotEntities % svs.q3_numSnapshotEntities];
			*state = ent->s;
		}
		else if (GGameType & GAME_ET)
		{
			etsharedEntity_t* ent = SVET_GentityNum(entityNumbers.snapshotEntities[i]);
			etentityState_t* state = &svs.et_snapshotEntities[svs.q3_nextSnapshotEntities % svs.q3_numSnapshotEntities];
			*state = ent->s;
		}
		else
		{
			q3sharedEntity_t* ent = SVQ3_GentityNum(entityNumbers.snapshotEntities[i]);
			q3entityState_t* state = &svs.q3_snapshotEntities[svs.q3_nextSnapshotEntities % svs.q3_numSnapshotEntities];
			*state = ent->s;
		}
		svs.q3_nextSnapshotEntities++;
		// this should never hit, map should always be restarted first in SVT3_Frame
		if (svs.q3_nextSnapshotEntities >= 0x7FFFFFFE)
		{
			common->FatalError("svs.q3_nextSnapshotEntities wrapped");
		}
		frame->num_entities++;
	}
}

//	Return the number of msec a given size message is supposed
// to take to clear, based on the current rate
// TTimo - use sv_maxRate or sv_dl_maxRate depending on regular or downloading client
static int SVT3_RateMsec(client_t* client, int messageSize)
{
	// include our header, IP header, and some overhead
	enum { HEADER_RATE_BYTES = 48 };

	// individual messages will never be larger than fragment size
	if (messageSize > 1500)
	{
		messageSize = 1500;
	}
	// low watermark for sv_maxRate, never 0 < sv_maxRate < 1000 (0 is no limitation)
	if (svt3_maxRate->integer && svt3_maxRate->integer < 1000)
	{
		Cvar_Set("sv_MaxRate", "1000");
	}
	int rate = client->rate;
	// work on the appropriate max rate (client or download)
	int maxRate;
	if (!*client->downloadName || !(GGameType & (GAME_WolfMP | GAME_ET)))
	{
		maxRate = svt3_maxRate->integer;
	}
	else
	{
		maxRate = svt3_dl_maxRate->integer;
	}
	if (maxRate)
	{
		if (maxRate < rate)
		{
			rate = maxRate;
		}
	}
	int rateMsec = (messageSize + HEADER_RATE_BYTES) * 1000 / rate;

	return rateMsec;
}

void SVT3_SendMessageToClient(QMsg* msg, client_t* client)
{
	// record information about the message
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageSize = msg->cursize;
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageSent = svs.q3_time;
	client->q3_frames[client->netchan.outgoingSequence & PACKET_MASK_Q3].messageAcked = -1;

	// send the datagram
	SVT3_Netchan_Transmit(client, msg);

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	if (client->netchan.remoteAddress.type == NA_LOOPBACK || (svt3_lanForceRate->integer && SOCK_IsLANAddress(client->netchan.remoteAddress)))
	{
		client->q3_nextSnapshotTime = svs.q3_time - 1;
		return;
	}

	// normal rate / snapshotMsec calculation
	int rateMsec = SVT3_RateMsec(client, msg->cursize);

	// TTimo - during a download, ignore the snapshotMsec
	// the update server on steroids, with this disabled and sv_fps 60, the download can reach 30 kb/s
	// on a regular server, we will still top at 20 kb/s because of sv_fps 20
	if (!*client->downloadName && rateMsec < client->q3_snapshotMsec)
	{
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->q3_snapshotMsec;
		client->q3_rateDelayed = false;
	}
	else
	{
		client->q3_rateDelayed = true;
	}

	client->q3_nextSnapshotTime = svs.q3_time + rateMsec;

	// don't pile up empty snapshots while connecting
	if (client->state != CS_ACTIVE)
	{
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if (!*client->downloadName && client->q3_nextSnapshotTime < svs.q3_time + 1000)
		{
			client->q3_nextSnapshotTime = svs.q3_time + 1000;
		}
	}
}

//	There is no need to send full snapshots to clients who are loading a map.
// So we send them "idle" packets with the bare minimum required to keep them on the server.
static void SVET_SendClientIdle(client_t* client)
{
	byte msg_buf[MAX_MSGLEN_WOLF];
	QMsg msg;
	msg.Init(msg_buf, sizeof(msg_buf));

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// (re)send any reliable server commands
	SVT3_UpdateServerCommandsToClient(client, &msg);

	// Add any download data if the client is downloading
	SVT3_WriteDownloadToClient(client, &msg);

	// check for overflow
	if (msg.overflowed)
	{
		common->Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();

		SVT3_DropClient(client, "Msg overflowed");
		return;
	}

	SVT3_SendMessageToClient(&msg, client);

	sv.wm_bpsTotalBytes += msg.cursize;			// NERVE - SMF - net debugging
	sv.wm_ubpsTotalBytes += msg.uncompsize / 8;	// NERVE - SMF - net debugging
}

void SVT3_SendClientSnapshot(client_t* client)
{
	if (GGameType & GAME_ET && client->state < CS_ACTIVE)
	{
		// bani - #760 - zombie clients need full snaps so they can still process reliable commands
		// (eg so they can pick up the disconnect reason)
		if (client->state != CS_ZOMBIE)
		{
			SVET_SendClientIdle(client);
			return;
		}
	}

	//RF, AI don't need snapshots built
	if (GGameType & GAME_WolfSP && client->q3_entity && client->q3_entity->GetSvFlagCastAI())
	{
		return;
	}

	// build the snapshot
	SVT3_BuildClientSnapshot(client);

	// bots need to have their snapshots build, but
	// the query them directly without needing to be sent
	if (client->q3_entity && client->q3_entity->GetSvFlags() & Q3SVF_BOT)
	{
		return;
	}

	QMsg msg;
	byte msg_buf[MAX_MSGLEN];
	msg.Init(msg_buf, GGameType & GAME_Quake3 ? MAX_MSGLEN_Q3 : MAX_MSGLEN_WOLF);
	if (!(GGameType & GAME_Quake3))
	{
		msg.allowoverflow = true;
	}

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	msg.WriteLong(client->q3_lastClientCommand);

	// (re)send any reliable server commands
	SVT3_UpdateServerCommandsToClient(client, &msg);

	// send over all the relevant entityState_t
	// and the playerState_t
	SVT3_WriteSnapshotToClient(client, &msg);

	// Add any download data if the client is downloading
	SVT3_WriteDownloadToClient(client, &msg);

	// check for overflow
	if (msg.overflowed)
	{
		common->Printf("WARNING: msg overflowed for %s\n", client->name);
		msg.Clear();

		if (GGameType & GAME_ET)
		{
			SVT3_DropClient(client, "Msg overflowed");
			return;
		}
	}

	SVT3_SendMessageToClient(&msg, client);

	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		sv.wm_bpsTotalBytes += msg.cursize;			// NERVE - SMF - net debugging
		sv.wm_ubpsTotalBytes += msg.uncompsize / 8;	// NERVE - SMF - net debugging
	}
}

void SVT3_SendClientMessages()
{
	if (GGameType & (GAME_WolfMP | GAME_ET))
	{
		sv.wm_bpsTotalBytes = 0;		// NERVE - SMF - net debugging
		sv.wm_ubpsTotalBytes = 0;		// NERVE - SMF - net debugging
	}

	if (GGameType & GAME_ET)
	{
		// Gordon: update any changed configstrings from this frame
		SVET_UpdateConfigStrings();
	}

	// send a message to each connected client
	int numclients = 0;			// NERVE - SMF - net debugging
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t* c = &svs.clients[i];

		// rain - changed <= CS_ZOMBIE to < CS_ZOMBIE so that the
		// disconnect reason is properly sent in the network stream
		if (c->state < CS_ZOMBIE)
		{
			continue;		// not connected
		}

		// RF, needed to insert this otherwise bots would cause error drops in sv_net_chan.c:
		// --> "netchan queue is not properly initialized in SVT3_Netchan_TransmitNextFragment\n"
		if (GGameType & GAME_ET && c->q3_entity && c->q3_entity->GetSvFlags() & Q3SVF_BOT)
		{
			continue;
		}

		if (svs.q3_time < c->q3_nextSnapshotTime)
		{
			continue;		// not time yet
		}

		numclients++;		// NERVE - SMF - net debugging

		// send additional message fragments if the last message
		// was too large to send at once
		if (c->netchan.unsentFragments)
		{
			c->q3_nextSnapshotTime = svs.q3_time +
								  SVT3_RateMsec(c, c->netchan.reliableOrUnsentLength - c->netchan.unsentFragmentStart);
			SVT3_Netchan_TransmitNextFragment(c);
			continue;
		}

		// generate and send a new message
		SVT3_SendClientSnapshot(c);
	}

	// net debugging
	if (GGameType & (GAME_WolfMP | GAME_ET) && svwm_showAverageBPS->integer && numclients > 0)
	{
		float ave = 0, uave = 0;

		for (int i = 0; i < MAX_BPS_WINDOW - 1; i++)
		{
			sv.wm_bpsWindow[i] = sv.wm_bpsWindow[i + 1];
			ave += sv.wm_bpsWindow[i];

			sv.wm_ubpsWindow[i] = sv.wm_ubpsWindow[i + 1];
			uave += sv.wm_ubpsWindow[i];
		}

		sv.wm_bpsWindow[MAX_BPS_WINDOW - 1] = sv.wm_bpsTotalBytes;
		ave += sv.wm_bpsTotalBytes;

		sv.wm_ubpsWindow[MAX_BPS_WINDOW - 1] = sv.wm_ubpsTotalBytes;
		uave += sv.wm_ubpsTotalBytes;

		if (sv.wm_bpsTotalBytes >= sv.wm_bpsMaxBytes)
		{
			sv.wm_bpsMaxBytes = sv.wm_bpsTotalBytes;
		}

		if (sv.wm_ubpsTotalBytes >= sv.wm_ubpsMaxBytes)
		{
			sv.wm_ubpsMaxBytes = sv.wm_ubpsTotalBytes;
		}

		sv.wm_bpsWindowSteps++;

		if (sv.wm_bpsWindowSteps >= MAX_BPS_WINDOW)
		{
			float comp_ratio;

			sv.wm_bpsWindowSteps = 0;

			ave = (ave / (float)MAX_BPS_WINDOW);
			uave = (uave / (float)MAX_BPS_WINDOW);

			comp_ratio = (1 - ave / uave) * 100.f;
			sv.wm_ucompAve += comp_ratio;
			sv.wm_ucompNum++;

			common->DPrintf("bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
				ave / (float)numclients, ave, sv.wm_bpsMaxBytes, uave, sv.wm_ubpsMaxBytes, comp_ratio, sv.wm_ucompAve / sv.wm_ucompNum);
		}
	}
}
