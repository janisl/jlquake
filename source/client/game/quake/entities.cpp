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
#include "../../client_main.h"
#include "../../chase.h"
#include "../particles.h"
#include "../dynamic_lights.h"
#include "../camera.h"
#include "../quake_hexen2/predict.h"
#include "../../../common/Common.h"
#include "../../../common/file_formats/spr.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/message_utils.h"
#include "../../../common/game_utils.h"
#include "../../../common/player_move.h"

Cvar* cl_doubleeyes;

q1entity_state_t clq1_baselines[MAX_EDICTS_QH];
q1entity_t clq1_entities[MAX_EDICTS_QH];
q1entity_t clq1_static_entities[MAX_STATIC_ENTITIES_Q1];

image_t* clq1_playertextures[BIGGEST_MAX_CLIENTS_QH];	// color translated skins

int clq1_playerindex;
int clqw_flagindex;

//	This error checks and tracks the total number of entities
static q1entity_t* CLQ1_EntityNum(int number)
{
	if (number >= cl.qh_num_entities)
	{
		if (number >= MAX_EDICTS_QH)
		{
			common->Error("CLQ1_EntityNum: %i is an invalid number", number);
		}
		while (cl.qh_num_entities <= number)
		{
			clq1_entities[cl.qh_num_entities].state.colormap = 0;
			cl.qh_num_entities++;
		}
	}
	return &clq1_entities[number];
}

