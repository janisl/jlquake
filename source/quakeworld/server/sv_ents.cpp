/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

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

// because there can be a lot of nails, there is a special
// network protocol for them
#define	MAX_NAILS	32
qhedict_t	*nails[MAX_NAILS];
int		numnails;

extern	int	sv_nailmodel, sv_supernailmodel, sv_playermodel;

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

	msg->WriteByte(qwsvc_nails);
	msg->WriteByte(numnails);

	for (n=0 ; n<numnails ; n++)
	{
		ent = nails[n];
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

//=============================================================================


/*
==================
SV_WriteDelta

Writes part of a packetentities message.
Can delta from either a baseline or a previous packet_entity
==================
*/
void SV_WriteDelta (q1entity_state_t *from, q1entity_state_t *to, QMsg *msg, qboolean force)
{
	int		bits;
	int		i;
	float	miss;

// send an update
	bits = 0;
	
	for (i=0 ; i<3 ; i++)
	{
		miss = to->origin[i] - from->origin[i];
		if ( miss < -0.1 || miss > 0.1 )
			bits |= QWU_ORIGIN1<<i;
	}

	if ( to->angles[0] != from->angles[0] )
		bits |= QWU_ANGLE1;
		
	if ( to->angles[1] != from->angles[1] )
		bits |= QWU_ANGLE2;
		
	if ( to->angles[2] != from->angles[2] )
		bits |= QWU_ANGLE3;
		
	if ( to->colormap != from->colormap )
		bits |= QWU_COLORMAP;
		
	if ( to->skinnum != from->skinnum )
		bits |= QWU_SKIN;
		
	if ( to->frame != from->frame )
		bits |= QWU_FRAME;
	
	if ( to->effects != from->effects )
		bits |= QWU_EFFECTS;
	
	if ( to->modelindex != from->modelindex )
		bits |= QWU_MODEL;

	if (bits & 511)
		bits |= QWU_MOREBITS;

	if (to->flags & QWU_SOLID)
		bits |= QWU_SOLID;

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
	if (i & QWU_REMOVE)
		Sys_Error ("QWU_REMOVE");
	msg->WriteShort(i);
	
	if (bits & QWU_MOREBITS)
		msg->WriteByte(bits&255);
	if (bits & QWU_MODEL)
		msg->WriteByte(to->modelindex);
	if (bits & QWU_FRAME)
		msg->WriteByte(to->frame);
	if (bits & QWU_COLORMAP)
		msg->WriteByte(to->colormap);
	if (bits & QWU_SKIN)
		msg->WriteByte(to->skinnum);
	if (bits & QWU_EFFECTS)
		msg->WriteByte(to->effects);
	if (bits & QWU_ORIGIN1)
		msg->WriteCoord(to->origin[0]);		
	if (bits & QWU_ANGLE1)
		msg->WriteAngle(to->angles[0]);
	if (bits & QWU_ORIGIN2)
		msg->WriteCoord(to->origin[1]);
	if (bits & QWU_ANGLE2)
		msg->WriteAngle(to->angles[1]);
	if (bits & QWU_ORIGIN3)
		msg->WriteCoord(to->origin[2]);
	if (bits & QWU_ANGLE3)
		msg->WriteAngle(to->angles[2]);
}

/*
=============
SV_EmitPacketEntities

Writes a delta update of a qwpacket_entities_t to the message.

=============
*/
void SV_EmitPacketEntities (client_t *client, qwpacket_entities_t *to, QMsg *msg)
{
	qhedict_t	*ent;
	client_frame_t	*fromframe;
	qwpacket_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;

	// this is the frame that we are going to delta update from
	if (client->delta_sequence != -1)
	{
		fromframe = &client->frames[client->delta_sequence & UPDATE_MASK_QW];
		from = &fromframe->entities;
		oldmax = from->num_entities;

		msg->WriteByte(qwsvc_deltapacketentities);
		msg->WriteByte(client->delta_sequence);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		msg->WriteByte(qwsvc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK_QW
//			, client->netchan.outgoing_sequence & UPDATE_MASK_QW);
	while (newindex < to->num_entities || oldindex < oldmax)
	{
		newnum = newindex >= to->num_entities ? 9999 : to->entities[newindex].number;
		oldnum = oldindex >= oldmax ? 9999 : from->entities[oldindex].number;

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (&from->entities[oldindex], &to->entities[newindex], msg, false);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			ent = EDICT_NUM(newnum);
//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (&ent->q1_baseline, &to->entities[newindex], msg, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			msg->WriteShort(oldnum | QWU_REMOVE);
			oldindex++;
			continue;
		}
	}

	msg->WriteShort(0);	// end of packetentities
}

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
	qwusercmd_t	cmd;
	int			pflags;

	for (j=0,cl=svs.clients ; j<MAX_CLIENTS_QW ; j++,cl++)
	{
		if (cl->state != cs_spawned)
			continue;

		ent = cl->edict;

		// ZOID visibility tracking
		if (ent != clent &&
			!(client->spec_track && client->spec_track - 1 == j)) 
		{
			if (cl->spectator)
				continue;

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
		}
		
		pflags = QWPF_MSEC | QWPF_COMMAND;
		
		if (ent->v.modelindex != sv_playermodel)
			pflags |= QWPF_MODEL;
		for (i=0 ; i<3 ; i++)
			if (ent->GetVelocity()[i])
				pflags |= QWPF_VELOCITY1ND<<i;
		if (ent->GetEffects())
			pflags |= QWPF_EFFECTS;
		if (ent->GetSkin())
			pflags |= QWPF_SKINNUM;
		if (ent->GetHealth() <= 0)
			pflags |= QWPF_DEAD;
		if (ent->GetMins()[2] != -24)
			pflags |= QWPF_GIB;

		if (cl->spectator)
		{	// only sent origin and velocity to spectators
			pflags &= QWPF_VELOCITY1ND | QWPF_VELOCITY2 | QWPF_VELOCITY3;
		}
		else if (ent == clent)
		{	// don't send a lot of data on personal entity
			pflags &= ~(QWPF_MSEC|QWPF_COMMAND);
			if (ent->GetWeaponFrame())
				pflags |= QWPF_WEAPONFRAME;
		}

		if (client->spec_track && client->spec_track - 1 == j &&
			ent->GetWeaponFrame()) 
			pflags |= QWPF_WEAPONFRAME;

		msg->WriteByte(qwsvc_playerinfo);
		msg->WriteByte(j);
		msg->WriteShort(pflags);

		for (i=0 ; i<3 ; i++)
			msg->WriteCoord(ent->GetOrigin()[i]);
		
		msg->WriteByte(ent->GetFrame());

		if (pflags & QWPF_MSEC)
		{
			msec = 1000*(sv.time - cl->localtime);
			if (msec > 255)
				msec = 255;
			msg->WriteByte(msec);
		}
		
		if (pflags & QWPF_COMMAND)
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

			MSGQW_WriteDeltaUsercmd (msg, &nullcmd, &cmd);
		}

		for (i=0 ; i<3 ; i++)
			if (pflags & (QWPF_VELOCITY1ND<<i) )
				msg->WriteShort(ent->GetVelocity()[i]);

		if (pflags & QWPF_MODEL)
			msg->WriteByte(ent->v.modelindex);

		if (pflags & QWPF_SKINNUM)
			msg->WriteByte(ent->GetSkin());

		if (pflags & QWPF_EFFECTS)
			msg->WriteByte(ent->GetEffects());

		if (pflags & QWPF_WEAPONFRAME)
			msg->WriteByte(ent->GetWeaponFrame());
	}
}


