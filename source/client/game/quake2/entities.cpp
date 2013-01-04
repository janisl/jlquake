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

#include "../../client_main.h"
#include "../../public.h"
#include "../particles.h"
#include "local.h"

q2entity_state_t clq2_parse_entities[MAX_PARSE_ENTITIES_Q2];

//	Returns the entity number and the header bits
static int CLQ2_ParseEntityBits(QMsg& message, unsigned* bits)
{
	unsigned total = message.ReadByte();
	if (total & Q2U_MOREBITS1)
	{
		unsigned b = message.ReadByte();
		total |= b << 8;
	}
	if (total & Q2U_MOREBITS2)
	{
		unsigned b = message.ReadByte();
		total |= b << 16;
	}
	if (total & Q2U_MOREBITS3)
	{
		unsigned b = message.ReadByte();
		total |= b << 24;
	}

	// count the bits for net profiling
	for (int i = 0; i < 32; i++)
	{
		if (total & (1 << i))
		{
			bitcounts[i]++;
		}
	}

	int number;
	if (total & Q2U_NUMBER16)
	{
		number = message.ReadShort();
	}
	else
	{
		number = message.ReadByte();
	}

	*bits = total;

	return number;
}

//	Can go from either a baseline or a previous packet_entity
static void CLQ2_ParseDelta(QMsg& message, q2entity_state_t* from, q2entity_state_t* to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy(from->origin, to->old_origin);
	to->number = number;

	if (bits & Q2U_MODEL)
	{
		to->modelindex = message.ReadByte();
	}
	if (bits & Q2U_MODEL2)
	{
		to->modelindex2 = message.ReadByte();
	}
	if (bits & Q2U_MODEL3)
	{
		to->modelindex3 = message.ReadByte();
	}
	if (bits & Q2U_MODEL4)
	{
		to->modelindex4 = message.ReadByte();
	}

	if (bits & Q2U_FRAME8)
	{
		to->frame = message.ReadByte();
	}
	if (bits & Q2U_FRAME16)
	{
		to->frame = message.ReadShort();
	}

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
	{
		to->skinnum = message.ReadLong();
	}
	else if (bits & Q2U_SKIN8)
	{
		to->skinnum = message.ReadByte();
	}
	else if (bits & Q2U_SKIN16)
	{
		to->skinnum = message.ReadShort();
	}

	if ((bits & (Q2U_EFFECTS8 | Q2U_EFFECTS16)) == (Q2U_EFFECTS8 | Q2U_EFFECTS16))
	{
		to->effects = message.ReadLong();
	}
	else if (bits & Q2U_EFFECTS8)
	{
		to->effects = message.ReadByte();
	}
	else if (bits & Q2U_EFFECTS16)
	{
		to->effects = message.ReadShort();
	}

	if ((bits & (Q2U_RENDERFX8 | Q2U_RENDERFX16)) == (Q2U_RENDERFX8 | Q2U_RENDERFX16))
	{
		to->renderfx = message.ReadLong();
	}
	else if (bits & Q2U_RENDERFX8)
	{
		to->renderfx = message.ReadByte();
	}
	else if (bits & Q2U_RENDERFX16)
	{
		to->renderfx = message.ReadShort();
	}

	if (bits & Q2U_ORIGIN1)
	{
		to->origin[0] = message.ReadCoord();
	}
	if (bits & Q2U_ORIGIN2)
	{
		to->origin[1] = message.ReadCoord();
	}
	if (bits & Q2U_ORIGIN3)
	{
		to->origin[2] = message.ReadCoord();
	}

	if (bits & Q2U_ANGLE1)
	{
		to->angles[0] = message.ReadAngle();
	}
	if (bits & Q2U_ANGLE2)
	{
		to->angles[1] = message.ReadAngle();
	}
	if (bits & Q2U_ANGLE3)
	{
		to->angles[2] = message.ReadAngle();
	}

	if (bits & Q2U_OLDORIGIN)
	{
		message.ReadPos(to->old_origin);
	}

	if (bits & Q2U_SOUND)
	{
		to->sound = message.ReadByte();
	}

	if (bits & Q2U_EVENT)
	{
		to->event = message.ReadByte();
	}
	else
	{
		to->event = 0;
	}

	if (bits & Q2U_SOLID)
	{
		to->solid = message.ReadShort();
	}
}

