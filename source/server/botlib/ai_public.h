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

//!!!!!!!!!!!!!!! Used by game VM !!!!!!!!!!!!!!!!!!!!!!!!!!!!

//
//	Goal
//

#define GFL_NONE                0
#define GFL_ITEM                1
#define GFL_ROAM                2
#define GFL_DROPPED             4	//	Quake 3
#define GFL_NOSLOWAPPROACH      4	//	Wolf
#define GFL_DEBUGPATH           32

//a bot goal
struct bot_goal_q3_t
{
	vec3_t origin;				//origin of the goal
	int areanum;				//area number of the goal
	vec3_t mins, maxs;			//mins and maxs of the goal
	int entitynum;				//number of the goal entity
	int number;					//goal number
	int flags;					//goal flags
	int iteminfo;				//item information
};

//a bot goal
struct bot_goal_et_t
{
	vec3_t origin;				//origin of the goal
	int areanum;				//area number of the goal
	vec3_t mins, maxs;			//mins and maxs of the goal
	int entitynum;				//number of the goal entity
	int number;					//goal number
	int flags;					//goal flags
	int iteminfo;				//item information
	int urgency;				//how urgent is the goal? should we be allowed to exit autonomy range to reach the goal?
	int goalEndTime;			// When is the shortest time this can end?
};

//interbreed the goal fuzzy logic
void BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
//mutate the goal fuzzy logic
void BotMutateGoalFuzzyLogic(int goalstate, float range);
//get the name name of the goal with the given number
void BotGoalName(int number, char* name, int size);
//reset avoid goals
void BotResetAvoidGoals(int goalstate);
//search for a goal for the given classname, the index can be used
//as a start point for the search when multiple goals are available with that same classname
int BotGetLevelItemGoalQ3(int index, const char* classname, bot_goal_q3_t* goal);
int BotGetLevelItemGoalET(int index, const char* classname, bot_goal_et_t* goal);
//get the map location with the given name
bool BotGetMapLocationGoalQ3(const char* name, bot_goal_q3_t* goal);
bool BotGetMapLocationGoalET(const char* name, bot_goal_et_t* goal);
//get the next camp spot in the map
int BotGetNextCampSpotGoalQ3(int num, bot_goal_q3_t* goal);
int BotGetNextCampSpotGoalET(int num, bot_goal_et_t* goal);
//dump the goal stack
void BotDumpGoalStack(int goalstate);
//push a goal onto the goal stack
void BotPushGoalQ3(int goalstate, bot_goal_q3_t* goal);
void BotPushGoalET(int goalstate, bot_goal_et_t* goal);
//pop a goal from the goal stack
void BotPopGoal(int goalstate);
//empty the bot's goal stack
void BotEmptyGoalStack(int goalstate);
//get the top goal from the stack
int BotGetTopGoalQ3(int goalstate, bot_goal_q3_t* goal);
int BotGetTopGoalET(int goalstate, bot_goal_et_t* goal);
//get the second goal on the stack
int BotGetSecondGoalQ3(int goalstate, bot_goal_q3_t* goal);
int BotGetSecondGoalET(int goalstate, bot_goal_et_t* goal);
//reset the whole goal state, but keep the item weights
void BotResetGoalState(int goalstate);
//loads item weights for the bot
int BotLoadItemWeights(int goalstate, const char* filename);
//frees the item weights of the bot
void BotFreeItemWeights(int goalstate);
//returns the handle of a newly allocated goal state
int BotAllocGoalState(int client);
//free the given goal state
void BotFreeGoalState(int handle);

//
//	Gen
//

bool GeneticParentsAndChildSelection(int numranks, float* ranks, int* parent1, int* parent2, int* child);
