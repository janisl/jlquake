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

//a bot goal
//!!!! Do not change so that mem-copy can be used with game versions.
struct bot_goal_t {
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

//setup the chat AI
int BotSetupChatAI();
//shutdown the chat AI
void BotShutdownChatAI();

//free cached bot characters
void BotShutdownCharacters();

//setup the goal AI
int BotSetupGoalAI( bool singleplayer );
//shut down the goal AI
void BotShutdownGoalAI();

//must be called every map change
void BotSetBrushModelTypes();
//setup movement AI
int BotSetupMoveAI();
//shutdown movement AI
void BotShutdownMoveAI();
int BotFuzzyPointReachabilityArea( const vec3_t origin );

//setup the weapon AI
int BotSetupWeaponAI();
//shut down the weapon AI
void BotShutdownWeaponAI();
