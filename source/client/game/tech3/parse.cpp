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

#include "../../client.h"
#include "local.h"
#include "../et/local.h"

int entLastVisible[MAX_CLIENTS_WM];

//	The systeminfo configstring has been changed, so parse
// new information out of it.  This will happen at every
// gamestate, and possibly during gameplay.
void CLT3_SystemInfoChanged()
{
	const char* systemInfo = GGameType & GAME_WolfSP ? cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[Q3CS_SYSTEMINFO] :
		GGameType & GAME_WolfMP ? cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[Q3CS_SYSTEMINFO] :
		GGameType & GAME_ET ? cl.et_gameState.stringData + cl.et_gameState.stringOffsets[Q3CS_SYSTEMINFO] :
		cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[Q3CS_SYSTEMINFO];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.q3_serverId = String::Atoi(Info_ValueForKey(systemInfo, "sv_serverid"));

	Com_Memset(&entLastVisible, 0, sizeof(entLastVisible));

	// don't set any vars when playing a demo
	if (clc.demoplaying)
	{
		return;
	}

	const char* s = Info_ValueForKey(systemInfo, "sv_cheats");
	if (String::Atoi(s) == 0)
	{
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey(systemInfo, "sv_paks");
	const char* t = Info_ValueForKey(systemInfo, "sv_pakNames");
	FS_PureServerSetLoadedPaks(s, t);

	s = Info_ValueForKey(systemInfo, "sv_referencedPaks");
	t = Info_ValueForKey(systemInfo, "sv_referencedPakNames");
	FS_PureServerSetReferencedPaks(s, t);

	bool gameSet = false;
	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while (s)
	{
		char key[BIG_INFO_KEY];
		char value[BIG_INFO_VALUE];
		Info_NextPair(&s, key, value);
		if (!key[0])
		{
			break;
		}
		// ehw!
		if (!String::ICmp(key, "fs_game"))
		{
			gameSet = true;
		}

		Cvar_Set(key, value);
	}
	// if game folder should not be set and it is set at the client side
	if (GGameType & GAME_Quake3 && !gameSet && *Cvar_VariableString("fs_game"))
	{
		Cvar_Set("fs_game", "");
	}

	if (GGameType & GAME_ET)
	{
		// Arnout: big hack to clear the image cache on a pure change
		if (Cvar_VariableValue("sv_pure"))
		{
			if (!cl_connectedToPureServer && cls.state <= CA_CONNECTED)
			{
				CLET_PurgeCache();
			}
			cl_connectedToPureServer = true;
		}
		else
		{
			if (cl_connectedToPureServer && cls.state <= CA_CONNECTED)
			{
				CLET_PurgeCache();
			}
			cl_connectedToPureServer = false;
		}
	}
	else
	{
		cl_connectedToPureServer = Cvar_VariableValue("sv_pure");
	}
}

//	Command strings are just saved off until cgame asks for them
// when it transitions a snapshot
void CLT3_ParseCommandString(QMsg* msg)
{
	int seq = msg->ReadLong();
	const char* s = msg->ReadString();

	// see if we have already executed stored it off
	if (clc.q3_serverCommandSequence >= seq)
	{
		return;
	}
	clc.q3_serverCommandSequence = seq;

	int maxReliableCommands = GGameType & GAME_Quake3 ? MAX_RELIABLE_COMMANDS_Q3 : MAX_RELIABLE_COMMANDS_WOLF;
	int index = seq & (maxReliableCommands - 1);
	String::NCpyZ(clc.q3_serverCommands[index], s, sizeof(clc.q3_serverCommands[index]));
}

static bool CLWM_IsEntVisible(wmentityState_t* ent)
{
	vec3_t start;
	VectorCopy(cl.wm_cgameClientLerpOrigin, start);
	start[2] += (cl.wm_snap.ps.viewheight - 1);
	if (cl.wm_snap.ps.leanf != 0)
	{
		vec3_t lright, v3ViewAngles;
		VectorCopy(cl.wm_snap.ps.viewangles, v3ViewAngles);
		v3ViewAngles[2] += cl.wm_snap.ps.leanf / 2.0f;
		AngleVectors(v3ViewAngles, NULL, lright, NULL);
		VectorMA(start, cl.wm_snap.ps.leanf, lright, start);
	}

	vec3_t end;
	VectorCopy(ent->pos.trBase, end);

	// Compute vector perpindicular to view to ent
	vec3_t forward;
	VectorSubtract(end, start, forward);
	VectorNormalizeFast(forward);
	vec3_t up;
	VectorSet(up, 0, 0, 1);
	vec3_t right;
	CrossProduct(forward, up, right);
	VectorNormalizeFast(right);
	vec3_t right2;
	VectorScale(right, 10, right2);
	VectorScale(right, 18, right);

	// Set viewheight
	float view_height;
	if (ent->animMovetype)
	{
		view_height = 16;
	}
	else
	{
		view_height = 40;
	}

	// First, viewpoint to viewpoint
	end[2] += view_height;
	q3trace_t tr;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// First-b, viewpoint to top of head
	end[2] += 16;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}
	end[2] -= 16;

	// Second, viewpoint to ent's origin
	end[2] -= view_height;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Third, to ent's right knee
	vec3_t temp;
	VectorAdd(end, right, temp);
	temp[2] += 8;
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Fourth, to ent's right shoulder
	VectorAdd(end, right2, temp);
	if (ent->animMovetype)
	{
		temp[2] += 28;
	}
	else
	{
		temp[2] += 52;
	}
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Fifth, to ent's left knee
	VectorScale(right, -1, right);
	VectorScale(right2, -1, right2);
	VectorAdd(end, right2, temp);
	temp[2] += 2;
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Sixth, to ent's left shoulder
	VectorAdd(end, right, temp);
	if (ent->animMovetype)
	{
		temp[2] += 16;
	}
	else
	{
		temp[2] += 36;
	}
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	return false;
}

