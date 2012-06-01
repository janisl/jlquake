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

void MoverBottomCenter(const aas_reachability_t* reach, vec3_t bottomcenter)
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

void BotClearMoveResult(bot_moveresult_t* moveresult)
{
	moveresult->failure = false;
	moveresult->type = 0;
	moveresult->blocked = false;
	moveresult->blockentity = GGameType & GAME_ET ? -1 : 0;
	moveresult->traveltype = 0;
	moveresult->flags = 0;
}

bool BotAirControl(const vec3_t origin, const vec3_t velocity, const vec3_t goal, vec3_t dir, float* speed)
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

void BotFuncBobStartEnd(const aas_reachability_t* reach, vec3_t start, vec3_t end, vec3_t origin)
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

static int GrappleStateQ3(bot_movestate_t* ms, aas_reachability_t* reach)
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

static int GrappleStateWolf(bot_movestate_t* ms, aas_reachability_t* reach)
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
int GrappleState(bot_movestate_t* ms, aas_reachability_t* reach)
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
int BotReachabilityTime(aas_reachability_t* reach)
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

bool MoverDown(const aas_reachability_t* reach)
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

int BotGetReachabilityToGoal(const vec3_t origin, int areanum,
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
