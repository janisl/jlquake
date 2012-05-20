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
#include "ai_weight.h"

#define ITEMINFO_OFS(x) (qintptr) & (((iteminfo_t*)0)->x)

static fielddef_t iteminfo_fields[] =
{
	{"name", ITEMINFO_OFS(name), FT_STRING},
	{"model", ITEMINFO_OFS(model), FT_STRING},
	{"modelindex", ITEMINFO_OFS(modelindex), FT_INT},
	{"type", ITEMINFO_OFS(type), FT_INT},
	{"index", ITEMINFO_OFS(index), FT_INT},
	{"respawntime", ITEMINFO_OFS(respawntime), FT_FLOAT},
	{"mins", ITEMINFO_OFS(mins), FT_FLOAT | FT_ARRAY, 3},
	{"maxs", ITEMINFO_OFS(maxs), FT_FLOAT | FT_ARRAY, 3},
	{0, 0, 0}
};

static structdef_t iteminfo_struct =
{
	sizeof(iteminfo_t), iteminfo_fields
};

static bot_goalstate_t* botgoalstates[MAX_BOTLIB_CLIENTS_ARRAY + 1];
//item configuration
itemconfig_t* itemconfig = NULL;
//level items
static levelitem_t* levelitemheap = NULL;
static levelitem_t* freelevelitems = NULL;
levelitem_t* levelitems = NULL;
int numlevelitems = 0;
//map locations
maplocation_t* maplocations = NULL;
//camp spots
campspot_t* campspots = NULL;
//the game type
int g_gametype = 0;
bool g_singleplayer;
//additional dropped item weight
libvar_t* droppedweight = NULL;

bot_goalstate_t* BotGoalStateFromHandle(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "goal state handle %d out of range\n", handle);
		return NULL;
	}
	if (!botgoalstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid goal state %d\n", handle);
		return NULL;
	}
	return botgoalstates[handle];
}

void BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child)
{
	bot_goalstate_t* p1 = BotGoalStateFromHandle(parent1);
	bot_goalstate_t* p2 = BotGoalStateFromHandle(parent2);
	bot_goalstate_t* c = BotGoalStateFromHandle(child);

	InterbreedWeightConfigs(p1->itemweightconfig, p2->itemweightconfig,
		c->itemweightconfig);
}

void BotMutateGoalFuzzyLogic(int goalstate, float range)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);

	EvolveWeightConfig(gs->itemweightconfig);
}

