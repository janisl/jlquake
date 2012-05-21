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

#define MAX_STRINGFIELD             80

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
//	Genetic
//

bool GeneticParentsAndChildSelection(int numranks, float* ranks, int* parent1, int* parent2, int* child);

//
//	Character
//

//loads a bot character from a file
int BotLoadCharacter(const char* charfile, float skill);
//frees a bot character
void BotFreeCharacter(int character);
//returns a float characteristic
float Characteristic_Float(int character, int index);
//returns a bounded float characteristic
float Characteristic_BFloat(int character, int index, float min, float max);
//returns an integer characteristic
int Characteristic_Integer(int character, int index);
//returns a bounded integer characteristic
int Characteristic_BInteger(int character, int index, int min, int max);
//returns a string characteristic
void Characteristic_String(int character, int index, char* buf, int size);

//
//	Weapon
//

//projectile flags
#define PFL_WINDOWDAMAGE            1		//projectile damages through window
#define PFL_RETURN                  2		//set when projectile returns to owner
//weapon flags
#define WFL_FIRERELEASED            1		//set when projectile is fired with key-up event
//damage types
#define DAMAGETYPE_IMPACT           1		//damage on impact
#define DAMAGETYPE_RADIAL           2		//radial damage
#define DAMAGETYPE_VISIBLE          4		//damage to all entities visible to the projectile

struct projectileinfo_t
{
	char name[MAX_STRINGFIELD];
	char model[MAX_STRINGFIELD];
	int flags;
	float gravity;
	int damage;
	float radius;
	int visdamage;
	int damagetype;
	int healthinc;
	float push;
	float detonation;
	float bounce;
	float bouncefric;
	float bouncestop;
};

struct weaponinfo_t
{
	int valid;					//true if the weapon info is valid
	int number;									//number of the weapon
	char name[MAX_STRINGFIELD];
	char model[MAX_STRINGFIELD];
	int level;
	int weaponindex;
	int flags;
	char projectile[MAX_STRINGFIELD];
	int numprojectiles;
	float hspread;
	float vspread;
	float speed;
	float acceleration;
	vec3_t recoil;
	vec3_t offset;
	vec3_t angleoffset;
	float extrazvelocity;
	int ammoamount;
	int ammoindex;
	float activate;
	float reload;
	float spinup;
	float spindown;
	projectileinfo_t proj;						//pointer to the used projectile
};

//loads the weapon weights
int BotLoadWeaponWeights(int weaponstate, const char* filename);
//returns the information of the current weapon
void BotGetWeaponInfo(int weaponstate, int weapon, weaponinfo_t* weaponinfo);
//returns the best weapon to fight with
int BotChooseBestFightWeapon(int weaponstate, int* inventory);
//resets the whole weapon state
void BotResetWeaponState(int weaponstate);
//returns a handle to a newly allocated weapon state
int BotAllocWeaponState();
//frees the weapon state
void BotFreeWeaponState(int weaponstate);

//
//	Chat
//

#define MAX_MESSAGE_SIZE_Q3     256
#define MAX_MESSAGE_SIZE_WOLF   150			//limit in game dll
#define MAX_CHATTYPE_NAME       32
#define MAX_MATCHVARIABLES      8

#define CHAT_GENDERLESS         0
#define CHAT_GENDERFEMALE       1
#define CHAT_GENDERMALE         2

#define CHAT_ALL                0
#define CHAT_TEAM               1
#define CHAT_TELL               2

//a console message
struct bot_consolemessage_q3_t
{
	int handle;
	float time;									//message time
	int type;									//message type
	char message[MAX_MESSAGE_SIZE_Q3];				//message
	int prev;
	int next;	//prev and next in list
};

struct bot_consolemessage_wolf_t
{
	int handle;
	float time;											//message time
	int type;											//message type
	char message[MAX_MESSAGE_SIZE_WOLF];				//message
	bot_consolemessage_wolf_t* prev, * next;	//prev and next in list
};

//match variable
struct bot_matchvariable_q3_t
{
	char offset;
	int length;
};

struct bot_matchvariable_wolf_t
{
	char* ptr;
	int length;
};

//returned to AI when a match is found
struct bot_match_q3_t
{
	char string[MAX_MESSAGE_SIZE_Q3];
	int type;
	int subtype;
	bot_matchvariable_q3_t variables[MAX_MATCHVARIABLES];
};

struct bot_match_wolf_t
{
	char string[MAX_MESSAGE_SIZE_WOLF];
	int type;
	int subtype;
	bot_matchvariable_wolf_t variables[MAX_MATCHVARIABLES];
};
