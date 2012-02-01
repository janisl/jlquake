//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "../../../core/file_formats/spr.h"
#include "local.h"

Cvar* cl_doubleeyes;

q1entity_state_t clq1_baselines[MAX_EDICTS_Q1];
q1entity_t clq1_entities[MAX_EDICTS_Q1];
q1entity_t clq1_static_entities[MAX_STATIC_ENTITIES_Q1];

image_t* clq1_playertextures[BIGGEST_MAX_CLIENTS_Q1];	// color translated skins

int clq1_playerindex;

//	This error checks and tracks the total number of entities
q1entity_t* CLQ1_EntityNum(int number)
{
	if (number >= cl.qh_num_entities)
	{
		if (number >= MAX_EDICTS_Q1)
		{
			throw DropException(va("CLQ1_EntityNum: %i is an invalid number", number));
		}
		while (cl.qh_num_entities <= number)
		{
			clq1_entities[cl.qh_num_entities].state.colormap = 0;
			cl.qh_num_entities++;
		}
	}
	return &clq1_entities[number];
}

static void CLQ1_ParseBaseline(QMsg& message, q1entity_state_t *es)
{
	es->modelindex = message.ReadByte();
	es->frame = message.ReadByte();
	es->colormap = message.ReadByte();
	es->skinnum = message.ReadByte();
	for (int i = 0; i < 3; i++)
	{
		es->origin[i] = message.ReadCoord();
		es->angles[i] = message.ReadAngle();
	}
}

void CLQ1_ParseSpawnBaseline(QMsg& message)
{
	int i = message.ReadShort();
	if (!(GGameType & GAME_QuakeWorld))
	{
		// must use CLQ1_EntityNum() to force cl.num_entities up
		CLQ1_EntityNum(i);
	}
	CLQ1_ParseBaseline(message, &clq1_baselines[i]);
}

void CLQ1_ParseSpawnStatic(QMsg& message)
{
	int i = cl.qh_num_statics;
	if (i >= MAX_STATIC_ENTITIES_Q1)
	{
		throw DropException("Too many static entities");
	}
	q1entity_t* ent = &clq1_static_entities[i];
	cl.qh_num_statics++;

	CLQ1_ParseBaseline(message, &ent->state);
	ent->state.colormap = 0;
}

