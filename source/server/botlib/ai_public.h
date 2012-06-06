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

//removes the console message from the chat state
void BotRemoveConsoleMessage(int chatstate, int handle);
//adds a console message to the chat state
void BotQueueConsoleMessage(int chatstate, int type, const char* message);
//returns the next console message from the state
int BotNextConsoleMessageQ3(int chatstate, bot_consolemessage_q3_t* cm);
int BotNextConsoleMessageWolf(int chatstate, bot_consolemessage_wolf_t* cm);
//returns the number of console messages currently stored in the state
int BotNumConsoleMessages(int chatstate);
//unify all the white spaces in the string
void UnifyWhiteSpaces(char* string);
//checks if the first string contains the second one, returns index into first string or -1 if not found
int StringContains(const char* str1, const char* str2, bool casesensitive);
//replace all the context related synonyms in the string
void BotReplaceSynonyms(char* string, unsigned int context);
//finds a match for the given string using the match templates
bool BotFindMatchQ3(const char* str, bot_match_q3_t* match, unsigned int context);
bool BotFindMatchWolf(const char* str, bot_match_wolf_t* match, unsigned int context);
//returns a variable from a match
void BotMatchVariableQ3(bot_match_q3_t* match, int variable, char* buf, int size);
void BotMatchVariableWolf(bot_match_wolf_t* match, int variable, char* buf, int size);
//loads a chat file for the chat state
int BotLoadChatFile(int chatstate, const char* chatfile, const char* chatname);
//returns the number of initial chat messages of the given type
int BotNumInitialChats(int chatstate, const char* type);
//selects a chat message of the given type
void BotInitialChat(int chatstate, const char* type, int mcontext,
	const char* var0, const char* var1, const char* var2, const char* var3,
	const char* var4, const char* var5, const char* var6, const char* var7);
//find and select a reply for the given message
bool BotReplyChat(int chatstate, const char* message, int mcontext, int vcontext,
	const char* var0, const char* var1, const char* var2, const char* var3,
	const char* var4, const char* var5, const char* var6, const char* var7);
//returns the length of the currently selected chat message
int BotChatLength(int chatstate);
//get the chat message ready to be output
void BotGetChatMessage(int chatstate, char* buf, int size);
//store the gender of the bot in the chat state
void BotSetChatGender(int chatstate, int gender);
//store the bot name in the chat state
void BotSetChatName(int chatstate, const char* name, int client);
//returns the handle to a newly allocated chat state
int BotAllocChatState();
//frees the chatstate
void BotFreeChatState(int handle);

//
//	Genetic
//

bool GeneticParentsAndChildSelection(int numranks, float* ranks, int* parent1, int* parent2, int* child);

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
//dump the avoid goals
void BotDumpAvoidGoals(int goalstate);
//remove the goal with the given number from the avoid goals
void BotRemoveFromAvoidGoals(int goalstate, int number);
//returns the avoid goal time
float BotAvoidGoalTime(int goalstate, int number);
//set the avoid goal time
void BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
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
//returns true if the bot touches the goal
bool BotTouchingGoalQ3(const vec3_t origin, const bot_goal_q3_t* goal);
bool BotTouchingGoalET(const vec3_t origin, const bot_goal_et_t* goal);

//
//	Move
//

//movement types
#define MOVE_WALK                       1
#define MOVE_CROUCH                     2
#define MOVE_JUMP                       4
#define MOVE_GRAPPLE                    8
#define MOVE_ROCKETJUMP                 16
#define MOVE_BFGJUMP                    32

//move flags
#define MFL_BARRIERJUMP                 1		//bot is performing a barrier jump
#define MFL_ONGROUND                    2		//bot is in the ground
#define MFL_SWIMMING                    4		//bot is swimming
#define MFL_AGAINSTLADDER               8		//bot is against a ladder
#define MFL_WATERJUMP                   16		//bot is waterjumping
#define MFL_TELEPORTED                  32		//bot is being teleported
#define Q3MFL_GRAPPLEPULL               64		//bot is being pulled by the grapple
#define Q3MFL_ACTIVEGRAPPLE             128		//bot is using the grapple hook
#define Q3MFL_GRAPPLERESET              256		//bot has reset the grapple
#define Q3MFL_WALK                      512		//bot should walk slowly
#define WOLFMFL_ACTIVEGRAPPLE           64		//bot is using the grapple hook
#define WOLFMFL_GRAPPLERESET            128		//bot has reset the grapple
#define WOLFMFL_WALK                    256		//bot should walk slowly

