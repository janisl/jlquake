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

#include "../../client.h"
#include "local.h"
//#include "../quake3/local.h"
//#include "../wolfsp/local.h"
//#include "../wolfmp/local.h"
//#include "../et/local.h"

int entLastVisible[MAX_CLIENTS_WM];

//	The systeminfo configstring has been changed, so parse
// new information out of it.  This will happen at every
// gamestate, and possibly during gameplay.
void CLT3_SystemInfoChanged()
{
	char* systemInfo;
	const char* s, * t;
	char key[BIG_INFO_KEY];
	char value[BIG_INFO_VALUE];

	systemInfo = GGameType & GAME_WolfSP ? cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[Q3CS_SYSTEMINFO] :
		GGameType & GAME_WolfMP ? cl.wm_gameState.stringData + cl.wm_gameState.stringOffsets[Q3CS_SYSTEMINFO] :
		GGameType & GAME_ET ? cl.et_gameState.stringData + cl.et_gameState.stringOffsets[Q3CS_SYSTEMINFO] :
		cl.q3_gameState.stringData + cl.q3_gameState.stringOffsets[Q3CS_SYSTEMINFO];
	// NOTE TTimo:
	// when the serverId changes, any further messages we send to the server will use this new serverId
	// in some cases, outdated cp commands might get sent with this news serverId
	cl.q3_serverId = String::Atoi(Info_ValueForKey(systemInfo, "sv_serverid"));

	Com_Memset(&entLastVisible, 0, sizeof(entLastVisible));

	// don't set any vars when playing a demo
	if (clc.demoplaying)
	{
		return;
	}

	s = Info_ValueForKey(systemInfo, "sv_cheats");
	if (String::Atoi(s) == 0)
	{
		Cvar_SetCheatState();
	}

	// check pure server string
	s = Info_ValueForKey(systemInfo, "sv_paks");
	t = Info_ValueForKey(systemInfo, "sv_pakNames");
	FS_PureServerSetLoadedPaks(s, t);

	s = Info_ValueForKey(systemInfo, "sv_referencedPaks");
	t = Info_ValueForKey(systemInfo, "sv_referencedPakNames");
	FS_PureServerSetReferencedPaks(s, t);

	bool gameSet = false;
	// scan through all the variables in the systeminfo and locally set cvars to match
	s = systemInfo;
	while (s)
	{
		Info_NextPair(&s, key, value);
		if (!key[0])
		{
			break;
		}
		// ehw!
		if (!String::ICmp(key, "fs_game"))
		{
			gameSet = true;
		}

		Cvar_Set(key, value);
	}
	// if game folder should not be set and it is set at the client side
	if (GGameType & GAME_Quake3 && !gameSet && *Cvar_VariableString("fs_game"))
	{
		Cvar_Set("fs_game", "");
	}

	if (GGameType & GAME_ET)
	{
		// Arnout: big hack to clear the image cache on a pure change
		if (Cvar_VariableValue("sv_pure"))
		{
			if (!cl_connectedToPureServer && cls.state <= CA_CONNECTED)
			{
				CLET_PurgeCache();
			}
			cl_connectedToPureServer = true;
		}
		else
		{
			if (cl_connectedToPureServer && cls.state <= CA_CONNECTED)
			{
				CLET_PurgeCache();
			}
			cl_connectedToPureServer = false;
		}
	}
	else
	{
		cl_connectedToPureServer = Cvar_VariableValue("sv_pure");
	}
}