//	Parse an entity update message from the server
//	If an entities model or origin changes from frame to frame, it must be
// relinked.  Other attributes can change without relinking.
void CLQ1_ParseUpdate(QMsg& message, int bits)
{
	if (clc.qh_signon == SIGNONS - 1)
	{
		// first update is the final signon stage
		clc.qh_signon = SIGNONS;
		CLQ1_SignonReply ();
	}

	if (bits & Q1U_MOREBITS)
	{
		int i = message.ReadByte();
		bits |= (i << 8);
	}

	int num;
	if (bits & Q1U_LONGENTITY)
	{
		num = message.ReadShort();
	}
	else
	{
		num = message.ReadByte();
	}

	q1entity_t* ent = CLQ1_EntityNum(num);
	const q1entity_state_t& baseline = clq1_baselines[num];

	for (int i = 0; i < 32; i++)
	{
		if (bits & (1 << i))
		{
			bitcounts[i]++;
		}
	}

	bool forcelink = ent->msgtime != cl.qh_mtime[1];	// no previous frame to lerp from

	ent->msgtime = cl.qh_mtime[0];

	int modnum;
	if (bits & Q1U_MODEL)
	{
		modnum = message.ReadByte();
		if (modnum >= MAX_MODELS_Q1)
		{
			throw DropException("CL_ParseModel: bad modnum");
		}
	}
	else
	{
		modnum = baseline.modelindex;
	}

	qhandle_t model = cl.model_draw[modnum];
	if (modnum != ent->state.modelindex)
	{
		ent->state.modelindex = modnum;
		// automatic animation (torches, etc) can be either all together
		// or randomized
		if (model)
		{
			if (R_ModelSyncType(model) == ST_RAND)
			{
				ent->syncbase = (float)(rand() & 0x7fff) / 0x7fff;
			}
			else
			{
				ent->syncbase = 0.0;
			}
		}
		else
		{
			forcelink = true;	// hack to make null model players work
		}
		if (num > 0 && num <= cl.qh_maxclients)
		{
			CLQ1_TranslatePlayerSkin(num - 1);
		}
	}
	
	if (bits & Q1U_FRAME)
	{
		ent->state.frame = message.ReadByte();
	}
	else
	{
		ent->state.frame = baseline.frame;
	}

	if (bits & Q1U_COLORMAP)
	{
		ent->state.colormap = message.ReadByte();
	}
	else
	{
		ent->state.colormap = baseline.colormap;
	}
	if (ent->state.colormap > cl.qh_maxclients)
	{
		throw Exception("i >= cl.maxclients");
	}

	int skin;
	if (bits & Q1U_SKIN)
	{
		skin = message.ReadByte();
	}
	else
	{
		skin = baseline.skinnum;
	}
	if (skin != ent->state.skinnum)
	{
		ent->state.skinnum = skin;
		if (num > 0 && num <= cl.qh_maxclients)
		{
			CLQ1_TranslatePlayerSkin(num - 1);
		}
	}

	if (bits & Q1U_EFFECTS)
	{
		ent->state.effects = message.ReadByte();
	}
	else
	{
		ent->state.effects = baseline.effects;
	}

	// shift the known values for interpolation
	VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
	VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);

	if (bits & Q1U_ORIGIN1)
	{
		ent->msg_origins[0][0] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][0] = baseline.origin[0];
	}
	if (bits & Q1U_ANGLE1)
	{
		ent->msg_angles[0][0] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][0] = baseline.angles[0];
	}

	if (bits & Q1U_ORIGIN2)
	{
		ent->msg_origins[0][1] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][1] = baseline.origin[1];
	}
	if (bits & Q1U_ANGLE2)
	{
		ent->msg_angles[0][1] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][1] = baseline.angles[1];
	}

	if (bits & Q1U_ORIGIN3)
	{
		ent->msg_origins[0][2] = message.ReadCoord();
	}
	else
	{
		ent->msg_origins[0][2] = baseline.origin[2];
	}
	if (bits & Q1U_ANGLE3)
	{
		ent->msg_angles[0][2] = message.ReadAngle();
	}
	else
	{
		ent->msg_angles[0][2] = baseline.angles[2];
	}

	if (bits & Q1U_NOLERP)
	{
		forcelink = true;
	}

	if (forcelink)
	{
		// didn't have an update last message
		VectorCopy(ent->msg_origins[0], ent->msg_origins[1]);
		VectorCopy(ent->msg_origins[0], ent->state.origin);
		VectorCopy(ent->msg_angles[0], ent->msg_angles[1]);
		VectorCopy(ent->msg_angles[0], ent->state.angles);
	}
}

//	Can go from either a baseline or a previous packet_entity
static void CLQW_ParseDelta(QMsg& message, q1entity_state_t* from, q1entity_state_t* to, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	to->number = bits & 511;
	bits &= ~511;

	if (bits & QWU_MOREBITS)
	{
		// read in the low order bits
		bits |= message.ReadByte();
	}

	// count the bits for net profiling
	for (int i = 0; i < 32; i++)
	{
		if (bits & (1 << i))
		{
			bitcounts[i]++;
		}
	}

	to->flags = bits;

	if (bits & QWU_MODEL)
	{
		to->modelindex = message.ReadByte();
	}
		
	if (bits & QWU_FRAME)
	{
		to->frame = message.ReadByte();
	}

	if (bits & QWU_COLORMAP)
	{
		to->colormap = message.ReadByte();
	}

	if (bits & QWU_SKIN)
	{
		to->skinnum = message.ReadByte();
	}

	if (bits & QWU_EFFECTS)
	{
		to->effects = message.ReadByte();
	}

	if (bits & QWU_ORIGIN1)
	{
		to->origin[0] = message.ReadCoord();
	}
		
	if (bits & QWU_ANGLE1)
	{
		to->angles[0] = message.ReadAngle();
	}

	if (bits & QWU_ORIGIN2)
	{
		to->origin[1] = message.ReadCoord();
	}
		
	if (bits & QWU_ANGLE2)
	{
		to->angles[1] = message.ReadAngle();
	}

	if (bits & QWU_ORIGIN3)
	{
		to->origin[2] = message.ReadCoord();
	}
		
	if (bits & QWU_ANGLE3)
	{
		to->angles[2] = message.ReadAngle();
	}

	if (bits & QWU_SOLID)
	{
		// FIXME
	}
}