void CLQ2_ParseBaseline(QMsg& message)
{
	q2entity_state_t nullstate;
	Com_Memset(&nullstate, 0, sizeof(nullstate));

	unsigned int bits;
	int newnum = CLQ2_ParseEntityBits(message, &bits);
	q2entity_state_t* es = &clq2_entities[newnum].baseline;
	CLQ2_ParseDelta(message, &nullstate, es, newnum, bits);
}

static void CLQ2_ParsePlayerstate(QMsg& message, q2frame_t* oldframe, q2frame_t* newframe)
{
	q2player_state_t* state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
	{
		*state = oldframe->playerstate;
	}
	else
	{
		Com_Memset(state, 0, sizeof(*state));
	}

	int flags = message.ReadShort();

	//
	// parse the q2pmove_state_t
	//
	if (flags & Q2PS_M_TYPE)
	{
		state->pmove.pm_type = (q2pmtype_t)message.ReadByte();
	}

	if (flags & Q2PS_M_ORIGIN)
	{
		state->pmove.origin[0] = message.ReadShort();
		state->pmove.origin[1] = message.ReadShort();
		state->pmove.origin[2] = message.ReadShort();
	}

	if (flags & Q2PS_M_VELOCITY)
	{
		state->pmove.velocity[0] = message.ReadShort();
		state->pmove.velocity[1] = message.ReadShort();
		state->pmove.velocity[2] = message.ReadShort();
	}

	if (flags & Q2PS_M_TIME)
	{
		state->pmove.pm_time = message.ReadByte();
	}

	if (flags & Q2PS_M_FLAGS)
	{
		state->pmove.pm_flags = message.ReadByte();
	}

	if (flags & Q2PS_M_GRAVITY)
	{
		state->pmove.gravity = message.ReadShort();
	}

	if (flags & Q2PS_M_DELTA_ANGLES)
	{
		state->pmove.delta_angles[0] = message.ReadShort();
		state->pmove.delta_angles[1] = message.ReadShort();
		state->pmove.delta_angles[2] = message.ReadShort();
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
		state->viewoffset[0] = message.ReadChar() * 0.25;
		state->viewoffset[1] = message.ReadChar() * 0.25;
		state->viewoffset[2] = message.ReadChar() * 0.25;
	}

	if (flags & Q2PS_VIEWANGLES)
	{
		state->viewangles[0] = message.ReadAngle16();
		state->viewangles[1] = message.ReadAngle16();
		state->viewangles[2] = message.ReadAngle16();
	}

	if (flags & Q2PS_KICKANGLES)
	{
		state->kick_angles[0] = message.ReadChar() * 0.25;
		state->kick_angles[1] = message.ReadChar() * 0.25;
		state->kick_angles[2] = message.ReadChar() * 0.25;
	}

	if (flags & Q2PS_WEAPONINDEX)
	{
		state->gunindex = message.ReadByte();
	}

	if (flags & Q2PS_WEAPONFRAME)
	{
		state->gunframe = message.ReadByte();
		state->gunoffset[0] = message.ReadChar() * 0.25;
		state->gunoffset[1] = message.ReadChar() * 0.25;
		state->gunoffset[2] = message.ReadChar() * 0.25;
		state->gunangles[0] = message.ReadChar() * 0.25;
		state->gunangles[1] = message.ReadChar() * 0.25;
		state->gunangles[2] = message.ReadChar() * 0.25;
	}

	if (flags & Q2PS_BLEND)
	{
		state->blend[0] = message.ReadByte() / 255.0;
		state->blend[1] = message.ReadByte() / 255.0;
		state->blend[2] = message.ReadByte() / 255.0;
		state->blend[3] = message.ReadByte() / 255.0;
	}

	if (flags & Q2PS_FOV)
	{
		state->fov = message.ReadByte();
	}

	if (flags & Q2PS_RDFLAGS)
	{
		state->rdflags = message.ReadByte();
	}

	// parse stats
	int statbits = message.ReadLong();
	for (int i = 0; i < MAX_STATS_Q2; i++)
	{
		if (statbits & (1 << i))
		{
			state->stats[i] = message.ReadShort();
		}
	}
}