/*
=============
SV_WriteEntitiesToClient

Encodes the current state of the world as
a qwsvc_packetentities messages and possibly
a qwsvc_nails message and
qwsvc_playerinfo messages
=============
*/
void SV_WriteEntitiesToClient (client_t *client, QMsg *msg)
{
	int		e, i;
	byte	*pvs;
	vec3_t	org;
	qhedict_t	*ent;
	qwpacket_entities_t	*pack;
	qhedict_t	*clent;
	client_frame_t	*frame;
	q1entity_state_t	*state;

	// this is the frame we are creating
	frame = &client->frames[client->netchan.incomingSequence & UPDATE_MASK_QW];

	// find the client's PVS
	clent = client->edict;
	VectorAdd (clent->GetOrigin(), clent->GetViewOfs(), org);
	pvs = SV_FatPVS (org);

	// send over the players in the PVS
	SV_WritePlayersToClient (client, clent, pvs, msg);
	
	// put other visible entities into either a packet_entities or a nails message
	pack = &frame->entities;
	pack->num_entities = 0;

	numnails = 0;

	for (e=MAX_CLIENTS_QW+1, ent=EDICT_NUM(e) ; e<sv.num_edicts ; e++, ent = NEXT_EDICT(ent))
	{
		// ignore ents without visible models
		if (!ent->v.modelindex || !*PR_GetString(ent->GetModel()))
			continue;

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

		if (SV_AddNailUpdate (ent))
			continue;	// added to the special update list

		// add to the packetentities
		if (pack->num_entities == QWMAX_PACKET_ENTITIES)
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
	}

	// encode the packet entities as a delta from the
	// last packetentities acknowledged by the client

	SV_EmitPacketEntities (client, pack, msg);

	// now add the specialized nail update
	SV_EmitNailUpdate (msg);
}