static bool CLET_IsEntVisible(etentityState_t* ent)
{
	vec3_t start;
	VectorCopy(cl.wm_cgameClientLerpOrigin, start);
	start[2] += (cl.et_snap.ps.viewheight - 1);
	if (cl.et_snap.ps.leanf != 0)
	{
		vec3_t lright, v3ViewAngles;
		VectorCopy(cl.et_snap.ps.viewangles, v3ViewAngles);
		v3ViewAngles[2] += cl.et_snap.ps.leanf / 2.0f;
		AngleVectors(v3ViewAngles, NULL, lright, NULL);
		VectorMA(start, cl.et_snap.ps.leanf, lright, start);
	}

	vec3_t end;
	VectorCopy(ent->pos.trBase, end);

	// Compute vector perpindicular to view to ent
	vec3_t forward;
	VectorSubtract(end, start, forward);
	VectorNormalizeFast(forward);
	vec3_t up;
	VectorSet(up, 0, 0, 1);
	vec3_t right;
	CrossProduct(forward, up, right);
	VectorNormalizeFast(right);
	vec3_t right2;
	VectorScale(right, 10, right2);
	VectorScale(right, 18, right);

	// Set viewheight
	float view_height;
	if (ent->animMovetype)
	{
		view_height = 16;
	}
	else
	{
		view_height = 40;
	}

	// First, viewpoint to viewpoint
	end[2] += view_height;
	q3trace_t tr;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// First-b, viewpoint to top of head
	end[2] += 16;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}
	end[2] -= 16;

	// Second, viewpoint to ent's origin
	end[2] -= view_height;
	CM_BoxTraceQ3(&tr, start, end, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Third, to ent's right knee
	vec3_t temp;
	VectorAdd(end, right, temp);
	temp[2] += 8;
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Fourth, to ent's right shoulder
	VectorAdd(end, right2, temp);
	if (ent->animMovetype)
	{
		temp[2] += 28;
	}
	else
	{
		temp[2] += 52;
	}
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Fifth, to ent's left knee
	VectorScale(right, -1, right);
	VectorScale(right2, -1, right2);
	VectorAdd(end, right2, temp);
	temp[2] += 2;
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	// Sixth, to ent's left shoulder
	VectorAdd(end, right, temp);
	if (ent->animMovetype)
	{
		temp[2] += 16;
	}
	else
	{
		temp[2] += 36;
	}
	CM_BoxTraceQ3(&tr, start, temp, NULL, NULL, 0, BSP46CONTENTS_SOLID, false);
	if (tr.fraction == 1.f)
	{
		return true;
	}

	return false;
}

