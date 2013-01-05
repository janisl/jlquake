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

#include "local.h"
#include "../../common/Common.h"
#include "../../common/message_utils.h"
#include "../../common/endian.h"

static byte svq2_fatpvs[65536 / 8];			// 32767 is MAX_MAP_LEAFS

//	Writes a delta update of an q2entity_state_t list to the message.
static void SVQ2_EmitPacketEntities(const q2client_frame_t* from, const q2client_frame_t* to, QMsg* msg)
{
	msg->WriteByte(q2svc_packetentities);

	int from_num_entities;
	if (!from)
	{
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	int newindex = 0;
	int oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		q2entity_state_t* newent;
		int newnum;
		if (newindex >= to->num_entities)
		{
			newnum = 9999;
		}
		else
		{
			newent = &svs.q2_client_entities[(to->first_entity + newindex) % svs.q2_num_client_entities];
			newnum = newent->number;
		}

		q2entity_state_t* oldent;
		int oldnum;
		if (oldindex >= from_num_entities)
		{
			oldnum = 9999;
		}
		else
		{
			oldent = &svs.q2_client_entities[(from->first_entity + oldindex) % svs.q2_num_client_entities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{	// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			// note that players are always 'newentities', this updates their oldorigin always
			// and prevents warping
			MSGQ2_WriteDeltaEntity(oldent, newent, msg, false, newent->number <= sv_maxclients->value);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{
			// this is a new entity, send it from the baseline
			MSGQ2_WriteDeltaEntity(&sv.q2_baselines[newnum], newent, msg, true, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{
			// the old entity isn't present in the new message
			int bits = Q2U_REMOVE;
			if (oldnum >= 256)
			{
				bits |= Q2U_NUMBER16 | Q2U_MOREBITS1;
			}

			msg->WriteByte(bits & 255);
			if (bits & 0x0000ff00)
			{
				msg->WriteByte((bits >> 8) & 255);
			}

			if (bits & Q2U_NUMBER16)
			{
				msg->WriteShort(oldnum);
			}
			else
			{
				msg->WriteByte(oldnum);
			}

			oldindex++;
			continue;
		}
	}

	msg->WriteShort(0);	// end of packetentities
}

static void SVQ2_WritePlayerstateToClient(const q2client_frame_t* from, const q2client_frame_t* to, QMsg* msg)
{
	const q2player_state_t* ps = &to->ps;
	const q2player_state_t* ops;
	q2player_state_t dummy;
	if (!from)
	{
		Com_Memset(&dummy, 0, sizeof(dummy));
		ops = &dummy;
	}
	else
	{
		ops = &from->ps;
	}

	//
	// determine what needs to be sent
	//
	int pflags = 0;

	if (ps->pmove.pm_type != ops->pmove.pm_type)
	{
		pflags |= Q2PS_M_TYPE;
	}

	if (ps->pmove.origin[0] != ops->pmove.origin[0] ||
		ps->pmove.origin[1] != ops->pmove.origin[1] ||
		ps->pmove.origin[2] != ops->pmove.origin[2])
	{
		pflags |= Q2PS_M_ORIGIN;
	}

	if (ps->pmove.velocity[0] != ops->pmove.velocity[0] ||
		ps->pmove.velocity[1] != ops->pmove.velocity[1] ||
		ps->pmove.velocity[2] != ops->pmove.velocity[2])
	{
		pflags |= Q2PS_M_VELOCITY;
	}

	if (ps->pmove.pm_time != ops->pmove.pm_time)
	{
		pflags |= Q2PS_M_TIME;
	}

	if (ps->pmove.pm_flags != ops->pmove.pm_flags)
	{
		pflags |= Q2PS_M_FLAGS;
	}

	if (ps->pmove.gravity != ops->pmove.gravity)
	{
		pflags |= Q2PS_M_GRAVITY;
	}

	if (ps->pmove.delta_angles[0] != ops->pmove.delta_angles[0] ||
		ps->pmove.delta_angles[1] != ops->pmove.delta_angles[1] ||
		ps->pmove.delta_angles[2] != ops->pmove.delta_angles[2])
	{
		pflags |= Q2PS_M_DELTA_ANGLES;
	}


	if (ps->viewoffset[0] != ops->viewoffset[0] ||
		ps->viewoffset[1] != ops->viewoffset[1] ||
		ps->viewoffset[2] != ops->viewoffset[2])
	{
		pflags |= Q2PS_VIEWOFFSET;
	}

	if (ps->viewangles[0] != ops->viewangles[0] ||
		ps->viewangles[1] != ops->viewangles[1] ||
		ps->viewangles[2] != ops->viewangles[2])
	{
		pflags |= Q2PS_VIEWANGLES;
	}

	if (ps->kick_angles[0] != ops->kick_angles[0] ||
		ps->kick_angles[1] != ops->kick_angles[1] ||
		ps->kick_angles[2] != ops->kick_angles[2])
	{
		pflags |= Q2PS_KICKANGLES;
	}

	if (ps->blend[0] != ops->blend[0] ||
		ps->blend[1] != ops->blend[1] ||
		ps->blend[2] != ops->blend[2] ||
		ps->blend[3] != ops->blend[3])
	{
		pflags |= Q2PS_BLEND;
	}

	if (ps->fov != ops->fov)
	{
		pflags |= Q2PS_FOV;
	}

	if (ps->rdflags != ops->rdflags)
	{
		pflags |= Q2PS_RDFLAGS;
	}

	if (ps->gunframe != ops->gunframe)
	{
		pflags |= Q2PS_WEAPONFRAME;
	}

	pflags |= Q2PS_WEAPONINDEX;

	//
	// write it
	//
	msg->WriteByte(q2svc_playerinfo);
	msg->WriteShort(pflags);

	//
	// write the q2pmove_state_t
	//
	if (pflags & Q2PS_M_TYPE)
	{
		msg->WriteByte(ps->pmove.pm_type);
	}

	if (pflags & Q2PS_M_ORIGIN)
	{
		msg->WriteShort(ps->pmove.origin[0]);
		msg->WriteShort(ps->pmove.origin[1]);
		msg->WriteShort(ps->pmove.origin[2]);
	}

	if (pflags & Q2PS_M_VELOCITY)
	{
		msg->WriteShort(ps->pmove.velocity[0]);
		msg->WriteShort(ps->pmove.velocity[1]);
		msg->WriteShort(ps->pmove.velocity[2]);
	}

	if (pflags & Q2PS_M_TIME)
	{
		msg->WriteByte(ps->pmove.pm_time);
	}

	if (pflags & Q2PS_M_FLAGS)
	{
		msg->WriteByte(ps->pmove.pm_flags);
	}

	if (pflags & Q2PS_M_GRAVITY)
	{
		msg->WriteShort(ps->pmove.gravity);
	}

	if (pflags & Q2PS_M_DELTA_ANGLES)
	{
		msg->WriteShort(ps->pmove.delta_angles[0]);
		msg->WriteShort(ps->pmove.delta_angles[1]);
		msg->WriteShort(ps->pmove.delta_angles[2]);
	}

	//
	// write the rest of the q2player_state_t
	//
	if (pflags & Q2PS_VIEWOFFSET)
	{
		msg->WriteChar(ps->viewoffset[0] * 4);
		msg->WriteChar(ps->viewoffset[1] * 4);
		msg->WriteChar(ps->viewoffset[2] * 4);
	}

	if (pflags & Q2PS_VIEWANGLES)
	{
		msg->WriteAngle16(ps->viewangles[0]);
		msg->WriteAngle16(ps->viewangles[1]);
		msg->WriteAngle16(ps->viewangles[2]);
	}

	if (pflags & Q2PS_KICKANGLES)
	{
		msg->WriteChar(ps->kick_angles[0] * 4);
		msg->WriteChar(ps->kick_angles[1] * 4);
		msg->WriteChar(ps->kick_angles[2] * 4);
	}

	if (pflags & Q2PS_WEAPONINDEX)
	{
		msg->WriteByte(ps->gunindex);
	}

	if (pflags & Q2PS_WEAPONFRAME)
	{
		msg->WriteByte(ps->gunframe);
		msg->WriteChar(ps->gunoffset[0] * 4);
		msg->WriteChar(ps->gunoffset[1] * 4);
		msg->WriteChar(ps->gunoffset[2] * 4);
		msg->WriteChar(ps->gunangles[0] * 4);
		msg->WriteChar(ps->gunangles[1] * 4);
		msg->WriteChar(ps->gunangles[2] * 4);
	}

	if (pflags & Q2PS_BLEND)
	{
		msg->WriteByte(ps->blend[0] * 255);
		msg->WriteByte(ps->blend[1] * 255);
		msg->WriteByte(ps->blend[2] * 255);
		msg->WriteByte(ps->blend[3] * 255);
	}
	if (pflags & Q2PS_FOV)
	{
		msg->WriteByte(ps->fov);
	}
	if (pflags & Q2PS_RDFLAGS)
	{
		msg->WriteByte(ps->rdflags);
	}

	// send stats
	int statbits = 0;
	for (int i = 0; i < MAX_STATS_Q2; i++)
	{
		if (ps->stats[i] != ops->stats[i])
		{
			statbits |= 1 << i;
		}
	}
	msg->WriteLong(statbits);
	for (int i = 0; i < MAX_STATS_Q2; i++)
	{
		if (statbits & (1 << i))
		{
			msg->WriteShort(ps->stats[i]);
		}
	}
}

void SVQ2_WriteFrameToClient(client_t* client, QMsg* msg)
{
	// this is the frame we are creating
	const q2client_frame_t* frame = &client->q2_frames[sv.q2_framenum & UPDATE_MASK_Q2];

	const q2client_frame_t* oldframe;
	int lastframe;
	if (client->q2_lastframe <= 0)
	{
		// client is asking for a retransmit
		oldframe = NULL;
		lastframe = -1;
	}
	else if (sv.q2_framenum - client->q2_lastframe >= (UPDATE_BACKUP_Q2 - 3))
	{
		// client hasn't gotten a good message through in a long time
		oldframe = NULL;
		lastframe = -1;
	}
	else
	{
		// we have a valid message to delta from
		oldframe = &client->q2_frames[client->q2_lastframe & UPDATE_MASK_Q2];
		lastframe = client->q2_lastframe;
	}

	msg->WriteByte(q2svc_frame);
	msg->WriteLong(sv.q2_framenum);
	msg->WriteLong(lastframe);	// what we are delta'ing from
	msg->WriteByte(client->q2_surpressCount);	// rate dropped packets
	client->q2_surpressCount = 0;

	// send over the areabits
	msg->WriteByte(frame->areabytes);
	msg->WriteData(frame->areabits, frame->areabytes);

	// delta encode the playerstate
	SVQ2_WritePlayerstateToClient(oldframe, frame, msg);

	// delta encode the entities
	SVQ2_EmitPacketEntities(oldframe, frame, msg);
}

//	The client will interpolate the view position,
// so we can't use a single PVS point
static void SVQ2_FatPVS(const vec3_t org)
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
		common->FatalError("SV_FatPVS: count < 1");
	}
	int longs = (CM_NumClusters() + 31) >> 5;

	// convert leafs to clusters
	for (int i = 0; i < count; i++)
	{
		leafs[i] = CM_LeafCluster(leafs[i]);
	}

	Com_Memcpy(svq2_fatpvs, CM_ClusterPVS(leafs[0]), longs << 2);
	// or in all the other leaf bits
	for (int i = 1; i < count; i++)
	{
		int j;
		for (j = 0; j < i; j++)
		{
			if (leafs[i] == leafs[j])
			{
				break;
			}
		}
		if (j != i)
		{
			continue;		// already have the cluster we want
		}
		byte* src = CM_ClusterPVS(leafs[i]);
		for (j = 0; j < longs; j++)
		{
			((long*)svq2_fatpvs)[j] |= ((long*)src)[j];
		}
	}
}

//	Decides which entities are going to be visible to the client, and
// copies off the playerstat and areabits.
void SVQ2_BuildClientFrame(client_t* client)
{
	q2edict_t* clent = client->q2_edict;
	if (!clent->client)
	{
		return;		// not in game yet

	}

	// this is the frame we are creating
	q2client_frame_t* frame = &client->q2_frames[sv.q2_framenum & UPDATE_MASK_Q2];

	frame->senttime = svs.realtime;	// save it for ping calc later

	// find the client's PVS
	vec3_t org;
	for (int i = 0; i < 3; i++)
	{
		org[i] = clent->client->ps.pmove.origin[i] * 0.125 + clent->client->ps.viewoffset[i];
	}

	int leafnum = CM_PointLeafnum(org);
	int clientarea = CM_LeafArea(leafnum);
	int clientcluster = CM_LeafCluster(leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits(frame->areabits, clientarea);

	// grab the current q2player_state_t
	frame->ps = clent->client->ps;


	SVQ2_FatPVS(org);
	byte* clientphs = CM_ClusterPHS(clientcluster);

	// build up the list of visible entities
	frame->num_entities = 0;
	frame->first_entity = svs.q2_next_client_entities;

	int c_fullsend = 0;

	for (int e = 1; e < ge->num_edicts; e++)
	{
		q2edict_t* ent = Q2_EDICT_NUM(e);

		// ignore ents without visible models
		if (ent->svflags & Q2SVF_NOCLIENT)
		{
			continue;
		}

		// ignore ents without visible models unless they have an effect
		if (!ent->s.modelindex && !ent->s.effects && !ent->s.sound &&
			!ent->s.event)
		{
			continue;
		}

		// ignore if not touching a PV leaf
		if (ent != clent)
		{
			// check area
			if (!CM_AreasConnected(clientarea, ent->areanum))
			{	// doors can legally straddle two areas, so
				// we may need to check another one
				if (!ent->areanum2 ||
					!CM_AreasConnected(clientarea, ent->areanum2))
				{
					continue;		// blocked by a door
				}
			}

			// beams just check one point for PHS
			if (ent->s.renderfx & Q2RF_BEAM)
			{
				int l = ent->clusternums[0];
				if (!(clientphs[l >> 3] & (1 << (l & 7))))
				{
					continue;
				}
			}
			else
			{
				// FIXME: if an ent has a model and a sound, but isn't
				// in the PVS, only the PHS, clear the model
				byte* bitvector = svq2_fatpvs;

				if (ent->num_clusters == -1)
				{
					// too many leafs for individual check, go by headnode
					if (!CM_HeadnodeVisible(ent->headnode, bitvector))
					{
						continue;
					}
					c_fullsend++;
				}
				else
				{
					// check individual leafs
					int i;
					for (i = 0; i < ent->num_clusters; i++)
					{
						int l = ent->clusternums[i];
						if (bitvector[l >> 3] & (1 << (l & 7)))
						{
							break;
						}
					}
					if (i == ent->num_clusters)
					{
						continue;		// not visible
					}
				}

				if (!ent->s.modelindex)
				{	// don't send sounds if they will be attenuated away
					vec3_t delta;
					float len;

					VectorSubtract(org, ent->s.origin, delta);
					len = VectorLength(delta);
					if (len > 400)
					{
						continue;
					}
				}
			}
		}

		// add it to the circular client_entities array
		q2entity_state_t* state = &svs.q2_client_entities[svs.q2_next_client_entities % svs.q2_num_client_entities];
		if (ent->s.number != e)
		{
			common->DPrintf("FIXING ENT->S.NUMBER!!!\n");
			ent->s.number = e;
		}
		*state = ent->s;

		// don't mark players missiles as solid
		if (ent->owner == client->q2_edict)
		{
			state->solid = 0;
		}

		svs.q2_next_client_entities++;
		frame->num_entities++;
	}
}

//	Save everything in the world out without deltas.
//	Used for recording footage for merged or assembled demos
void SVQ2_RecordDemoMessage()
{
	if (!svs.q2_demofile)
	{
		return;
	}

	q2entity_state_t nostate;
	Com_Memset(&nostate, 0, sizeof(nostate));
	QMsg buf;
	byte buf_data[32768];
	buf.InitOOB(buf_data, sizeof(buf_data));

	// write a frame message that doesn't contain a q2player_state_t
	buf.WriteByte(q2svc_frame);
	buf.WriteLong(sv.q2_framenum);

	buf.WriteByte(q2svc_packetentities);

	for (int e = 1; e < ge->num_edicts; e++)
	{
		q2edict_t* ent = Q2_EDICT_NUM(e);
		// ignore ents without visible models unless they have an effect
		if (ent->inuse &&
			ent->s.number &&
			(ent->s.modelindex || ent->s.effects || ent->s.sound || ent->s.event) &&
			!(ent->svflags & Q2SVF_NOCLIENT))
		{
			MSGQ2_WriteDeltaEntity(&nostate, &ent->s, &buf, false, true);
		}
	}

	buf.WriteShort(0);		// end of packetentities

	// now add the accumulated multicast information
	buf.WriteData(svs.q2_demo_multicast._data, svs.q2_demo_multicast.cursize);
	svs.q2_demo_multicast.Clear();

	// now write the entire message to the file, prefixed by the length
	int len = LittleLong(buf.cursize);
	FS_Write(&len, 4, svs.q2_demofile);
	FS_Write(buf._data, buf.cursize, svs.q2_demofile);
}