//	Parses deltas from the given base and adds the resulting entity
// to the current frame
static void CLQ2_DeltaEntity(QMsg& message, q2frame_t* frame, int newnum, q2entity_state_t* old, int bits)
{
	q2centity_t* ent = &clq2_entities[newnum];

	q2entity_state_t* state = &clq2_parse_entities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q2 - 1)];
	cl.parseEntitiesNum++;
	frame->num_entities++;

	CLQ2_ParseDelta(message, old, state, newnum, bits);

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

//	An q2svc_packetentities has just been parsed, deal with the
// rest of the data stream.
static void CLQ2_ParsePacketEntities(QMsg& message, q2frame_t* oldframe, q2frame_t* newframe)
{
	newframe->parse_entities = cl.parseEntitiesNum;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	int oldindex = 0;
	int oldnum;
	q2entity_state_t* oldstate;
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
		unsigned int bits;
		int newnum = CLQ2_ParseEntityBits(message, &bits);
		if (newnum >= MAX_EDICTS_Q2)
		{
			common->Error("CLQ2_ParsePacketEntities: bad number:%i", newnum);
		}

		if (message.readcount > message.cursize)
		{
			common->Error("CLQ2_ParsePacketEntities: end of message");
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
			CLQ2_DeltaEntity(message, newframe, oldnum, oldstate, 0);

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
			CLQ2_DeltaEntity(message, newframe, newnum, oldstate, bits);

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
			CLQ2_DeltaEntity(message, newframe, newnum, &clq2_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
		{
			common->Printf("   unchanged: %i\n", oldnum);
		}
		CLQ2_DeltaEntity(message, newframe, oldnum, oldstate, 0);

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

static void CLQ2_FireEntityEvents(q2frame_t* frame)
{
	for (int pnum = 0; pnum < frame->num_entities; pnum++)
	{
		int num = (frame->parse_entities + pnum) & (MAX_PARSE_ENTITIES_Q2 - 1);
		q2entity_state_t* s1 = &clq2_parse_entities[num];
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

void CLQ2_ParseFrame(QMsg& message)
{
	Com_Memset(&cl.q2_frame, 0, sizeof(cl.q2_frame));

	cl.q2_frame.serverframe = message.ReadLong();
	cl.q2_frame.deltaframe = message.ReadLong();
	cl.q2_frame.servertime = cl.q2_frame.serverframe * 100;

	// BIG HACK to let old demos continue to work
	if (cls.q2_serverProtocol != 26)
	{
		cl.q2_surpressCount = message.ReadByte();
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
	q2frame_t* old;
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
	int len = message.ReadByte();
	message.ReadData(&cl.q2_frame.areabits, len);

	// read playerinfo
	int cmd = message.ReadByte();
	SHOWNET(message, svcq2_strings[cmd]);
	if (cmd != q2svc_playerinfo)
	{
		common->Error("CLQ2_ParseFrame: not playerinfo");
	}
	CLQ2_ParsePlayerstate(message, old, &cl.q2_frame);

	// read packet entities
	cmd = message.ReadByte();
	SHOWNET(message, svcq2_strings[cmd]);
	if (cmd != q2svc_packetentities)
	{
		common->Error("CLQ2_ParseFrame: not packetentities");
	}
	CLQ2_ParsePacketEntities(message, old, &cl.q2_frame);

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
		CLQ2_FireEntityEvents(&cl.q2_frame);
		CLQ2_CheckPredictionError();
	}
}

void CLQ2_AddPacketEntities(q2frame_t* frame)
{
	// bonus items rotate at a fixed rate
	float autorotate = AngleMod(cl.serverTime / 10);

	// brush models can auto animate their frames
	int autoanim = 2 * cl.serverTime / 1000;

	for (int pnum = 0; pnum < frame->num_entities; pnum++)
	{
		q2entity_state_t* s1 = &clq2_parse_entities[(frame->parse_entities + pnum) & (MAX_PARSE_ENTITIES_Q2 - 1)];

		q2centity_t* cent = &clq2_entities[s1->number];

		unsigned int effects = s1->effects;
		unsigned int renderfx_old = s1->renderfx;

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		//	Convert Quake 2 render flags to new ones.
		unsigned int renderfx = 0;
		if (renderfx_old & Q2RF_MINLIGHT)
		{
			renderfx |= RF_MINLIGHT;
		}
		if (renderfx_old & Q2RF_VIEWERMODEL)
		{
			renderfx |= RF_THIRD_PERSON;
		}
		if (renderfx_old & Q2RF_WEAPONMODEL)
		{
			renderfx |= RF_FIRST_PERSON;
		}
		if (renderfx_old & Q2RF_DEPTHHACK)
		{
			renderfx |= RF_DEPTHHACK;
		}
		if (renderfx_old & Q2RF_TRANSLUCENT)
		{
			renderfx |= RF_TRANSLUCENT;
		}
		if (renderfx_old & Q2RF_GLOW)
		{
			renderfx |= RF_GLOW;
		}
		if (renderfx_old & Q2RF_IR_VISIBLE)
		{
			renderfx |= RF_IR_VISIBLE;
		}

		// set frame
		if (effects & Q2EF_ANIM01)
		{
			ent.frame = autoanim & 1;
		}
		else if (effects & Q2EF_ANIM23)
		{
			ent.frame = 2 + (autoanim & 1);
		}
		else if (effects & Q2EF_ANIM_ALL)
		{
			ent.frame = autoanim;
		}
		else if (effects & Q2EF_ANIM_ALLFAST)
		{
			ent.frame = cl.serverTime / 100;
		}
		else
		{
			ent.frame = s1->frame;
		}

		// quad and pent can do different things on client
		if (effects & Q2EF_PENT)
		{
			effects &= ~Q2EF_PENT;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_RED;
		}

		if (effects & Q2EF_QUAD)
		{
			effects &= ~Q2EF_QUAD;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_BLUE;
		}
//======
// PMM
		if (effects & Q2EF_DOUBLE)
		{
			effects &= ~Q2EF_DOUBLE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_DOUBLE;
		}

		if (effects & Q2EF_HALF_DAMAGE)
		{
			effects &= ~Q2EF_HALF_DAMAGE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_HALF_DAM;
		}
// pmm
//======
		ent.oldframe = cent->prev.frame;
		ent.backlerp = 1.0 - cl.q2_lerpfrac;

		if (renderfx_old & (Q2RF_FRAMELERP | Q2RF_BEAM))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy(cent->current.origin, ent.origin);
			VectorCopy(cent->current.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (int i = 0; i < 3; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.q2_lerpfrac *
												   (cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// create a new entity

		// tweak the color of beams
		if (renderfx_old & Q2RF_BEAM)
		{
			// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.reType = RT_BEAM;
			ent.shaderRGBA[3] = 76;
			ent.skinNum = (s1->skinnum >> ((rand() % 4) * 8)) & 0xff;
			ent.hModel = 0;
		}
		else
		{
			// set skin
			if (s1->modelindex == 255)
			{	// use custom player skin
				ent.skinNum = 0;
				q2clientinfo_t* ci = &cl.q2_clientinfo[s1->skinnum & 0xff];
				ent.customSkin = R_GetImageHandle(ci->skin);
				ent.hModel = ci->model;
				if (!ent.customSkin || !ent.hModel)
				{
					ent.customSkin = R_GetImageHandle(cl.q2_baseclientinfo.skin);
					ent.hModel = cl.q2_baseclientinfo.model;
				}

//============
//PGM
				if (renderfx_old & Q2RF_USE_DISGUISE)
				{
					if (!String::NCmp(R_GetImageName(ent.customSkin), "players/male", 12))
					{
						ent.customSkin = R_GetImageHandle(R_RegisterSkinQ2("players/male/disguise.pcx"));
						ent.hModel = R_RegisterModel("players/male/tris.md2");
					}
					else if (!String::NCmp(R_GetImageName(ent.customSkin), "players/female", 14))
					{
						ent.customSkin = R_GetImageHandle(R_RegisterSkinQ2("players/female/disguise.pcx"));
						ent.hModel = R_RegisterModel("players/female/tris.md2");
					}
					else if (!String::NCmp(R_GetImageName(ent.customSkin), "players/cyborg", 14))
					{
						ent.customSkin = R_GetImageHandle(R_RegisterSkinQ2("players/cyborg/disguise.pcx"));
						ent.hModel = R_RegisterModel("players/cyborg/tris.md2");
					}
				}
//PGM
//============
			}
			else
			{
				ent.skinNum = s1->skinnum;
				ent.hModel = cl.model_draw[s1->modelindex];
			}
		}

		// only used for black hole model right now, FIXME: do better
		if (renderfx_old == Q2RF_TRANSLUCENT)
		{
			ent.shaderRGBA[3] = 178;
		}

		// render effects (fullbright, translucent, etc)
		if ((effects & Q2EF_COLOR_SHELL))
		{
			// renderfx go on color shell entity
			ent.renderfx = 0;
		}
		else
		{
			ent.renderfx = renderfx;
		}

		// calculate angles
		vec3_t angles;
		if (effects & Q2EF_ROTATE)
		{	// some bonus items auto-rotate
			angles[0] = 0;
			angles[1] = autorotate;
			angles[2] = 0;
		}
		// RAFAEL
		else if (effects & Q2EF_SPINNINGLIGHTS)
		{
			angles[0] = 0;
			angles[1] = AngleMod(cl.serverTime / 2) + s1->angles[1];
			angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors(angles, forward, NULL, NULL);
				VectorMA(ent.origin, 64, forward, start);
				R_AddLightToScene(start, 100, 1, 0, 0);
			}
		}
		else
		{	// interpolate angles
			for (int i = 0; i < 3; i++)
			{
				float a1 = cent->current.angles[i];
				float a2 = cent->prev.angles[i];
				angles[i] = LerpAngle(a2, a1, cl.q2_lerpfrac);
			}
		}
		AnglesToAxis(angles, ent.axis);

		if (s1->number == cl.playernum + 1)
		{
			ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
			// FIXME: still pass to refresh

			if (effects & Q2EF_FLAG1)
			{
				R_AddLightToScene(ent.origin, 225, 1.0, 0.1, 0.1);
			}
			else if (effects & Q2EF_FLAG2)
			{
				R_AddLightToScene(ent.origin, 225, 0.1, 0.1, 1.0);
			}
			else if (effects & Q2EF_TAGTRAIL)						//PGM
			{
				R_AddLightToScene(ent.origin, 225, 1.0, 1.0, 0.0);	//PGM
			}
			else if (effects & Q2EF_TRACKERTRAIL)					//PGM
			{
				R_AddLightToScene(ent.origin, 225, -1.0, -1.0, -1.0);	//PGM

			}
			continue;
		}

		// if set to invisible, skip
		if (!s1->modelindex)
		{
			continue;
		}

		if (effects & Q2EF_BFG)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			ent.shaderRGBA[3] = 76;
		}

		// RAFAEL
		if (effects & Q2EF_PLASMA)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			ent.shaderRGBA[3] = 153;
		}

		if (effects & Q2EF_SPHERETRANS)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if (effects & Q2EF_TRACKERTRAIL)
			{
				ent.shaderRGBA[3] = 153;
			}
			else
			{
				ent.shaderRGBA[3] = 76;
			}
		}
//pmm

		// add to refresh list
		R_AddRefEntityToScene(&ent);

		// color shells generate a seperate entity for the main model
		if (effects & Q2EF_COLOR_SHELL)
		{
			ent.renderfx = renderfx | RF_TRANSLUCENT | RF_COLOUR_SHELL;
			// PMM - rewrote, reordered to handle new shells & mixing
			// PMM -special case for godmode
			if ((renderfx_old & Q2RF_SHELL_RED) &&
				(renderfx_old & Q2RF_SHELL_BLUE) &&
				(renderfx_old & Q2RF_SHELL_GREEN))
			{
				ent.shaderRGBA[0] = 255;
				ent.shaderRGBA[1] = 255;
				ent.shaderRGBA[2] = 255;
			}
			else if (renderfx_old & (Q2RF_SHELL_RED | Q2RF_SHELL_BLUE | Q2RF_SHELL_DOUBLE))
			{
				ent.shaderRGBA[0] = 0;
				ent.shaderRGBA[1] = 0;
				ent.shaderRGBA[2] = 0;

				if (renderfx_old & Q2RF_SHELL_RED)
				{
					ent.shaderRGBA[0] = 255;
					if (renderfx_old & (Q2RF_SHELL_BLUE | Q2RF_SHELL_DOUBLE))
					{
						ent.shaderRGBA[2] = 255;
					}
				}
				else if (renderfx_old & Q2RF_SHELL_BLUE)
				{
					if (renderfx_old & Q2RF_SHELL_DOUBLE)
					{
						ent.shaderRGBA[1] = 255;
						ent.shaderRGBA[2] = 255;
					}
					else
					{
						ent.shaderRGBA[2] = 255;
					}
				}
				else if (renderfx_old & Q2RF_SHELL_DOUBLE)
				{
					ent.shaderRGBA[0] = 230;
					ent.shaderRGBA[1] = 178;
				}
			}
			else if (renderfx_old & (Q2RF_SHELL_HALF_DAM | Q2RF_SHELL_GREEN))
			{
				ent.shaderRGBA[0] = 0;
				ent.shaderRGBA[1] = 0;
				ent.shaderRGBA[2] = 0;
				// PMM - new colors
				if (renderfx_old & Q2RF_SHELL_HALF_DAM)
				{
					ent.shaderRGBA[0] = 143;
					ent.shaderRGBA[1] = 150;
					ent.shaderRGBA[2] = 115;
				}
				if (renderfx_old & Q2RF_SHELL_GREEN)
				{
					ent.shaderRGBA[1] = 255;
				}
			}
			ent.shaderRGBA[3] = 76;
			R_AddRefEntityToScene(&ent);
		}

		ent.skinNum = 0;
		ent.renderfx = 0;
		ent.shaderRGBA[3] = 0;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				q2clientinfo_t* ci = &cl.q2_clientinfo[s1->skinnum & 0xff];
				int i = (s1->skinnum >> 8);	// 0 is default weapon model
				if (!clq2_vwep->value || i > MAX_CLIENTWEAPONMODELS_Q2 - 1)
				{
					i = 0;
				}
				ent.hModel = ci->weaponmodel[i];
				if (!ent.hModel)
				{
					if (i != 0)
					{
						ent.hModel = ci->weaponmodel[0];
					}
					if (!ent.hModel)
					{
						ent.hModel = cl.q2_baseclientinfo.weaponmodel[0];
					}
				}
			}
			//PGM - hack to allow translucent linked models (defender sphere's shell)
			//		set the high bit 0x80 on modelindex2 to enable translucency
			else if (s1->modelindex2 & 0x80)
			{
				ent.hModel = cl.model_draw[s1->modelindex2 & 0x7F];
				ent.shaderRGBA[3] = 82;
				ent.renderfx = RF_TRANSLUCENT;
			}
			//PGM
			else
			{
				ent.hModel = cl.model_draw[s1->modelindex2];
			}
			R_AddRefEntityToScene(&ent);

			//PGM - make sure these get reset.
			ent.renderfx = 0;
			ent.shaderRGBA[3] = 0;
			//PGM
		}
		if (s1->modelindex3)
		{
			ent.hModel = cl.model_draw[s1->modelindex3];
			R_AddRefEntityToScene(&ent);
		}
		if (s1->modelindex4)
		{
			ent.hModel = cl.model_draw[s1->modelindex4];
			R_AddRefEntityToScene(&ent);
		}

		if (effects & Q2EF_POWERSCREEN)
		{
			ent.hModel = clq2_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.renderfx |= RF_TRANSLUCENT | RF_COLOUR_SHELL;
			ent.shaderRGBA[0] = 0;
			ent.shaderRGBA[1] = 255;
			ent.shaderRGBA[2] = 0;
			ent.shaderRGBA[3] = 76;
			R_AddRefEntityToScene(&ent);
		}

		// add automatic particle trails
		if ((effects & ~Q2EF_ROTATE))
		{
			if (effects & Q2EF_ROCKET)
			{
				CLQ2_RocketTrail(cent->lerp_origin, ent.origin, cent);
				R_AddLightToScene(ent.origin, 200, 1, 1, 0);
			}
			// PGM - Do not reorder Q2EF_BLASTER and Q2EF_HYPERBLASTER.
			// Q2EF_BLASTER | Q2EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & Q2EF_BLASTER)
			{
//				CLQ2_BlasterTrail (cent->lerp_origin, ent.origin);
//PGM
				if (effects & Q2EF_TRACKER)	// lame... problematic?
				{
					CLQ2_BlasterTrail2(cent->lerp_origin, ent.origin);
					R_AddLightToScene(ent.origin, 200, 0, 1, 0);
				}
				else
				{
					CLQ2_BlasterTrail(cent->lerp_origin, ent.origin);
					R_AddLightToScene(ent.origin, 200, 1, 1, 0);
				}
//PGM
			}
			else if (effects & Q2EF_HYPERBLASTER)
			{
				if (effects & Q2EF_TRACKER)						// PGM	overloaded for blaster2.
				{
					R_AddLightToScene(ent.origin, 200, 0, 1, 0);		// PGM
				}
				else											// PGM
				{
					R_AddLightToScene(ent.origin, 200, 1, 1, 0);
				}
			}
			else if (effects & Q2EF_GIB)
			{
				CLQ2_DiminishingTrail(cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & Q2EF_GRENADE)
			{
				CLQ2_DiminishingTrail(cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & Q2EF_FLIES)
			{
				CLQ2_FlyEffect(cent, ent.origin);
			}
			else if (effects & Q2EF_BFG)
			{
				static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

				int i;
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BfgParticles(ent.origin);
					i = 200;
				}
				else
				{
					i = bfg_lightramp[s1->frame];
				}
				R_AddLightToScene(ent.origin, i, 0, 1, 0);
			}
			// RAFAEL
			else if (effects & Q2EF_TRAP)
			{
				ent.origin[2] += 32;
				CLQ2_TrapParticles(ent.origin);
				int i = (rand() % 100) + 100;
				R_AddLightToScene(ent.origin, i, 1, 0.8, 0.1);
			}
			else if (effects & Q2EF_FLAG1)
			{
				CLQ2_FlagTrail(cent->lerp_origin, ent.origin, 242);
				R_AddLightToScene(ent.origin, 225, 1, 0.1, 0.1);
			}
			else if (effects & Q2EF_FLAG2)
			{
				CLQ2_FlagTrail(cent->lerp_origin, ent.origin, 115);
				R_AddLightToScene(ent.origin, 225, 0.1, 0.1, 1);
			}
//======
//ROGUE
			else if (effects & Q2EF_TAGTRAIL)
			{
				CLQ2_TagTrail(cent->lerp_origin, ent.origin, 220);
				R_AddLightToScene(ent.origin, 225, 1.0, 1.0, 0.0);
			}
			else if (effects & Q2EF_TRACKERTRAIL)
			{
				if (effects & Q2EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.serverTime / 500.0) + 1.0));
					R_AddLightToScene(ent.origin, intensity, -1.0, -1.0, -1.0);
				}
				else
				{
					CLQ2_Tracker_Shell(cent->lerp_origin);
					R_AddLightToScene(ent.origin, 155, -1.0, -1.0, -1.0);
				}
			}
			else if (effects & Q2EF_TRACKER)
			{
				CLQ2_TrackerTrail(cent->lerp_origin, ent.origin, 0);
				R_AddLightToScene(ent.origin, 200, -1, -1, -1);
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & Q2EF_GREENGIB)
			{
				CLQ2_DiminishingTrail(cent->lerp_origin, ent.origin, cent, effects);
			}
			// RAFAEL
			else if (effects & Q2EF_IONRIPPER)
			{
				CLQ2_IonripperTrail(cent->lerp_origin, ent.origin);
				R_AddLightToScene(ent.origin, 100, 1, 0.5, 0.5);
			}
			// RAFAEL
			else if (effects & Q2EF_BLUEHYPERBLASTER)
			{
				R_AddLightToScene(ent.origin, 200, 0, 0, 1);
			}
			// RAFAEL
			else if (effects & Q2EF_PLASMA)
			{
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BlasterTrail(cent->lerp_origin, ent.origin);
				}
				R_AddLightToScene(ent.origin, 130, 1, 0.5, 0.5);
			}
		}

		VectorCopy(ent.origin, cent->lerp_origin);
	}
}

//	Called to get the sound spatialization origin
void CLQ2_GetEntitySoundOrigin(int ent, vec3_t org)
{
	if (ent < 0 || ent >= MAX_EDICTS_Q2)
	{
		common->Error("CLQ2_GetEntitySoundOrigin: bad ent");
	}
	q2centity_t* old = &clq2_entities[ent];
	VectorCopy(old->lerp_origin, org);

	// FIXME: bmodel issues...
}
