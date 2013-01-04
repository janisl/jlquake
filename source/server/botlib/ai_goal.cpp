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

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/content_types.h"
#include "local.h"
#include "ai_weight.h"
#include "../quake3serverdefs.h"

//minimum avoid goal time
#define AVOID_MINIMUM_TIME      10
//default avoid goal time
#define AVOID_DEFAULT_TIME      30
//avoid dropped goal time
#define AVOID_DROPPED_TIME      10
//avoid dropped goal time
#define AVOID_DROPPED_TIME_WOLF 5
#define TRAVELTIME_SCALE        0.01

//item flags
#define IFL_NOTFREE             1		//not in free for all
#define IFL_NOTTEAM             2		//not in team play
#define IFL_NOTSINGLE           4		//not in single player
#define IFL_NOTBOT              8		//bot should never go for this
#define IFL_ROAM                16		//bot roam goal

#define MAX_AVOIDGOALS          256
#define MAX_GOALSTACK           8

struct levelitem_t
{
	int number;							//number of the level item
	int iteminfo;						//index into the item info
	int flags;							//item flags
	float weight;						//fixed roam weight
	vec3_t origin;						//origin of the item
	int goalareanum;					//area the item is in
	vec3_t goalorigin;					//goal origin within the area
	int entitynum;						//entity number
	float timeout;						//item is removed after this time
	levelitem_t* prev;
	levelitem_t* next;
};

struct iteminfo_t
{
	char classname[32];					//classname of the item
	char name[MAX_STRINGFIELD];			//name of the item
	char model[MAX_STRINGFIELD];		//model of the item
	int modelindex;						//model index
	int type;							//item type
	int index;							//index in the inventory
	float respawntime;					//respawn time
	vec3_t mins;						//mins of the item
	vec3_t maxs;						//maxs of the item
	int number;							//number of the item info
};

struct itemconfig_t
{
	int numiteminfo;
	iteminfo_t* iteminfo;
};

//goal state
struct bot_goalstate_t
{
	weightconfig_t* itemweightconfig;		//weight config
	int* itemweightindex;					//index from item to weight

	int client;								//client using this goal state
	int lastreachabilityarea;				//last area with reachabilities the bot was in

	bot_goal_t goalstack[MAX_GOALSTACK];	//goal stack
	int goalstacktop;						//the top of the goal stack

	int avoidgoals[MAX_AVOIDGOALS];			//goals to avoid
	float avoidgoaltimes[MAX_AVOIDGOALS];	//times to avoid the goals
};

//location in the map "target_location"
struct maplocation_t
{
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	maplocation_t* next;
};

//camp spots "info_camp"
struct campspot_t
{
	vec3_t origin;
	int areanum;
	char name[MAX_EPAIRKEY];
	float range;
	float weight;
	float wait;
	float random;
	campspot_t* next;
};

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
static itemconfig_t* itemconfig = NULL;
//level items
static levelitem_t* levelitemheap = NULL;
static levelitem_t* freelevelitems = NULL;
static levelitem_t* levelitems = NULL;
static int numlevelitems = 0;
//map locations
static maplocation_t* maplocations = NULL;
//camp spots
static campspot_t* campspots = NULL;
//the game type
static int g_gametype = 0;
static bool g_singleplayer;
//additional dropped item weight
static libvar_t* droppedweight = NULL;

static bot_goalstate_t* BotGoalStateFromHandle(int handle)
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

static void InitLevelItemHeap()
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

static levelitem_t* AllocLevelItem()
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

static void FreeLevelItem(levelitem_t* li)
{
	li->next = freelevelitems;
	freelevelitems = li;
}

static void AddLevelItemToList(levelitem_t* li)
{
	if (levelitems)
	{
		levelitems->prev = li;
	}
	li->prev = NULL;
	li->next = levelitems;
	levelitems = li;
}

static void RemoveLevelItemFromList(levelitem_t* li)
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

static void BotFreeInfoEntities()
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