static void CLQW_FlushEntityPacket(QMsg& message)
{
	Log::develWrite("CLQW_FlushEntityPacket\n");

	q1entity_state_t olde;
	Com_Memset(&olde, 0, sizeof(olde));

	cl.qh_validsequence = 0;		// can't render a frame
	cl.qw_frames[clc.netchan.incomingSequence&UPDATE_MASK_QW].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		int word = (unsigned short)message.ReadShort();
		if (message.badread)
		{
			// something didn't parse right...
			throw DropException("msg_badread in packetentities");
			return;
		}

		if (!word)
			break;	// done

		q1entity_state_t newe;
		CLQW_ParseDelta (message, &olde, &newe, word);
	}
}

//	An qwsvc_packetentities has just been parsed, deal with the
// rest of the data stream.
static void CLQW_ParsePacketEntities(QMsg& message, bool delta)
{
	int newpacket = clc.netchan.incomingSequence & UPDATE_MASK_QW;
	qwpacket_entities_t* newp = &cl.qw_frames[newpacket].packet_entities;
	cl.qw_frames[newpacket].invalid = false;

	int oldpacket;
	if (delta)
	{
		byte from = message.ReadByte();

		oldpacket = cl.qw_frames[newpacket].delta_sequence;

		if ((from & UPDATE_MASK_QW) != (oldpacket & UPDATE_MASK_QW))
		{
			Log::develWrite("WARNING: from mismatch\n");
		}
	}
	else
	{
		oldpacket = -1;
	}

	bool full = false;
	qwpacket_entities_t* oldp;
	qwpacket_entities_t dummy;
	if (oldpacket != -1)
	{
		if (clc.netchan.outgoingSequence - oldpacket >= UPDATE_BACKUP_QW - 1)
		{
			// we can't use this, it is too old
			CLQW_FlushEntityPacket(message);
			return;
		}
		cl.qh_validsequence = clc.netchan.incomingSequence;
		oldp = &cl.qw_frames[oldpacket & UPDATE_MASK_QW].packet_entities;
	}
	else
	{
		// this is a full update that we can start delta compressing from now
		oldp = &dummy;
		dummy.num_entities = 0;
		cl.qh_validsequence = clc.netchan.incomingSequence;
		full = true;
	}

	int oldindex = 0;
	int newindex = 0;
	newp->num_entities = 0;

	while (1)
	{
		int word = (unsigned short)message.ReadShort();
		if (message.badread)
		{
			// something didn't parse right...
			throw DropException("msg_badread in packetentities");
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{
				// copy all the rest of the entities from the old packet
				if (newindex >= QWMAX_PACKET_ENTITIES)
				{
					throw DropException("CLQW_ParsePacketEntities: newindex == QWMAX_PACKET_ENTITIES");
				}
				newp->entities[newindex] = oldp->entities[oldindex];
				newindex++;
				oldindex++;
			}
			break;
		}
		int newnum = word & 511;
		int oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;

		while (newnum > oldnum)
		{
			if (full)
			{
				Log::write("WARNING: oldcopy on full update");
				CLQW_FlushEntityPacket(message);
				return;
			}

			// copy one of the old entities over to the new packet unchanged
			if (newindex >= QWMAX_PACKET_ENTITIES)
			{
				throw DropException("CLQW_ParsePacketEntities: newindex == QWMAX_PACKET_ENTITIES");
			}
			newp->entities[newindex] = oldp->entities[oldindex];
			newindex++;
			oldindex++;
			oldnum = oldindex >= oldp->num_entities ? 9999 : oldp->entities[oldindex].number;
		}

		if (newnum < oldnum)
		{
			// new from baseline
			if (word & QWU_REMOVE)
			{
				if (full)
				{
					cl.qh_validsequence = 0;
					Log::write("WARNING: QWU_REMOVE on full update\n");
					CLQW_FlushEntityPacket(message);
					return;
				}
				continue;
			}
			if (newindex >= QWMAX_PACKET_ENTITIES)
			{
				throw DropException("CLQW_ParsePacketEntities: newindex == QWMAX_PACKET_ENTITIES");
			}
			CLQW_ParseDelta(message, &clq1_baselines[newnum], &newp->entities[newindex], word);
			newindex++;
			continue;
		}

		if (newnum == oldnum)
		{
			// delta from previous
			if (full)
			{
				cl.qh_validsequence = 0;
				Log::write("WARNING: delta on full update");
			}
			if (word & QWU_REMOVE)
			{
				oldindex++;
				continue;
			}
			CLQW_ParseDelta(message, &oldp->entities[oldindex], &newp->entities[newindex], word);
			newindex++;
			oldindex++;
		}

	}

	newp->num_entities = newindex;
}

