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
//#include "../quake3/local.h"
//#include "../wolfsp//local.h"
//#include "../wolfmp//local.h"
//#include "../et/local.h"

const char* SVT3_GetReliableCommand(client_t* cl, int index)
{
	if (!(GGameType & GAME_WolfSP))
	{
		return cl->q3_reliableCommands[index];
	}

	static const char* nullStr = "";
	if (!cl->ws_reliableCommands.bufSize)
	{
		return nullStr;
	}

	if (!cl->ws_reliableCommands.commandLengths[index])
	{
		return nullStr;
	}

	return cl->ws_reliableCommands.commands[index];
}