void BotDumpAvoidGoals(int goalstate)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	for (int i = 0; i < MAX_AVOIDGOALS; i++)
	{
		if (gs->avoidgoaltimes[i] >= AAS_Time())
		{
			char name[32];
			BotGoalName(gs->avoidgoals[i], name, 32);
			Log_Write("avoid goal %s, number %d for %f seconds", name,
				gs->avoidgoals[i], gs->avoidgoaltimes[i] - AAS_Time());
		}
	}
}

static void BotAddToAvoidGoals(bot_goalstate_t* gs, int number, float avoidtime)
{
	for (int i = 0; i < MAX_AVOIDGOALS; i++)
	{
		//if the avoid goal is already stored
		if (gs->avoidgoals[i] == number)
		{
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = AAS_Time() + avoidtime;
			return;
		}
	}

	for (int i = 0; i < MAX_AVOIDGOALS; i++)
	{
		//if this avoid goal has expired
		if (gs->avoidgoaltimes[i] < AAS_Time())
		{
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = AAS_Time() + avoidtime;
			return;
		}
	}
}

void BotRemoveFromAvoidGoals(int goalstate, int number)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	for (int i = 0; i < MAX_AVOIDGOALS; i++)
	{
		if (gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= AAS_Time())
		{
			gs->avoidgoaltimes[i] = 0;
			return;
		}
	}
}

float BotAvoidGoalTime(int goalstate, int number)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return 0;
	}
	for (int i = 0; i < MAX_AVOIDGOALS; i++)
	{
		if (gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= AAS_Time())
		{
			return gs->avoidgoaltimes[i] - AAS_Time();
		}
	}
	return 0;
}

void BotSetAvoidGoalTime(int goalstate, int number, float avoidtime)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return;
	}
	if (avoidtime < 0)
	{
		if (!itemconfig)
		{
			return;
		}

		for (levelitem_t* li = levelitems; li; li = li->next)
		{
			if (li->number == number)
			{
				avoidtime = itemconfig->iteminfo[li->iteminfo].respawntime;
				if (!avoidtime)
				{
					avoidtime = AVOID_DEFAULT_TIME;
				}
				if (avoidtime < AVOID_MINIMUM_TIME)
				{
					avoidtime = AVOID_MINIMUM_TIME;
				}
				BotAddToAvoidGoals(gs, number, avoidtime);
				return;
			}
		}
		return;
	}
	else
	{
		BotAddToAvoidGoals(gs, number, avoidtime);
	}
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
		}
		else if (GGameType & GAME_WolfMP)
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

static void BotPushGoal(int goalstate, const bot_goal_t* goal, size_t size)
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
	Com_Memcpy(&gs->goalstack[gs->goalstacktop], goal, size);
}

void BotPushGoalQ3(int goalstate, const bot_goal_q3_t* goal)
{
	BotPushGoal(goalstate, reinterpret_cast<const bot_goal_t*>(goal), sizeof(bot_goal_q3_t));
}

