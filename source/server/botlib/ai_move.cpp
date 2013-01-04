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

#define MAX_AVOIDREACH                  1
#define MAX_AVOIDSPOTS                  32

//used to avoid reachability links for some time after being used
#define AVOIDREACH_TIME_Q3      6		//avoid links for 6 seconds after use
#define AVOIDREACH_TIME_ET      4
#define AVOIDREACH_TRIES        4

//prediction times
#define PREDICTIONTIME_JUMP 3		//in seconds

//hook commands
#define CMD_HOOKOFF             "hookoff"
#define CMD_HOOKON              "hookon"

//weapon indexes for weapon jumping
#define WEAPONINDEX_ROCKET_LAUNCHER     5

#define MODELTYPE_FUNC_PLAT     1
#define MODELTYPE_FUNC_BOB      2
#define MODELTYPE_FUNC_DOOR     3
#define MODELTYPE_FUNC_STATIC   4

struct bot_avoidspot_t
{
	vec3_t origin;
	float radius;
	int type;
};

//movement state
//NOTE: the moveflags MFL_ONGROUND, MFL_TELEPORTED, MFL_WATERJUMP and
//		MFL_GRAPPLEPULL must be set outside the movement code
struct bot_movestate_t
{
	//input vars (all set outside the movement code)
	vec3_t origin;								//origin of the bot
	vec3_t velocity;							//velocity of the bot
	vec3_t viewoffset;							//view offset
	int entitynum;								//entity number of the bot
	int client;									//client number of the bot
	float thinktime;							//time the bot thinks
	int presencetype;							//presencetype of the bot
	vec3_t viewangles;							//view angles of the bot
	//state vars
	int areanum;								//area the bot is in
	int lastareanum;							//last area the bot was in
	int lastgoalareanum;						//last goal area number
	int lastreachnum;							//last reachability number
	vec3_t lastorigin;							//origin previous cycle
	float lasttime;
	int reachareanum;							//area number of the reachabilty
	int moveflags;								//movement flags
	int jumpreach;								//set when jumped
	float grapplevisible_time;					//last time the grapple was visible
	float lastgrappledist;						//last distance to the grapple end
	float reachability_time;					//time to use current reachability
	int avoidreach[MAX_AVOIDREACH];				//reachabilities to avoid
	float avoidreachtimes[MAX_AVOIDREACH];		//times to avoid the reachabilities
	int avoidreachtries[MAX_AVOIDREACH];		//number of tries before avoiding
	bot_avoidspot_t avoidspots[MAX_AVOIDSPOTS];	//spots to avoid
	int numavoidspots;
};

static libvar_t* sv_maxstep;
static libvar_t* sv_maxbarrier;
static libvar_t* sv_gravity;
static libvar_t* weapindex_rocketlauncher;
static libvar_t* weapindex_bfg10k;
static libvar_t* weapindex_grapple;
static libvar_t* entitytypemissile;
static libvar_t* offhandgrapple;
static libvar_t* cmd_grappleoff;
static libvar_t* cmd_grappleon;

//type of model, func_plat or func_bobbing
static int modeltypes[MAX_MODELS_Q3];

static bot_movestate_t* botmovestates[MAX_BOTLIB_CLIENTS_ARRAY + 1];

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

static bot_movestate_t* BotMoveStateFromHandle(int handle)
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

static bool BotValidTravel(const aas_reachability_t* reach, int travelflags)
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

static void BotAddToAvoidReach(bot_movestate_t* ms, int number, float avoidtime)
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

static int BotAvoidSpots(const vec3_t origin, const aas_reachability_t* reach, const bot_avoidspot_t* avoidspots, int numavoidspots)
{
	bool checkbetween;
	switch (reach->traveltype & TRAVELTYPE_MASK)
	{
	case TRAVEL_WALK: checkbetween = true; break;
	case TRAVEL_CROUCH: checkbetween = true; break;
	case TRAVEL_BARRIERJUMP: checkbetween = true; break;
	case TRAVEL_LADDER: checkbetween = true; break;
	case TRAVEL_WALKOFFLEDGE: checkbetween = false; break;
	case TRAVEL_JUMP: checkbetween = false; break;
	case TRAVEL_SWIM: checkbetween = true; break;
	case TRAVEL_WATERJUMP: checkbetween = true; break;
	case TRAVEL_TELEPORT: checkbetween = false; break;
	case TRAVEL_ELEVATOR: checkbetween = false; break;
	case TRAVEL_GRAPPLEHOOK: checkbetween = false; break;
	case TRAVEL_ROCKETJUMP: checkbetween = false; break;
	case TRAVEL_BFGJUMP: checkbetween = false; break;
	case TRAVEL_JUMPPAD: checkbetween = false; break;
	case TRAVEL_FUNCBOB: checkbetween = false; break;
	default: checkbetween = true; break;
	}

	int type = AVOID_CLEAR;
	for (int i = 0; i < numavoidspots; i++)
	{
		float squaredradius = Square(avoidspots[i].radius);
		float squareddist = DistanceFromLineSquared(avoidspots[i].origin, origin, reach->start);
		// if moving towards the avoid spot
		if (squareddist < squaredradius &&
			VectorDistanceSquared(avoidspots[i].origin, origin) > squareddist)
		{
			type = avoidspots[i].type;
		}
		else if (checkbetween)
		{
			squareddist = DistanceFromLineSquared(avoidspots[i].origin, reach->start, reach->end);
			// if moving towards the avoid spot
			if (squareddist < squaredradius &&
				VectorDistanceSquared(avoidspots[i].origin, reach->start) > squareddist)
			{
				type = avoidspots[i].type;
			}
		}
		else
		{
			VectorDistanceSquared(avoidspots[i].origin, reach->end);
			// if the reachability leads closer to the avoid spot
			if (squareddist < squaredradius &&
				VectorDistanceSquared(avoidspots[i].origin, reach->start) > squareddist)
			{
				type = avoidspots[i].type;
			}
		}
		if (type == AVOID_ALWAYS)
		{
			return type;
		}
	}
	return type;
}

void BotAddAvoidSpot(int movestate, const vec3_t origin, float radius, int type)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	if (type == AVOID_CLEAR)
	{
		ms->numavoidspots = 0;
		return;
	}

	if (ms->numavoidspots >= MAX_AVOIDSPOTS)
	{
		return;
	}
	VectorCopy(origin, ms->avoidspots[ms->numavoidspots].origin);
	ms->avoidspots[ms->numavoidspots].radius = radius;
	ms->avoidspots[ms->numavoidspots].type = type;
	ms->numavoidspots++;
}

int BotAddToTarget(const vec3_t start, const vec3_t end, float maxdist, float* dist, vec3_t target)
{
	vec3_t dir;
	VectorSubtract(end, start, dir);
	float curdist = VectorNormalize(dir);
	if (*dist + curdist < maxdist)
	{
		VectorCopy(end, target);
		*dist += curdist;
		return false;
	}
	else
	{
		VectorMA(start, maxdist - *dist, dir, target);
		*dist = maxdist;
		return true;
	}
}

static void MoverBottomCenter(const aas_reachability_t* reach, vec3_t bottomcenter)
{
	vec3_t mins, maxs, origin, mids;
	vec3_t angles = {0, 0, 0};

	int modelnum = reach->facenum & 0x0000FFFF;
	//get some bsp model info
	AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);

	if (!AAS_OriginOfMoverWithModelNum(modelnum, origin))
	{
		BotImport_Print(PRT_MESSAGE, "no entity with model %d\n", modelnum);
	}
	//get a point just above the plat in the bottom position
	VectorAdd(mins, maxs, mids);
	VectorMA(origin, 0.5, mids, bottomcenter);
	bottomcenter[2] = reach->start[2];
}

static void BotClearMoveResult(bot_moveresult_t* moveresult)
{
	moveresult->failure = false;
	moveresult->type = 0;
	moveresult->blocked = false;
	moveresult->blockentity = GGameType & GAME_ET ? -1 : 0;
	moveresult->traveltype = 0;
	moveresult->flags = 0;
}

static bool BotAirControl(const vec3_t origin, const vec3_t velocity, const vec3_t goal, vec3_t dir, float* speed)
{
	vec3_t org, vel;
	float dist;
	int i;

	VectorCopy(origin, org);
	VectorScale(velocity, 0.1, vel);
	for (i = 0; i < 50; i++)
	{
		vel[2] -= sv_gravity->value * 0.01;
		//if going down and next position would be below the goal
		if (vel[2] < 0 && org[2] + vel[2] < goal[2])
		{
			VectorScale(vel, (goal[2] - org[2]) / vel[2], vel);
			VectorAdd(org, vel, org);
			VectorSubtract(goal, org, dir);
			dist = VectorNormalize(dir);
			if (dist > 32)
			{
				dist = 32;
			}
			*speed = 400 - (400 - 13 * dist);
			return true;
		}
		else
		{
			VectorAdd(org, vel, org);
		}
	}
	VectorSet(dir, 0, 0, 0);
	*speed = 400;
	return false;
}

static void BotFuncBobStartEnd(const aas_reachability_t* reach, vec3_t start, vec3_t end, vec3_t origin)
{
	int modelnum = reach->facenum & 0x0000FFFF;
	if (!AAS_OriginOfMoverWithModelNum(modelnum, origin))
	{
		BotImport_Print(PRT_MESSAGE, "BotFuncBobStartEnd: no entity with model %d\n", modelnum);
		VectorSet(start, 0, 0, 0);
		VectorSet(end, 0, 0, 0);
		return;
	}
	vec3_t angles = {0, 0, 0};
	vec3_t mins, maxs;
	AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);
	vec3_t mid;
	VectorAdd(mins, maxs, mid);
	VectorScale(mid, 0.5, mid);
	VectorCopy(mid, start);
	VectorCopy(mid, end);
	int spawnflags = reach->facenum >> 16;
	int num0 = reach->edgenum >> 16;
	if (num0 > 0x00007FFF)
	{
		num0 |= 0xFFFF0000;
	}
	int num1 = reach->edgenum & 0x0000FFFF;
	if (num1 > 0x00007FFF)
	{
		num1 |= 0xFFFF0000;
	}
	if (spawnflags & 1)
	{
		start[0] = num0;
		end[0] = num1;

		origin[0] += mid[0];
		origin[1] = mid[1];
		origin[2] = mid[2];
	}
	else if (spawnflags & 2)
	{
		start[1] = num0;
		end[1] = num1;

		origin[0] = mid[0];
		origin[1] += mid[1];
		origin[2] = mid[2];
	}
	else
	{
		start[2] = num0;
		end[2] = num1;

		origin[0] = mid[0];
		origin[1] = mid[1];
		origin[2] += mid[2];
	}
}

static int GrappleStateQ3(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	//if the grapple hook is pulling
	if (ms->moveflags & Q3MFL_GRAPPLEPULL)
	{
		return 2;
	}
	//check for a visible grapple missile entity
	//or visible grapple entity
	for (int i = AAS_NextEntity(0); i; i = AAS_NextEntity(i))
	{
		if (AAS_EntityType(i) == (int)entitytypemissile->value)
		{
			aas_entityinfo_t entinfo;
			AAS_EntityInfo(i, &entinfo);
			if (entinfo.weapon == (int)weapindex_grapple->value)
			{
				return 1;
			}
		}
	}
	//no valid grapple at all
	return 0;
}

static int GrappleStateWolf(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	//JL Always 0
	static int grapplemodelindex;
	static libvar_t* laserhook;

	if (!laserhook)
	{
		laserhook = LibVar("laserhook", "0");
	}
	if (!laserhook->value && !grapplemodelindex)
	{
		grapplemodelindex = 0;
	}
	for (int i = AAS_NextEntity(0); i; i = AAS_NextEntity(i))
	{
		if ((!laserhook->value && AAS_EntityModelindex(i) == grapplemodelindex))
		{
			aas_entityinfo_t entinfo;
			AAS_EntityInfo(i, &entinfo);
			//if the origin is equal to the last visible origin
			if (VectorCompare(entinfo.origin, entinfo.lastvisorigin))
			{
				vec3_t dir;
				VectorSubtract(entinfo.origin, reach->end, dir);
				//if hooked near the reachability end
				if (VectorLength(dir) < 32)
				{
					return 2;
				}
			}
			else
			{
				//still shooting hook
				return 1;
			}
		}
	}
	//no valid grapple at all
	return 0;
}

// 0  no valid grapple hook visible
// 1  the grapple hook is still flying
// 2  the grapple hooked into a wall
static int GrappleState(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	if (GGameType & GAME_Quake3)
	{
		return GrappleStateQ3(ms, reach);
	}
	else
	{
		return GrappleStateWolf(ms, reach);
	}
}

// time before the reachability times out
static int BotReachabilityTime(aas_reachability_t* reach)
{
	switch (reach->traveltype & TRAVELTYPE_MASK)
	{
	case TRAVEL_WALK: return 5;
	case TRAVEL_CROUCH: return 5;
	case TRAVEL_BARRIERJUMP: return 5;
	case TRAVEL_LADDER: return 6;
	case TRAVEL_WALKOFFLEDGE: return 5;
	case TRAVEL_JUMP: return 5;
	case TRAVEL_SWIM: return 5;
	case TRAVEL_WATERJUMP: return 5;
	case TRAVEL_TELEPORT: return 5;
	case TRAVEL_ELEVATOR: return 10;
	case TRAVEL_GRAPPLEHOOK: return 8;
	case TRAVEL_ROCKETJUMP: return 6;
	case TRAVEL_JUMPPAD: return 10;
	case TRAVEL_FUNCBOB: return 10;
	case TRAVEL_BFGJUMP: if (GGameType & GAME_Quake3) return 6;
	default:
		BotImport_Print(PRT_ERROR, "travel type %d not implemented yet\n", reach->traveltype);
		return 8;
	}
}

void BotResetAvoidReach(int movestate)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	Com_Memset(ms->avoidreach, 0, MAX_AVOIDREACH * sizeof(int));
	Com_Memset(ms->avoidreachtimes, 0, MAX_AVOIDREACH * sizeof(float));
	Com_Memset(ms->avoidreachtries, 0, MAX_AVOIDREACH * sizeof(int));
}

