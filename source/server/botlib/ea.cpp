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
#include "local.h"

#define MAX_USERMOVE                400

//JL BUG BUG BUG In Quake 3 this overlaps with crouch, in Wold games with move left.
#define ACTION_JUMPEDLASTFRAME      128

static bot_input_t* botinputs;

void EA_Gesture(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_GESTURE : WOLFACTION_GESTURE;
}

void EA_SelectWeapon(int client, int weapon)
{
	bot_input_t* bi = &botinputs[client];
	bi->weapon = weapon;
}

void EA_Attack(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= ACTION_ATTACK;
}

void EA_Talk(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_TALK : WOLFACTION_TALK;
}

void EA_Use(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= ACTION_USE;
}

void EA_Respawn(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_RESPAWN : WOLFACTION_RESPAWN;
}

void EA_Jump(int client)
{
	bot_input_t* bi = &botinputs[client];
	if (bi->actionflags & ACTION_JUMPEDLASTFRAME)
	{
		bi->actionflags &= ~(GGameType & GAME_Quake3 ? Q3ACTION_JUMP : WOLFACTION_JUMP);
	}
	else
	{
		bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_JUMP : WOLFACTION_JUMP;
	}
}

void EA_DelayedJump(int client)
{
	bot_input_t* bi = &botinputs[client];
	if (bi->actionflags & ACTION_JUMPEDLASTFRAME)
	{
		bi->actionflags &= ~(GGameType & GAME_Quake3 ? Q3ACTION_DELAYEDJUMP : WOLFACTION_DELAYEDJUMP);
	}
	else
	{
		bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_DELAYEDJUMP : WOLFACTION_DELAYEDJUMP;
	}
}

void EA_Crouch(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_CROUCH : WOLFACTION_CROUCH;
}

void EA_Walk(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_WALK : WOLFACTION_WALK;
}

void EA_MoveUp(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_MOVEUP : WOLFACTION_MOVEUP;
}

void EA_MoveDown(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_MOVEDOWN : WOLFACTION_MOVEDOWN;
}

void EA_MoveForward(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_MOVEFORWARD : WOLFACTION_MOVEFORWARD;
}

void EA_MoveBack(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_MOVEBACK : WOLFACTION_MOVEBACK;
}

void EA_MoveLeft(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_MOVELEFT : WOLFACTION_MOVELEFT;
}

void EA_MoveRight(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= GGameType & GAME_Quake3 ? Q3ACTION_MOVERIGHT : WOLFACTION_MOVERIGHT;
}

void EA_Reload(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= WOLFACTION_RELOAD;
}

void EA_Prone(int client)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= ETACTION_PRONE;
}

void EA_Action(int client, int action)
{
	bot_input_t* bi = &botinputs[client];
	bi->actionflags |= action;
}

void EA_Move(int client, const vec3_t dir, float speed)
{
	bot_input_t* bi = &botinputs[client];

	VectorCopy(dir, bi->dir);
	//cap speed
	if (speed > MAX_USERMOVE)
	{
		speed = MAX_USERMOVE;
	}
	else if (speed < -MAX_USERMOVE)
	{
		speed = -MAX_USERMOVE;
	}
	bi->speed = speed;
}

void EA_View(int client, vec3_t viewangles)
{
	bot_input_t* bi = &botinputs[client];
	VectorCopy(viewangles, bi->viewangles);
}

void EA_GetInput(int client, float thinktime, bot_input_t* input)
{
	bot_input_t* bi = &botinputs[client];
	bi->thinktime = thinktime;
	Com_Memcpy(input, bi, sizeof(bot_input_t));
}

void EA_ResetInputQ3(int client)
{
	bot_input_t* bi;
	int jumped = false;

	bi = &botinputs[client];
	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & Q3ACTION_JUMP;
	bi->actionflags = 0;
	if (jumped)
	{
		bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	}
}

void EA_ResetInputWolf(int client, bot_input_t* init)
{
	bot_input_t* bi;
	int jumped = false;

	bi = &botinputs[client];
	bi->actionflags &= ~ACTION_JUMPEDLASTFRAME;

	bi->thinktime = 0;
	VectorClear(bi->dir);
	bi->speed = 0;
	jumped = bi->actionflags & WOLFACTION_JUMP;
	bi->actionflags = 0;
	if (jumped)
	{
		bi->actionflags |= ACTION_JUMPEDLASTFRAME;
	}

	if (init)
	{
		memcpy(bi, init, sizeof(bot_input_t));
	}
}

int EA_Setup()
{
	//initialize the bot inputs
	botinputs = (bot_input_t*)Mem_ClearedAlloc(botlibglobals.maxclients * sizeof(bot_input_t));
	return BLERR_NOERROR;
}

void EA_Shutdown()
{
	Mem_Free(botinputs);
	botinputs = NULL;
}

void EA_Say(int client, const char* str)
{
	BotClientCommand(client, va("say %s", str));
}

void EA_SayTeam(int client, const char* str)
{
	BotClientCommand(client, va("say_team %s", str));
}

void EA_UseItem(int client, const char* it)
{
	BotClientCommand(client, va("use %s", it));
}

void EA_DropItem(int client, const char* it)
{
	BotClientCommand(client, va("drop %s", it));
}

void EA_UseInv(int client, const char* inv)
{
	BotClientCommand(client, va("invuse %s", inv));
}

void EA_DropInv(int client, const char* inv)
{
	BotClientCommand(client, va("invdrop %s", inv));
}

void EA_Command(int client, const char* command)
{
	BotClientCommand(client, command);
}