static itemconfig_t* LoadItemConfig(const char* filename)
{
	int max_iteminfo = (int)LibVarValue("max_iteminfo", "256");
	if (max_iteminfo < 0)
	{
		BotImport_Print(PRT_ERROR, "max_iteminfo = %d\n", max_iteminfo);
		max_iteminfo = 256;
		LibVarSet("max_iteminfo", "256");
	}

	if (GGameType & GAME_Quake3)
	{
		PC_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	char path[MAX_QPATH];
	String::NCpyZ(path, filename, MAX_QPATH);
	source_t* source = LoadSourceFile(path);
	if (!source)
	{
		BotImport_Print(PRT_ERROR, "counldn't load %s\n", path);
		return NULL;
	}
	//initialize item config
	itemconfig_t* ic = (itemconfig_t*)Mem_ClearedAlloc(sizeof(itemconfig_t) +
		max_iteminfo * sizeof(iteminfo_t));
	ic->iteminfo = (iteminfo_t*)((char*)ic + sizeof(itemconfig_t));
	ic->numiteminfo = 0;
	//parse the item config file
	token_t token;
	while (PC_ReadToken(source, &token))
	{
		if (!String::Cmp(token.string, "iteminfo"))
		{
			if (ic->numiteminfo >= max_iteminfo)
			{
				SourceError(source, "more than %d item info defined\n", max_iteminfo);
				Mem_Free(ic);
				FreeSource(source);
				return NULL;
			}
			iteminfo_t* ii = &ic->iteminfo[ic->numiteminfo];
			Com_Memset(ii, 0, sizeof(iteminfo_t));
			if (!PC_ExpectTokenType(source, TT_STRING, 0, &token))
			{
				Mem_Free(ic);
				FreeSource(source);
				return NULL;
			}
			StripDoubleQuotes(token.string);
			String::NCpy(ii->classname, token.string, sizeof(ii->classname) - 1);
			if (!ReadStructure(source, &iteminfo_struct, (char*)ii))
			{
				Mem_Free(ic);
				FreeSource(source);
				return NULL;
			}
			ii->number = ic->numiteminfo;
			ic->numiteminfo++;
		}
		else
		{
			SourceError(source, "unknown definition %s\n", token.string);
			Mem_Free(ic);
			FreeSource(source);
			return NULL;
		}
	}
	FreeSource(source);

	if (!ic->numiteminfo)
	{
		BotImport_Print(PRT_WARNING, "no item info loaded\n");
	}
	BotImport_Print(PRT_MESSAGE, "loaded %s\n", path);
	return ic;
}

// index to find the weight function of an iteminfo
static int* ItemWeightIndex(weightconfig_t* iwc, itemconfig_t* ic)
{
	//initialize item weight index
	int* index = (int*)Mem_ClearedAlloc(sizeof(int) * ic->numiteminfo);

	for (int i = 0; i < ic->numiteminfo; i++)
	{
		index[i] = FindFuzzyWeight(iwc, ic->iteminfo[i].classname);
		if (index[i] < 0)
		{
			Log_Write("item info %d \"%s\" has no fuzzy weight\r\n", i, ic->iteminfo[i].classname);
		}	//end if
	}	//end for
	return index;
}

void InitLevelItemHeap()
{
	if (levelitemheap)
	{
		Mem_Free(levelitemheap);
	}

	int max_levelitems = (int)LibVarValue("max_levelitems", "256");
	levelitemheap = (levelitem_t*)Mem_ClearedAlloc(max_levelitems * sizeof(levelitem_t));

	for (int i = 0; i < max_levelitems - 1; i++)
	{
		levelitemheap[i].next = &levelitemheap[i + 1];
	}
	levelitemheap[max_levelitems - 1].next = NULL;
	freelevelitems = levelitemheap;
}

levelitem_t* AllocLevelItem()
{
	levelitem_t* li = freelevelitems;
	if (!li)
	{
		BotImport_Print(PRT_FATAL, "out of level items\n");
		return NULL;
	}

	freelevelitems = freelevelitems->next;
	Com_Memset(li, 0, sizeof(levelitem_t));
	return li;
}

void FreeLevelItem(levelitem_t* li)
{
	li->next = freelevelitems;
	freelevelitems = li;
}

void AddLevelItemToList(levelitem_t* li)
{
	if (levelitems)
	{
		levelitems->prev = li;
	}
	li->prev = NULL;
	li->next = levelitems;
	levelitems = li;
}

void RemoveLevelItemFromList(levelitem_t* li)
{
	if (li->prev)
	{
		li->prev->next = li->next;
	}
	else
	{
		levelitems = li->next;
	}
	if (li->next)
	{
		li->next->prev = li->prev;
	}
}

void BotFreeInfoEntities()
{
	maplocation_t* nextml;
	for (maplocation_t* ml = maplocations; ml; ml = nextml)
	{
		nextml = ml->next;
		Mem_Free(ml);
	}
	maplocations = NULL;
	campspot_t* nextcs;
	for (campspot_t* cs = campspots; cs; cs = nextcs)
	{
		nextcs = cs->next;
		Mem_Free(cs);
	}
	campspots = NULL;
}

void BotGoalName(int number, char* name, int size)
{
	if (!itemconfig)
	{
		return;
	}

	for (levelitem_t* li = levelitems; li; li = li->next)
	{
		if (li->number == number)
		{
			String::NCpy(name, itemconfig->iteminfo[li->iteminfo].name, size - 1);
			name[size - 1] = '\0';
			return;
		}
	}
	String::Cpy(name, "");
	return;
}

void BotResetAvoidGoals(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	Com_Memset(gs->avoidgoals, 0, MAX_AVOIDGOALS * sizeof(int));
	Com_Memset(gs->avoidgoaltimes, 0, MAX_AVOIDGOALS * sizeof(float));
}

int BotGetLevelItemGoalQ3(int index, const char* name, bot_goal_q3_t* goal)
{
	if (!itemconfig)
	{
		return -1;
	}
	levelitem_t* li = levelitems;
	if (index >= 0)
	{
		for (; li; li = li->next)
		{
			if (li->number == index)
			{
				li = li->next;
				break;
			}
		}
	}
	for (; li; li = li->next)
	{
		if (GGameType & GAME_WolfSP)
		{
			if (g_gametype == WSGT_SINGLE_PLAYER)
			{
				if (li->flags & IFL_NOTSINGLE)
				{
					continue;
				}
			}
			else if (g_gametype >= WSGT_TEAM)
			{
				if (li->flags & IFL_NOTTEAM)
				{
					continue;
				}
			}
			else
			{
				if (li->flags & IFL_NOTFREE)
				{
					continue;
				}
			}
		}
		else if (GGameType & GAME_WolfMP)
		{
			if (g_gametype == WMGT_SINGLE_PLAYER)
			{
				if (li->flags & IFL_NOTSINGLE)
				{
					continue;
				}
			}
			else if (g_gametype >= WMGT_TEAM)
			{
				if (li->flags & IFL_NOTTEAM)
				{
					continue;
				}
			}
			else
			{
				if (li->flags & IFL_NOTFREE)
				{
					continue;
				}
			}
		}
		else
		{
			if (g_gametype == Q3GT_SINGLE_PLAYER)
			{
				if (li->flags & IFL_NOTSINGLE)
				{
					continue;
				}
			}
			else if (g_gametype >= Q3GT_TEAM)
			{
				if (li->flags & IFL_NOTTEAM)
				{
					continue;
				}
			}
			else
			{
				if (li->flags & IFL_NOTFREE)
				{
					continue;
				}
			}
			if (li->flags & IFL_NOTBOT)
			{
				continue;
			}
		}

		if (!String::ICmp(name, itemconfig->iteminfo[li->iteminfo].name))
		{
			goal->areanum = li->goalareanum;
			VectorCopy(li->goalorigin, goal->origin);
			goal->entitynum = li->entitynum;
			VectorCopy(itemconfig->iteminfo[li->iteminfo].mins, goal->mins);
			VectorCopy(itemconfig->iteminfo[li->iteminfo].maxs, goal->maxs);
			goal->number = li->number;
			if (GGameType & GAME_Quake3)
			{
				goal->flags = GFL_ITEM;
				if (li->timeout)
				{
					goal->flags |= GFL_DROPPED;
				}
			}
			return li->number;
		}
	}
	return -1;
}

int BotGetLevelItemGoalET(int index, const char* name, bot_goal_et_t* goal)
{
	if (!itemconfig)
	{
		return -1;
	}
	for (levelitem_t* li = levelitems; li; li = li->next)
	{
		if (li->number <= index)
		{
			continue;
		}

		if (g_singleplayer)
		{
			if (li->flags & IFL_NOTSINGLE)
			{
				continue;
			}
		}

		if (!String::ICmp(name, itemconfig->iteminfo[li->iteminfo].name))
		{
			goal->areanum = li->goalareanum;
			VectorCopy(li->goalorigin, goal->origin);
			goal->entitynum = li->entitynum;
			VectorCopy(itemconfig->iteminfo[li->iteminfo].mins, goal->mins);
			VectorCopy(itemconfig->iteminfo[li->iteminfo].maxs, goal->maxs);
			goal->number = li->number;
			return li->number;
		}
	}
	return -1;
}

static bool BotGetMapLocationGoal(const char* name, bot_goal_t* goal)
{
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};
	for (maplocation_t* ml = maplocations; ml; ml = ml->next)
	{
		if (!String::ICmp(ml->name, name))
		{
			goal->areanum = ml->areanum;
			VectorCopy(ml->origin, goal->origin);
			goal->entitynum = 0;
			VectorCopy(mins, goal->mins);
			VectorCopy(maxs, goal->maxs);
			return true;
		}
	}
	return false;
}