// move result flags
#define MOVERESULT_MOVEMENTVIEW         1		//bot uses view for movement
#define MOVERESULT_SWIMVIEW             2		//bot uses view for swimming
#define MOVERESULT_WAITING              4		//bot is waiting for something
#define MOVERESULT_MOVEMENTVIEWSET      8		//bot has set the view in movement code
#define MOVERESULT_MOVEMENTWEAPON       16		//bot uses weapon for movement
#define MOVERESULT_ONTOPOFOBSTACLE      32		//bot is ontop of obstacle
#define MOVERESULT_ONTOPOF_FUNCBOB      64		//bot is ontop of a func_bobbing
#define MOVERESULT_ONTOPOF_ELEVATOR     128		//bot is ontop of an elevator (func_plat)
#define Q3MOVERESULT_BLOCKEDBYAVOIDSPOT 256		//bot is blocked by an avoid spot
#define WOLFMOVERESULT_FUTUREVIEW       256		// RF, if we want to look ahead of time, this is a good direction

// avoid spot types
#define AVOID_CLEAR                     0		//clear all avoid spots
#define AVOID_ALWAYS                    1		//avoid always
#define AVOID_DONTBLOCK                 2		//never totally block

// restult types
#define RESULTTYPE_ELEVATORUP           1		//elevator is up
#define Q3RESULTTYPE_WAITFORFUNCBOBBING 2		//waiting for func bobbing to arrive
#define Q3RESULTTYPE_INSOLIDAREA        8		//stuck in solid area, this is bad
#define WOLFRESULTTYPE_INVISIBLEGRAPPLE 2

//structure used to initialize the movement state
//the or_moveflags MFL_ONGROUND, MFL_TELEPORTED and MFL_WATERJUMP come from the playerstate
struct bot_initmove_q3_t
{
	vec3_t origin;				//origin of the bot
	vec3_t velocity;			//velocity of the bot
	vec3_t viewoffset;			//view offset
	int entitynum;				//entity number of the bot
	int client;					//client number of the bot
	float thinktime;			//time the bot thinks
	int presencetype;			//presencetype of the bot
	vec3_t viewangles;			//view angles of the bot
	int or_moveflags;			//values ored to the movement flags
};

//structure used to initialize the movement state
//the or_moveflags MFL_ONGROUND, MFL_TELEPORTED and MFL_WATERJUMP come from the playerstate
struct bot_initmove_et_t
{
	vec3_t origin;				//origin of the bot
	vec3_t velocity;			//velocity of the bot
	vec3_t viewoffset;			//view offset
	int entitynum;				//entity number of the bot
	int client;					//client number of the bot
	float thinktime;			//time the bot thinks
	int presencetype;			//presencetype of the bot
	vec3_t viewangles;			//view angles of the bot
	int or_moveflags;			//values ored to the movement flags
	int areanum;
};

//NOTE: the ideal_viewangles are only valid if MFL_MOVEMENTVIEW is set
struct bot_moveresult_t
{
	int failure;				//true if movement failed all together
	int type;					//failure or blocked type
	int blocked;				//true if blocked by an entity
	int blockentity;			//entity blocking the bot
	int traveltype;				//last executed travel type
	int flags;					//result flags
	int weapon;					//weapon used for movement
	vec3_t movedir;				//movement direction
	vec3_t ideal_viewangles;	//ideal viewangles for the movement
};

//returns the handle of a newly allocated movestate
int BotAllocMoveState();
//frees the movestate with the given handle
void BotFreeMoveState(int handle);
//initialize movement state before performing any movement
void BotInitMoveStateQ3(int handle, bot_initmove_q3_t* initmove);
//initialize movement state
void BotInitMoveStateET(int handle, bot_initmove_et_t* initmove);
//add a spot to avoid (if type == AVOID_CLEAR all spots are removed)
void BotAddAvoidSpot(int movestate, const vec3_t origin, float radius, int type);
//reset avoid reachability
void BotResetAvoidReach(int movestate);
void BotResetAvoidReachAndMove(int movestate);
//resets the last avoid reachability
void BotResetLastAvoidReach(int movestate);
//resets the whole move state
void BotResetMoveState(int movestate);
//view target based on movement
int BotMovementViewTargetQ3(int movestate, const bot_goal_q3_t* goal, int travelflags, float lookahead, vec3_t target);
int BotMovementViewTargetET(int movestate, const bot_goal_et_t* goal, int travelflags, float lookahead, vec3_t target);
//returns a reachability area if the origin is in one
int BotReachabilityArea(const vec3_t origin, int client);
//predict the position of a player based on movement towards a goal
bool BotPredictVisiblePositionQ3(const vec3_t origin, int areanum, const bot_goal_q3_t* goal, int travelflags, vec3_t target);
bool BotPredictVisiblePositionET(const vec3_t origin, int areanum, const bot_goal_et_t* goal, int travelflags, vec3_t target);
//moves the bot in the specified direction using the specified type of movement
bool BotMoveInDirection(int movestate, const vec3_t dir, float speed, int type);

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