void CLQW_ParsePacketEntities(QMsg& message)
{
	CLQW_ParsePacketEntities(message, false);
}

void CLQW_ParseDeltaPacketEntities(QMsg& message)
{
	CLQW_ParsePacketEntities(message, true);
}

void CLQW_ParsePlayerinfo(QMsg& message)
{
	int num = message.ReadByte();
	if (num > MAX_CLIENTS_QW)
	{
		throw Exception("CLQW_ParsePlayerinfo: bad num");
	}

	qwframe_t* frame = &cl.qw_frames[cl.qh_parsecount &  UPDATE_MASK_QW];
	qwplayer_state_t* state = &frame->playerstate[num];

	int flags = state->flags = message.ReadShort();

	state->messagenum = cl.qh_parsecount;
	state->origin[0] = message.ReadCoord();
	state->origin[1] = message.ReadCoord();
	state->origin[2] = message.ReadCoord();

	state->frame = message.ReadByte();

	// the other player's last move was likely some time
	// before the packet was sent out, so accurately track
	// the exact time it was valid at
	if (flags & QWPF_MSEC)
	{
		int msec = message.ReadByte();
		state->state_time = frame->senttime - msec * 0.001;
	}
	else
	{
		state->state_time = frame->senttime;
	}

	if (flags & QWPF_COMMAND)
	{
		qwusercmd_t nullcmd;
		Com_Memset(&nullcmd, 0, sizeof(nullcmd));
		MSGQW_ReadDeltaUsercmd(&message, &nullcmd, &state->command);
	}

	for (int i = 0; i < 3; i++)
	{
		if (flags & (QWPF_VELOCITY1ND << i))
		{
			state->velocity[i] = message.ReadShort();
		}
		else
		{
			state->velocity[i] = 0;
		}
	}
	if (flags & QWPF_MODEL)
	{
		state->modelindex = message.ReadByte();
	}
	else
	{
		state->modelindex = clq1_playerindex;
	}

	if (flags & QWPF_SKINNUM)
	{
		state->skinnum = message.ReadByte();
	}
	else
	{
		state->skinnum = 0;
	}

	if (flags & QWPF_EFFECTS)
	{
		state->effects = message.ReadByte();
	}
	else
	{
		state->effects = 0;
	}

	if (flags & QWPF_WEAPONFRAME)
	{
		state->weaponframe = message.ReadByte();
	}
	else
	{
		state->weaponframe = 0;
	}

	VectorCopy(state->command.angles, state->viewangles);
}