//	Parses deltas from the given base and adds the resulting entity
// to the current frame
static void CLQ3_DeltaEntity(QMsg* msg, q3clSnapshot_t* frame, int newnum, q3entityState_t* old,
	bool unchanged)
{
	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	q3entityState_t* state = &cl.q3_parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q3 - 1)];

	if (unchanged)
	{
		*state = *old;
	}
	else
	{
		MSGQ3_ReadDeltaEntity(msg, old, state, newnum);
	}

	if (state->number == (MAX_GENTITIES_Q3 - 1))
	{
		return;		// entity was delta removed
	}
	cl.parseEntitiesNum++;
	frame->numEntities++;
}

//	Parses deltas from the given base and adds the resulting entity
// to the current frame
static void CLWS_DeltaEntity(QMsg* msg, wsclSnapshot_t* frame, int newnum, wsentityState_t* old,
	bool unchanged)
{
	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	wsentityState_t* state = &cl.ws_parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q3 - 1)];

	if (unchanged)
	{
		*state = *old;
	}
	else
	{
		MSGWS_ReadDeltaEntity(msg, old, state, newnum);
	}

	if (state->number == (MAX_GENTITIES_Q3 - 1))
	{
		return;		// entity was delta removed
	}
	cl.parseEntitiesNum++;
	frame->numEntities++;
}

//	Parses deltas from the given base and adds the resulting entity
// to the current frame
static void CLWM_DeltaEntity(QMsg* msg, wmclSnapshot_t* frame, int newnum, wmentityState_t* old,
	bool unchanged)
{
	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	wmentityState_t* state = &cl.wm_parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q3 - 1)];

	if (unchanged)
	{
		*state = *old;
	}
	else
	{
		MSGWM_ReadDeltaEntity(msg, old, state, newnum);
	}

	if (state->number == (MAX_GENTITIES_Q3 - 1))
	{
		return;		// entity was delta removed
	}

	// DHM - Nerve :: Only draw clients if visible
	if (clc.wm_onlyVisibleClients)
	{
		if (state->number < MAX_CLIENTS_WM)
		{
			if (CLWM_IsEntVisible(state))
			{
				entLastVisible[state->number] = frame->serverTime;
				state->eFlags &= ~WMEF_NODRAW;
			}
			else
			{
				if (entLastVisible[state->number] < (frame->serverTime - 600))
				{
					state->eFlags |= WMEF_NODRAW;
				}
			}
		}
	}

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

//	Parses deltas from the given base and adds the resulting entity
// to the current frame
static void CLET_DeltaEntity(QMsg* msg, etclSnapshot_t* frame, int newnum, etentityState_t* old,
	bool unchanged)
{
	// save the parsed entity state into the big circular buffer so
	// it can be used as the source for a later delta
	etentityState_t* state = &cl.et_parseEntities[cl.parseEntitiesNum & (MAX_PARSE_ENTITIES_Q3 - 1)];

	if (unchanged)
	{
		*state = *old;
	}
	else
	{
		MSGET_ReadDeltaEntity(msg, old, state, newnum);
	}

	if (state->number == (MAX_GENTITIES_Q3 - 1))
	{
		return;		// entity was delta removed
	}

	// DHM - Nerve :: Only draw clients if visible
	if (clc.wm_onlyVisibleClients)
	{
		if (state->number < MAX_CLIENTS_ET)
		{
			if (CLET_IsEntVisible(state))
			{
				entLastVisible[state->number] = frame->serverTime;
				state->eFlags &= ~ETEF_NODRAW;
			}
			else
			{
				if (entLastVisible[state->number] < (frame->serverTime - 600))
				{
					state->eFlags |= ETEF_NODRAW;
				}
			}
		}
	}

	cl.parseEntitiesNum++;
	frame->numEntities++;
}

static void CLQ3_ParsePacketEntities(QMsg* msg, q3clSnapshot_t* oldframe, q3clSnapshot_t* newframe)
{
	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	int oldindex = 0;
	q3entityState_t* oldstate = NULL;
	int oldnum;
	if (!oldframe)
	{
		oldnum = 99999;
	}
	else
	{
		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.q3_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		// read the entity index number
		int newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);

		if (newnum == (MAX_GENTITIES_Q3 - 1))
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			common->Error("CLQ3_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CLQ3_DeltaEntity(msg, newframe, oldnum, oldstate, true);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.q3_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CLQ3_DeltaEntity(msg, newframe, newnum, oldstate, false);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.q3_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{
			// delta from baseline
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CLQ3_DeltaEntity(msg, newframe, newnum, &cl.q3_entityBaselines[newnum], false);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->integer == 3)
		{
			common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CLQ3_DeltaEntity(msg, newframe, oldnum, oldstate, true);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.q3_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}
}