bool BotGetMapLocationGoalQ3(const char* name, bot_goal_q3_t* goal)
{
	return BotGetMapLocationGoal(name, reinterpret_cast<bot_goal_t*>(goal));
}

bool BotGetMapLocationGoalET(const char* name, bot_goal_et_t* goal)
{
	return BotGetMapLocationGoal(name, reinterpret_cast<bot_goal_t*>(goal));
}

static int BotGetNextCampSpotGoal(int num, bot_goal_t* goal)
{
	vec3_t mins = {-8, -8, -8}, maxs = {8, 8, 8};
	if (num < 0)
	{
		num = 0;
	}
	int i = num;
	for (campspot_t* cs = campspots; cs; cs = cs->next)
	{
		if (--i < 0)
		{
			goal->areanum = cs->areanum;
			VectorCopy(cs->origin, goal->origin);
			goal->entitynum = 0;
			VectorCopy(mins, goal->mins);
			VectorCopy(maxs, goal->maxs);
			return num + 1;
		}
	}
	return 0;
}

int BotGetNextCampSpotGoalQ3(int num, bot_goal_q3_t* goal)
{
	return BotGetNextCampSpotGoal(num, reinterpret_cast<bot_goal_t*>(goal));
}

int BotGetNextCampSpotGoalET(int num, bot_goal_et_t* goal)
{
	return BotGetNextCampSpotGoal(num, reinterpret_cast<bot_goal_t*>(goal));
}