static void CLQ1_ParseBaseline(QMsg& message, q1entity_state_t* es)
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
		common->Error("Too many static entities");
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
		CLQ1_SignonReply();
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
			common->Error("CL_ParseModel: bad modnum");
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
		common->FatalError("i >= cl.maxclients");
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
	common->DPrintf("CLQW_FlushEntityPacket\n");

	q1entity_state_t olde;
	Com_Memset(&olde, 0, sizeof(olde));

	cl.qh_validsequence = 0;		// can't render a frame
	cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW].invalid = true;

	// read it all, but ignore it
	while (1)
	{
		int word = (unsigned short)message.ReadShort();
		if (message.badread)
		{
			// something didn't parse right...
			common->Error("msg_badread in packetentities");
			return;
		}

		if (!word)
		{
			break;	// done

		}
		q1entity_state_t newe;
		CLQW_ParseDelta(message, &olde, &newe, word);
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
			common->DPrintf("WARNING: from mismatch\n");
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
			common->Error("msg_badread in packetentities");
		}

		if (!word)
		{
			while (oldindex < oldp->num_entities)
			{
				// copy all the rest of the entities from the old packet
				if (newindex >= QWMAX_PACKET_ENTITIES)
				{
					common->Error("CLQW_ParsePacketEntities: newindex == QWMAX_PACKET_ENTITIES");
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
				common->Printf("WARNING: oldcopy on full update");
				CLQW_FlushEntityPacket(message);
				return;
			}

			// copy one of the old entities over to the new packet unchanged
			if (newindex >= QWMAX_PACKET_ENTITIES)
			{
				common->Error("CLQW_ParsePacketEntities: newindex == QWMAX_PACKET_ENTITIES");
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
					common->Printf("WARNING: QWU_REMOVE on full update\n");
					CLQW_FlushEntityPacket(message);
					return;
				}
				continue;
			}
			if (newindex >= QWMAX_PACKET_ENTITIES)
			{
				common->Error("CLQW_ParsePacketEntities: newindex == QWMAX_PACKET_ENTITIES");
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
				common->Printf("WARNING: delta on full update");
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
	if (num > MAX_CLIENTS_QHW)
	{
		common->FatalError("CLQW_ParsePlayerinfo: bad num");
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
}

static void CLQ1_LinkStaticEntities()
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
		rent.syncBase = pent->syncbase;
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

static void CLQ1_HandleRefEntColormap(refEntity_t* entity, int colourMap)
{
	//	We can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (colourMap && !String::Cmp(R_ModelName(entity->hModel), "progs/player.mdl"))
	{
		entity->customSkin = R_GetImageHandle(clq1_playertextures[colourMap - 1]);
	}
}

static void CLQW_HandlePlayerSkin(refEntity_t* Ent, int PlayerNum)
{
	//	We can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (!cl.q1_players[PlayerNum].skin)
	{
		CLQW_SkinFind(&cl.q1_players[PlayerNum]);
		CLQ1_TranslatePlayerSkin(PlayerNum);
	}
	Ent->customSkin = R_GetImageHandle(clq1_playertextures[PlayerNum]);
}

static void CLQ1_RelinkEntities()
{
	// determine partial update time
	float frac = CLQH_LerpPoint();

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
		rent.syncBase = ent->syncbase;
		CLQ1_HandleRefEntColormap(&rent, ent->state.colormap);
		rent.skinNum = ent->state.skinnum;
		if (i == cl.viewentity && !chase_active->value)
		{
			rent.renderfx |= RF_THIRD_PERSON;
		}
		R_AddRefEntityToScene(&rent);
	}
}

static void CLQW_LinkPacketEntities()
{
	qwpacket_entities_t* pack = &cl.qw_frames[clc.netchan.incomingSequence & UPDATE_MASK_QW].packet_entities;
	qwpacket_entities_t* PrevPack = &cl.qw_frames[(clc.netchan.incomingSequence - 1) & UPDATE_MASK_QW].packet_entities;

	float autorotate = AngleMod(100 * cl.qh_serverTimeFloat);

	float f = 0;		// FIXME: no interpolation right now

	for (int pnum = 0; pnum < pack->num_entities; pnum++)
	{
		q1entity_state_t* s1 = &pack->entities[pnum];
		q1entity_state_t* s2 = s1;	// FIXME: no interpolation right now

		// spawn light flashes, even ones coming from invisible objects
		if ((s1->effects & (QWEF_BLUE | QWEF_RED)) == (QWEF_BLUE | QWEF_RED))
		{
			CLQ1_DimLight(s1->number, s1->origin, 3);
		}
		else if (s1->effects & QWEF_BLUE)
		{
			CLQ1_DimLight(s1->number, s1->origin, 1);
		}
		else if (s1->effects & QWEF_RED)
		{
			CLQ1_DimLight(s1->number, s1->origin, 2);
		}
		else if (s1->effects & Q1EF_BRIGHTLIGHT)
		{
			CLQ1_BrightLight(s1->number, s1->origin);
		}
		else if (s1->effects & Q1EF_DIMLIGHT)
		{
			CLQ1_DimLight(s1->number, s1->origin, 0);
		}

		// if set to invisible, skip
		if (!s1->modelindex)
		{
			continue;
		}

		// create a new entity
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		qhandle_t model = cl.model_draw[s1->modelindex];
		ent.hModel = model;

		// set colormap
		if (s1->colormap && (s1->colormap < MAX_CLIENTS_QHW) && s1->modelindex == clq1_playerindex)
		{
			CLQW_HandlePlayerSkin(&ent, s1->colormap - 1);
		}

		// set skin
		ent.skinNum = s1->skinnum;

		// set frame
		ent.frame = s1->frame;

		int ModelFlags = R_ModelFlags(model);
		// rotate binary objects locally
		vec3_t angles;
		if (ModelFlags & Q1MDLEF_ROTATE)
		{
			angles[0] = 0;
			angles[1] = autorotate;
			angles[2] = 0;
		}
		else
		{
			for (int i = 0; i < 3; i++)
			{
				float a1 = s1->angles[i];
				float a2 = s2->angles[i];
				if (a1 - a2 > 180)
				{
					a1 -= 360;
				}
				if (a1 - a2 < -180)
				{
					a1 += 360;
				}
				angles[i] = a2 + f * (a1 - a2);
			}
		}
		CLQ1_SetRefEntAxis(&ent, angles);

		// calculate origin
		for (int i = 0; i < 3; i++)
		{
			ent.origin[i] = s2->origin[i] + f * (s1->origin[i] - s2->origin[i]);
		}
		R_AddRefEntityToScene(&ent);

		// add automatic particle trails
		if (!ModelFlags)
		{
			continue;
		}

		// scan the old entity display list for a matching
		int i;
		vec3_t old_origin;
		for (i = 0; i < PrevPack->num_entities; i++)
		{
			if (PrevPack->entities[i].number == s1->number)
			{
				VectorCopy(PrevPack->entities[i].origin, old_origin);
				break;
			}
		}
		if (i == PrevPack->num_entities)
		{
			continue;		// not in last message
		}

		for (i = 0; i < 3; i++)
		{
			if (abs(old_origin[i] - ent.origin[i]) > 128)
			{
				// no trail if too far
				VectorCopy(ent.origin, old_origin);
				break;
			}
		}
		if (ModelFlags & Q1MDLEF_ROCKET)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 0);
			CLQ1_RocketLight(s1->number, ent.origin);
		}
		else if (ModelFlags & Q1MDLEF_GRENADE)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 1);
		}
		else if (ModelFlags & Q1MDLEF_GIB)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 2);
		}
		else if (ModelFlags & Q1MDLEF_ZOMGIB)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 4);
		}
		else if (ModelFlags & Q1MDLEF_TRACER)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 3);
		}
		else if (ModelFlags & Q1MDLEF_TRACER2)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 5);
		}
		else if (ModelFlags & Q1MDLEF_TRACER3)
		{
			CLQ1_TrailParticles(old_origin, ent.origin, 6);
		}
	}
}

