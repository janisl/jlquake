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

libvar_t* sv_maxstep;
libvar_t* sv_maxbarrier;
libvar_t* sv_gravity;
libvar_t* weapindex_rocketlauncher;
libvar_t* weapindex_bfg10k;
libvar_t* weapindex_grapple;
libvar_t* entitytypemissile;
libvar_t* offhandgrapple;
libvar_t* cmd_grappleoff;
libvar_t* cmd_grappleon;

//type of model, func_plat or func_bobbing
int modeltypes[MAX_MODELS_Q3];

bot_movestate_t* botmovestates[MAX_BOTLIB_CLIENTS_ARRAY + 1];

int BotAllocMoveState()
{
	for (int i = 1; i <= MAX_BOTLIB_CLIENTS; i++)
	{
		if (!botmovestates[i])
		{
			botmovestates[i] = (bot_movestate_t*)Mem_ClearedAlloc(sizeof(bot_movestate_t));
			return i;
		}
	}
	return 0;
}

void BotFreeMoveState(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "move state handle %d out of range\n", handle);
		return;
	}
	if (!botmovestates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid move state %d\n", handle);
		return;
	}
	Mem_Free(botmovestates[handle]);
	botmovestates[handle] = NULL;
}

bot_movestate_t* BotMoveStateFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "move state handle %d out of range\n", handle);
		return NULL;
	}
	if (!botmovestates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid move state %d\n", handle);
		return NULL;
	}
	return botmovestates[handle];
}

//	Provide a means of resetting the avoidreach, so if a bot stops moving,
// they don't avoid the area they were heading for
void BotInitAvoidReach(int handle)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(handle);
	if (!ms)
	{
		return;
	}

	Com_Memset(ms->avoidreach, 0, sizeof(ms->avoidreach));
	Com_Memset(ms->avoidreachtries, 0, sizeof(ms->avoidreachtries));
	Com_Memset(ms->avoidreachtimes, 0, sizeof(ms->avoidreachtimes));
}

void BotInitMoveStateQ3(int handle, bot_initmove_q3_t* initmove)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(handle);
	if (!ms)
	{
		return;
	}
	VectorCopy(initmove->origin, ms->origin);
	VectorCopy(initmove->velocity, ms->velocity);
	VectorCopy(initmove->viewoffset, ms->viewoffset);
	ms->entitynum = initmove->entitynum;
	ms->client = initmove->client;
	ms->thinktime = initmove->thinktime;
	ms->presencetype = initmove->presencetype;
	VectorCopy(initmove->viewangles, ms->viewangles);
	//
	ms->moveflags &= ~MFL_ONGROUND;
	if (initmove->or_moveflags & MFL_ONGROUND)
	{
		ms->moveflags |= MFL_ONGROUND;
	}
	ms->moveflags &= ~MFL_TELEPORTED;
	if (initmove->or_moveflags & MFL_TELEPORTED)
	{
		ms->moveflags |= MFL_TELEPORTED;
	}
	ms->moveflags &= ~MFL_WATERJUMP;
	if (initmove->or_moveflags & MFL_WATERJUMP)
	{
		ms->moveflags |= MFL_WATERJUMP;
	}
	if (GGameType & GAME_Quake3)
	{
		ms->moveflags &= ~Q3MFL_WALK;
		if (initmove->or_moveflags & Q3MFL_WALK)
		{
			ms->moveflags |= Q3MFL_WALK;
		}
		ms->moveflags &= ~Q3MFL_GRAPPLEPULL;
		if (initmove->or_moveflags & Q3MFL_GRAPPLEPULL)
		{
			ms->moveflags |= Q3MFL_GRAPPLEPULL;
		}
	}
	else
	{
		ms->moveflags &= ~WOLFMFL_WALK;
		if (initmove->or_moveflags & WOLFMFL_WALK)
		{
			ms->moveflags |= WOLFMFL_WALK;
		}
	}
}