void BotDumpGoalStack(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	for (int i = 1; i <= gs->goalstacktop; i++)
	{
		char name[32];
		BotGoalName(gs->goalstack[i].number, name, 32);
		Log_Write("%d: %s", i, name);
	}
}

void BotPushGoalQ3(int goalstate, bot_goal_q3_t* goal)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	if (gs->goalstacktop >= MAX_GOALSTACK - 1)
	{
		BotImport_Print(PRT_ERROR, "goal heap overflow\n");
		BotDumpGoalStack(goalstate);
		return;
	}
	gs->goalstacktop++;
	Com_Memcpy(&gs->goalstack[gs->goalstacktop], goal, sizeof(bot_goal_q3_t));
}

void BotPushGoalET(int goalstate, bot_goal_et_t* goal)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	if (gs->goalstacktop >= MAX_GOALSTACK - 1)
	{
		BotImport_Print(PRT_ERROR, "goal heap overflow\n");
		BotDumpGoalStack(goalstate);
		return;
	}
	gs->goalstacktop++;
	Com_Memcpy(&gs->goalstack[gs->goalstacktop], goal, sizeof(bot_goal_et_t));
}

void BotPopGoal(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	if (gs->goalstacktop > 0)
	{
		gs->goalstacktop--;
	}
}

void BotEmptyGoalStack(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	gs->goalstacktop = 0;
}

int BotGetTopGoalQ3(int goalstate, bot_goal_q3_t* goal)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (!gs->goalstacktop)
	{
		return false;
	}
	Com_Memcpy(goal, &gs->goalstack[gs->goalstacktop], sizeof(bot_goal_q3_t));
	return true;
}

int BotGetTopGoalET(int goalstate, bot_goal_et_t* goal)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (!gs->goalstacktop)
	{
		return false;
	}
	memcpy(goal, &gs->goalstack[gs->goalstacktop], sizeof(bot_goal_et_t));
	return true;
}

int BotGetSecondGoalQ3(int goalstate, bot_goal_q3_t* goal)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (gs->goalstacktop <= 1)
	{
		return false;
	}
	Com_Memcpy(goal, &gs->goalstack[gs->goalstacktop - 1], sizeof(bot_goal_q3_t));
	return true;
}