void CLQ1_SetRefEntAxis(refEntity_t* ent, vec3_t ent_angles)
{
	vec3_t angles;
	angles[YAW] = ent_angles[YAW];
	angles[ROLL] = ent_angles[ROLL];
	if (R_IsMeshModel(ent->hModel))
	{
		// stupid quake bug
		angles[PITCH] = -ent_angles[PITCH];
	}
	else
	{
		angles[PITCH] = ent_angles[PITCH];
	}

	AnglesToAxis(angles, ent->axis);

	if (!String::Cmp(R_ModelName(ent->hModel), "progs/eyes.mdl") &&
		((GGameType & GAME_QuakeWorld) || cl_doubleeyes->integer))
	{
		// double size of eyes, since they are really hard to see in gl
		ent->renderfx |= RF_LIGHTING_ORIGIN;
		VectorCopy(ent->origin, ent->lightingOrigin);
		ent->origin[2] -= (22 + 8);

		VectorScale(ent->axis[0], 2, ent->axis[0]);
		VectorScale(ent->axis[1], 2, ent->axis[1]);
		VectorScale(ent->axis[2], 2, ent->axis[2]);
		ent->nonNormalizedAxes = true;
	}

	// HACK HACK HACK -- no fullbright colors, so make torches full light
	if (!String::Cmp(R_ModelName(ent->hModel), "progs/flame2.mdl") ||
		!String::Cmp(R_ModelName(ent->hModel), "progs/flame.mdl"))
	{
		ent->renderfx |= RF_ABSOLUTE_LIGHT;
		ent->radius = 1.0;
	}
}

void CLQ1_LinkStaticEntities()
{
	q1entity_t* pent = clq1_static_entities;
	for (int i = 0; i < cl.qh_num_statics; i++, pent++)
	{
		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(pent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[pent->state.modelindex];
		CLQ1_SetRefEntAxis(&rent, pent->state.angles);
		rent.frame = pent->state.frame;
		rent.skinNum = pent->state.skinnum;
		rent.shaderTime = pent->syncbase;
		R_AddRefEntityToScene(&rent);
	}
}

//	Translates a skin texture by the per-player color lookup
void CLQ1_TranslatePlayerSkin(int playernum)
{
	q1player_info_t* player = &cl.q1_players[playernum];
	if (GGameType & GAME_QuakeWorld)
	{
		if (!player->name[0])
		{
			return;
		}

		char s[512];
		String::Cpy(s, Info_ValueForKey(player->userinfo, "skin"));
		String::StripExtension(s, s);
		if (player->skin && !String::ICmp(s, player->skin->name))
		{
			player->skin = NULL;
		}

		if (player->_topcolor == player->topcolor &&
			player->_bottomcolor == player->bottomcolor && player->skin)
		{
			return;
		}

		player->_topcolor = player->topcolor;
		player->_bottomcolor = player->bottomcolor;
	}

	byte translate[256];
	CL_CalcQuakeSkinTranslation(player->topcolor, player->bottomcolor, translate);

	//
	// locate the original skin pixels
	//
	if (GGameType & GAME_QuakeWorld)
	{
		if (!player->skin)
		{
			CLQW_SkinFind(player);
		}
		byte* original = CLQW_SkinCache(player->skin);
		if (original != NULL)
		{
			//skin data width
			R_CreateOrUpdateTranslatedSkin(clq1_playertextures[playernum], va("*player%d", playernum),
				original, translate, 320, 200);
		}
		else
		{
			R_CreateOrUpdateTranslatedModelSkinQ1(clq1_playertextures[playernum], va("*player%d", playernum),
				cl.model_draw[clq1_playerindex], translate);
		}
	}
	else
	{
		q1entity_t* ent = &clq1_entities[1 + playernum];

		R_CreateOrUpdateTranslatedModelSkinQ1(clq1_playertextures[playernum], va("*player%d", playernum),
			cl.model_draw[ent->state.modelindex], translate);
	}
}

static void R_HandleRefEntColormap(refEntity_t* Ent, int ColorMap)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (ColorMap && !String::Cmp(R_ModelName(Ent->hModel), "progs/player.mdl"))
	{
	    Ent->customSkin = R_GetImageHandle(clq1_playertextures[ColorMap - 1]);
	}
}