void BotPushGoalET(int goalstate, const bot_goal_et_t* goal)
{
	BotPushGoal(goalstate, reinterpret_cast<const bot_goal_t*>(goal), sizeof(bot_goal_et_t));
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

static void BotInitInfoEntities()
{
	BotFreeInfoEntities();

	int numlocations = 0;
	int numcampspots = 0;
	for (int ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		char classname[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}

		//map locations
		if (!String::Cmp(classname, "target_location"))
		{
			maplocation_t* ml = (maplocation_t*)Mem_ClearedAlloc(sizeof(maplocation_t));
			AAS_VectorForBSPEpairKey(ent, "origin", ml->origin);
			AAS_ValueForBSPEpairKey(ent, "message", ml->name, sizeof(ml->name));
			ml->areanum = AAS_PointAreaNum(ml->origin);
			ml->next = maplocations;
			maplocations = ml;
			numlocations++;
		}
		//camp spots
		else if (!String::Cmp(classname, "info_camp"))
		{
			campspot_t* cs = (campspot_t*)Mem_ClearedAlloc(sizeof(campspot_t));
			AAS_VectorForBSPEpairKey(ent, "origin", cs->origin);
			AAS_ValueForBSPEpairKey(ent, "message", cs->name, sizeof(cs->name));
			AAS_FloatForBSPEpairKey(ent, "range", &cs->range);
			AAS_FloatForBSPEpairKey(ent, "weight", &cs->weight);
			AAS_FloatForBSPEpairKey(ent, "wait", &cs->wait);
			AAS_FloatForBSPEpairKey(ent, "random", &cs->random);
			cs->areanum = AAS_PointAreaNum(cs->origin);
			if (!cs->areanum)
			{
				BotImport_Print(PRT_MESSAGE, "camp spot at %1.1f %1.1f %1.1f in solid\n", cs->origin[0], cs->origin[1], cs->origin[2]);
				Mem_Free(cs);
				continue;
			}
			cs->next = campspots;
			campspots = cs;
			numcampspots++;
		}
	}
	if (bot_developer)
	{
		BotImport_Print(PRT_MESSAGE, "%d map locations\n", numlocations);
		BotImport_Print(PRT_MESSAGE, "%d camp spots\n", numcampspots);
	}
}

void BotInitLevelItems()
{
	//initialize the map locations and camp spots
	BotInitInfoEntities();

	//initialize the level item heap
	InitLevelItemHeap();
	levelitems = NULL;
	numlevelitems = 0;

	itemconfig_t* ic = itemconfig;
	if (!ic)
	{
		return;
	}

	//if there's no AAS file loaded
	if (!AAS_Loaded())
	{
		return;
	}

	//update the modelindexes of the item info
	for (int i = 0; i < ic->numiteminfo; i++)
	{
		if (!ic->iteminfo[i].modelindex)
		{
			Log_Write("item %s has modelindex 0", ic->iteminfo[i].classname);
		}
	}

	for (int ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	{
		char classname[MAX_EPAIRKEY];
		if (!AAS_ValueForBSPEpairKey(ent, "classname", classname, MAX_EPAIRKEY))
		{
			continue;
		}

		int spawnflags = 0;
		AAS_IntForBSPEpairKey(ent, "spawnflags", &spawnflags);

		int i;
		for (i = 0; i < ic->numiteminfo; i++)
		{
			if (!String::Cmp(classname, ic->iteminfo[i].classname))
			{
				break;
			}
		}
		if (i >= ic->numiteminfo)
		{
			Log_Write("entity %s unknown item\r\n", classname);
			continue;
		}
		//get the origin of the item
		vec3_t origin;
		if (!AAS_VectorForBSPEpairKey(ent, "origin", origin))
		{
			BotImport_Print(PRT_ERROR, "item %s without origin\n", classname);
			continue;
		}

		int goalareanum = 0;
		//if it is a floating item
		if (spawnflags & 1)
		{
			if (!(GGameType & GAME_Quake3))
			{
				continue;
			}
			//if the item is not floating in water
			if (!(AAS_PointContents(origin) & BSP46CONTENTS_WATER))
			{
				vec3_t end;
				VectorCopy(origin, end);
				end[2] -= 32;
				bsp_trace_t trace = AAS_Trace(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, end, -1, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
				//if the item not near the ground
				if (trace.fraction >= 1)
				{
					//if the item is not reachable from a jumppad
					goalareanum = AAS_BestReachableFromJumpPadArea(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs);
					Log_Write("item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
					if (!goalareanum)
					{
						continue;
					}
				}
			}
		}

		levelitem_t* li = AllocLevelItem();
		if (!li)
		{
			return;
		}

		li->number = ++numlevelitems;
		li->timeout = 0;
		li->entitynum = 0;

		li->flags = 0;
		int value;
		AAS_IntForBSPEpairKey(ent, "notfree", &value);
		if (value)
		{
			li->flags |= IFL_NOTFREE;
		}
		AAS_IntForBSPEpairKey(ent, "notteam", &value);
		if (value)
		{
			li->flags |= IFL_NOTTEAM;
		}
		AAS_IntForBSPEpairKey(ent, "notsingle", &value);
		if (value)
		{
			li->flags |= IFL_NOTSINGLE;
		}
		if (GGameType & GAME_Quake3)
		{
			AAS_IntForBSPEpairKey(ent, "notbot", &value);
			if (value)
			{
				li->flags |= IFL_NOTBOT;
			}
			if (!String::Cmp(classname, "item_botroam"))
			{
				li->flags |= IFL_ROAM;
				AAS_FloatForBSPEpairKey(ent, "weight", &li->weight);
			}
		}
		//if not a stationary item
		if (!(spawnflags & 1))
		{
			if (!AAS_DropToFloor(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs))
			{
				BotImport_Print(PRT_MESSAGE, "%s in solid at (%1.1f %1.1f %1.1f)\n",
					classname, origin[0], origin[1], origin[2]);
			}
		}
		//item info of the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy(origin, li->origin);

		if (goalareanum)
		{
			li->goalareanum = goalareanum;
			VectorCopy(origin, li->goalorigin);
		}
		else
		{
			//get the item goal area and goal origin
			li->goalareanum = AAS_BestReachableArea(origin,
				ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
				li->goalorigin);
			if (!li->goalareanum)
			{
				BotImport_Print(PRT_MESSAGE, "%s not reachable for bots at (%1.1f %1.1f %1.1f)\n",
					classname, origin[0], origin[1], origin[2]);
			}
		}

		AddLevelItemToList(li);
	}
	BotImport_Print(PRT_MESSAGE, "found %d level items\n", numlevelitems);
}

void BotUpdateEntityItems()
{
	int ent, i, modelindex;
	vec3_t dir;
	levelitem_t* li, * nextli;
	aas_entityinfo_t entinfo;
	itemconfig_t* ic;

	//timeout current entity items if necessary
	for (li = levelitems; li; li = nextli)
	{
		nextli = li->next;
		//if it is a item that will time out
		if (li->timeout)
		{
			//timeout the item
			if (li->timeout < AAS_Time())
			{
				RemoveLevelItemFromList(li);
				FreeLevelItem(li);
			}	//end if
		}	//end if
	}	//end for
		//find new entity items
	ic = itemconfig;
	if (!itemconfig)
	{
		return;
	}
	//
	for (ent = AAS_NextEntity(0); ent; ent = AAS_NextEntity(ent))
	{
		if (AAS_EntityType(ent) != Q3ET_ITEM)
		{
			continue;
		}
		//get the model index of the entity
		modelindex = AAS_EntityModelindex(ent);
		//
		if (!modelindex)
		{
			continue;
		}
		//get info about the entity
		AAS_EntityInfo(ent, &entinfo);
		//FIXME: don't do this
		//skip all floating items for now
		if (!(GGameType & GAME_Quake3) && entinfo.groundent != Q3ENTITYNUM_WORLD)
		{
			continue;
		}
		//if the entity is still moving
		if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
			entinfo.origin[1] != entinfo.lastvisorigin[1] ||
			entinfo.origin[2] != entinfo.lastvisorigin[2])
		{
			continue;
		}
		//check if the entity is already stored as a level item
		for (li = levelitems; li; li = li->next)
		{
			if (GGameType & GAME_Quake3)
			{
				//if the level item is linked to an entity
				if (li->entitynum && li->entitynum == ent)
				{
					//the entity is re-used if the models are different
					if (ic->iteminfo[li->iteminfo].modelindex != modelindex)
					{
						//remove this level item
						RemoveLevelItemFromList(li);
						FreeLevelItem(li);
						li = NULL;
						break;
					}
					else
					{
						if (entinfo.origin[0] != li->origin[0] ||
							entinfo.origin[1] != li->origin[1] ||
							entinfo.origin[2] != li->origin[2])
						{
							VectorCopy(entinfo.origin, li->origin);
							//also update the goal area number
							li->goalareanum = AAS_BestReachableArea(li->origin,
								ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
								li->goalorigin);
						}
						break;
					}
				}
			}
			else
			{
				//if the model of the level item and the entity are different
				if (ic->iteminfo[li->iteminfo].modelindex != modelindex)
				{
					continue;
				}
				//if the level item is linked to an entity
				if (li->entitynum)
				{
					if (li->entitynum == ent)
					{
						VectorCopy(entinfo.origin, li->origin);
						break;
					}	//end if
				}	//end if
				else
				{
					//check if the entity is very close
					VectorSubtract(li->origin, entinfo.origin, dir);
					if (VectorLength(dir) < 30)
					{
						//found an entity for this level item
						li->entitynum = ent;
						//keep updating the entity origin
						VectorCopy(entinfo.origin, li->origin);
						//also update the goal area number
						li->goalareanum = AAS_BestReachableArea(li->origin,
							ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
							li->goalorigin);
						//Log_Write("found item %s entity", ic->iteminfo[li->iteminfo].classname);
						break;
					}	//end if
						//else BotImport_Print(PRT_MESSAGE, "item %s has no attached entity\n",
						//						ic->iteminfo[li->iteminfo].name);
				}	//end else
			}
		}	//end for
		if (li)
		{
			continue;
		}
		if (GGameType & GAME_Quake3)
		{
			//try to link the entity to a level item
			for (li = levelitems; li; li = li->next)
			{
				//if this level item is already linked
				if (li->entitynum)
				{
					continue;
				}

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
				//if the model of the level item and the entity are the same
				if (ic->iteminfo[li->iteminfo].modelindex == modelindex)
				{
					//check if the entity is very close
					VectorSubtract(li->origin, entinfo.origin, dir);
					if (VectorLength(dir) < 30)
					{
						//found an entity for this level item
						li->entitynum = ent;
						//if the origin is different
						if (entinfo.origin[0] != li->origin[0] ||
							entinfo.origin[1] != li->origin[1] ||
							entinfo.origin[2] != li->origin[2])
						{
							//update the level item origin
							VectorCopy(entinfo.origin, li->origin);
							//also update the goal area number
							li->goalareanum = AAS_BestReachableArea(li->origin,
								ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
								li->goalorigin);
						}
#ifdef DEBUG
						Log_Write("linked item %s to an entity", ic->iteminfo[li->iteminfo].classname);
#endif
						break;
					}
				}
			}
			if (li)
			{
				continue;
			}
		}
		//check if the model is from a known item
		for (i = 0; i < ic->numiteminfo; i++)
		{
			if (ic->iteminfo[i].modelindex == modelindex)
			{
				break;
			}	//end if
		}	//end for
			//if the model is not from a known item
		if (i >= ic->numiteminfo)
		{
			continue;
		}
		//allocate a new level item
		li = AllocLevelItem();
		//
		if (!li)
		{
			continue;
		}
		//entity number of the level item
		li->entitynum = ent;
		//number for the level item
		li->number = numlevelitems + ent;
		//set the item info index for the level item
		li->iteminfo = i;
		//origin of the item
		VectorCopy(entinfo.origin, li->origin);
		//get the item goal area and goal origin
		li->goalareanum = AAS_BestReachableArea(li->origin,
			ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
			li->goalorigin);
		//never go for items dropped into jumppads
		if (AAS_AreaJumpPad(li->goalareanum))
		{
			FreeLevelItem(li);
			continue;
		}	//end if
			//time this item out after 30 seconds
			//dropped items disappear after 30 seconds
		li->timeout = AAS_Time() + 30;
		//add the level item to the list
		AddLevelItemToList(li);
	}
}

static bool BotTouchingGoal(const vec3_t origin, const bot_goal_t* goal)
{
	int i;
	vec3_t boxmins, boxmaxs;
	vec3_t absmins, absmaxs;
	vec3_t safety_maxs = {0, 0, 0};	//{4, 4, 10};
	vec3_t safety_mins = {0, 0, 0};	//{-4, -4, 0};

	AAS_PresenceTypeBoundingBox(PRESENCE_NORMAL, boxmins, boxmaxs);
	VectorSubtract(goal->mins, boxmaxs, absmins);
	VectorSubtract(goal->maxs, boxmins, absmaxs);
	VectorAdd(absmins, goal->origin, absmins);
	VectorAdd(absmaxs, goal->origin, absmaxs);
	//make the box a little smaller for safety
	VectorSubtract(absmaxs, safety_maxs, absmaxs);
	VectorSubtract(absmins, safety_mins, absmins);

	for (i = 0; i < 3; i++)
	{
		if (origin[i] < absmins[i] || origin[i] > absmaxs[i])
		{
			return false;
		}
	}
	return true;
}

bool BotTouchingGoalQ3(const vec3_t origin, const bot_goal_q3_t* goal)
{
	return BotTouchingGoal(origin, reinterpret_cast<const bot_goal_t*>(goal));
}

bool BotTouchingGoalET(const vec3_t origin, const bot_goal_et_t* goal)
{
	return BotTouchingGoal(origin, reinterpret_cast<const bot_goal_t*>(goal));
}

// pops a new long term goal on the goal stack in the goalstate
bool BotChooseLTGItem(int goalstate, const vec3_t origin, const int* inventory, int travelflags)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (!gs->itemweightconfig)
	{
		return false;
	}
	//get the area the bot is in
	int areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	if (!areanum || !AAS_AreaReachability(areanum))
	{
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	}
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if (!areanum)
	{
		return false;
	}
	//the item configuration
	itemconfig_t* ic = itemconfig;
	if (!itemconfig)
	{
		return false;
	}
	//best weight and item so far
	float bestweight = 0;
	levelitem_t* bestitem = NULL;
	//go through the items in the level
	for (levelitem_t* li = levelitems; li; li = li->next)
	{
		if (GGameType & GAME_ET)
		{
			if (g_singleplayer)
			{
				if (li->flags & IFL_NOTSINGLE)
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
		}
		if (GGameType & GAME_Quake3 && li->flags & IFL_NOTBOT)
		{
			continue;
		}
		//if the item is not in a possible goal area
		if (!li->goalareanum)
		{
			continue;
		}
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if (GGameType & GAME_Quake3 && !li->entitynum && !(li->flags & IFL_ROAM))
		{
			continue;
		}
		//get the fuzzy weight function for this item
		iteminfo_t* iteminfo = &ic->iteminfo[li->iteminfo];
		int weightnum = gs->itemweightindex[iteminfo->number];
		if (weightnum < 0)
		{
			continue;
		}
		//if this goal is in the avoid goals
		if (!(GGameType & GAME_Quake3) && BotAvoidGoalTime(goalstate, li->number) > 0)
		{
			continue;
		}

		float weight = FuzzyWeightUndecided(inventory, gs->itemweightconfig, weightnum);
		//HACK: to make dropped items more attractive
		if (li->timeout)
		{
			weight += GGameType & GAME_Quake3 ? droppedweight->value : 1000;
		}
		//use weight scale for item_botroam
		if (GGameType & GAME_Quake3 && li->flags & IFL_ROAM)
		{
			weight *= li->weight;
		}

		if (weight > 0)
		{
			//get the travel time towards the goal area
			int t = AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			//if the goal is reachable
			if (t > 0)
			{
				if (GGameType & GAME_Quake3)
				{
					//if this item won't respawn before we get there
					float avoidtime = BotAvoidGoalTime(goalstate, li->number);
					if (avoidtime - t * 0.009 > 0)
					{
						continue;
					}
				}

				weight /= (float)t * TRAVELTIME_SCALE;
				//
				if (weight > bestweight)
				{
					bestweight = weight;
					bestitem = li;
				}
			}
		}
	}
	//if no goal item found
	if (!bestitem)
	{
		return false;
	}
	//create a bot goal for this item
	bot_goal_t goal;
	Com_Memset(&goal, 0, sizeof(bot_goal_t));
	iteminfo_t* iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy(bestitem->goalorigin, goal.origin);
	VectorCopy(iteminfo->mins, goal.mins);
	VectorCopy(iteminfo->maxs, goal.maxs);
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if (GGameType & GAME_Quake3)
	{
		if (bestitem->timeout)
		{
			goal.flags |= GFL_DROPPED;
		}
		if (bestitem->flags & IFL_ROAM)
		{
			goal.flags |= GFL_ROAM;
		}
	}
	goal.iteminfo = bestitem->iteminfo;
	float avoidtime;
	if (GGameType & GAME_Quake3)
	{
		//if it's a dropped item
		if (bestitem->timeout)
		{
			avoidtime = AVOID_DROPPED_TIME;
		}
		else
		{
			avoidtime = iteminfo->respawntime;
			if (!avoidtime)
			{
				avoidtime = AVOID_DEFAULT_TIME;
			}
			if (avoidtime < AVOID_MINIMUM_TIME)
			{
				avoidtime = AVOID_MINIMUM_TIME;
			}
		}
	}
	else
	{
		avoidtime = iteminfo->respawntime * 0.5;
		if (avoidtime < 10)
		{
			avoidtime = AVOID_DEFAULT_TIME;
		}
		//if it's a dropped item
		if (bestitem->timeout)
		{
			avoidtime = AVOID_DROPPED_TIME_WOLF;
		}
	}
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals(gs, bestitem->number, avoidtime);
	//push the goal on the stack
	BotPushGoal(goalstate, &goal, sizeof(goal));
	return true;
}

static bool BotChooseNBGItem(int goalstate, const vec3_t origin, const int* inventory, int travelflags,
	const bot_goal_t* ltg, float maxtime)
{
	bot_goalstate_t* gs = BotGoalStateFromHandle(goalstate);
	if (!gs)
	{
		return false;
	}
	if (!gs->itemweightconfig)
	{
		return false;
	}
	//get the area the bot is in
	int areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	if (!areanum || !AAS_AreaReachability(areanum))
	{
		//use the last valid area the bot was in
		areanum = gs->lastreachabilityarea;
	}
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if (!areanum)
	{
		return false;
	}

	int ltg_time;
	if (ltg)
	{
		ltg_time = AAS_AreaTravelTimeToGoalArea(areanum, origin, ltg->areanum, travelflags);
	}
	else
	{
		ltg_time = 99999;
	}
	//the item configuration
	itemconfig_t* ic = itemconfig;
	if (!itemconfig)
	{
		return false;
	}
	//best weight and item so far
	float bestweight = 0;
	levelitem_t* bestitem = NULL;
	//go through the items in the level
	for (levelitem_t* li = levelitems; li; li = li->next)
	{
		if (GGameType & GAME_ET)
		{
			if (g_singleplayer)
			{
				if (li->flags & IFL_NOTSINGLE)
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
		}
		if (GGameType & GAME_Quake3 && li->flags & IFL_NOTBOT)
		{
			continue;
		}
		//if the item is in a possible goal area
		if (!li->goalareanum)
		{
			continue;
		}
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if (GGameType & GAME_Quake3 && !li->entitynum && !(li->flags & IFL_ROAM))
		{
			continue;
		}
		//get the fuzzy weight function for this item
		iteminfo_t* iteminfo = &ic->iteminfo[li->iteminfo];
		int weightnum = gs->itemweightindex[iteminfo->number];
		if (weightnum < 0)
		{
			continue;
		}
		//if this goal is in the avoid goals
		if (!(GGameType & GAME_Quake3) && BotAvoidGoalTime(goalstate, li->number) > 0)
		{
			continue;
		}

		float weight = FuzzyWeightUndecided(inventory, gs->itemweightconfig, weightnum);
		//HACK: to make dropped items more attractive
		if (li->timeout)
		{
			weight += GGameType & GAME_Quake3 ? droppedweight->value : 1000;
		}
		//use weight scale for item_botroam
		if (GGameType & GAME_Quake3 && li->flags & IFL_ROAM)
		{
			weight *= li->weight;
		}

		if (weight > 0)
		{
			//get the travel time towards the goal area
			int t = AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			//if the goal is reachable
			if (t > 0 && t < maxtime)
			{
				if (GGameType & GAME_Quake3)
				{
					//if this item won't respawn before we get there
					float avoidtime = BotAvoidGoalTime(goalstate, li->number);
					if (avoidtime - t * 0.009 > 0)
					{
						continue;
					}
				}

				weight /= (float)t * TRAVELTIME_SCALE;

				if (weight > bestweight)
				{
					t = 0;
					if (ltg && !li->timeout)
					{
						//get the travel time from the goal to the long term goal
						t = AAS_AreaTravelTimeToGoalArea(li->goalareanum, li->goalorigin, ltg->areanum, travelflags);
					}
					//if the travel back is possible and doesn't take too long
					if (t <= ltg_time)
					{
						bestweight = weight;
						bestitem = li;
					}
				}
			}
		}
	}
	//if no goal item found
	if (!bestitem)
	{
		return false;
	}
	//create a bot goal for this item
	bot_goal_t goal;
	Com_Memset(&goal, 0, sizeof(bot_goal_t));
	iteminfo_t* iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy(bestitem->goalorigin, goal.origin);
	VectorCopy(iteminfo->mins, goal.mins);
	VectorCopy(iteminfo->maxs, goal.maxs);
	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->entitynum;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if (GGameType & GAME_Quake3)
	{
		if (bestitem->timeout)
		{
			goal.flags |= GFL_DROPPED;
		}
		if (bestitem->flags & IFL_ROAM)
		{
			goal.flags |= GFL_ROAM;
		}
	}
	goal.iteminfo = bestitem->iteminfo;
	float avoidtime;
	if (GGameType & GAME_Quake3)
	{
		//if it's a dropped item
		if (bestitem->timeout)
		{
			avoidtime = AVOID_DROPPED_TIME;
		}
		else
		{
			avoidtime = iteminfo->respawntime;
			if (!avoidtime)
			{
				avoidtime = AVOID_DEFAULT_TIME;
			}
			if (avoidtime < AVOID_MINIMUM_TIME)
			{
				avoidtime = AVOID_MINIMUM_TIME;
			}
		}
	}
	else
	{
		avoidtime = iteminfo->respawntime * 0.5;
		if (avoidtime < 10)
		{
			avoidtime = AVOID_DEFAULT_TIME;
		}
		//if it's a dropped item
		if (bestitem->timeout)
		{
			avoidtime = AVOID_DROPPED_TIME_WOLF;
		}
	}
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals(gs, bestitem->number, avoidtime);
	//push the goal on the stack
	BotPushGoal(goalstate, &goal, sizeof(goal));
	return true;
}

bool BotChooseNBGItemQ3(int goalstate, const vec3_t origin, const int* inventory, int travelflags,
	const bot_goal_q3_t* ltg, float maxtime)
{
	return BotChooseNBGItem(goalstate, origin, inventory, travelflags,
		reinterpret_cast<const bot_goal_t*>(ltg), maxtime);
}

bool BotChooseNBGItemET(int goalstate, const vec3_t origin, const int* inventory, int travelflags,
	const bot_goal_et_t* ltg, float maxtime)
{
	return BotChooseNBGItem(goalstate, origin, inventory, travelflags,
		reinterpret_cast<const bot_goal_t*>(ltg), maxtime);
}

static bool BotItemGoalInVisButNotVisible(int viewer, const vec3_t eye, const vec3_t viewangles, const bot_goal_t* goal)
{
	if (!(goal->flags & GFL_ITEM))
	{
		return false;
	}

	vec3_t middle;
	VectorAdd(goal->mins, goal->mins, middle);
	VectorScale(middle, 0.5, middle);
	VectorAdd(goal->origin, middle, middle);

	bsp_trace_t trace = AAS_Trace(eye, NULL, NULL, middle, viewer, BSP46CONTENTS_SOLID);
	//if the goal middle point is visible
	if (trace.fraction >= 1)
	{
		//the goal entity number doesn't have to be valid
		//just assume it's valid
		if (goal->entitynum <= 0)
		{
			return false;
		}

		//if the entity data isn't valid
		aas_entityinfo_t entinfo;
		AAS_EntityInfo(goal->entitynum, &entinfo);
		//NOTE: for some wacko reason entities are sometimes
		// not updated
		//if (!entinfo.valid) return true;
		if (entinfo.ltime < AAS_Time() - 0.5)
		{
			return true;
		}
	}
	return false;
}

bool BotItemGoalInVisButNotVisibleQ3(int viewer, const vec3_t eye, const vec3_t viewangles, const bot_goal_q3_t* goal)
{
	return BotItemGoalInVisButNotVisible(viewer, eye, viewangles, reinterpret_cast<const bot_goal_t*>(goal));
}

bool BotItemGoalInVisButNotVisibleET(int viewer, const vec3_t eye, const vec3_t viewangles, const bot_goal_et_t* goal)
{
	return BotItemGoalInVisButNotVisible(viewer, eye, viewangles, reinterpret_cast<const bot_goal_t*>(goal));
}