int BotGetSecondGoalET(int goalstate, bot_goal_et_t* goal)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (gs->goalstacktop <= 1)
	{
		return false;
	}
	Com_Memcpy(goal, &gs->goalstack[gs->goalstacktop - 1], sizeof(bot_goal_et_t));
	return true;
}

void BotResetGoalState(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	Com_Memset(gs->goalstack, 0, MAX_GOALSTACK * sizeof(bot_goal_t));
	gs->goalstacktop = 0;
	BotResetAvoidGoals(goalstate);
}

int BotLoadItemWeights(int goalstate, const char* filename)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADITEMWEIGHTS : WOLFBLERR_CANNOTLOADITEMWEIGHTS;
	}
	//load the weight configuration
	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	gs->itemweightconfig = ReadWeightConfig(filename);
	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder("");
	}
	if (!gs->itemweightconfig)
	{
		BotImport_Print(PRT_FATAL, "couldn't load weights\n");
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADITEMWEIGHTS : WOLFBLERR_CANNOTLOADITEMWEIGHTS;
	}
	//if there's no item configuration
	if (!itemconfig)
	{
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADITEMWEIGHTS : WOLFBLERR_CANNOTLOADITEMWEIGHTS;
	}
	//create the item weight index
	gs->itemweightindex = ItemWeightIndex(gs->itemweightconfig, itemconfig);
	//everything went ok
	return BLERR_NOERROR;
}

void BotFreeItemWeights(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	if (gs->itemweightconfig)
	{
		FreeWeightConfig(gs->itemweightconfig);
	}
	if (gs->itemweightindex)
	{
		Mem_Free(gs->itemweightindex);
	}
}

int BotAllocGoalState(int client)
{
	for (int i = 1; i <= MAX_BOTLIB_CLIENTS; i++)
	{
		if (!botgoalstates[i])
		{
			botgoalstates[i] = (bot_goalstate_t*)Mem_ClearedAlloc(sizeof(bot_goalstate_t));
			botgoalstates[i]->client = client;
			return i;
		}
	}
	return 0;
}

void BotFreeGoalState(int handle)
{
	if (handle <= 0 || handle > MAX_BOTLIB_CLIENTS)
	{
		BotImport_Print(PRT_FATAL, "goal state handle %d out of range\n", handle);
		return;
	}
	if (!botgoalstates[handle])
	{
		BotImport_Print(PRT_FATAL, "invalid goal state handle %d\n", handle);
		return;
	}
	BotFreeItemWeights(handle);
	Mem_Free(botgoalstates[handle]);
	botgoalstates[handle] = NULL;
}

int BotSetupGoalAI(bool singleplayer)
{
	if (GGameType & GAME_ET)
	{
		g_singleplayer = singleplayer;
	}
	else
	{
		//check if teamplay is on
		g_gametype = LibVarValue("g_gametype", "0");
	}
	//item configuration file
	const char* filename = LibVarString("itemconfig", "items.c");
	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder(BOTFILESBASEFOLDER);
	}
	//load the item configuration
	itemconfig = LoadItemConfig(filename);
	if (GGameType & GAME_ET)
	{
		PS_SetBaseFolder("");
	}
	if (!itemconfig)
	{
		BotImport_Print(PRT_FATAL, "couldn't load item config\n");
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTLOADITEMCONFIG : WOLFBLERR_CANNOTLOADITEMCONFIG;
	}

	if (GGameType & GAME_Quake3)
	{
		droppedweight = LibVar("droppedweight", "1000");
	}
	//everything went ok
	return BLERR_NOERROR;
}

void BotShutdownGoalAI()
{
	if (itemconfig)
	{
		Mem_Free(itemconfig);
	}
	itemconfig = NULL;
	if (levelitemheap)
	{
		Mem_Free(levelitemheap);
	}
	levelitemheap = NULL;
	freelevelitems = NULL;
	levelitems = NULL;
	numlevelitems = 0;

	BotFreeInfoEntities();

	for (int i = 1; i <= MAX_CLIENTS_Q3; i++)
	{
		if (botgoalstates[i])
		{
			BotFreeGoalState(i);
		}
	}
}
