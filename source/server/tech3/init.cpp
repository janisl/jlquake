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

bool SVT3_AddReliableCommand(client_t* cl, int index, const char* cmd)
{
	if (!(GGameType & GAME_WolfSP))
	{
		String::NCpyZ(cl->q3_reliableCommands[index], cmd, sizeof(cl->q3_reliableCommands[index]));
		return true;
	}

	if (!cl->ws_reliableCommands.bufSize)
	{
		return false;
	}

	int length = String::Length(cmd);

	if ((cl->ws_reliableCommands.rover - cl->ws_reliableCommands.buf) + length + 1 >= cl->ws_reliableCommands.bufSize)
	{
		// go back to the start
		cl->ws_reliableCommands.rover = cl->ws_reliableCommands.buf;
	}

	// make sure this position won't overwrite another command
	int i;
	char* ch;
	for (i = length, ch = cl->ws_reliableCommands.rover; i && !*ch; i--, ch++)
	{
		// keep going until we find a bad character, or enough space is found
	}
	// if the test failed
	if (i)
	{
		// find a valid spot to place the new string
		// start at the beginning (keep it simple)
		for (i = 0, ch = cl->ws_reliableCommands.buf; i < cl->ws_reliableCommands.bufSize; i++, ch++)
		{
			if (!*ch && (!i || !*(ch - 1)))			// make sure we dont start at the terminator of another string
			{
				// see if this is the start of a valid segment
				int j;
				char* ch2;
				for (ch2 = ch, j = 0; i < cl->ws_reliableCommands.bufSize - 1 && j < length + 1 && !*ch2; i++, ch2++, j++)
				{
					// loop
				}
				//
				if (j == length + 1)
				{
					// valid segment found
					cl->ws_reliableCommands.rover = ch;
					break;
				}
				//
				if (i == cl->ws_reliableCommands.bufSize - 1)
				{
					// ran out of room, not enough space for string
					return false;
				}
				//
				ch = &cl->ws_reliableCommands.buf[i];	// continue where ch2 left off
			}
		}
	}

	// insert the command at the rover
	cl->ws_reliableCommands.commands[index] = cl->ws_reliableCommands.rover;
	String::NCpyZ(cl->ws_reliableCommands.commands[index], cmd, length + 1);
	cl->ws_reliableCommands.commandLengths[index] = length;

	// move the rover along
	cl->ws_reliableCommands.rover += length + 1;

	return true;
}

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