void BotResetAvoidReachAndMove(int movestate)
{
	BotResetAvoidReach(movestate);

	// RF, also clear movestate stuff
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	ms->lastareanum = 0;
	ms->lastgoalareanum = 0;
	ms->lastreachnum = 0;
}

void BotResetLastAvoidReach(int movestate)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	float latesttime = 0;
	int latest = 0;
	for (int i = 0; i < MAX_AVOIDREACH; i++)
	{
		if (ms->avoidreachtimes[i] > latesttime)
		{
			latesttime = ms->avoidreachtimes[i];
			latest = i;
		}
	}
	if (latesttime)
	{
		ms->avoidreachtimes[latest] = 0;
		if (ms->avoidreachtries[latest] > 0)
		{
			ms->avoidreachtries[latest]--;
		}
	}
}

void BotResetMoveState(int movestate)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	Com_Memset(ms, 0, sizeof(bot_movestate_t));
}

int BotSetupMoveAI()
{
	BotSetBrushModelTypes();
	sv_maxstep = LibVar("sv_step", "18");
	sv_maxbarrier = LibVar("sv_maxbarrier", "32");
	sv_gravity = LibVar("sv_gravity", "800");
	if (GGameType & GAME_Quake3)
	{
		weapindex_rocketlauncher = LibVar("weapindex_rocketlauncher", "5");
		weapindex_bfg10k = LibVar("weapindex_bfg10k", "9");
		weapindex_grapple = LibVar("weapindex_grapple", "10");
		entitytypemissile = LibVar("entitytypemissile", "3");
		offhandgrapple = LibVar("offhandgrapple", "0");
		cmd_grappleon = LibVar("cmd_grappleon", "grappleon");
		cmd_grappleoff = LibVar("cmd_grappleoff", "grappleoff");
	}
	return BLERR_NOERROR;
}

void BotShutdownMoveAI()
{
	for (int i = 1; i <= MAX_CLIENTS_Q3; i++)
	{
		if (botmovestates[i])
		{
			Mem_Free(botmovestates[i]);
			botmovestates[i] = NULL;
		}
	}
}

static bool MoverDown(const aas_reachability_t* reach)
{
	int modelnum = reach->facenum & 0x0000FFFF;
	//get some bsp model info
	vec3_t angles = {0, 0, 0};
	vec3_t mins, maxs;
	AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);

	vec3_t origin;
	if (!AAS_OriginOfMoverWithModelNum(modelnum, origin))
	{
		BotImport_Print(PRT_MESSAGE, "no entity with model %d\n", modelnum);
		return false;
	}
	//if the top of the plat is below the reachability start point
	if (origin[2] + maxs[2] < reach->start[2])
	{
		return true;
	}
	return false;
}

static int BotGetReachabilityToGoal(const vec3_t origin, int areanum,
	int lastgoalareanum, int lastareanum,
	const int* avoidreach, const float* avoidreachtimes, const int* avoidreachtries,
	const bot_goal_t* goal, int travelflags, int movetravelflags,
	const bot_avoidspot_t* avoidspots, int numavoidspots, int* flags)
{
	bool useAvoidPass = false;

again:

	//if not in a valid area
	if (!areanum)
	{
		return 0;
	}

	if (AAS_AreaDoNotEnter(areanum) || AAS_AreaDoNotEnter(goal->areanum))
	{
		travelflags |= TFL_DONOTENTER;
		movetravelflags |= TFL_DONOTENTER;
	}
	if (!(GGameType & GAME_Quake3) && (AAS_AreaDoNotEnterLarge(areanum) || AAS_AreaDoNotEnterLarge(goal->areanum)))
	{
		travelflags |= WOLFTFL_DONOTENTER_LARGE;
		movetravelflags |= WOLFTFL_DONOTENTER_LARGE;
	}
	//use the routing to find the next area to go to
	int besttime = 0;
	int bestreachnum = 0;

	for (int reachnum = AAS_NextAreaReachability(areanum, 0); reachnum;
		 reachnum = AAS_NextAreaReachability(areanum, reachnum))
	{
		if (!(GGameType & (GAME_WolfSP | GAME_WolfMP)))
		{
			int i;
			//check if it isn't an reachability to avoid
			for (i = 0; i < MAX_AVOIDREACH; i++)
			{
				if (avoidreach[i] == reachnum && avoidreachtimes[i] >= AAS_Time())
				{
					break;
				}
			}
			// RF, if this is a "useAvoidPass" then we should only accept avoidreach reachabilities
			if ((!useAvoidPass && i != MAX_AVOIDREACH && avoidreachtries[i] > AVOIDREACH_TRIES) ||
				(useAvoidPass && !(i != MAX_AVOIDREACH && avoidreachtries[i] > AVOIDREACH_TRIES)))
			{
#ifdef DEBUG
				if (bot_developer)
				{
					BotImport_Print(PRT_MESSAGE, "avoiding reachability %d\n", avoidreach[i]);
				}
#endif
				continue;
			}
		}
		//get the reachability from the number
		aas_reachability_t reach;
		AAS_ReachabilityFromNum(reachnum, &reach);
		//NOTE: do not go back to the previous area if the goal didn't change
		//NOTE: is this actually avoidance of local routing minima between two areas???
		if (lastgoalareanum == goal->areanum && reach.areanum == lastareanum)
		{
			continue;
		}
		//if the travel isn't valid
		if (!BotValidTravel(&reach, movetravelflags))
		{
			continue;
		}
		// if the area is disabled
		if (GGameType & (GAME_WolfSP | GAME_ET) && !AAS_AreaReachability(reach.areanum))
		{
			continue;
		}
		int t;
		if (GGameType & GAME_WolfSP)
		{
			//get the travel time (ignore routes that leads us back our current area)
			t = AAS_AreaTravelTimeToGoalAreaCheckLoop(reach.areanum, reach.end, goal->areanum, travelflags, areanum);
		}
		else
		{
			//get the travel time
			t = AAS_AreaTravelTimeToGoalArea(reach.areanum, reach.end, goal->areanum, travelflags);
		}
		//if the goal area isn't reachable from the reachable area
		if (!t)
		{
			continue;
		}

		//if the bot should not use this reachability to avoid bad spots
		if (GGameType & GAME_Quake3 && BotAvoidSpots(origin, &reach, avoidspots, numavoidspots))
		{
			if (flags)
			{
				*flags |= Q3MOVERESULT_BLOCKEDBYAVOIDSPOT;
			}
			continue;
		}

		//add the travel time towards the area
		if (GGameType & (GAME_WolfMP | GAME_ET))
		{
			// Ridah, not sure why this was disabled, but it causes looped links in the route-cache
			t += reach.traveltime + AAS_AreaTravelTime(areanum, origin, reach.start);
		}
		else
		{
			t += reach.traveltime;
		}
		//if the travel time is better than the ones already found
		if (!besttime || t < besttime)
		{
			besttime = t;
			bestreachnum = reachnum;
		}
	}

	// RF, if we didnt find a route, then try again only looking through avoidreach reachabilities
	if (GGameType & GAME_ET && !bestreachnum && !useAvoidPass)
	{
		useAvoidPass = true;
		goto again;
	}

	return bestreachnum;
}

