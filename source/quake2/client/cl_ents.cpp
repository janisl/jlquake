/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_ents.c -- entity parsing and management

#include "client.h"

/*
=========================================================================

FRAME PARSING

=========================================================================
*/

/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int CL_ParseEntityBits(unsigned* bits)
{
	unsigned b, total;
	int i;
	int number;

	total = net_message.ReadByte();
	if (total & Q2U_MOREBITS1)
	{
		b = net_message.ReadByte();
		total |= b << 8;
	}
	if (total & Q2U_MOREBITS2)
	{
		b = net_message.ReadByte();
		total |= b << 16;
	}
	if (total & Q2U_MOREBITS3)
	{
		b = net_message.ReadByte();
		total |= b << 24;
	}

	// count the bits for net profiling
	for (i = 0; i < 32; i++)
		if (total & (1 << i))
		{
			bitcounts[i]++;
		}

	if (total & Q2U_NUMBER16)
	{
		number = net_message.ReadShort();
	}
	else
	{
		number = net_message.ReadByte();
	}

	*bits = total;

	return number;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CL_ParseDelta(q2entity_state_t* from, q2entity_state_t* to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy(from->origin, to->old_origin);
	to->number = number;

	if (bits & Q2U_MODEL)
	{
		to->modelindex = net_message.ReadByte();
	}
	if (bits & Q2U_MODEL2)
	{
		to->modelindex2 = net_message.ReadByte();
	}
	if (bits & Q2U_MODEL3)
	{
		to->modelindex3 = net_message.ReadByte();
	}
	if (bits & Q2U_MODEL4)
	{
		to->modelindex4 = net_message.ReadByte();
	}

	if (bits & Q2U_FRAME8)
	{
		to->frame = net_message.ReadByte();
	}
	if (bits & Q2U_FRAME16)
	{
		to->frame = net_message.ReadShort();
	}

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
	{
		to->skinnum = net_message.ReadLong();
	}
	else if (bits & Q2U_SKIN8)
	{
		to->skinnum = net_message.ReadByte();
	}
	else if (bits & Q2U_SKIN16)
	{
		to->skinnum = net_message.ReadShort();
	}

	if ((bits & (Q2U_EFFECTS8 | Q2U_EFFECTS16)) == (Q2U_EFFECTS8 | Q2U_EFFECTS16))
	{
		to->effects = net_message.ReadLong();
	}
	else if (bits & Q2U_EFFECTS8)
	{
		to->effects = net_message.ReadByte();
	}
	else if (bits & Q2U_EFFECTS16)
	{
		to->effects = net_message.ReadShort();
	}

	if ((bits & (Q2U_RENDERFX8 | Q2U_RENDERFX16)) == (Q2U_RENDERFX8 | Q2U_RENDERFX16))
	{
		to->renderfx = net_message.ReadLong();
	}
	else if (bits & Q2U_RENDERFX8)
	{
		to->renderfx = net_message.ReadByte();
	}
	else if (bits & Q2U_RENDERFX16)
	{
		to->renderfx = net_message.ReadShort();
	}

	if (bits & Q2U_ORIGIN1)
	{
		to->origin[0] = net_message.ReadCoord();
	}
	if (bits & Q2U_ORIGIN2)
	{
		to->origin[1] = net_message.ReadCoord();
	}
	if (bits & Q2U_ORIGIN3)
	{
		to->origin[2] = net_message.ReadCoord();
	}

	if (bits & Q2U_ANGLE1)
	{
		to->angles[0] = net_message.ReadAngle();
	}
	if (bits & Q2U_ANGLE2)
	{
		to->angles[1] = net_message.ReadAngle();
	}
	if (bits & Q2U_ANGLE3)
	{
		to->angles[2] = net_message.ReadAngle();
	}

	if (bits & Q2U_OLDORIGIN)
	{
		net_message.ReadPos(to->old_origin);
	}

	if (bits & Q2U_SOUND)
	{
		to->sound = net_message.ReadByte();
	}

	if (bits & Q2U_EVENT)
	{
		to->event = net_message.ReadByte();
	}
	else
	{
		to->event = 0;
	}

	if (bits & Q2U_SOLID)
	{
		to->solid = net_message.ReadShort();
	}
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity(q2frame_t* frame, int newnum, q2entity_state_t* old, int bits)
{
	q2centity_t* ent;
	q2entity_state_t* state;

	ent = &clq2_entities[newnum];

	state = &clq2_parse_entities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q2 - 1)];
	cl.parseEntitiesNum++;
	frame->num_entities++;

	CL_ParseDelta(old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex ||
		state->modelindex2 != ent->current.modelindex2 ||
		state->modelindex3 != ent->current.modelindex3 ||
		state->modelindex4 != ent->current.modelindex4 ||
		abs(state->origin[0] - ent->current.origin[0]) > 512 ||
		abs(state->origin[1] - ent->current.origin[1]) > 512 ||
		abs(state->origin[2] - ent->current.origin[2]) > 512 ||
		state->event == Q2EV_PLAYER_TELEPORT ||
		state->event == Q2EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.q2_frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		ent->trailcount = 1024;		// for diminishing rocket / grenade trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == Q2EV_OTHER_TELEPORT)
		{
			VectorCopy(state->origin, ent->prev.origin);
			VectorCopy(state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy(state->old_origin, ent->prev.origin);
			VectorCopy(state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.q2_frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An q2svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities(q2frame_t* oldframe, q2frame_t* newframe)
{
	int newnum;
	unsigned int bits;
	q2entity_state_t* oldstate;
	int oldindex, oldnum;

	newframe->parse_entities = cl.parseEntitiesNum;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
	{
		oldnum = 99999;
	}
	else
	{
		if (oldindex >= oldframe->num_entities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &clq2_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES_Q2 - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CL_ParseEntityBits(&bits);
		if (newnum >= MAX_EDICTS_Q2)
		{
			Com_Error(ERR_DROP,"CL_ParsePacketEntities: bad number:%i", newnum);
		}

		if (net_message.readcount > net_message.cursize)
		{
			Com_Error(ERR_DROP,"CL_ParsePacketEntities: end of message");
		}

		if (!newnum)
		{
			break;
		}

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
			{
				common->Printf("   unchanged: %i\n", oldnum);
			}
			CL_DeltaEntity(newframe, oldnum, oldstate, 0);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &clq2_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES_Q2 - 1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & Q2U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet->value == 3)
			{
				common->Printf("   remove: %i\n", newnum);
			}
			if (oldnum != newnum)
			{
				common->Printf("Q2U_REMOVE: oldnum != newnum\n");
			}

			oldindex++;

			if (oldindex >= oldframe->num_entities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &clq2_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES_Q2 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet->value == 3)
			{
				common->Printf("   delta: %i\n", newnum);
			}
			CL_DeltaEntity(newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &clq2_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES_Q2 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet->value == 3)
			{
				common->Printf("   baseline: %i\n", newnum);
			}
			CL_DeltaEntity(newframe, newnum, &clq2_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
		{
			common->Printf("   unchanged: %i\n", oldnum);
		}
		CL_DeltaEntity(newframe, oldnum, oldstate, 0);

		oldindex++;

		if (oldindex >= oldframe->num_entities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &clq2_parse_entities[(oldframe->parse_entities + oldindex) & (MAX_PARSE_ENTITIES_Q2 - 1)];
			oldnum = oldstate->number;
		}
	}
}



/*
===================
CL_ParsePlayerstate
===================
*/
void CL_ParsePlayerstate(q2frame_t* oldframe, q2frame_t* newframe)
{
	int flags;
	q2player_state_t* state;
	int i;
	int statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
	{
		*state = oldframe->playerstate;
	}
	else
	{
		Com_Memset(state, 0, sizeof(*state));
	}

	flags = net_message.ReadShort();

	//
	// parse the q2pmove_state_t
	//
	if (flags & Q2PS_M_TYPE)
	{
		state->pmove.pm_type = (q2pmtype_t)net_message.ReadByte();
	}

	if (flags & Q2PS_M_ORIGIN)
	{
		state->pmove.origin[0] = net_message.ReadShort();
		state->pmove.origin[1] = net_message.ReadShort();
		state->pmove.origin[2] = net_message.ReadShort();
	}

	if (flags & Q2PS_M_VELOCITY)
	{
		state->pmove.velocity[0] = net_message.ReadShort();
		state->pmove.velocity[1] = net_message.ReadShort();
		state->pmove.velocity[2] = net_message.ReadShort();
	}

	if (flags & Q2PS_M_TIME)
	{
		state->pmove.pm_time = net_message.ReadByte();
	}

	if (flags & Q2PS_M_FLAGS)
	{
		state->pmove.pm_flags = net_message.ReadByte();
	}

	if (flags & Q2PS_M_GRAVITY)
	{
		state->pmove.gravity = net_message.ReadShort();
	}

	if (flags & Q2PS_M_DELTA_ANGLES)
	{
		state->pmove.delta_angles[0] = net_message.ReadShort();
		state->pmove.delta_angles[1] = net_message.ReadShort();
		state->pmove.delta_angles[2] = net_message.ReadShort();
	}

	if (cl.q2_attractloop)
	{
		state->pmove.pm_type = Q2PM_FREEZE;		// demo playback

	}
	//
	// parse the rest of the q2player_state_t
	//
	if (flags & Q2PS_VIEWOFFSET)
	{
		state->viewoffset[0] = net_message.ReadChar() * 0.25;
		state->viewoffset[1] = net_message.ReadChar() * 0.25;
		state->viewoffset[2] = net_message.ReadChar() * 0.25;
	}

	if (flags & Q2PS_VIEWANGLES)
	{
		state->viewangles[0] = net_message.ReadAngle16();
		state->viewangles[1] = net_message.ReadAngle16();
		state->viewangles[2] = net_message.ReadAngle16();
	}

	if (flags & Q2PS_KICKANGLES)
	{
		state->kick_angles[0] = net_message.ReadChar() * 0.25;
		state->kick_angles[1] = net_message.ReadChar() * 0.25;
		state->kick_angles[2] = net_message.ReadChar() * 0.25;
	}

	if (flags & Q2PS_WEAPONINDEX)
	{
		state->gunindex = net_message.ReadByte();
	}

	if (flags & Q2PS_WEAPONFRAME)
	{
		state->gunframe = net_message.ReadByte();
		state->gunoffset[0] = net_message.ReadChar() * 0.25;
		state->gunoffset[1] = net_message.ReadChar() * 0.25;
		state->gunoffset[2] = net_message.ReadChar() * 0.25;
		state->gunangles[0] = net_message.ReadChar() * 0.25;
		state->gunangles[1] = net_message.ReadChar() * 0.25;
		state->gunangles[2] = net_message.ReadChar() * 0.25;
	}

	if (flags & Q2PS_BLEND)
	{
		state->blend[0] = net_message.ReadByte() / 255.0;
		state->blend[1] = net_message.ReadByte() / 255.0;
		state->blend[2] = net_message.ReadByte() / 255.0;
		state->blend[3] = net_message.ReadByte() / 255.0;
	}

	if (flags & Q2PS_FOV)
	{
		state->fov = net_message.ReadByte();
	}

	if (flags & Q2PS_RDFLAGS)
	{
		state->rdflags = net_message.ReadByte();
	}

	// parse stats
	statbits = net_message.ReadLong();
	for (i = 0; i < MAX_STATS_Q2; i++)
		if (statbits & (1 << i))
		{
			state->stats[i] = net_message.ReadShort();
		}
}


/*
==================
CL_FireEntityEvents

==================
*/
void CL_FireEntityEvents(q2frame_t* frame)
{
	q2entity_state_t* s1;
	int pnum, num;

	for (pnum = 0; pnum < frame->num_entities; pnum++)
	{
		num = (frame->parse_entities + pnum) & (MAX_PARSE_ENTITIES_Q2 - 1);
		s1 = &clq2_parse_entities[num];
		if (s1->event)
		{
			CLQ2_EntityEvent(s1);
		}

		// Q2EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & Q2EF_TELEPORTER)
		{
			CLQ2_TeleporterParticles(s1->origin);
		}
	}
}


/*
================
CL_ParseFrame
================
*/
void CL_ParseFrame(void)
{
	int cmd;
	int len;
	q2frame_t* old;

	Com_Memset(&cl.q2_frame, 0, sizeof(cl.q2_frame));

	cl.q2_frame.serverframe = net_message.ReadLong();
	cl.q2_frame.deltaframe = net_message.ReadLong();
	cl.q2_frame.servertime = cl.q2_frame.serverframe * 100;

	// BIG HACK to let old demos continue to work
	if (cls.q2_serverProtocol != 26)
	{
		cl.q2_surpressCount = net_message.ReadByte();
	}

	if (cl_shownet->value == 3)
	{
		common->Printf("   frame:%i  delta:%i\n", cl.q2_frame.serverframe,
			cl.q2_frame.deltaframe);
	}

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (cl.q2_frame.deltaframe <= 0)
	{
		cl.q2_frame.valid = true;		// uncompressed frame
		old = NULL;
		cls.q2_demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.q2_frames[cl.q2_frame.deltaframe & UPDATE_MASK_Q2];
		if (!old->valid)
		{	// should never happen
			common->Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.q2_frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			common->Printf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parse_entities > MAX_PARSE_ENTITIES_Q2 - 128)
		{
			common->Printf("Delta parse_entities too old.\n");
		}
		else
		{
			cl.q2_frame.valid = true;	// valid delta parse
		}
	}

	// clamp time
	if (cl.serverTime > cl.q2_frame.servertime)
	{
		cl.serverTime = cl.q2_frame.servertime;
	}
	else if (cl.serverTime < cl.q2_frame.servertime - 100)
	{
		cl.serverTime = cl.q2_frame.servertime - 100;
	}

	// read areabits
	len = net_message.ReadByte();
	net_message.ReadData(&cl.q2_frame.areabits, len);

	// read playerinfo
	cmd = net_message.ReadByte();
	SHOWNET(svc_strings[cmd]);
	if (cmd != q2svc_playerinfo)
	{
		common->Error("CL_ParseFrame: not playerinfo");
	}
	CL_ParsePlayerstate(old, &cl.q2_frame);

	// read packet entities
	cmd = net_message.ReadByte();
	SHOWNET(svc_strings[cmd]);
	if (cmd != q2svc_packetentities)
	{
		common->Error("CL_ParseFrame: not packetentities");
	}
	CL_ParsePacketEntities(old, &cl.q2_frame);

	// save the frame off in the backup array for later delta comparisons
	cl.q2_frames[cl.q2_frame.serverframe & UPDATE_MASK_Q2] = cl.q2_frame;

	if (cl.q2_frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != CA_ACTIVE)
		{
			cls.state = CA_ACTIVE;
			cl.q2_predicted_origin[0] = cl.q2_frame.playerstate.pmove.origin[0] * 0.125;
			cl.q2_predicted_origin[1] = cl.q2_frame.playerstate.pmove.origin[1] * 0.125;
			cl.q2_predicted_origin[2] = cl.q2_frame.playerstate.pmove.origin[2] * 0.125;
			VectorCopy(cl.q2_frame.playerstate.viewangles, cl.q2_predicted_angles);
			if (cls.q2_disable_servercount != cl.servercount &&
				cl.q2_refresh_prepped)
			{
				SCR_EndLoadingPlaque();		// get rid of loading plaque
			}
		}
		cl.q2_sound_prepped = true;	// can start mixing ambient sounds

		// fire entity events
		CL_FireEntityEvents(&cl.q2_frame);
		CL_CheckPredictionError();
	}
}

/*
===============
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
===============
*/
void CL_GetEntitySoundOrigin(int ent, vec3_t org)
{
	q2centity_t* old;

	if (ent < 0 || ent >= MAX_EDICTS_Q2)
	{
		common->Error("CL_GetEntitySoundOrigin: bad ent");
	}
	old = &clq2_entities[ent];
	VectorCopy(old->lerp_origin, org);

	// FIXME: bmodel issues...
}