//	Called when the CTF flags are set
static void CLQW_AddFlagModels(refEntity_t* ent, int team, vec3_t angles)
{
	if (clqw_flagindex == -1)
	{
		return;
	}

	float f = 14;
	if (ent->frame >= 29 && ent->frame <= 40)
	{
		if (ent->frame >= 29 && ent->frame <= 34)	//axpain
		{
			if      (ent->frame == 29)
			{
				f = f + 2;
			}
			else if (ent->frame == 30)
			{
				f = f + 8;
			}
			else if (ent->frame == 31)
			{
				f = f + 12;
			}
			else if (ent->frame == 32)
			{
				f = f + 11;
			}
			else if (ent->frame == 33)
			{
				f = f + 10;
			}
			else if (ent->frame == 34)
			{
				f = f + 4;
			}
		}
		else if (ent->frame >= 35 && ent->frame <= 40)		// pain
		{
			if      (ent->frame == 35)
			{
				f = f + 2;
			}
			else if (ent->frame == 36)
			{
				f = f + 10;
			}
			else if (ent->frame == 37)
			{
				f = f + 10;
			}
			else if (ent->frame == 38)
			{
				f = f + 8;
			}
			else if (ent->frame == 39)
			{
				f = f + 4;
			}
			else if (ent->frame == 40)
			{
				f = f + 2;
			}
		}
	}
	else if (ent->frame >= 103 && ent->frame <= 118)
	{
		if      (ent->frame >= 103 && ent->frame <= 104)
		{
			f = f + 6;												//nailattack
		}
		else if (ent->frame >= 105 && ent->frame <= 106)
		{
			f = f + 6;												//light
		}
		else if (ent->frame >= 107 && ent->frame <= 112)
		{
			f = f + 7;												//rocketattack
		}
		else if (ent->frame >= 112 && ent->frame <= 118)
		{
			f = f + 7;												//shotattack
		}
	}

	refEntity_t newent;
	Com_Memset(&newent, 0, sizeof(newent));

	newent.reType = RT_MODEL;
	newent.hModel = cl.model_draw[clqw_flagindex];
	newent.skinNum = team;

	vec3_t v_forward, v_right, v_up;
	AngleVectors(angles, v_forward, v_right, v_up);
	v_forward[2] = -v_forward[2];	// reverse z component
	for (int i = 0; i < 3; i++)
	{
		newent.origin[i] = ent->origin[i] - f * v_forward[i] + 22 * v_right[i];
	}
	newent.origin[2] -= 16;

	vec3_t flag_angles;
	VectorCopy(angles, flag_angles);
	flag_angles[2] -= 45;
	CLQ1_SetRefEntAxis(&newent, flag_angles);
	R_AddRefEntityToScene(&newent);
}

