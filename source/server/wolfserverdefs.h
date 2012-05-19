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

enum
{
	WSGT_FFA,				// free for all
	WSGT_SINGLE_PLAYER = 2,	// single player tournament
	//-- team games go after this --
	WSGT_TEAM,				// team deathmatch
};

enum
{
	WMGT_FFA,				// free for all
	WMGT_SINGLE_PLAYER = 2,	// single player tournament
	//-- team games go after this --
	WMGT_TEAM,				// team deathmatch
	WMGT_WOLF = 5,			// DHM - Nerve :: Wolfenstein Multiplayer
	WMGT_WOLF_STOPWATCH,	// NERVE - SMF - stopwatch gametype
	WMGT_WOLF_CP,			// NERVE - SMF - checkpoint gametype
	WMGT_WOLF_CPH,			// JPW NERVE - Capture & Hold gametype
};
