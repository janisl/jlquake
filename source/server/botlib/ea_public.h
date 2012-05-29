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

//action flags
#define ACTION_ATTACK           0x0000001
#define ACTION_USE              0x0000002
#define Q3ACTION_RESPAWN        0x0000008
#define Q3ACTION_JUMP           0x0000010
#define Q3ACTION_MOVEUP         0x0000020
#define Q3ACTION_CROUCH         0x0000080
#define Q3ACTION_MOVEDOWN       0x0000100
#define Q3ACTION_MOVEFORWARD    0x0000200
#define Q3ACTION_MOVEBACK       0x0000800
#define Q3ACTION_MOVELEFT       0x0001000
#define Q3ACTION_MOVERIGHT      0x0002000
#define Q3ACTION_DELAYEDJUMP    0x0008000
#define Q3ACTION_TALK           0x0010000
#define Q3ACTION_GESTURE        0x0020000
#define Q3ACTION_WALK           0x0080000
#define WOLFACTION_RESPAWN      4
#define WOLFACTION_JUMP         8
#define WOLFACTION_MOVEUP       8
#define WOLFACTION_CROUCH       16
#define WOLFACTION_MOVEDOWN     16
#define WOLFACTION_MOVEFORWARD  32
#define WOLFACTION_MOVEBACK     64
#define WOLFACTION_MOVELEFT     128
#define WOLFACTION_MOVERIGHT    256
#define WOLFACTION_DELAYEDJUMP  512
#define WOLFACTION_TALK         1024
#define WOLFACTION_GESTURE      2048
#define WOLFACTION_WALK         4096
#define WOLFACTION_RELOAD       8192
#define ETACTION_PRONE          16384

//the bot input, will be converted to an usercmd_t
struct bot_input_t
{
	float thinktime;		//time since last output (in seconds)
	vec3_t dir;				//movement direction
	float speed;			//speed in the range [0, 400]
	vec3_t viewangles;		//the view angles
	int actionflags;		//one of the ACTION_? flags
	int weapon;				//weapon to use
};

//regular elementary actions
void EA_Gesture(int client);
void EA_SelectWeapon(int client, int weapon);
void EA_Attack(int client);
void EA_Talk(int client);
void EA_Use(int client);
void EA_Respawn(int client);
void EA_Jump(int client);
void EA_DelayedJump(int client);
void EA_Crouch(int client);
void EA_Walk(int client);
void EA_MoveUp(int client);
void EA_MoveDown(int client);
void EA_MoveForward(int client);
void EA_MoveBack(int client);
void EA_MoveLeft(int client);
void EA_MoveRight(int client);
void EA_Reload(int client);
void EA_Prone(int client);
void EA_Action(int client, int action);
void EA_Move(int client, vec3_t dir, float speed);
void EA_View(int client, vec3_t viewangles);
