/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_ai_goal.h
 *
 * desc:		goal AI
 *
 *
 *****************************************************************************/

//remove the goal with the given number from the avoid goals
void BotRemoveFromAvoidGoals(int goalstate, int number);
//dump the avoid goals
void BotDumpAvoidGoals(int goalstate);
//choose the best long term goal item for the bot
int BotChooseLTGItem(int goalstate, vec3_t origin, int* inventory, int travelflags);
//choose the best nearby goal item for the bot
int BotChooseNBGItem(int goalstate, vec3_t origin, int* inventory, int travelflags,
	bot_goal_q3_t* ltg, float maxtime);
//returns true if the bot touches the goal
int BotTouchingGoal(vec3_t origin, bot_goal_q3_t* goal);
//returns true if the goal should be visible but isn't
int BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, bot_goal_q3_t* goal);
//returns the avoid goal time
float BotAvoidGoalTime(int goalstate, int number);
//initializes the items in the level
void BotInitLevelItems(void);
//regularly update dynamic entity items (dropped weapons, flags etc.)
void BotUpdateEntityItems(void);