static int BotMovementViewTarget(int movestate, const bot_goal_t* goal, int travelflags, float lookahead, vec3_t target)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return false;
	}
	int reachnum = 0;
	//if the bot has no goal or no last reachability
	if (!ms->lastreachnum || !goal)
	{
		return false;
	}

	reachnum = ms->lastreachnum;
	vec3_t end;
	VectorCopy(ms->origin, end);
	int lastareanum = ms->lastareanum;
	float dist = 0;
	while (reachnum && dist < lookahead)
	{
		aas_reachability_t reach;
		AAS_ReachabilityFromNum(reachnum, &reach);
		if (BotAddToTarget(end, reach.start, lookahead, &dist, target))
		{
			return true;
		}
		//never look beyond teleporters
		if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_TELEPORT)
		{
			return true;
		}
		//never look beyond the weapon jump point
		if (GGameType & GAME_Quake3 && (reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_ROCKETJUMP)
		{
			return true;
		}
		if (GGameType & GAME_Quake3 && (reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_BFGJUMP)
		{
			return true;
		}
		//don't add jump pad distances
		if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_JUMPPAD &&
			(reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_ELEVATOR &&
			(reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_FUNCBOB)
		{
			if (BotAddToTarget(reach.start, reach.end, lookahead, &dist, target))
			{
				return true;
			}
		}
		reachnum = BotGetReachabilityToGoal(reach.end, reach.areanum,
			ms->lastgoalareanum, lastareanum,
			ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries,
			goal, travelflags, travelflags, NULL, 0, NULL);
		VectorCopy(reach.end, end);
		lastareanum = reach.areanum;
		if (lastareanum == goal->areanum)
		{
			BotAddToTarget(reach.end, goal->origin, lookahead, &dist, target);
			return true;
		}
	}

	return false;
}

int BotMovementViewTargetQ3(int movestate, const bot_goal_q3_t* goal, int travelflags, float lookahead, vec3_t target)
{
	return BotMovementViewTarget(movestate, reinterpret_cast<const bot_goal_t*>(goal), travelflags, lookahead, target);
}

int BotMovementViewTargetET(int movestate, const bot_goal_et_t* goal, int travelflags, float lookahead, vec3_t target)
{
	return BotMovementViewTarget(movestate, reinterpret_cast<const bot_goal_t*>(goal), travelflags, lookahead, target);
}

static int BotFirstReachabilityArea(const vec3_t origin, const int* areas, int numareas, bool distCheck)
{
	int best = 0;
	float bestDist = 999999;
	for (int i = 0; i < numareas; i++)
	{
		if (AAS_AreaReachability(areas[i]))
		{
			// make sure this area is visible
			vec3_t center;
			if (!AAS_AreaWaypoint(areas[i], center))
			{
				AAS_AreaCenter(areas[i], center);
			}
			if (distCheck)
			{
				float dist = VectorDistance(center, origin);
				if (center[2] > origin[2])
				{
					dist += 32 * (center[2] - origin[2]);
				}
				if (dist < bestDist)
				{
					bsp_trace_t tr = AAS_Trace(origin, NULL, NULL, center, -1, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
					if (tr.fraction == 1.0 && !tr.startsolid && !tr.allsolid)
					{
						best = areas[i];
						bestDist = dist;
					}
				}
			}
			else
			{
				bsp_trace_t tr = AAS_Trace(origin, NULL, NULL, center, -1, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
				if (tr.fraction == 1.0 && !tr.startsolid && !tr.allsolid)
				{
					best = areas[i];
					break;
				}
			}
		}
	}

	return best;
}

static int BotFuzzyPointReachabilityAreaET(const vec3_t origin)
{
	int areanum = AAS_PointAreaNum(origin);
	if (!AAS_AreaReachability(areanum))
	{
		areanum = 0;
	}
	if (areanum)
	{
		return areanum;
	}

	// try a line trace from beneath to above
	vec3_t start;
	VectorCopy(origin, start);
	vec3_t end;
	VectorCopy(origin, end);
	start[2] -= 30;
	end[2] += 40;
	int areas[100];
	int numareas = AAS_TraceAreas(start, end, areas, NULL, 100);
	int bestarea = 0;
	if (numareas > 0)
	{
		bestarea = BotFirstReachabilityArea(origin, areas, numareas, false);
	}
	if (bestarea)
	{
		return bestarea;
	}

	// try a small box around the origin
	vec3_t maxs;
	maxs[0] = 4;
	maxs[1] = 4;
	maxs[2] = 4;
	vec3_t mins;
	VectorSubtract(origin, maxs, mins);
	VectorAdd(origin, maxs, maxs);
	numareas = AAS_BBoxAreas(mins, maxs, areas, 100);
	if (numareas > 0)
	{
		bestarea = BotFirstReachabilityArea(origin, areas, numareas, true);
	}
	if (bestarea)
	{
		return bestarea;
	}

	AAS_PresenceTypeBoundingBox(PRESENCE_NORMAL, mins, maxs);
	VectorAdd(mins, origin, mins);
	VectorAdd(maxs, origin, maxs);
	numareas = AAS_BBoxAreas(mins, maxs, areas, 100);
	if (numareas > 0)
	{
		bestarea = BotFirstReachabilityArea(origin, areas, numareas, true);
	}
	if (bestarea)
	{
		return bestarea;
	}
	return 0;
}

int BotFuzzyPointReachabilityArea(const vec3_t origin)
{
	if (GGameType & GAME_ET)
	{
		return BotFuzzyPointReachabilityAreaET(origin);
	}

	int firstareanum = 0;
	int areanum = AAS_PointAreaNum(origin);
	if (areanum)
	{
		firstareanum = areanum;
		if (AAS_AreaReachability(areanum))
		{
			return areanum;
		}
	}
	vec3_t end;
	VectorCopy(origin, end);
	end[2] += 4;
	int areas[10];
	vec3_t points[10];
	int numareas = AAS_TraceAreas(origin, end, areas, points, 10);
	for (int j = 0; j < numareas; j++)
	{
		if (AAS_AreaReachability(areas[j]))
		{
			return areas[j];
		}
	}
	float bestdist = 999999;
	int bestareanum = 0;
	for (int z = 1; z >= -1; z -= 1)
	{
		for (int x = 1; x >= -1; x -= (GGameType & GAME_WolfSP ? 2 : 1))
		{
			for (int y = 1; y >= -1; y -= (GGameType & GAME_WolfSP ? 2 : 1))
			{
				VectorCopy(origin, end);
				if (GGameType & GAME_WolfSP)
				{
					// Ridah, increased this for Wolf larger bounding boxes
					end[0] += x * 256;
					end[1] += y * 256;
					end[2] += z * 200;
				}
				else if (GGameType & GAME_WolfMP)
				{
					// Ridah, increased this for Wolf larger bounding boxes
					end[0] += x * 16;
					end[1] += y * 16;
					end[2] += z * 24;
				}
				else
				{
					end[0] += x * 8;
					end[1] += y * 8;
					end[2] += z * 12;
				}
				numareas = AAS_TraceAreas(origin, end, areas, points, 10);
				for (int j = 0; j < numareas; j++)
				{
					if (AAS_AreaReachability(areas[j]))
					{
						vec3_t v;
						VectorSubtract(points[j], origin, v);
						float dist = VectorLength(v);
						if (dist < bestdist)
						{
							bestareanum = areas[j];
							bestdist = dist;
						}
					}
					if (!firstareanum)
					{
						firstareanum = areas[j];
					}
				}
			}
		}
		if (bestareanum)
		{
			return bestareanum;
		}
	}
	return firstareanum;
}

int BotReachabilityArea(const vec3_t origin, int client)
{
	//check if the bot is standing on something
	vec3_t mins, maxs;
	AAS_PresenceTypeBoundingBox(PRESENCE_CROUCH, mins, maxs);
	vec3_t up = {0, 0, 1};
	vec3_t end;
	VectorMA(origin, -3, up, end);
	bsp_trace_t bsptrace = AAS_Trace(origin, mins, maxs, end, client, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (!bsptrace.startsolid && bsptrace.fraction < 1 && bsptrace.ent != Q3ENTITYNUM_NONE)
	{
		//if standing on the world the bot should be in a valid area
		if (bsptrace.ent == Q3ENTITYNUM_WORLD)
		{
			return BotFuzzyPointReachabilityArea(origin);
		}

		int modelnum = AAS_EntityModelindex(bsptrace.ent);
		int modeltype = modeltypes[modelnum];

		//if standing on a func_plat or func_bobbing then the bot is assumed to be
		//in the area the reachability points to
		if (modeltype == MODELTYPE_FUNC_PLAT || modeltype == MODELTYPE_FUNC_BOB)
		{
			int reachnum = AAS_NextModelReachability(0, modelnum);
			if (reachnum)
			{
				aas_reachability_t reach;
				AAS_ReachabilityFromNum(reachnum, &reach);
				return reach.areanum;
			}
		}

		//if the bot is swimming the bot should be in a valid area
		if (AAS_Swimming(origin))
		{
			return BotFuzzyPointReachabilityArea(origin);
		}

		int areanum = BotFuzzyPointReachabilityArea(origin);
		//if the bot is in an area with reachabilities
		if (areanum && AAS_AreaReachability(areanum))
		{
			return areanum;
		}
		//trace down till the ground is hit because the bot is standing on some other entity
		vec3_t org;
		VectorCopy(origin, org);
		VectorCopy(org, end);
		end[2] -= 800;
		aas_trace_t trace = AAS_TraceClientBBox(org, end, PRESENCE_CROUCH, -1);
		if (!trace.startsolid)
		{
			VectorCopy(trace.endpos, org);
		}

		return BotFuzzyPointReachabilityArea(org);
	}

	return BotFuzzyPointReachabilityArea(origin);
}

static bool BotOnMover(const vec3_t origin, int entnum, const aas_reachability_t* reach)
{
	vec3_t angles = {0, 0, 0};
	vec3_t boxmins = {-16, -16, -8}, boxmaxs = {16, 16, 8};
	bsp_trace_t trace;

	int modelnum = reach->facenum & 0x0000FFFF;
	//get some bsp model info
	vec3_t mins, maxs;
	AAS_BSPModelMinsMaxs(modelnum, angles, mins, maxs);

	vec3_t modelorigin;
	if (!AAS_OriginOfMoverWithModelNum(modelnum, modelorigin))
	{
		BotImport_Print(PRT_MESSAGE, "no entity with model %d\n", modelnum);
		return false;
	}

	for (int i = 0; i < 2; i++)
	{
		if (origin[i] > modelorigin[i] + maxs[i] + 16)
		{
			return false;
		}
		if (origin[i] < modelorigin[i] + mins[i] - 16)
		{
			return false;
		}
	}

	vec3_t org;
	VectorCopy(origin, org);
	org[2] += 24;
	vec3_t end;
	VectorCopy(origin, end);
	end[2] -= 48;

	trace = AAS_Trace(org, boxmins, boxmaxs, end, entnum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (!trace.startsolid && !trace.allsolid)
	{
		//NOTE: the reachability face number is the model number of the elevator
		if (trace.ent != Q3ENTITYNUM_NONE && AAS_EntityModelNum(trace.ent) == modelnum)
		{
			return true;
		}
	}
	return false;
}

static int BotOnTopOfEntity(const bot_movestate_t* ms)
{
	vec3_t mins, maxs, end, up = {0, 0, 1};
	AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
	VectorMA(ms->origin, -3, up, end);
	bsp_trace_t trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (!trace.startsolid && (trace.ent != Q3ENTITYNUM_WORLD && trace.ent != Q3ENTITYNUM_NONE))
	{
		return trace.ent;
	}
	return -1;
}

static bool BotVisible(int ent, const vec3_t eye, const vec3_t target)
{
	bsp_trace_t trace = AAS_Trace(eye, NULL, NULL, target, ent, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
	if (trace.fraction >= 1)
	{
		return true;
	}
	return false;
}

static bool BotPredictVisiblePosition(const vec3_t origin, int areanum, const bot_goal_t* goal, int travelflags, vec3_t target)
{
	aas_reachability_t reach;
	int reachnum, lastgoalareanum, lastareanum, i;
	int avoidreach[MAX_AVOIDREACH];
	float avoidreachtimes[MAX_AVOIDREACH];
	int avoidreachtries[MAX_AVOIDREACH];
	vec3_t end;

	//if the bot has no goal or no last reachability
	if (!goal)
	{
		return false;
	}
	//if the areanum is not valid
	if (!areanum)
	{
		return false;
	}
	//if the goal areanum is not valid
	if (!goal->areanum)
	{
		return false;
	}

	Com_Memset(avoidreach, 0, MAX_AVOIDREACH * sizeof(int));
	lastgoalareanum = goal->areanum;
	lastareanum = areanum;
	VectorCopy(origin, end);
	//only do 20 hops
	for (i = 0; i < 20 && (areanum != goal->areanum); i++)
	{
		//
		reachnum = BotGetReachabilityToGoal(end, areanum,
			lastgoalareanum, lastareanum,
			avoidreach, avoidreachtimes, avoidreachtries,
			goal, travelflags, travelflags, NULL, 0, NULL);
		if (!reachnum)
		{
			return false;
		}
		AAS_ReachabilityFromNum(reachnum, &reach);
		//
		if (BotVisible(goal->entitynum, goal->origin, reach.start))
		{
			VectorCopy(reach.start, target);
			return true;
		}	//end if
			//
		if (BotVisible(goal->entitynum, goal->origin, reach.end))
		{
			VectorCopy(reach.end, target);
			return true;
		}	//end if
			//
		if (reach.areanum == goal->areanum)
		{
			VectorCopy(reach.end, target);
			return true;
		}	//end if
			//
		lastareanum = areanum;
		areanum = reach.areanum;
		VectorCopy(reach.end, end);
	}

	return false;
}

bool BotPredictVisiblePositionQ3(const vec3_t origin, int areanum, const bot_goal_q3_t* goal, int travelflags, vec3_t target)
{
	return BotPredictVisiblePosition(origin, areanum, reinterpret_cast<const bot_goal_t*>(goal), travelflags, target);
}

bool BotPredictVisiblePositionET(const vec3_t origin, int areanum, const bot_goal_et_t* goal, int travelflags, vec3_t target)
{
	return BotPredictVisiblePosition(origin, areanum, reinterpret_cast<const bot_goal_t*>(goal), travelflags, target);
}

static float BotGapDistance(const vec3_t origin, const vec3_t hordir, int entnum)
{
	//do gap checking
	float startz = origin[2];
	//this enables walking down stairs more fluidly
	vec3_t start;
	VectorCopy(origin, start);
	vec3_t end;
	VectorCopy(origin, end);
	end[2] -= 60;
	aas_trace_t trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, entnum);
	if (trace.fraction >= 1)
	{
		return 1;
	}
	startz = trace.endpos[2] + 1;

	for (float dist = 8; dist <= 100; dist += 8)
	{
		VectorMA(origin, dist, hordir, start);
		start[2] = startz + 24;
		VectorCopy(start, end);
		end[2] -= 48 + sv_maxbarrier->value;
		trace = AAS_TraceClientBBox(start, end, PRESENCE_CROUCH, entnum);
		//if solid is found the bot can't walk any further and fall into a gap
		if (!trace.startsolid)
		{
			//if it is a gap
			if (trace.endpos[2] < startz - sv_maxstep->value - 8)
			{
				VectorCopy(trace.endpos, end);
				end[2] -= 20;
				if (AAS_PointContents(end) & (GGameType & GAME_Quake3 ? BSP46CONTENTS_WATER : BSP46CONTENTS_WATER | BSP46CONTENTS_SLIME))
				{
					break;
				}
				//if a gap is found slow down
				return dist;
			}
			startz = trace.endpos[2];
		}
	}
	return 0;
}

static bool BotCheckBarrierJump(bot_movestate_t* ms, const vec3_t dir, float speed)
{
	aas_trace_t trace;

	vec3_t end;
	VectorCopy(ms->origin, end);
	end[2] += sv_maxbarrier->value;
	//trace right up
	trace = AAS_TraceClientBBox(ms->origin, end, PRESENCE_NORMAL, ms->entitynum);
	//this shouldn't happen... but we check anyway
	if (trace.startsolid)
	{
		return false;
	}
	//if very low ceiling it isn't possible to jump up to a barrier
	if (trace.endpos[2] - ms->origin[2] < sv_maxstep->value)
	{
		return false;
	}

	vec3_t hordir;
	hordir[0] = dir[0];
	hordir[1] = dir[1];
	hordir[2] = 0;
	VectorNormalize(hordir);
	VectorMA(ms->origin, ms->thinktime * speed * 0.5, hordir, end);
	vec3_t start;
	VectorCopy(trace.endpos, start);
	end[2] = trace.endpos[2];
	//trace from previous trace end pos horizontally in the move direction
	trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, ms->entitynum);
	//again this shouldn't happen
	if (trace.startsolid)
	{
		return false;
	}

	VectorCopy(trace.endpos, start);
	VectorCopy(trace.endpos, end);
	end[2] = ms->origin[2];
	//trace down from the previous trace end pos
	trace = AAS_TraceClientBBox(start, end, PRESENCE_NORMAL, ms->entitynum);
	//if solid
	if (trace.startsolid)
	{
		return false;
	}
	//if no obstacle at all
	if (trace.fraction >= 1.0)
	{
		return false;
	}
	//if less than the maximum step height
	if (trace.endpos[2] - ms->origin[2] < sv_maxstep->value)
	{
		return false;
	}

	EA_Jump(ms->client);
	EA_Move(ms->client, hordir, speed);
	ms->moveflags |= MFL_BARRIERJUMP;
	//there is a barrier
	return true;
}

static bool BotSwimInDirection(const bot_movestate_t* ms, const vec3_t dir, float speed, int type)
{
	vec3_t normdir;
	VectorCopy(dir, normdir);
	VectorNormalize(normdir);
	EA_Move(ms->client, normdir, speed);
	return true;
}

static bool BotWalkInDirection(bot_movestate_t* ms, const vec3_t dir, float speed, int type)
{
	if (GGameType & GAME_Quake3 && AAS_OnGround(ms->origin, ms->presencetype, ms->entitynum))
	{
		ms->moveflags |= MFL_ONGROUND;
	}
	//if the bot is on the ground
	if (ms->moveflags & MFL_ONGROUND)
	{
		//if there is a barrier the bot can jump on
		if (BotCheckBarrierJump(ms, dir, speed))
		{
			return true;
		}
		//remove barrier jump flag
		ms->moveflags &= ~MFL_BARRIERJUMP;
		//get the presence type for the movement
		int presencetype;
		if ((type & MOVE_CROUCH) && !(type & MOVE_JUMP))
		{
			presencetype = PRESENCE_CROUCH;
		}
		else
		{
			presencetype = PRESENCE_NORMAL;
		}
		//horizontal direction
		vec3_t hordir;
		hordir[0] = dir[0];
		hordir[1] = dir[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//if the bot is not supposed to jump
		if (!(type & MOVE_JUMP))
		{
			//if there is a gap, try to jump over it
			if (BotGapDistance(ms->origin, hordir, ms->entitynum) > 0)
			{
				type |= MOVE_JUMP;
			}
		}
		//get command movement
		vec3_t cmdmove;
		VectorScale(hordir, speed, cmdmove);
		vec3_t velocity;
		VectorCopy(ms->velocity, velocity);

		int maxframes, cmdframes, stopevent;
		if (type & MOVE_JUMP)
		{
			//BotImport_Print(PRT_MESSAGE, "trying jump\n");
			cmdmove[2] = 400;
			maxframes = PREDICTIONTIME_JUMP / 0.1;
			cmdframes = 1;
			stopevent = SE_HITGROUND | SE_HITGROUNDDAMAGE |
						SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA;
		}
		else
		{
			maxframes = 2;
			cmdframes = 2;
			stopevent = SE_HITGROUNDDAMAGE |
						SE_ENTERWATER | SE_ENTERSLIME | SE_ENTERLAVA;
		}

		vec3_t origin;
		VectorCopy(ms->origin, origin);
		origin[2] += 0.5;
		aas_clientmove_t move;
		AAS_PredictClientMovement(&move, ms->entitynum, origin, presencetype, 0, true,
			velocity, cmdmove, cmdframes, maxframes, 0.1f,
			stopevent, 0, false);						//true);
		//if prediction time wasn't enough to fully predict the movement
		if (move.frames >= maxframes && (type & MOVE_JUMP))
		{
			return false;
		}
		//don't enter slime or lava and don't fall from too high
		if (move.stopevent & (GGameType & GAME_Quake3 ?
			SE_ENTERSLIME | SE_ENTERLAVA | SE_HITGROUNDDAMAGE : SE_ENTERLAVA | SE_HITGROUNDDAMAGE))
		{
			return false;
		}
		//if ground was hit
		if (move.stopevent & SE_HITGROUND)
		{
			//check for nearby gap
			vec3_t tmpdir;
			VectorNormalize2(move.velocity, tmpdir);
			float dist = BotGapDistance(move.endpos, tmpdir, ms->entitynum);
			if (dist > 0)
			{
				return false;
			}
			//
			dist = BotGapDistance(move.endpos, hordir, ms->entitynum);
			if (dist > 0)
			{
				return false;
			}
		}
		//get horizontal movement
		vec3_t tmpdir;
		tmpdir[0] = move.endpos[0] - ms->origin[0];
		tmpdir[1] = move.endpos[1] - ms->origin[1];
		tmpdir[2] = 0;

		//the bot is blocked by something
		if (VectorLength(tmpdir) < speed * ms->thinktime * 0.5)
		{
			return false;
		}
		//perform the movement
		if (type & MOVE_JUMP)
		{
			EA_Jump(ms->client);
		}
		if (type & MOVE_CROUCH)
		{
			EA_Crouch(ms->client);
		}
		EA_Move(ms->client, hordir, speed);
		//movement was succesfull
		return true;
	}
	else
	{
		if (ms->moveflags & MFL_BARRIERJUMP)
		{
			//if near the top or going down
			if (ms->velocity[2] < 50)
			{
				EA_Move(ms->client, dir, speed);
			}
		}
		//FIXME: do air control to avoid hazards
		return true;
	}
}

bool BotMoveInDirection(int movestate, const vec3_t dir, float speed, int type)
{
	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return false;
	}
	//if swimming
	if (AAS_Swimming(ms->origin))
	{
		return BotSwimInDirection(ms, dir, speed, type);
	}
	else
	{
		return BotWalkInDirection(ms, dir, speed, type);
	}
}

static void BotCheckBlocked(const bot_movestate_t* ms, const vec3_t dir, int checkbottom, bot_moveresult_t* result)
{
	vec3_t mins, maxs, end, up = {0, 0, 1};
	bsp_trace_t trace;

	if (GGameType & GAME_WolfSP)
	{
		// RF, not required for Wolf AI
		return;
	}

	//test for entities obstructing the bot's path
	AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
	//
	if (Q_fabs(DotProduct(dir, up)) < 0.7)
	{
		mins[2] += sv_maxstep->value;	//if the bot can step on
		maxs[2] -= 10;	//a little lower to avoid low ceiling
	}
	VectorMA(ms->origin, GGameType & GAME_ET ? 16 : 3, dir, end);
	trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP | BSP46CONTENTS_BODY);
	//if not started in solid and not hitting the world entity
	if (!trace.startsolid && (trace.ent != Q3ENTITYNUM_WORLD && trace.ent != Q3ENTITYNUM_NONE))
	{
		result->blocked = true;
		result->blockentity = trace.ent;
	}
	//if not in an area with reachability
	else if (checkbottom && !AAS_AreaReachability(ms->areanum))
	{
		//check if the bot is standing on something
		AAS_PresenceTypeBoundingBox(ms->presencetype, mins, maxs);
		VectorMA(ms->origin, -3, up, end);
		trace = AAS_Trace(ms->origin, mins, maxs, end, ms->entitynum, BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP);
		if (!trace.startsolid && (trace.ent != Q3ENTITYNUM_WORLD && trace.ent != Q3ENTITYNUM_NONE))
		{
			result->blocked = true;
			result->blockentity = trace.ent;
			result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
		}
	}
}

static bot_moveresult_t BotTravel_Walk(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//first walk straight to the reachability start
	vec3_t hordir;
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	float dist = VectorNormalize(hordir);

	BotCheckBlocked(ms, hordir, true, &result);

	if (dist < (GGameType & GAME_Quake3 ? 10 : 32))
	{
		//walk straight to the reachability end
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		dist = VectorNormalize(hordir);
	}

	//if going towards a crouch area
	// Ridah, some areas have a 0 presence (!?!)
	if ((GGameType & GAME_Quake3 || AAS_AreaPresenceType(reach->areanum) & PRESENCE_CROUCH) &&
		!(AAS_AreaPresenceType(reach->areanum) & PRESENCE_NORMAL))
	{
		//if pretty close to the reachable area
		if (dist < 20)
		{
			EA_Crouch(ms->client);
		}
	}

	dist = BotGapDistance(ms->origin, hordir, ms->entitynum);

	float speed;
	if (ms->moveflags & (GGameType & GAME_Quake3 ? Q3MFL_WALK : WOLFMFL_WALK))
	{
		if (dist > 0)
		{
			speed = 200 - (180 - 1 * dist);
		}
		else
		{
			speed = 200;
		}
		EA_Walk(ms->client);
	}
	else
	{
		if (dist > 0)
		{
			speed = 400 - (360 - 2 * dist);
		}
		else
		{
			speed = 400;
		}
	}
	//elemantary action move in direction
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_Crouch(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	float speed = 400;
	//walk straight to reachability end
	vec3_t hordir;
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	VectorNormalize(hordir);

	BotCheckBlocked(ms, hordir, true, &result);
	//elemantary actions
	EA_Crouch(ms->client);
	EA_Move(ms->client, hordir, speed);

	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_BarrierJump(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//walk straight to reachability start
	vec3_t hordir;
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	float dist = VectorNormalize(hordir);

	BotCheckBlocked(ms, hordir, true, &result);
	//if pretty close to the barrier
	if (dist < (GGameType & GAME_ET ? 12 : 9))
	{
		EA_Jump(ms->client);
		if (!(GGameType & GAME_Quake3))
		{
			// Ridah, do the movement also, so we have momentum to get onto the barrier
			hordir[0] = reach->end[0] - reach->start[0];
			hordir[1] = reach->end[1] - reach->start[1];
			hordir[2] = 0;
			VectorNormalize(hordir);

			float speed;
			if (GGameType & GAME_ET)
			{
				dist = 90;
				speed = 400 - (360 - 4 * dist);
			}
			else
			{
				dist = 60;
				speed = 360 - (360 - 6 * dist);
			}
			EA_Move(ms->client, hordir, speed);
		}
	}
	else
	{
		float speed;
		if (GGameType & GAME_ET)
		{
			if (dist > 90)
			{
				dist = 90;
			}
			speed = 400 - (360 - 4 * dist);
		}
		else
		{
			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);
		}
		EA_Move(ms->client, hordir, speed);
	}
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotFinishTravel_BarrierJump(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//if near the top or going down
	if (ms->velocity[2] < 250)
	{
		vec3_t hordir;
		if (GGameType & GAME_Quake3)
		{
			hordir[0] = reach->end[0] - ms->origin[0];
			hordir[1] = reach->end[1] - ms->origin[1];
		}
		else
		{
			// Ridah, extend the end point a bit, so we strive to get over the ledge more
			vec3_t end;
			VectorSubtract(reach->end, reach->start, end);
			end[2] = 0;
			VectorNormalize(end);
			VectorMA(reach->end, 32, end, end);
			hordir[0] = end[0] - ms->origin[0];
			hordir[1] = end[1] - ms->origin[1];
		}
		hordir[2] = 0;
		float dist = VectorNormalize(hordir);

		BotCheckBlocked(ms, hordir, true, &result);

		if (GGameType & GAME_Quake3)
		{
			EA_Move(ms->client, hordir, 400);
		}
		else
		{
			if (dist > 60)
			{
				dist = 60;
			}
			float speed = 400 - (400 - 6 * dist);
			EA_Move(ms->client, hordir, speed);
		}
		VectorCopy(hordir, result.movedir);
	}
	else if (GGameType & GAME_ET)
	{
		// hold crouch in case we are going for a crouch area
		if (AAS_AreaPresenceType(reach->areanum) & PRESENCE_CROUCH)
		{
			EA_Crouch(ms->client);
		}
	}

	return result;
}

static bot_moveresult_t BotTravel_Swim(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//swim straight to reachability end
	vec3_t dir;
	VectorSubtract(reach->start, ms->origin, dir);
	VectorNormalize(dir);

	BotCheckBlocked(ms, dir, true, &result);
	//elemantary actions
	EA_Move(ms->client, dir, 400);

	VectorCopy(dir, result.movedir);
	VecToAngles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_SWIMVIEW;

	return result;
}

static bot_moveresult_t BotTravel_WaterJump(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//swim straight to reachability end
	vec3_t dir;
	VectorSubtract(reach->end, ms->origin, dir);
	vec3_t hordir;
	VectorCopy(dir, hordir);
	hordir[2] = 0;
	dir[2] += 15 + crandom() * 40;
	VectorNormalize(dir);
	float dist = VectorNormalize(hordir);
	//elemantary actions
	EA_MoveForward(ms->client);
	//move up if close to the actual out of water jump spot
	if (dist < 40)
	{
		EA_MoveUp(ms->client);
	}
	//set the ideal view angles
	VecToAngles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_MOVEMENTVIEW;

	VectorCopy(dir, result.movedir);

	return result;
}

static bot_moveresult_t BotFinishTravel_WaterJump(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//if waterjumping there's nothing to do
	if (ms->moveflags & MFL_WATERJUMP)
	{
		return result;
	}
	//if not touching any water anymore don't do anything
	//otherwise the bot sometimes keeps jumping?
	vec3_t pnt;
	VectorCopy(ms->origin, pnt);
	pnt[2] -= 32;	//extra for q2dm4 near red armor/mega health
	if (!(AAS_PointContents(pnt) & (BSP46CONTENTS_LAVA | BSP46CONTENTS_SLIME | BSP46CONTENTS_WATER)))
	{
		return result;
	}
	//swim straight to reachability end
	vec3_t dir;
	VectorSubtract(reach->end, ms->origin, dir);
	dir[0] += crandom() * 10;
	dir[1] += crandom() * 10;
	dir[2] += 70 + crandom() * 10;
	VectorNormalize(dir);
	//elemantary actions
	EA_Move(ms->client, dir, 400);
	//set the ideal view angles
	VecToAngles(dir, result.ideal_viewangles);
	result.flags |= MOVERESULT_MOVEMENTVIEW;

	VectorCopy(dir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_WalkOffLedge(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//check if the bot is blocked by anything
	vec3_t dir;
	VectorSubtract(reach->start, ms->origin, dir);
	VectorNormalize(dir);
	BotCheckBlocked(ms, dir, true, &result);
	//if the reachability start and end are practially above each other
	VectorSubtract(reach->end, reach->start, dir);
	dir[2] = 0;
	float reachhordist = VectorLength(dir);
	//walk straight to the reachability start
	vec3_t hordir;
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	float dist = VectorNormalize(hordir);

	//if pretty close to the start focus on the reachability end
	// Ridah, tweaked this
	float speed;
	if (GGameType & GAME_Quake3 ? (dist < 48) :
		(dist < 72 && DotProduct(dir, hordir) < 0))			// walk in the direction of start -> end
	{
		if (GGameType & GAME_Quake3)
		{
			hordir[0] = reach->end[0] - ms->origin[0];
			hordir[1] = reach->end[1] - ms->origin[1];
			hordir[2] = 0;
		}
		else
		{
			VectorCopy(dir, hordir);
		}
		VectorNormalize(hordir);
		//
		if (reachhordist < 20)
		{
			// RF, increased this to speed up travel speed down steps
			speed = GGameType & GAME_WolfSP ? 200 : 100;
		}	//end if
		else if (!AAS_HorizontalVelocityForJump(0, reach->start, reach->end, &speed))
		{
			speed = 400;
		}	//end if
		if (!(GGameType & GAME_Quake3))
		{
			// looks better crouching off a ledge
			EA_Crouch(ms->client);
		}
	}	//end if
	else
	{
		if (reachhordist < 20)
		{
			if (dist > 64)
			{
				dist = 64;
			}
			speed = 400 - (256 - 4 * dist) * (GGameType & GAME_WolfSP ? 0.5: 1);		// RF changed this for steps
		}	//end if
		else
		{
			speed = 400;
			// Ridah, tweaked this
			if (!(GGameType & GAME_Quake3) && dist < 128)
			{
				speed *= GGameType & GAME_WolfSP ? 0.5 + 0.5 * (dist / 128) : (dist / 128);
			}
		}	//end else
	}	//end else
		//
	BotCheckBlocked(ms, hordir, true, &result);
	//elemantary action
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotFinishTravel_WalkOffLedge(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t dir;
	VectorSubtract(reach->end, ms->origin, dir);
	BotCheckBlocked(ms, dir, true, &result);

	vec3_t v;
	VectorSubtract(reach->end, ms->origin, v);
	v[2] = 0;
	float dist = VectorNormalize(v);
	vec3_t end;
	if (dist > 16)
	{
		VectorMA(reach->end, 16, v, end);
	}
	else
	{
		VectorCopy(reach->end, end);
	}

	vec3_t hordir;
	float speed;
	if (!BotAirControl(ms->origin, ms->velocity, end, hordir, &speed))
	{
		//go straight to the reachability end
		VectorCopy(dir, hordir);
		hordir[2] = 0;

		dist = VectorNormalize(hordir);
		speed = 400;
	}

	if (!(GGameType & GAME_Quake3))
	{
		// looks better crouching off a ledge
		EA_Crouch(ms->client);
	}
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_Jump(bot_movestate_t* ms, const aas_reachability_t* reach)
{
	float dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);

	vec3_t runstart;
	AAS_JumpReachRunStart(reach, runstart);

	vec3_t hordir;
	hordir[0] = runstart[0] - reach->start[0];
	hordir[1] = runstart[1] - reach->start[1];
	hordir[2] = 0;
	VectorNormalize(hordir);

	vec3_t start;
	VectorCopy(reach->start, start);
	start[2] += 1;
	VectorMA(reach->start, 80, hordir, runstart);
	//check for a gap
	for (dist1 = 0; dist1 < 80; dist1 += 10)
	{
		vec3_t end;
		VectorMA(start, dist1 + 10, hordir, end);
		end[2] += 1;
		if (AAS_PointAreaNum(end) != ms->reachareanum)
		{
			break;
		}
	}
	if (dist1 < 80)
	{
		VectorMA(reach->start, dist1, hordir, runstart);
	}

	vec3_t dir1;
	VectorSubtract(ms->origin, reach->start, dir1);
	dir1[2] = 0;
	dist1 = VectorNormalize(dir1);
	vec3_t dir2;
	VectorSubtract(ms->origin, runstart, dir2);
	dir2[2] = 0;
	dist2 = VectorNormalize(dir2);
	//if just before the reachability start
	if (DotProduct(dir1, dir2) < -0.8 || dist2 < (GGameType & GAME_ET ? 12 : 5))
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//elemantary action jump
		if (dist1 < 24)
		{
			EA_Jump(ms->client);
		}
		else if (dist1 < 32)
		{
			EA_DelayedJump(ms->client);
		}
		EA_Move(ms->client, hordir, 600);

		ms->jumpreach = ms->lastreachnum;
	}
	else
	{
		hordir[0] = runstart[0] - ms->origin[0];
		hordir[1] = runstart[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);

		if (dist2 > 80)
		{
			dist2 = 80;
		}
		speed = 400 - (400 - 5 * dist2);
		EA_Move(ms->client, hordir, speed);
	}
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotFinishTravel_Jump(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//if not jumped yet
	if (!ms->jumpreach)
	{
		return result;
	}
	//go straight to the reachability end
	vec3_t hordir;
	hordir[0] = reach->end[0] - ms->origin[0];
	hordir[1] = reach->end[1] - ms->origin[1];
	hordir[2] = 0;
	float dist = VectorNormalize(hordir);

	vec3_t hordir2;
	hordir2[0] = reach->end[0] - reach->start[0];
	hordir2[1] = reach->end[1] - reach->start[1];
	hordir2[2] = 0;
	VectorNormalize(hordir2);

	if (DotProduct(hordir, hordir2) < -0.5 && dist < 24)
	{
		return result;
	}
	//always use max speed when traveling through the air
	float speed = 800;

	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_LadderQ3(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t dir;
	VectorSubtract(reach->end, ms->origin, dir);
	VectorNormalize(dir);
	//set the ideal view angles, facing the ladder up or down
	vec3_t viewdir;
	viewdir[0] = dir[0];
	viewdir[1] = dir[1];
	viewdir[2] = 3 * dir[2];
	VecToAngles(viewdir, result.ideal_viewangles);
	//elemantary action
	vec3_t origin = {0, 0, 0};
	EA_Move(ms->client, origin, 0);
	EA_MoveForward(ms->client);
	//set movement view flag so the AI can see the view is focussed
	result.flags |= MOVERESULT_MOVEMENTVIEW;
	//save the movement direction
	VectorCopy(dir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_LadderWolf(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	vec3_t origin = {0, 0, 0};

	// RF, heavily modified, wolf has different ladder movement

	bot_moveresult_t result;
	BotClearMoveResult(&result);

	// if it's a descend reachability
	vec3_t dir;
	if (GGameType & GAME_ET && reach->start[2] > reach->end[2])
	{
		if (!(ms->moveflags & MFL_ONGROUND))
		{
			VectorSubtract(reach->end, reach->start, dir);
			dir[2] = 0;
			float dist = VectorNormalize(dir);
			//set the ideal view angles, facing the ladder up or down
			vec3_t viewdir;
			viewdir[0] = dir[0];
			viewdir[1] = dir[1];
			viewdir[2] = 0;	// straight forward goes up
			if (dist >= 3.1)			// vertical ladder -> ladder reachability has length 3, dont inverse in this case
			{
				VectorInverse(viewdir);
			}
			VectorNormalize(viewdir);
			VecToAngles(viewdir, result.ideal_viewangles);
			//elemantary action
			EA_Move(ms->client, origin, 0);
			EA_MoveBack(ms->client);		// only move back if we are touching the ladder brush
			// check for sideways adjustments to stay on the center of the ladder
			vec3_t p;
			VectorMA(ms->origin, 18, viewdir, p);
			vec3_t v1;
			VectorCopy(reach->start, v1);
			v1[2] = ms->origin[2];
			vec3_t v2;
			VectorCopy(reach->end, v2);
			v2[2] = ms->origin[2];
			vec3_t vec;
			VectorSubtract(v2, v1, vec);
			VectorNormalize(vec);
			VectorMA(v1, -32, vec, v1);
			VectorMA(v2,  32, vec, v2);
			vec3_t pos;
			ProjectPointOntoVectorFromPoints(p, v1, v2, pos);
			VectorSubtract(pos, p, vec);
			if (VectorLength(vec) > 2)
			{
				vec3_t right;
				AngleVectors(result.ideal_viewangles, NULL, right, NULL);
				if (DotProduct(vec, right) > 0)
				{
					EA_MoveRight(ms->client);
				}
				else
				{
					EA_MoveLeft(ms->client);
				}
			}
			//set movement view flag so the AI can see the view is focussed
			result.flags |= MOVERESULT_MOVEMENTVIEW;
		}
		else
		{
			// find a postion back away from the edge of the ladder
			vec3_t hordir;
			VectorSubtract(reach->end, reach->start, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			vec3_t pos;
			VectorMA(reach->start, -24, hordir, pos);
			vec3_t v1;
			VectorCopy(reach->end, v1);
			v1[2] = pos[2];
			// project our position onto the vector
			vec3_t p;
			ProjectPointOntoVectorBounded(ms->origin, pos, v1, p);
			VectorSubtract(p, ms->origin, dir);
			//make sure the horizontal movement is large anough
			VectorCopy(dir, hordir);
			hordir[2] = 0;
			float dist = VectorNormalize(hordir);
			if (dist < 64)			// within range, go for the end
			{
				VectorSubtract(reach->end, reach->start, dir);
				//make sure the horizontal movement is large anough
				dir[2] = 0;
				VectorNormalize(dir);
				//set the ideal view angles, facing the ladder up or down
				vec3_t viewdir;
				viewdir[0] = dir[0];
				viewdir[1] = dir[1];
				viewdir[2] = 0;
				VectorInverse(viewdir);
				VectorNormalize(viewdir);
				VecToAngles(viewdir, result.ideal_viewangles);
				result.flags |= MOVERESULT_MOVEMENTVIEW;
				// if we are still on ground, then start moving backwards until we are in air
				if ((dist < 4) && (ms->moveflags & MFL_ONGROUND))
				{
					vec3_t vec;
					VectorSubtract(reach->end, ms->origin, vec);
					vec[2] = 0;
					VectorNormalize(vec);
					EA_Move(ms->client, vec, 100);
					VectorCopy(vec, result.movedir);
					result.ideal_viewangles[PITCH] = 45;	// face downwards
					return result;
				}
			}

			dir[0] = hordir[0];
			dir[1] = hordir[1];
			dir[2] = 0;
			if (dist > 150)
			{
				dist = 150;
			}
			float speed = 400 - (300 - 2 * dist);
			EA_Move(ms->client, dir, speed);
		}
	}
	else
	{
		if ((ms->moveflags & MFL_AGAINSTLADDER)
			//NOTE: not a good idea for ladders starting in water
			|| !(ms->moveflags & MFL_ONGROUND))
		{
			VectorSubtract(reach->end, reach->start, dir);
			VectorNormalize(dir);
			//set the ideal view angles, facing the ladder up or down
			vec3_t viewdir;
			viewdir[0] = dir[0];
			viewdir[1] = dir[1];
			viewdir[2] = dir[2];
			if (dir[2] < 0)			// going down, so face the other way (towards ladder)
			{
				VectorInverse(viewdir);
			}
			else if (GGameType & GAME_WolfMP)
			{
				viewdir[2] = 0;	// straight forward goes up
			}
			if (!(GGameType & GAME_WolfMP))
			{
				viewdir[2] = 0;	// straight forward goes up
				VectorNormalize(viewdir);
			}
			VecToAngles(viewdir, result.ideal_viewangles);
			//elemantary action
			EA_Move(ms->client, origin, 0);
			if (!(GGameType & GAME_WolfMP) && dir[2] < 0)			// going down, so face the other way
			{
				EA_MoveBack(ms->client);
			}
			else
			{
				EA_MoveForward(ms->client);
			}
			if (!(GGameType & GAME_WolfMP))
			{
				// check for sideways adjustments to stay on the center of the ladder
				vec3_t p;
				VectorMA(ms->origin, 18, viewdir, p);
				vec3_t v1;
				VectorCopy(reach->start, v1);
				v1[2] = ms->origin[2];
				vec3_t v2;
				VectorCopy(reach->end, v2);
				v2[2] = ms->origin[2];
				vec3_t vec;
				VectorSubtract(v2, v1, vec);
				VectorNormalize(vec);
				VectorMA(v1, -32, vec, v1);
				VectorMA(v2,  32, vec, v2);
				vec3_t pos;
				if (GGameType & GAME_ET)
				{
					ProjectPointOntoVectorBounded(p, v1, v2, pos);
				}
				else
				{
					ProjectPointOntoVectorFromPoints(p, v1, v2, pos);
				}
				VectorSubtract(pos, p, vec);
				if (VectorLength(vec) > 2)
				{
					vec3_t right;
					AngleVectors(result.ideal_viewangles, NULL, right, NULL);
					if (DotProduct(vec, right) > 0)
					{
						EA_MoveRight(ms->client);
					}
					else
					{
						EA_MoveLeft(ms->client);
					}
				}
			}
			//set movement view flag so the AI can see the view is focussed
			result.flags |= MOVERESULT_MOVEMENTVIEW;
		}
		else
		{
			// find a postion back away from the base of the ladder
			vec3_t hordir;
			VectorSubtract(reach->end, reach->start, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			vec3_t pos;
			VectorMA(reach->start, -24, hordir, pos);
			if (GGameType & GAME_WolfSP)
			{
				// project our position onto the vector
				vec3_t p;
				ProjectPointOntoVectorFromPoints(ms->origin, pos, reach->start, p);
				VectorSubtract(p, ms->origin, dir);
			}
			else if (GGameType & GAME_WolfMP)
			{
				VectorSubtract(pos, ms->origin, dir);
			}
			else
			{
				vec3_t v1;
				VectorCopy(reach->end, v1);
				v1[2] = pos[2];
				// project our position onto the vector
				vec3_t p;
				ProjectPointOntoVectorBounded(ms->origin, pos, v1, p);
				VectorSubtract(p, ms->origin, dir);
			}
			//make sure the horizontal movement is large anough
			VectorCopy(dir, hordir);
			hordir[2] = 0;
			float dist = VectorNormalize(hordir);
			if (dist < (GGameType & GAME_WolfSP ? 8 : GGameType & GAME_WolfMP ? 32 : 16))	// within range, go for the end
			{
				VectorSubtract(reach->end, ms->origin, dir);
				//make sure the horizontal movement is large anough
				VectorCopy(dir, hordir);
				hordir[2] = 0;
				dist = VectorNormalize(hordir);
				if (!(GGameType & GAME_WolfMP))
				{
					//set the ideal view angles, facing the ladder up or down
					vec3_t viewdir;
					viewdir[0] = dir[0];
					viewdir[1] = dir[1];
					viewdir[2] = 0;
					if (GGameType & GAME_WolfSP && dir[2] < 0)	// going down, so face the other way
					{
						VectorInverse(viewdir);
					}
					VectorNormalize(viewdir);
					VecToAngles(viewdir, result.ideal_viewangles);
					result.flags |= MOVERESULT_MOVEMENTVIEW;
				}
			}

			dir[0] = hordir[0];
			dir[1] = hordir[1];
			dir[2] = 0;
			if (dist > 50)
			{
				dist = 50;
			}
			float speed;
			if (GGameType & GAME_WolfMP)
			{
				speed = 400 - (200 - 4 * dist);
			}
			else
			{
				speed = 400 - (300 - 6 * dist);
			}
			EA_Move(ms->client, dir, speed);
		}
	}
	//save the movement direction
	VectorCopy(dir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_Ladder(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	if (GGameType & GAME_Quake3)
	{
		return BotTravel_LadderQ3(ms, reach);
	}
	else
	{
		return BotTravel_LadderWolf(ms, reach);
	}
}

static bot_moveresult_t BotTravel_Teleport(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//if the bot is being teleported
	if (ms->moveflags & MFL_TELEPORTED)
	{
		return result;
	}

	//walk straight to center of the teleporter
	vec3_t hordir;
	VectorSubtract(reach->start, ms->origin, hordir);
	if (!(ms->moveflags & MFL_SWIMMING))
	{
		hordir[2] = 0;
	}
	float dist = VectorNormalize(hordir);

	BotCheckBlocked(ms, hordir, true, &result);

	if (dist < 30)
	{
		EA_Move(ms->client, hordir, 200);
	}
	else
	{
		EA_Move(ms->client, hordir, 400);
	}

	if (ms->moveflags & MFL_SWIMMING)
	{
		result.flags |= MOVERESULT_SWIMVIEW;
	}

	VectorCopy(hordir, result.movedir);
	return result;
}

static bot_moveresult_t BotTravel_Elevator(bot_movestate_t* ms, const aas_reachability_t* reach)
{
	float dist, dist1, dist2, speed;
	bot_moveresult_t result;

	BotClearMoveResult(&result);
	//if standing on the plat
	if (BotOnMover(ms->origin, ms->entitynum, reach))
	{
		//if vertically not too far from the end point
		if (abs(ms->origin[2] - reach->end[2]) < sv_maxbarrier->value)
		{
			//move to the end point
			vec3_t hordir;
			VectorSubtract(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			if (!BotCheckBarrierJump(ms, hordir, 100))
			{
				EA_Move(ms->client, hordir, 400);
			}
			VectorCopy(hordir, result.movedir);
		}
		//if not really close to the center of the elevator
		else
		{
			vec3_t bottomcenter;
			MoverBottomCenter(reach, bottomcenter);
			vec3_t hordir;
			VectorSubtract(bottomcenter, ms->origin, hordir);
			hordir[2] = 0;
			dist = VectorNormalize(hordir);

			if (dist > 10)
			{
				//move to the center of the plat
				if (dist > 100)
				{
					dist = 100;
				}
				speed = 400 - (400 - 4 * dist);

				EA_Move(ms->client, hordir, speed);
				VectorCopy(hordir, result.movedir);
			}
		}
	}
	else
	{
		//if very near the reachability end
		vec3_t dir;
		VectorSubtract(reach->end, ms->origin, dir);
		dist = VectorLength(dir);
		if (dist < 64)
		{
			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);

			if ((ms->moveflags & MFL_SWIMMING) || !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}
			VectorCopy(dir, result.movedir);

			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//stop using this reachability
			ms->reachability_time = 0;
			return result;
		}
		//get direction and distance to reachability start
		vec3_t dir1;
		VectorSubtract(reach->start, ms->origin, dir1);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir1[2] = 0;
		}
		dist1 = VectorNormalize(dir1);
		//if the elevator isn't down
		if (!MoverDown(reach))
		{
			dist = dist1;
			VectorCopy(dir1, dir);

			BotCheckBlocked(ms, dir, false, &result);

			if (dist > 60)
			{
				dist = 60;
			}
			speed = 360 - (360 - 6 * dist);

			if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}
			VectorCopy(dir, result.movedir);

			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//this isn't a failure... just wait till the elevator comes down
			result.type = RESULTTYPE_ELEVATORUP;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}
		//get direction and distance to elevator bottom center
		vec3_t bottomcenter;
		MoverBottomCenter(reach, bottomcenter);
		vec3_t dir2;
		VectorSubtract(bottomcenter, ms->origin, dir2);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir2[2] = 0;
		}
		dist2 = VectorNormalize(dir2);
		//if very close to the reachability start or
		//closer to the elevator center or
		//between reachability start and elevator center
		if (dist1 < 20 || dist2 < dist1 || DotProduct(dir1, dir2) < 0)
		{
			dist = dist2;
			VectorCopy(dir2, dir);
		}
		else//closer to the reachability start
		{
			dist = dist1;
			VectorCopy(dir1, dir);
		}

		BotCheckBlocked(ms, dir, false, &result);

		if (dist > 60)
		{
			dist = 60;
		}
		speed = 400 - (400 - 6 * dist);
	
		if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
		{
			EA_Move(ms->client, dir, speed);
		}
		VectorCopy(dir, result.movedir);

		if (ms->moveflags & MFL_SWIMMING)
		{
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}
	return result;
}

static bot_moveresult_t BotFinishTravel_Elevator(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t bottomcenter;
	MoverBottomCenter(reach, bottomcenter);
	vec3_t bottomdir;
	VectorSubtract(bottomcenter, ms->origin, bottomdir);

	vec3_t topdir;
	VectorSubtract(reach->end, ms->origin, topdir);

	if (Q_fabs(bottomdir[2]) < Q_fabs(topdir[2]))
	{
		VectorNormalize(bottomdir);
		EA_Move(ms->client, bottomdir, 300);
	}
	else
	{
		VectorNormalize(topdir);
		EA_Move(ms->client, topdir, 300);
	}
	return result;
}

static bot_moveresult_t BotTravel_FuncBobbing(bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t bob_start, bob_end, bob_origin;
	BotFuncBobStartEnd(reach, bob_start, bob_end, bob_origin);
	//if standing ontop of the func_bobbing
	if (BotOnMover(ms->origin, ms->entitynum, reach))
	{
		//if near end point of reachability
		vec3_t dir;
		VectorSubtract(bob_origin, bob_end, dir);
		if (VectorLength(dir) < 24)
		{
			//move to the end point
			vec3_t hordir;
			VectorSubtract(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			if (!BotCheckBarrierJump(ms, hordir, 100))
			{
				EA_Move(ms->client, hordir, 400);
			}
			VectorCopy(hordir, result.movedir);
		}
		//if not really close to the center of the elevator
		else
		{
			vec3_t bottomcenter;
			MoverBottomCenter(reach, bottomcenter);
			vec3_t hordir;
			VectorSubtract(bottomcenter, ms->origin, hordir);
			hordir[2] = 0;
			float dist = VectorNormalize(hordir);

			if (dist > 10)
			{
				//move to the center of the plat
				if (dist > 100)
				{
					dist = 100;
				}
				float speed = 400 - (400 - 4 * dist);
				//
				EA_Move(ms->client, hordir, speed);
				VectorCopy(hordir, result.movedir);
			}
		}
	}
	else
	{
		//if very near the reachability end
		vec3_t dir;
		VectorSubtract(reach->end, ms->origin, dir);
		float dist = VectorLength(dir);
		if (dist < 64)
		{
			if (dist > 60)
			{
				dist = 60;
			}
			float speed = 360 - (360 - 6 * dist);
			//if swimming or no barrier jump
			if ((ms->moveflags & MFL_SWIMMING) || !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}
			VectorCopy(dir, result.movedir);

			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//stop using this reachability
			ms->reachability_time = 0;
			return result;
		}
		//get direction and distance to reachability start
		vec3_t dir1;
		VectorSubtract(reach->start, ms->origin, dir1);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir1[2] = 0;
		}
		float dist1 = VectorNormalize(dir1);
		//if func_bobbing is Not it's start position
		VectorSubtract(bob_origin, bob_start, dir);
		if (VectorLength(dir) > 16)
		{
			dist = dist1;
			VectorCopy(dir1, dir);

			BotCheckBlocked(ms, dir, false, &result);

			if (dist > 60)
			{
				dist = 60;
			}
			float speed = 360 - (360 - 6 * dist);

			if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
			{
				if (speed > 5)
				{
					EA_Move(ms->client, dir, speed);
				}
			}
			VectorCopy(dir, result.movedir);

			if (ms->moveflags & MFL_SWIMMING)
			{
				result.flags |= MOVERESULT_SWIMVIEW;
			}
			//this isn't a failure... just wait till the func_bobbing arrives
			result.type = GGameType & GAME_Quake3 ? Q3RESULTTYPE_WAITFORFUNCBOBBING : RESULTTYPE_ELEVATORUP;
			result.flags |= MOVERESULT_WAITING;
			return result;
		}
		//get direction and distance to func_bob bottom center
		vec3_t bottomcenter;
		MoverBottomCenter(reach, bottomcenter);
		vec3_t dir2;
		VectorSubtract(bottomcenter, ms->origin, dir2);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir2[2] = 0;
		}
		float dist2 = VectorNormalize(dir2);
		//if very close to the reachability start or
		//closer to the elevator center or
		//between reachability start and func_bobbing center
		if (dist1 < 20 || dist2 < dist1 || DotProduct(dir1, dir2) < 0)
		{
			dist = dist2;
			VectorCopy(dir2, dir);
		}
		else//closer to the reachability start
		{
			dist = dist1;
			VectorCopy(dir1, dir);
		}

		BotCheckBlocked(ms, dir, false, &result);

		if (dist > 60)
		{
			dist = 60;
		}
		float speed = 400 - (400 - 6 * dist);

		if (!(ms->moveflags & MFL_SWIMMING) && !BotCheckBarrierJump(ms, dir, 50))
		{
			EA_Move(ms->client, dir, speed);
		}
		VectorCopy(dir, result.movedir);

		if (ms->moveflags & MFL_SWIMMING)
		{
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}
	return result;
}

static bot_moveresult_t BotFinishTravel_FuncBobbing(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t bob_start, bob_end, bob_origin;
	BotFuncBobStartEnd(reach, bob_start, bob_end, bob_origin);

	vec3_t dir;
	VectorSubtract(bob_origin, bob_end, dir);
	float dist = VectorLength(dir);
	//if the func_bobbing is near the end
	if (dist < 16)
	{
		vec3_t hordir;
		VectorSubtract(reach->end, ms->origin, hordir);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			hordir[2] = 0;
		}
		dist = VectorNormalize(hordir);

		if (dist > 60)
		{
			dist = 60;
		}
		float speed = 360 - (360 - 6 * dist);

		if (speed > 5)
		{
			EA_Move(ms->client, dir, speed);
		}
		VectorCopy(dir, result.movedir);

		if (ms->moveflags & MFL_SWIMMING)
		{
			result.flags |= MOVERESULT_SWIMVIEW;
		}
	}
	else
	{
		vec3_t bottomcenter;
		MoverBottomCenter(reach, bottomcenter);
		vec3_t hordir;
		VectorSubtract(bottomcenter, ms->origin, hordir);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			hordir[2] = 0;
		}
		dist = VectorNormalize(hordir);

		if (dist > 5)
		{
			//move to the center of the plat
			if (dist > 100)
			{
				dist = 100;
			}
			float speed = 400 - (400 - 4 * dist);

			EA_Move(ms->client, hordir, speed);
			VectorCopy(hordir, result.movedir);
		}
	}
	return result;
}

static bot_moveresult_t BotTravel_RocketJump(bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t hordir;
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;

	float dist = VectorNormalize(hordir);
	if (GGameType & GAME_Quake3)
	{
		//look in the movement direction
		VecToAngles(hordir, result.ideal_viewangles);
		//look straight down
		result.ideal_viewangles[PITCH] = 90;
	}

	if (dist < 5 && (!(GGameType & GAME_Quake3) ||
		(Q_fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 5 &&
		Q_fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 5)))
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//elemantary action jump
		EA_Jump(ms->client);
		EA_Attack(ms->client);
		EA_Move(ms->client, hordir, 800);

		ms->jumpreach = ms->lastreachnum;
	}
	else
	{
		if (dist > 80)
		{
			dist = 80;
		}
		float speed = 400 - (400 - 5 * dist);
		EA_Move(ms->client, hordir, speed);
	}

	//look in the movement direction
	VecToAngles(hordir, result.ideal_viewangles);
	//look straight down
	result.ideal_viewangles[PITCH] = 90;
	//set the view angles directly
	EA_View(ms->client, result.ideal_viewangles);
	//view is important for the movment
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	//select the rocket launcher
	EA_SelectWeapon(ms->client, GGameType & GAME_Quake3 ? (int)weapindex_rocketlauncher->value : WEAPONINDEX_ROCKET_LAUNCHER);
	//weapon is used for movement
	result.weapon = GGameType & GAME_Quake3 ? (int)weapindex_rocketlauncher->value : WEAPONINDEX_ROCKET_LAUNCHER;
	result.flags |= MOVERESULT_MOVEMENTWEAPON;

	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_BFGJump(bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);

	vec3_t hordir;
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;

	float dist = VectorNormalize(hordir);

	if (dist < 5 &&
		Q_fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 5 &&
		Q_fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 5)
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//elemantary action jump
		EA_Jump(ms->client);
		EA_Attack(ms->client);
		EA_Move(ms->client, hordir, 800);

		ms->jumpreach = ms->lastreachnum;
	}
	else
	{
		if (dist > 80)
		{
			dist = 80;
		}
		float speed = 400 - (400 - 5 * dist);
		EA_Move(ms->client, hordir, speed);
	}
	//look in the movement direction
	VecToAngles(hordir, result.ideal_viewangles);
	//look straight down
	result.ideal_viewangles[PITCH] = 90;
	//set the view angles directly
	EA_View(ms->client, result.ideal_viewangles);
	//view is important for the movment
	result.flags |= MOVERESULT_MOVEMENTVIEWSET;
	//select the rocket launcher
	EA_SelectWeapon(ms->client, (int)weapindex_bfg10k->value);
	//weapon is used for movement
	result.weapon = (int)weapindex_bfg10k->value;
	result.flags |= MOVERESULT_MOVEMENTWEAPON;

	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotFinishTravel_WeaponJump(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//if not jumped yet
	if (!ms->jumpreach)
	{
		return result;
	}

	vec3_t hordir;
	if (GGameType & GAME_Quake3)
	{
		float speed;
		if (!BotAirControl(ms->origin, ms->velocity, reach->end, hordir, &speed))
		{
			//go straight to the reachability end
			VectorSubtract(reach->end, ms->origin, hordir);
			hordir[2] = 0;
			VectorNormalize(hordir);
			speed = 400;
		}
		EA_Move(ms->client, hordir, speed);
	}
	else
	{
		//go straight to the reachability end
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		//always use max speed when traveling through the air
		EA_Move(ms->client, hordir, 800);
	}
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotTravel_JumpPad(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//first walk straight to the reachability start
	vec3_t hordir;
	hordir[0] = reach->start[0] - ms->origin[0];
	hordir[1] = reach->start[1] - ms->origin[1];
	hordir[2] = 0;
	VectorNormalize(hordir);

	BotCheckBlocked(ms, hordir, true, &result);
	float speed = 400;
	//elemantary action move in direction
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotFinishTravel_JumpPad(const bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	vec3_t hordir;
	float speed;
	if (!BotAirControl(ms->origin, ms->velocity, reach->end, hordir, &speed))
	{
		hordir[0] = reach->end[0] - ms->origin[0];
		hordir[1] = reach->end[1] - ms->origin[1];
		hordir[2] = 0;
		VectorNormalize(hordir);
		speed = 400;
	}
	BotCheckBlocked(ms, hordir, true, &result);
	//elemantary action move in direction
	EA_Move(ms->client, hordir, speed);
	VectorCopy(hordir, result.movedir);

	return result;
}

static bot_moveresult_t BotMoveInGoalArea(bot_movestate_t* ms, const bot_goal_t* goal)
{
	bot_moveresult_t result;
	BotClearMoveResult(&result);
	//walk straight to the goal origin
	vec3_t dir;
	dir[0] = goal->origin[0] - ms->origin[0];
	dir[1] = goal->origin[1] - ms->origin[1];
	if (ms->moveflags & MFL_SWIMMING)
	{
		dir[2] = goal->origin[2] - ms->origin[2];
		result.traveltype = TRAVEL_SWIM;
	}
	else
	{
		dir[2] = 0;
		result.traveltype = TRAVEL_WALK;
	}

	float dist = VectorNormalize(dir);
	if (dist > 100 || (!(GGameType & GAME_Quake3) && goal->flags & GFL_NOSLOWAPPROACH))
	{
		dist = 100;
	}
	float speed = 400 - (400 - 4 * dist);
	if (speed < 10)
	{
		speed = 0;
	}

	BotCheckBlocked(ms, dir, true, &result);
	//elemantary action move in direction
	EA_Move(ms->client, dir, speed);
	VectorCopy(dir, result.movedir);

	if (ms->moveflags & MFL_SWIMMING)
	{
		VecToAngles(dir, result.ideal_viewangles);
		result.flags |= MOVERESULT_SWIMVIEW;
	}

	ms->lastreachnum = 0;
	ms->lastareanum = 0;
	ms->lastgoalareanum = goal->areanum;
	VectorCopy(ms->origin, ms->lastorigin);
	if (!(GGameType & GAME_Quake3))
	{
		ms->lasttime = AAS_Time();
	}

	return result;
}

static void BotResetGrapple(bot_movestate_t* ms)
{
	aas_reachability_t reach;
	AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
	//if not using the grapple hook reachability anymore
	if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_GRAPPLEHOOK)
	{
		if ((ms->moveflags & (GGameType & GAME_Quake3 ? Q3MFL_ACTIVEGRAPPLE :
			WOLFMFL_ACTIVEGRAPPLE)) || ms->grapplevisible_time)
		{
			if (GGameType & GAME_Quake3)
			{
				if (offhandgrapple->value)
				{
					EA_Command(ms->client, cmd_grappleoff->string);
				}
				ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
			}
			else
			{
				EA_Command(ms->client, CMD_HOOKOFF);
				ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
			}
			ms->grapplevisible_time = 0;
		}
	}
}

static bot_moveresult_t BotTravel_Grapple(bot_movestate_t* ms, const aas_reachability_t* reach)
{
	bot_moveresult_t result;
	float dist, speed;
	vec3_t dir, viewdir, org;
	int state, areanum;
	bsp_trace_t trace;

	BotClearMoveResult(&result);

	if (GGameType & GAME_Quake3)
	{
		if (ms->moveflags & Q3MFL_GRAPPLERESET)
		{
			if (offhandgrapple->value)
			{
				EA_Command(ms->client, cmd_grappleoff->string);
			}
			ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
			return result;
		}

		if (!(int)offhandgrapple->value)
		{
			result.weapon = weapindex_grapple->value;
			result.flags |= MOVERESULT_MOVEMENTWEAPON;
		}
	}
	else
	{
		if (ms->moveflags & WOLFMFL_GRAPPLERESET)
		{
			EA_Command(ms->client, CMD_HOOKOFF);
			ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
			return result;
		}
	}

	if (ms->moveflags & (GGameType & GAME_Quake3 ? Q3MFL_ACTIVEGRAPPLE : WOLFMFL_ACTIVEGRAPPLE))
	{
		state = GrappleState(ms, reach);

		VectorSubtract(reach->end, ms->origin, dir);
		dir[2] = 0;
		dist = VectorLength(dir);
		//if very close to the grapple end or the grappled is hooked and
		//the bot doesn't get any closer
		if (state && dist < 48)
		{
			if (ms->lastgrappledist - dist < 1)
			{
				if (GGameType & GAME_Quake3)
				{
					if (offhandgrapple->value)
					{
						EA_Command(ms->client, cmd_grappleoff->string);
					}
					ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
					ms->moveflags |= Q3MFL_GRAPPLERESET;
				}
				else
				{
					EA_Command(ms->client, CMD_HOOKOFF);
					ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
					ms->moveflags |= WOLFMFL_GRAPPLERESET;
				}
				ms->reachability_time = 0;	//end the reachability
				if (GGameType & GAME_Quake3)
				{
					return result;
				}
			}
		}
		//if no valid grapple at all, or the grapple hooked and the bot
		//isn't moving anymore
		else if (!state || (state == 2 && dist > ms->lastgrappledist - 2))
		{
			if (ms->grapplevisible_time < AAS_Time() - 0.4)
			{
				if (GGameType & GAME_Quake3)
				{
					if (offhandgrapple->value)
					{
						EA_Command(ms->client, cmd_grappleoff->string);
					}
					ms->moveflags &= ~Q3MFL_ACTIVEGRAPPLE;
					ms->moveflags |= Q3MFL_GRAPPLERESET;
				}
				else
				{
					EA_Command(ms->client, CMD_HOOKOFF);
					ms->moveflags &= ~WOLFMFL_ACTIVEGRAPPLE;
					ms->moveflags |= WOLFMFL_GRAPPLERESET;
				}
				ms->reachability_time = 0;	//end the reachability
				return result;
			}
		}
		else
		{
			ms->grapplevisible_time = AAS_Time();
		}

		if (GGameType & GAME_Quake3 && !(int)offhandgrapple->value)
		{
			EA_Attack(ms->client);
		}
		//remember the current grapple distance
		ms->lastgrappledist = dist;
	}
	else
	{
		ms->grapplevisible_time = AAS_Time();

		VectorSubtract(reach->start, ms->origin, dir);
		if (!(ms->moveflags & MFL_SWIMMING))
		{
			dir[2] = 0;
		}
		VectorAdd(ms->origin, ms->viewoffset, org);
		VectorSubtract(reach->end, org, viewdir);

		dist = VectorNormalize(dir);
		VecToAngles(viewdir, result.ideal_viewangles);
		result.flags |= MOVERESULT_MOVEMENTVIEW;

		if (dist < 5 &&
			Q_fabs(AngleDiff(result.ideal_viewangles[0], ms->viewangles[0])) < 2 &&
			Q_fabs(AngleDiff(result.ideal_viewangles[1], ms->viewangles[1])) < 2)
		{
			if (GGameType & GAME_Quake3)
			{
				//check if the grapple missile path is clear
				VectorAdd(ms->origin, ms->viewoffset, org);
				trace = AAS_Trace(org, NULL, NULL, reach->end, ms->entitynum, BSP46CONTENTS_SOLID);
				VectorSubtract(reach->end, trace.endpos, dir);
				if (VectorLength(dir) > 16)
				{
					result.failure = true;
					return result;
				}
				//activate the grapple
				if (offhandgrapple->value)
				{
					EA_Command(ms->client, cmd_grappleon->string);
				}
				else
				{
					EA_Attack(ms->client);
				}
				ms->moveflags |= Q3MFL_ACTIVEGRAPPLE;
			}
			else
			{
				EA_Command(ms->client, CMD_HOOKON);
				ms->moveflags |= WOLFMFL_ACTIVEGRAPPLE;
			}
			ms->lastgrappledist = 999999;
		}
		else
		{
			if (dist < 70)
			{
				speed = 300 - (300 - 4 * dist);
			}
			else
			{
				speed = 400;
			}

			BotCheckBlocked(ms, dir, true, &result);
			//elemantary action move in direction
			EA_Move(ms->client, dir, speed);
			VectorCopy(dir, result.movedir);
		}
		//if in another area before actually grappling
		areanum = AAS_PointAreaNum(ms->origin);
		if (areanum && areanum != ms->reachareanum)
		{
			ms->reachability_time = 0;
		}
	}
	return result;
}

void BotMoveToGoal(bot_moveresult_t* result, int movestate, const bot_goal_t* goal, int travelflags)
{
	BotClearMoveResult(result);

	bot_movestate_t* ms = BotMoveStateFromHandle(movestate);
	if (!ms)
	{
		return;
	}
	//reset the grapple before testing if the bot has a valid goal
	//because the bot could loose all it's goals when stuck to a wall
	BotResetGrapple(ms);
	//
	if (!goal)
	{
#ifdef DEBUG
		BotImport_Print(PRT_MESSAGE, "client %d: movetogoal -> no goal\n", ms->client);
#endif
		result->failure = true;
		return;
	}
	//remove some of the move flags
	ms->moveflags &= ~(MFL_SWIMMING | MFL_AGAINSTLADDER);
	//set some of the move flags
	//NOTE: the MFL_ONGROUND flag is also set in the higher AI
	if (AAS_OnGround(ms->origin, ms->presencetype, ms->entitynum))
	{
		ms->moveflags |= MFL_ONGROUND;
	}

	int reachnum = 0;
	if (ms->moveflags & MFL_ONGROUND)
	{
		int modeltype, modelnum;

		int ent = BotOnTopOfEntity(ms);

		if (ent != -1)
		{
			modelnum = AAS_EntityModelindex(ent);
			if (modelnum >= 0 && modelnum < MAX_MODELS_Q3)
			{
				modeltype = modeltypes[modelnum];

				if (modeltype == MODELTYPE_FUNC_PLAT)
				{
					aas_reachability_t reach;
					AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
					//if the bot is Not using the elevator
					if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_ELEVATOR ||
						//NOTE: the face number is the plat model number
						(reach.facenum & 0x0000FFFF) != modelnum)
					{
						reachnum = AAS_NextModelReachability(0, modelnum);
						if (reachnum)
						{
							//BotImport_Print(PRT_MESSAGE, "client %d: accidentally ended up on func_plat\n", ms->client);
							AAS_ReachabilityFromNum(reachnum, &reach);
							ms->lastreachnum = reachnum;
							ms->reachability_time = AAS_Time() + BotReachabilityTime(&reach);
						}
						else
						{
							if (bot_developer)
							{
								BotImport_Print(PRT_MESSAGE, "client %d: on func_plat without reachability\n", ms->client);
							}
							result->blocked = true;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}
					result->flags |= MOVERESULT_ONTOPOF_ELEVATOR;
				}
				else if (modeltype == MODELTYPE_FUNC_BOB)
				{
					aas_reachability_t reach;
					AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
					//if the bot is Not using the func bobbing
					if ((reach.traveltype & TRAVELTYPE_MASK) != TRAVEL_FUNCBOB ||
						//NOTE: the face number is the func_bobbing model number
						(reach.facenum & 0x0000FFFF) != modelnum)
					{
						reachnum = AAS_NextModelReachability(0, modelnum);
						if (reachnum)
						{
							//BotImport_Print(PRT_MESSAGE, "client %d: accidentally ended up on func_bobbing\n", ms->client);
							AAS_ReachabilityFromNum(reachnum, &reach);
							ms->lastreachnum = reachnum;
							ms->reachability_time = AAS_Time() + BotReachabilityTime(&reach);
						}
						else
						{
							if (bot_developer)
							{
								BotImport_Print(PRT_MESSAGE, "client %d: on func_bobbing without reachability\n", ms->client);
							}
							result->blocked = true;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}
					result->flags |= MOVERESULT_ONTOPOF_FUNCBOB;
				}
				else if (GGameType & GAME_Quake3)
				{
					if (modeltype == MODELTYPE_FUNC_STATIC || modeltype == MODELTYPE_FUNC_DOOR)
					{
						// check if ontop of a door bridge ?
						ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);
						// if not in a reachability area
						if (!AAS_AreaReachability(ms->areanum))
						{
							result->blocked = true;
							result->blockentity = ent;
							result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
							return;
						}
					}
					else
					{
						result->blocked = true;
						result->blockentity = ent;
						result->flags |= MOVERESULT_ONTOPOFOBSTACLE;
						return;
					}
				}
			}
		}
	}
	//if swimming
	if (AAS_Swimming(ms->origin))
	{
		ms->moveflags |= MFL_SWIMMING;
	}
	//if against a ladder
	if (AAS_AgainstLadder(ms->origin, ms->areanum))
	{
		ms->moveflags |= MFL_AGAINSTLADDER;
	}
	//if the bot is on the ground, swimming or against a ladder
	if (ms->moveflags & (MFL_ONGROUND | MFL_SWIMMING | MFL_AGAINSTLADDER))
	{
		aas_reachability_t lastreach;
		AAS_ReachabilityFromNum(ms->lastreachnum, &lastreach);
		//reachability area the bot is in
		if (GGameType & GAME_ET)
		{
			if (!ms->areanum)
			{
				result->failure = true;
				return;
			}
		}
		else
		{
			ms->areanum = BotFuzzyPointReachabilityArea(ms->origin);

			if (GGameType & GAME_Quake3 && !ms->areanum)
			{
				result->failure = true;
				result->blocked = true;
				result->blockentity = 0;
				result->type = Q3RESULTTYPE_INSOLIDAREA;
				return;
			}
		}
		//if the bot is in the goal area
		if (ms->areanum == goal->areanum)
		{
			*result = BotMoveInGoalArea(ms, goal);
			return;
		}
		//assume we can use the reachability from the last frame
		reachnum = ms->lastreachnum;
		//if there is a last reachability
		if (reachnum)
		{
			aas_reachability_t reach;
			AAS_ReachabilityFromNum(reachnum, &reach);
			//check if the reachability is still valid
			if (!(AAS_TravelFlagForType(reach.traveltype) & travelflags))
			{
				reachnum = 0;
			}
			//special grapple hook case
			else if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_GRAPPLEHOOK)
			{
				if (ms->reachability_time < AAS_Time() ||
					(ms->moveflags & (GGameType & GAME_Quake3 ? Q3MFL_GRAPPLERESET : WOLFMFL_GRAPPLERESET)))
				{
					reachnum = 0;
				}
			}
			//special elevator case
			else if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_ELEVATOR ||
				(reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_FUNCBOB)
			{
				if ((result->flags & MOVERESULT_ONTOPOF_FUNCBOB) ||
					(result->flags & MOVERESULT_ONTOPOF_FUNCBOB))
				{
					ms->reachability_time = AAS_Time() + 5;
				}
				//if the bot was going for an elevator and reached the reachability area
				if (ms->areanum == reach.areanum ||
					ms->reachability_time < AAS_Time())
				{
					reachnum = 0;
				}
			}
			else
			{
				if (GGameType & GAME_ET && ms->reachability_time < AAS_Time())
				{
					// the reachability timed out, add it to the ignore list
					BotAddToAvoidReach(ms, reachnum, AVOIDREACH_TIME_ET + 4);
				}
#ifdef DEBUG
				if (bot_developer)
				{
					if (ms->reachability_time < AAS_Time())
					{
						BotImport_Print(PRT_MESSAGE, "client %d: reachability timeout in ", ms->client);
						AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
						BotImport_Print(PRT_MESSAGE, "\n");
					}
				}
#endif
				//if the goal area changed or the reachability timed out
				//or the area changed
				if (ms->lastgoalareanum != goal->areanum ||
					ms->reachability_time < AAS_Time() ||
					ms->lastareanum != ms->areanum ||
					//@TODO.  The hardcoded distance here should actually be tied to speed.  As it was, 20 was too big for
					// slow moving walking Nazis.
					(!(GGameType & GAME_Quake3) && (ms->lasttime > (AAS_Time() - 0.5)) &&
					(VectorDistance(ms->origin, ms->lastorigin) < (GGameType & GAME_ET ? 5 : 20) * (AAS_Time() - ms->lasttime))))
				{
					reachnum = 0;
				}
			}
		}
		int resultflags = 0;
		//if the bot needs a new reachability
		if (!reachnum)
		{
			//if the area has no reachability links
			if (!AAS_AreaReachability(ms->areanum))
			{
#ifdef DEBUG
				if (bot_developer)
				{
					BotImport_Print(PRT_MESSAGE, "area %d no reachability\n", ms->areanum);
				}
#endif
			}
			//get a new reachability leading towards the goal
			reachnum = BotGetReachabilityToGoal(ms->origin, ms->areanum,
				ms->lastgoalareanum, ms->lastareanum,
				ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries,
				goal, travelflags, travelflags,
				ms->avoidspots, ms->numavoidspots, &resultflags);
			//the area number the reachability starts in
			ms->reachareanum = ms->areanum;
			//reset some state variables
			ms->jumpreach = 0;						//for TRAVEL_JUMP
			ms->moveflags &= ~(GGameType & GAME_Quake3 ? Q3MFL_GRAPPLERESET : WOLFMFL_GRAPPLERESET);	//for TRAVEL_GRAPPLEHOOK
			//if there is a reachability to the goal
			aas_reachability_t reach;
			if (reachnum)
			{
				AAS_ReachabilityFromNum(reachnum, &reach);
				//set a timeout for this reachability
				ms->reachability_time = AAS_Time() + BotReachabilityTime(&reach) + (GGameType & GAME_ET ?
					0.01 * (AAS_AreaTravelTime(ms->areanum, ms->origin, reach.start) * 2) + 100 : 0);

				if (GGameType & GAME_Quake3)
				{
					//add the reachability to the reachabilities to avoid for a while
					BotAddToAvoidReach(ms, reachnum, AVOIDREACH_TIME_Q3);
				}
				if (GGameType & GAME_ET)
				{
					if (reach.traveltype == TRAVEL_LADDER)
					{
						ms->reachability_time += 4.0;	// allow more time to navigate ladders
					}
					//
					if (reach.traveltype != TRAVEL_LADDER)			// don't avoid ladders unless we were unable to reach them in time
					{
						BotAddToAvoidReach(ms, reachnum, 3);		// add a short avoid reach time
					}
				}
			}
#ifdef DEBUG
			else if (bot_developer)
			{
				BotImport_Print(PRT_MESSAGE, "goal not reachable\n");
				Com_Memset(&reach, 0, sizeof(aas_reachability_t));	//make compiler happy
			}
			if (bot_developer)
			{
				//if still going for the same goal
				if (ms->lastgoalareanum == goal->areanum)
				{
					if (ms->lastareanum == reach.areanum)
					{
						BotImport_Print(PRT_MESSAGE, "same goal, going back to previous area\n");
					}
				}
			}
#endif
		}

		ms->lastreachnum = reachnum;
		ms->lastgoalareanum = goal->areanum;
		ms->lastareanum = ms->areanum;
		//if the bot has a reachability
		aas_reachability_t reach;
		if (reachnum)
		{
			//get the reachability from the number
			AAS_ReachabilityFromNum(reachnum, &reach);
			result->traveltype = reach.traveltype;

			if (GGameType & GAME_ET && goal->flags & GFL_DEBUGPATH)
			{
				AAS_ClearShownPolygons();
				AAS_ClearShownDebugLines();
				AAS_PrintTravelType(reach.traveltype);
				// src area
				AAS_ShowAreaPolygons(ms->areanum, 1, true);
				// dest area
				AAS_ShowAreaPolygons(goal->areanum, 3, true);
				// reachability
				AAS_ShowReachability(&reach);
				AAS_ShowAreaPolygons(reach.areanum, 2, true);
			}

			switch (reach.traveltype & TRAVELTYPE_MASK)
			{
			case TRAVEL_WALK: *result = BotTravel_Walk(ms, &reach); break;
			case TRAVEL_CROUCH: *result = BotTravel_Crouch(ms, &reach); break;
			case TRAVEL_BARRIERJUMP: *result = BotTravel_BarrierJump(ms, &reach); break;
			case TRAVEL_LADDER: *result = BotTravel_Ladder(ms, &reach); break;
			case TRAVEL_WALKOFFLEDGE: *result = BotTravel_WalkOffLedge(ms, &reach); break;
			case TRAVEL_JUMP: *result = BotTravel_Jump(ms, &reach); break;
			case TRAVEL_SWIM: *result = BotTravel_Swim(ms, &reach); break;
			case TRAVEL_WATERJUMP: *result = BotTravel_WaterJump(ms, &reach); break;
			case TRAVEL_TELEPORT: *result = BotTravel_Teleport(ms, &reach); break;
			case TRAVEL_ELEVATOR: *result = BotTravel_Elevator(ms, &reach); break;
			case TRAVEL_GRAPPLEHOOK: *result = BotTravel_Grapple(ms, &reach); break;
			case TRAVEL_ROCKETJUMP: *result = BotTravel_RocketJump(ms, &reach); break;
			case TRAVEL_JUMPPAD: *result = BotTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotTravel_FuncBobbing(ms, &reach); break;
			case TRAVEL_BFGJUMP: if (GGameType & GAME_Quake3) { *result = BotTravel_BFGJump(ms, &reach); break; }
			default:
				BotImport_Print(PRT_FATAL, "travel type %d not implemented yet\n", (reach.traveltype & TRAVELTYPE_MASK));
				break;
			}
			if (GGameType & GAME_Quake3)
			{
				result->traveltype = reach.traveltype;
				result->flags |= resultflags;
			}
		}
		else
		{
			result->failure = true;
			if (GGameType & GAME_Quake3)
			{
				result->flags |= resultflags;
			}
			Com_Memset(&reach, 0, sizeof(aas_reachability_t));
		}
#ifdef DEBUG
		if (bot_developer)
		{
			if (result->failure)
			{
				BotImport_Print(PRT_MESSAGE, "client %d: movement failure in ", ms->client);
				AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
				BotImport_Print(PRT_MESSAGE, "\n");
			}
		}
#endif
	}
	else
	{
		//special handling of jump pads when the bot uses a jump pad without knowing it
		bool foundjumppad = false;
		vec3_t end;
		VectorMA(ms->origin, -2 * ms->thinktime, ms->velocity, end);
		int areas[16];
		int numareas = AAS_TraceAreas(ms->origin, end, areas, NULL, 16);
		for (int i = numareas - 1; i >= 0; i--)
		{
			if (AAS_AreaJumpPad(areas[i]))
			{
				foundjumppad = true;
				int lastreachnum = BotGetReachabilityToGoal(end, areas[i],
					ms->lastgoalareanum, ms->lastareanum,
					ms->avoidreach, ms->avoidreachtimes, ms->avoidreachtries,
					goal, travelflags, TFL_JUMPPAD, ms->avoidspots, ms->numavoidspots, NULL);
				if (lastreachnum)
				{
					ms->lastreachnum = lastreachnum;
					ms->lastareanum = areas[i];
					break;
				}
				else
				{
					for (lastreachnum = AAS_NextAreaReachability(areas[i], 0); lastreachnum;
						 lastreachnum = AAS_NextAreaReachability(areas[i], lastreachnum))
					{
						//get the reachability from the number
						aas_reachability_t reach;
						AAS_ReachabilityFromNum(lastreachnum, &reach);
						if ((reach.traveltype & TRAVELTYPE_MASK) == TRAVEL_JUMPPAD)
						{
							ms->lastreachnum = lastreachnum;
							ms->lastareanum = areas[i];
							break;
						}
					}
					if (lastreachnum)
					{
						break;
					}
				}
			}
		}
		if (bot_developer)
		{
			//if a jumppad is found with the trace but no reachability is found
			if (foundjumppad && !ms->lastreachnum)
			{
				BotImport_Print(PRT_MESSAGE, "client %d didn't find jumppad reachability\n", ms->client);
			}
		}

		if (ms->lastreachnum)
		{
			aas_reachability_t reach;
			AAS_ReachabilityFromNum(ms->lastreachnum, &reach);
			result->traveltype = reach.traveltype;

			switch (reach.traveltype & TRAVELTYPE_MASK)
			{
			case TRAVEL_WALK: *result = BotTravel_Walk(ms, &reach); break;
			case TRAVEL_CROUCH:	/*do nothing*/ break;
			case TRAVEL_BARRIERJUMP: *result = BotFinishTravel_BarrierJump(ms, &reach); break;
			case TRAVEL_LADDER: *result = BotTravel_Ladder(ms, &reach); break;
			case TRAVEL_WALKOFFLEDGE: *result = BotFinishTravel_WalkOffLedge(ms, &reach); break;
			case TRAVEL_JUMP: *result = BotFinishTravel_Jump(ms, &reach); break;
			case TRAVEL_SWIM: *result = BotTravel_Swim(ms, &reach); break;
			case TRAVEL_WATERJUMP: *result = BotFinishTravel_WaterJump(ms, &reach); break;
			case TRAVEL_TELEPORT: /*do nothing*/ break;
			case TRAVEL_ELEVATOR: *result = BotFinishTravel_Elevator(ms, &reach); break;
			case TRAVEL_GRAPPLEHOOK: *result = BotTravel_Grapple(ms, &reach); break;
			case TRAVEL_ROCKETJUMP: *result = BotFinishTravel_WeaponJump(ms, &reach); break;
			case TRAVEL_JUMPPAD: *result = BotFinishTravel_JumpPad(ms, &reach); break;
			case TRAVEL_FUNCBOB: *result = BotFinishTravel_FuncBobbing(ms, &reach); break;
			case TRAVEL_BFGJUMP: if (GGameType & GAME_Quake3) { *result = BotFinishTravel_WeaponJump(ms, &reach); break; }
			default:
				BotImport_Print(PRT_FATAL, "(last) travel type %d not implemented yet\n", (reach.traveltype & TRAVELTYPE_MASK));
				break;
			}
			if (GGameType & GAME_Quake3)
			{
				result->traveltype = reach.traveltype;
			}
#ifdef DEBUG
			if (bot_developer)
			{
				if (result->failure)
				{
					BotImport_Print(PRT_MESSAGE, "client %d: movement failure in finish ", ms->client);
					AAS_PrintTravelType(reach.traveltype & TRAVELTYPE_MASK);
					BotImport_Print(PRT_MESSAGE, "\n");
				}
			}
#endif
		}
	}
	//FIXME: is it right to do this here?
	if (result->blocked)
	{
		ms->reachability_time -= 10 * ms->thinktime;
	}
	//copy the last origin
	VectorCopy(ms->origin, ms->lastorigin);
	ms->lasttime = AAS_Time();

	if (GGameType & (GAME_WolfSP | GAME_WolfMP))
	{
		// RF, try to look in the direction we will be moving ahead of time
		if (reachnum > 0 && !(result->flags & (MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW)))
		{
			aas_reachability_t reach;
			AAS_ReachabilityFromNum(reachnum, &reach);
			if (reach.areanum != goal->areanum)
			{
				if (GGameType & GAME_WolfSP)
				{
					int freachnum, straveltime;
					if (AAS_AreaRouteToGoalArea(reach.areanum, reach.end, goal->areanum, travelflags, &straveltime, &freachnum))
					{
						int ltraveltime = 999999;
						int ftraveltime;
						while (AAS_AreaRouteToGoalArea(reach.areanum, reach.end, goal->areanum, travelflags, &ftraveltime, &freachnum))
						{
							// make sure we are not in a loop
							if (ftraveltime > ltraveltime)
							{
								break;
							}
							ltraveltime = ftraveltime;
							//
							AAS_ReachabilityFromNum(freachnum, &reach);
							if (reach.areanum == goal->areanum)
							{
								vec3_t dir;
								VectorSubtract(goal->origin, ms->origin, dir);
								VectorNormalize(dir);
								VecToAngles(dir, result->ideal_viewangles);
								result->flags |= WOLFMOVERESULT_FUTUREVIEW;
								break;
							}
							if (straveltime - ftraveltime > 120)
							{
								vec3_t dir;
								VectorSubtract(reach.end, ms->origin, dir);
								VectorNormalize(dir);
								VecToAngles(dir, result->ideal_viewangles);
								result->flags |= WOLFMOVERESULT_FUTUREVIEW;
								break;
							}
						}
					}
				}
				else
				{
					int ftraveltime, freachnum;
					if (AAS_AreaRouteToGoalArea(reach.areanum, reach.end, goal->areanum, travelflags, &ftraveltime, &freachnum))
					{
						AAS_ReachabilityFromNum(freachnum, &reach);
						vec3_t dir;
						VectorSubtract(reach.end, ms->origin, dir);
						VectorNormalize(dir);
						VecToAngles(dir, result->ideal_viewangles);
						result->flags |= WOLFMOVERESULT_FUTUREVIEW;
					}
				}
			}
			else
			{
				vec3_t dir;
				VectorSubtract(goal->origin, ms->origin, dir);
				VectorNormalize(dir);
				VecToAngles(dir, result->ideal_viewangles);
				result->flags |= WOLFMOVERESULT_FUTUREVIEW;
			}
		}
	}

	//return the movement result
	return;
}

void BotMoveToGoalQ3(bot_moveresult_t* result, int movestate, const bot_goal_q3_t* goal, int travelflags)
{
	BotMoveToGoal(result, movestate, reinterpret_cast<const bot_goal_t*>(goal), travelflags);
}

void BotMoveToGoalET(bot_moveresult_t* result, int movestate, const bot_goal_et_t* goal, int travelflags)
{
	BotMoveToGoal(result, movestate, reinterpret_cast<const bot_goal_t*>(goal), travelflags);
}