static void CLWS_ParsePacketEntities(QMsg* msg, wsclSnapshot_t* oldframe, wsclSnapshot_t* newframe)
{
	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	int oldindex = 0;
	wsentityState_t* oldstate = NULL;
	int oldnum;
	if (!oldframe)
	{
		oldnum = 99999;
	}
	else
	{
		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.ws_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		// read the entity index number
		int newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);

		if (newnum == (MAX_GENTITIES_Q3 - 1))
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			common->Error("CLWS_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CLWS_DeltaEntity(msg, newframe, oldnum, oldstate, true);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.ws_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CLWS_DeltaEntity(msg, newframe, newnum, oldstate, false);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.ws_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{
			// delta from baseline
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CLWS_DeltaEntity(msg, newframe, newnum, &cl.ws_entityBaselines[newnum], false);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->integer == 3)
		{
			common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CLWS_DeltaEntity(msg, newframe, oldnum, oldstate, true);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.ws_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}
}

static void CLWM_ParsePacketEntities(QMsg* msg, wmclSnapshot_t* oldframe, wmclSnapshot_t* newframe)
{
	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	int oldindex = 0;
	wmentityState_t* oldstate = NULL;
	int oldnum;
	if (!oldframe)
	{
		oldnum = 99999;
	}
	else
	{
		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.wm_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		// read the entity index number
		int newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);

		if (newnum == (MAX_GENTITIES_Q3 - 1))
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			common->Error("CLWM_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CLWM_DeltaEntity(msg, newframe, oldnum, oldstate, true);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.wm_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CLWM_DeltaEntity(msg, newframe, newnum, oldstate, false);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.wm_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{
			// delta from baseline
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CLWM_DeltaEntity(msg, newframe, newnum, &cl.wm_entityBaselines[newnum], false);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->integer == 3)
		{
			common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CLWM_DeltaEntity(msg, newframe, oldnum, oldstate, true);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.wm_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	if (clwm_shownuments->integer)
	{
		common->Printf("Entities in packet: %i\n", newframe->numEntities);
	}
}

static void CLET_ParsePacketEntities(QMsg* msg, etclSnapshot_t* oldframe, etclSnapshot_t* newframe)
{
	newframe->parseEntitiesNum = cl.parseEntitiesNum;
	newframe->numEntities = 0;

	// delta from the entities present in oldframe
	int oldindex = 0;
	etentityState_t* oldstate = NULL;
	int oldnum;
	if (!oldframe)
	{
		oldnum = 99999;
	}
	else
	{
		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.et_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		// read the entity index number
		int newnum = msg->ReadBits(GENTITYNUM_BITS_Q3);

		if (newnum == (MAX_GENTITIES_Q3 - 1))
		{
			break;
		}

		if (msg->readcount > msg->cursize)
		{
			common->Error("CLET_ParsePacketEntities: end of message");
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
			}
			CLET_DeltaEntity(msg, newframe, oldnum, oldstate, true);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.et_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
		}
		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  delta: %i\n", msg->readcount, newnum);
			}
			CLET_DeltaEntity(msg, newframe, newnum, oldstate, false);

			oldindex++;

			if (oldindex >= oldframe->numEntities)
			{
				oldnum = 99999;
			}
			else
			{
				oldstate = &cl.et_parseEntities[
					(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{
			// delta from baseline
			if (cl_shownet->integer == 3)
			{
				common->Printf("%3i:  baseline: %i\n", msg->readcount, newnum);
			}
			CLET_DeltaEntity(msg, newframe, newnum, &cl.et_entityBaselines[newnum], false);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{
		// one or more entities from the old packet are unchanged
		if (cl_shownet->integer == 3)
		{
			common->Printf("%3i:  unchanged: %i\n", msg->readcount, oldnum);
		}
		CLET_DeltaEntity(msg, newframe, oldnum, oldstate, true);

		oldindex++;

		if (oldindex >= oldframe->numEntities)
		{
			oldnum = 99999;
		}
		else
		{
			oldstate = &cl.et_parseEntities[
				(oldframe->parseEntitiesNum + oldindex) & (MAX_PARSE_ENTITIES_Q3 - 1)];
			oldnum = oldstate->number;
		}
	}

	if (clwm_shownuments->integer)
	{
		common->Printf("Entities in packet: %i\n", newframe->numEntities);
	}
}

//	If the snapshot is parsed properly, it will be copied to
// cl.snap and saved in cl.snapshots[].  If the snapshot is invalid
// for any reason, no changes to the state will be made at all.
void CLQ3_ParseSnapshot(QMsg* msg)
{
	int len;
	q3clSnapshot_t* old;
	q3clSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.q3_reliableAcknowledge = msg->ReadLong();

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.snap if it is valid
	Com_Memset(&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to q3svc_snapshot
	newSnap.serverCommandNum = clc.q3_serverCommandSequence;

	newSnap.serverTime = msg->ReadLong();

	newSnap.messageNum = clc.q3_serverMessageSequence;

	deltaNum = msg->ReadByte();
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = msg->ReadByte();

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = true;		// uncompressed frame
		old = NULL;
		clc.q3_demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.q3_snapshots[newSnap.deltaNum & PACKET_MASK_Q3];
		if (!old->valid)
		{
			// should never happen
			common->Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			common->Printf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES_Q3 - 128)
		{
			common->Printf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = true;	// valid delta parse
		}
	}

	// read areamask
	len = msg->ReadByte();
	msg->ReadData(&newSnap.areamask, len);

	// read playerinfo
	SHOWNET(*msg, "playerstate");
	if (old)
	{
		MSGQ3_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSGQ3_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	SHOWNET(*msg, "packet entities");
	CLQ3_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.q3_snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP_Q3)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP_Q3 - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.q3_snapshots[oldMessageNum & PACKET_MASK_Q3].valid = false;
	}

	// copy to the current good spot
	cl.q3_snap = newSnap;
	cl.q3_snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP_Q3; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK_Q3;
		if (cl.q3_snap.ps.commandTime >= cl.q3_outPackets[packetNum].p_serverTime)
		{
			cl.q3_snap.ping = cls.realtime - cl.q3_outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.q3_snapshots[cl.q3_snap.messageNum & PACKET_MASK_Q3] = cl.q3_snap;

	if (cl_shownet->integer == 3)
	{
		common->Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.q3_snap.messageNum,
			cl.q3_snap.deltaNum, cl.q3_snap.ping);
	}

	cl.q3_newSnapshots = true;
}

//	If the snapshot is parsed properly, it will be copied to
// cl.ws_snap and saved in cl.ws_snapshots[].  If the snapshot is invalid
// for any reason, no changes to the state will be made at all.
void CLWS_ParseSnapshot(QMsg* msg)
{
	int len;
	wsclSnapshot_t* old;
	wsclSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.q3_reliableAcknowledge = msg->ReadLong();

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.ws_snap if it is valid
	memset(&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to q3svc_snapshot
	newSnap.serverCommandNum = clc.q3_serverCommandSequence;

	newSnap.serverTime = msg->ReadLong();

	newSnap.messageNum = clc.q3_serverMessageSequence;

	deltaNum = msg->ReadByte();
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = msg->ReadByte();

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = true;		// uncompressed frame
		old = NULL;
		clc.q3_demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.ws_snapshots[newSnap.deltaNum & PACKET_MASK_Q3];
		if (!old->valid)
		{
			// should never happen
			common->Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			common->Printf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES_Q3 - 128)
		{
			common->Printf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = true;	// valid delta parse
		}
	}

	// read areamask
	len = msg->ReadByte();
	msg->ReadData(&newSnap.areamask, len);

	// read playerinfo
	SHOWNET(*msg, "playerstate");
	if (old)
	{
		MSGWS_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSGWS_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	SHOWNET(*msg, "packet entities");
	CLWS_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.ws_snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP_Q3)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP_Q3 - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.ws_snapshots[oldMessageNum & PACKET_MASK_Q3].valid = false;
	}

	// copy to the current good spot
	cl.ws_snap = newSnap;
	cl.ws_snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP_Q3; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK_Q3;
		if (cl.ws_snap.ps.commandTime >= cl.q3_outPackets[packetNum].p_serverTime)
		{
			cl.ws_snap.ping = cls.realtime - cl.q3_outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.ws_snapshots[cl.ws_snap.messageNum & PACKET_MASK_Q3] = cl.ws_snap;

	if (cl_shownet->integer == 3)
	{
		common->Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.ws_snap.messageNum,
			cl.ws_snap.deltaNum, cl.ws_snap.ping);
	}

	cl.q3_newSnapshots = true;
}

//	If the snapshot is parsed properly, it will be copied to
// cl.wm_snap and saved in cl.wm_snapshots[].  If the snapshot is invalid
// for any reason, no changes to the state will be made at all.
void CLWM_ParseSnapshot(QMsg* msg)
{
	int len;
	wmclSnapshot_t* old;
	wmclSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.q3_reliableAcknowledge = msg->ReadLong();

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.wm_snap if it is valid
	memset(&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to q3svc_snapshot
	newSnap.serverCommandNum = clc.q3_serverCommandSequence;

	newSnap.serverTime = msg->ReadLong();

	newSnap.messageNum = clc.q3_serverMessageSequence;

	deltaNum = msg->ReadByte();
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = msg->ReadByte();

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = true;		// uncompressed frame
		old = NULL;
		clc.q3_demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.wm_snapshots[newSnap.deltaNum & PACKET_MASK_Q3];
		if (!old->valid)
		{
			// should never happen
			common->Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			common->DPrintf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES_Q3 - 128)
		{
			common->DPrintf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = true;	// valid delta parse
		}
	}

	// read areamask
	len = msg->ReadByte();

	if (len > (int)sizeof(newSnap.areamask))
	{
		common->Error("CLWM_ParseSnapshot: Invalid size %d for areamask.", len);
		return;
	}

	msg->ReadData(&newSnap.areamask, len);

	// read playerinfo
	SHOWNET(*msg, "playerstate");
	if (old)
	{
		MSGWM_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSGWM_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	SHOWNET(*msg, "packet entities");
	CLWM_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.wm_snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP_Q3)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP_Q3 - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.wm_snapshots[oldMessageNum & PACKET_MASK_Q3].valid = false;
	}

	// copy to the current good spot
	cl.wm_snap = newSnap;
	cl.wm_snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP_Q3; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK_Q3;
		if (cl.wm_snap.ps.commandTime >= cl.q3_outPackets[packetNum].p_serverTime)
		{
			cl.wm_snap.ping = cls.realtime - cl.q3_outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.wm_snapshots[cl.wm_snap.messageNum & PACKET_MASK_Q3] = cl.wm_snap;

	if (cl_shownet->integer == 3)
	{
		common->Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.wm_snap.messageNum,
			cl.wm_snap.deltaNum, cl.wm_snap.ping);
	}

	cl.q3_newSnapshots = true;
}

//	If the snapshot is parsed properly, it will be copied to
// cl.et_snap and saved in cl.et_snapshots[].  If the snapshot is invalid
// for any reason, no changes to the state will be made at all.
void CLET_ParseSnapshot(QMsg* msg)
{
	int len;
	etclSnapshot_t* old;
	etclSnapshot_t newSnap;
	int deltaNum;
	int oldMessageNum;
	int i, packetNum;

	// get the reliable sequence acknowledge number
	// NOTE: now sent with all server to client messages
	//clc.q3_reliableAcknowledge = msg->ReadLong();

	// read in the new snapshot to a temporary buffer
	// we will only copy to cl.et_snap if it is valid
	memset(&newSnap, 0, sizeof(newSnap));

	// we will have read any new server commands in this
	// message before we got to q3svc_snapshot
	newSnap.serverCommandNum = clc.q3_serverCommandSequence;

	newSnap.serverTime = msg->ReadLong();

	newSnap.messageNum = clc.q3_serverMessageSequence;

	deltaNum = msg->ReadByte();
	if (!deltaNum)
	{
		newSnap.deltaNum = -1;
	}
	else
	{
		newSnap.deltaNum = newSnap.messageNum - deltaNum;
	}
	newSnap.snapFlags = msg->ReadByte();

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (newSnap.deltaNum <= 0)
	{
		newSnap.valid = true;		// uncompressed frame
		old = NULL;
		if (clc.demorecording)
		{
			clc.q3_demowaiting = false;	// we can start recording now
//			if(clet_autorecord->integer) {
//				Cvar_Set( "g_synchronousClients", "0" );
//			}
		}
		else
		{
			if (clet_autorecord->integer /*&& Cvar_VariableValue( "g_synchronousClients")*/)
			{
				char mapname[MAX_QPATH];
				char* period;
				qtime_t time;

				Com_RealTime(&time);

				String::NCpyZ(mapname, cl.q3_mapname, MAX_QPATH);
				for (period = mapname; *period; period++)
				{
					if (*period == '.')
					{
						*period = '\0';
						break;
					}
				}

				for (period = mapname; *period; period++)
				{
					if (*period == '/')
					{
						break;
					}
				}
				if (*period)
				{
					period++;
				}

				char demoName[MAX_QPATH];
				String::Sprintf(demoName, sizeof(demoName), "%s_%i_%i", period, time.tm_mday, time.tm_mon + 1);
				char name[MAX_QPATH];
				String::Sprintf(name, sizeof(name), "demos/%s.dm_%d", demoName, ETPROTOCOL_VERSION);

				CLT3_Record(demoName, name);
			}
		}
	}
	else
	{
		old = &cl.et_snapshots[newSnap.deltaNum & PACKET_MASK_Q3];
		if (!old->valid)
		{
			// should never happen
			common->Printf("Delta from invalid frame (not supposed to happen!).\n");
		}
		else if (old->messageNum != newSnap.deltaNum)
		{
			// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			common->DPrintf("Delta frame too old.\n");
		}
		else if (cl.parseEntitiesNum - old->parseEntitiesNum > MAX_PARSE_ENTITIES_Q3 - 128)
		{
			common->DPrintf("Delta parseEntitiesNum too old.\n");
		}
		else
		{
			newSnap.valid = true;	// valid delta parse
		}
	}

	// read areamask
	len = msg->ReadByte();

	if (len > (int)sizeof(newSnap.areamask))
	{
		common->Error("CLET_ParseSnapshot: Invalid size %d for areamask.", len);
		return;
	}

	msg->ReadData(&newSnap.areamask, len);

	// read playerinfo
	SHOWNET(*msg, "playerstate");
	if (old)
	{
		MSGET_ReadDeltaPlayerstate(msg, &old->ps, &newSnap.ps);
	}
	else
	{
		MSGET_ReadDeltaPlayerstate(msg, NULL, &newSnap.ps);
	}

	// read packet entities
	SHOWNET(*msg, "packet entities");
	CLET_ParsePacketEntities(msg, old, &newSnap);

	// if not valid, dump the entire thing now that it has
	// been properly read
	if (!newSnap.valid)
	{
		return;
	}

	// clear the valid flags of any snapshots between the last
	// received and this one, so if there was a dropped packet
	// it won't look like something valid to delta from next
	// time we wrap around in the buffer
	oldMessageNum = cl.et_snap.messageNum + 1;

	if (newSnap.messageNum - oldMessageNum >= PACKET_BACKUP_Q3)
	{
		oldMessageNum = newSnap.messageNum - (PACKET_BACKUP_Q3 - 1);
	}
	for (; oldMessageNum < newSnap.messageNum; oldMessageNum++)
	{
		cl.et_snapshots[oldMessageNum & PACKET_MASK_Q3].valid = false;
	}

	// copy to the current good spot
	cl.et_snap = newSnap;
	cl.et_snap.ping = 999;
	// calculate ping time
	for (i = 0; i < PACKET_BACKUP_Q3; i++)
	{
		packetNum = (clc.netchan.outgoingSequence - 1 - i) & PACKET_MASK_Q3;
		if (cl.et_snap.ps.commandTime >= cl.q3_outPackets[packetNum].p_serverTime)
		{
			cl.et_snap.ping = cls.realtime - cl.q3_outPackets[packetNum].p_realtime;
			break;
		}
	}
	// save the frame off in the backup array for later delta comparisons
	cl.et_snapshots[cl.et_snap.messageNum & PACKET_MASK_Q3] = cl.et_snap;

	if (cl_shownet->integer == 3)
	{
		common->Printf("   snapshot:%i  delta:%i  ping:%i\n", cl.et_snap.messageNum,
			cl.et_snap.deltaNum, cl.et_snap.ping);
	}

	cl.q3_newSnapshots = true;
}

void CLET_ParseBinaryMessage(QMsg* msg)
{
	msg->Uncompressed();

	int size = msg->cursize - msg->readcount;
	if (size <= 0 || size > MAX_BINARY_MESSAGE_ET)
	{
		return;
	}

	CLET_CGameBinaryMessageReceived((char*)&msg->_data[msg->readcount], size, cl.et_snap.serverTime);
}