void CLQ1_RelinkEntities()
{
	// determine partial update time	
	float frac = CLQH_LerpPoint();

	R_ClearScene();

	//
	// interpolate player info
	//
	for (int i = 0; i < 3; i++)
	{
		cl.qh_velocity[i] = cl.qh_mvelocity[1][i] + 
			frac * (cl.qh_mvelocity[0][i] - cl.qh_mvelocity[1][i]);
	}

	if (clc.demoplaying)
	{
		// interpolate the angles	
		for (int j = 0; j < 3; j++)
		{
			float d = cl.qh_mviewangles[0][j] - cl.qh_mviewangles[1][j];
			if (d > 180)
			{
				d -= 360;
			}
			else if (d < -180)
			{
				d += 360;
			}
			cl.viewangles[j] = cl.qh_mviewangles[1][j] + frac * d;
		}
	}

	float bobjrotate = AngleMod(100 * cl.qh_serverTimeFloat);

	// start on the entity after the world
	q1entity_t* ent = clq1_entities + 1;
	for (int i = 1; i < cl.qh_num_entities; i++,ent++)
	{
		if (!ent->state.modelindex)
		{
			// empty slot
			continue;
		}

		// if the object wasn't included in the last packet, remove it
		if (ent->msgtime != cl.qh_mtime[0])
		{
			ent->state.modelindex = 0;
			continue;
		}

		vec3_t oldorg;
		VectorCopy(ent->state.origin, oldorg);

		// if the delta is large, assume a teleport and don't lerp
		float f = frac;
		vec3_t delta;
		for (int j = 0; j < 3; j++)
		{
			delta[j] = ent->msg_origins[0][j] - ent->msg_origins[1][j];
			if (delta[j] > 100 || delta[j] < -100)
			{
				f = 1;		// assume a teleportation, not a motion
			}
		}

		// interpolate the origin and angles
		for (int j = 0; j < 3; j++)
		{
			ent->state.origin[j] = ent->msg_origins[1][j] + f * delta[j];

			float d = ent->msg_angles[0][j] - ent->msg_angles[1][j];
			if (d > 180)
			{
				d -= 360;
			}
			else if (d < -180)
			{
				d += 360;
			}
			ent->state.angles[j] = ent->msg_angles[1][j] + f * d;
		}
		

		int ModelFlags = R_ModelFlags(cl.model_draw[ent->state.modelindex]);
		// rotate binary objects locally
		if (ModelFlags & Q1MDLEF_ROTATE)
		{
			ent->state.angles[1] = bobjrotate;
		}

		if (ent->state.effects & Q1EF_BRIGHTFIELD)
		{
			CLQ1_BrightFieldParticles(ent->state.origin);
		}
		if (ent->state.effects & Q1EF_MUZZLEFLASH)
		{
			CLQ1_MuzzleFlashLight(i, ent->state.origin, ent->state.angles);
		}
		if (ent->state.effects & Q1EF_BRIGHTLIGHT)
		{
			CLQ1_BrightLight(i, ent->state.origin);
		}
		if (ent->state.effects & Q1EF_DIMLIGHT)
		{			
			CLQ1_DimLight(i, ent->state.origin, 0);
		}

		if (ModelFlags & Q1MDLEF_GIB)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 2);
		}
		else if (ModelFlags & Q1MDLEF_ZOMGIB)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 4);
		}
		else if (ModelFlags & Q1MDLEF_TRACER)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 3);
		}
		else if (ModelFlags & Q1MDLEF_TRACER2)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 5);
		}
		else if (ModelFlags & Q1MDLEF_ROCKET)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 0);
			CLQ1_RocketLight(i, ent->state.origin);
		}
		else if (ModelFlags & Q1MDLEF_GRENADE)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 1);
		}
		else if (ModelFlags & Q1MDLEF_TRACER3)
		{
			CLQ1_TrailParticles(oldorg, ent->state.origin, 6);
		}

		refEntity_t rent;
		Com_Memset(&rent, 0, sizeof(rent));
		rent.reType = RT_MODEL;
		VectorCopy(ent->state.origin, rent.origin);
		rent.hModel = cl.model_draw[ent->state.modelindex];
		CLQ1_SetRefEntAxis(&rent, ent->state.angles);
		rent.frame = ent->state.frame;
		rent.shaderTime = ent->syncbase;
		R_HandleRefEntColormap(&rent, ent->state.colormap);
		rent.skinNum = ent->state.skinnum;
		if (i == cl.viewentity && !chase_active->value)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntityToScene(&rent);
	}
}