void BotInitMoveStateET(int handle, bot_initmove_et_t* initmove)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(handle);
	if (!ms)
	{
		return;
	}
	VectorCopy(initmove->origin, ms->origin);
	VectorCopy(initmove->velocity, ms->velocity);
	VectorCopy(initmove->viewoffset, ms->viewoffset);
	ms->entitynum = initmove->entitynum;
	ms->client = initmove->client;
	ms->thinktime = initmove->thinktime;
	ms->presencetype = initmove->presencetype;
	ms->areanum = initmove->areanum;
	VectorCopy(initmove->viewangles, ms->viewangles);
	//
	ms->moveflags &= ~MFL_ONGROUND;
	if (initmove->or_moveflags & MFL_ONGROUND)
	{
		ms->moveflags |= MFL_ONGROUND;
	}
	ms->moveflags &= ~MFL_TELEPORTED;
	if (initmove->or_moveflags & MFL_TELEPORTED)
	{
		ms->moveflags |= MFL_TELEPORTED;
	}
	ms->moveflags &= ~MFL_WATERJUMP;
	if (initmove->or_moveflags & MFL_WATERJUMP)
	{
		ms->moveflags |= MFL_WATERJUMP;
	}
	ms->moveflags &= ~WOLFMFL_WALK;
	if (initmove->or_moveflags & WOLFMFL_WALK)
	{
		ms->moveflags |= WOLFMFL_WALK;
	}
}

void BotSetBrushModelTypes()
{
	Com_Memset(modeltypes, 0, MAX_MODELS_Q3 * sizeof(int));

	for (int ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		char classname[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}
		char model[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent, "model", model, MAX_EPAIRKEY))
		{
			continue;
		}
		int modelnum;
		if (model[0])
		{
			modelnum = String::Atoi(model + 1);
		}
		else
		{
			modelnum = 0;
		}

		if (modelnum < 0 || modelnum > MAX_MODELS_Q3)
		{
			BotImport_Print(PRT_MESSAGE, "entity %s model number out of range\n", classname);
			continue;
		}

		if (!String::ICmp(classname, "func_bobbing"))
		{
			modeltypes[modelnum] = MODELTYPE_FUNC_BOB;
		}
		else if (!String::ICmp(classname, "func_plat"))
		{
			modeltypes[modelnum] = MODELTYPE_FUNC_PLAT;
		}
		else if (GGameType & GAME_Quake3 && !String::ICmp(classname, "func_door"))
		{
			modeltypes[modelnum] = MODELTYPE_FUNC_DOOR;
		}
		else if (GGameType & GAME_Quake3 && !String::ICmp(classname, "func_static"))
		{
			modeltypes[modelnum] = MODELTYPE_FUNC_STATIC;
		}
	}
}

bool BotValidTravel(const aas_reachability_t* reach, int travelflags)
{
	//if the reachability uses an unwanted travel type
	if (AAS_TravelFlagForType(reach->traveltype) & ~travelflags)
	{
		return false;
	}
	//don't go into areas with bad travel types
	if (AAS_AreaContentsTravelFlags(reach->areanum) & ~travelflags)
	{
		return false;
	}
	return true;
}

void BotAddToAvoidReach(bot_movestate_t* ms, int number, float avoidtime)
{
	for (int i = 0; i < MAX_AVOIDREACH; i++)
	{
		if (ms->avoidreach[i] == number)
		{
			if (ms->avoidreachtimes[i] > AAS_Time())
			{
				ms->avoidreachtries[i]++;
			}
			else
			{
				ms->avoidreachtries[i] = 1;
			}
			ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
			return;
		}
	}
	//add the reachability to the reachabilities to avoid for a while
	for (int i = 0; i < MAX_AVOIDREACH; i++)
	{
		if (ms->avoidreachtimes[i] < AAS_Time())
		{
			ms->avoidreach[i] = number;
			ms->avoidreachtimes[i] = AAS_Time() + avoidtime;
			ms->avoidreachtries[i] = 1;
			return;
		}
	}
}