//	Create visible entities in the correct position
// for all current players
static void CLQW_LinkPlayers()
{
	double playertime = cls.realtime * 0.001 - cls.qh_latency + 0.02;
	if (playertime > cls.realtime * 0.001)
	{
		playertime = cls.realtime * 0.001;
	}

	qwframe_t* frame = &cl.qw_frames[cl.qh_parsecount & UPDATE_MASK_QW];

	q1player_info_t* info = cl.q1_players;
	qwplayer_state_t* state = frame->playerstate;
	for (int j = 0; j < MAX_CLIENTS_QHW; j++, info++, state++)
	{
		if (state->messagenum != cl.qh_parsecount)
		{
			continue;	// not present this frame

		}
		// spawn light flashes, even ones coming from invisible objects
		if ((state->effects & (QWEF_BLUE | QWEF_RED)) == (QWEF_BLUE | QWEF_RED))
		{
			CLQ1_DimLight(j, state->origin, 3);
		}
		else if (state->effects & QWEF_BLUE)
		{
			CLQ1_DimLight(j, state->origin, 1);
		}
		else if (state->effects & QWEF_RED)
		{
			CLQ1_DimLight(j, state->origin, 2);
		}
		else if (state->effects & Q1EF_BRIGHTLIGHT)
		{
			CLQ1_BrightLight(j, state->origin);
		}
		else if (state->effects & Q1EF_DIMLIGHT)
		{
			CLQ1_DimLight(j, state->origin, 0);
		}

		// the player object never gets added
		if (j == cl.playernum)
		{
			continue;
		}

		if (!state->modelindex)
		{
			continue;
		}

		if (!Cam_DrawPlayer(j))
		{
			continue;
		}

		// grab an entity to fill in
		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		ent.hModel = cl.model_draw[state->modelindex];
		ent.skinNum = state->skinnum;
		ent.frame = state->frame;
		if (state->modelindex == clq1_playerindex)
		{
			// use custom skin
			CLQW_HandlePlayerSkin(&ent, j);
		}

		//
		// angles
		//
		vec3_t angles;
		angles[PITCH] = -state->viewangles[PITCH] / 3;
		angles[YAW] = state->viewangles[YAW];
		angles[ROLL] = 0;
		angles[ROLL] = VQH_CalcRoll(angles, state->velocity) * 4;
		CLQ1_SetRefEntAxis(&ent, angles);

		// only predict half the move to minimize overruns
		int msec = 500 * (playertime - state->state_time);
		if (msec <= 0 || (!cl_predict_players->value && !cl_predict_players2->value))
		{
			VectorCopy(state->origin, ent.origin);
		}
		else
		{
			// predict players movement
			if (msec > 255)
			{
				msec = 255;
			}
			state->command.msec = msec;

			int oldphysent = qh_pmove.numphysent;
			CLQHW_SetSolidPlayers(j);
			qwplayer_state_t exact;
			CLQW_PredictUsercmd(state, &exact, &state->command, false);
			qh_pmove.numphysent = oldphysent;
			VectorCopy(exact.origin, ent.origin);
		}
		R_AddRefEntityToScene(&ent);

		if (state->effects & QWEF_FLAG1)
		{
			CLQW_AddFlagModels(&ent, 0, angles);
		}
		else if (state->effects & QWEF_FLAG2)
		{
			CLQW_AddFlagModels(&ent, 1, angles);
		}
	}
}

void CLQ1_EmitEntities()
{
	CLQ1_RelinkEntities();
	CLQ1_UpdateTEnts();
	CLQ1_LinkStaticEntities();
}

void CLQW_EmitEntities()
{
	CLQW_LinkPlayers();
	CLQW_LinkPacketEntities();
	CLQ1_LinkProjectiles();
	CLQ1_UpdateTEnts();
	CLQ1_LinkStaticEntities();
}
