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


#include "client.h"

#include "../game/botlib.h"

extern botlib_export_t* botlib_export;

vm_t* uivm;

extern char cl_cdkey[34];

void CL_GetGlconfig(wsglconfig_t* config);
void CL_AddRefEntityToScene(const wsrefEntity_t* ent);
void CL_RenderScene(const wsrefdef_t* refdef);
int CL_LerpTag(orientation_t* tag,  const wsrefEntity_t* refent, const char* tagName, int startIndex);

/*
====================
GetClientState
====================
*/
static void GetClientState(uiClientState_t* state)
{
	state->connectPacketCount = clc.q3_connectPacketCount;
	state->connState = cls.state;
	String::NCpyZ(state->servername, cls.servername, sizeof(state->servername));
	String::NCpyZ(state->updateInfoString, cls.q3_updateInfoString, sizeof(state->updateInfoString));
	String::NCpyZ(state->messageString, clc.q3_serverMessage, sizeof(state->messageString));
	state->clientNum = cl.ws_snap.ps.clientNum;
}

/*
====================
LAN_LoadCachedServers
====================
*/
void LAN_LoadCachedServers()
{
	// TTimo: stub, this is only relevant to MP, SP kills the servercache.dat (and favorites)
	// show_bug.cgi?id=445
	/*
	  int size;
	  fileHandle_t fileIn;
	  cls.numglobalservers = cls.nummplayerservers = cls.numfavoriteservers = 0;
	  cls.numGlobalServerAddresses = 0;
	  if (FS_SV_FOpenFileRead("servercache.dat", &fileIn)) {
	      FS_Read(&cls.numglobalservers, sizeof(int), fileIn);
	      FS_Read(&cls.nummplayerservers, sizeof(int), fileIn);
	      FS_Read(&cls.numfavoriteservers, sizeof(int), fileIn);
	      FS_Read(&size, sizeof(int), fileIn);
	      if (size == sizeof(cls.globalServers) + sizeof(cls.favoriteServers) + sizeof(cls.mplayerServers)) {
	          FS_Read(&cls.globalServers, sizeof(cls.globalServers), fileIn);
	          FS_Read(&cls.mplayerServers, sizeof(cls.mplayerServers), fileIn);
	          FS_Read(&cls.favoriteServers, sizeof(cls.favoriteServers), fileIn);
	      } else {
	          cls.numglobalservers = cls.nummplayerservers = cls.numfavoriteservers = 0;
	          cls.numGlobalServerAddresses = 0;
	      }
	      FS_FCloseFile(fileIn);
	  }
	*/
}

/*
====================
LAN_SaveServersToCache
====================
*/
void LAN_SaveServersToCache()
{
	// TTimo: stub, this is only relevant to MP, SP kills the servercache.dat (and favorites)
	// show_bug.cgi?id=445
	/*
	  int size;
	  fileHandle_t fileOut;
	#ifdef __MACOS__	//DAJ MacOS file typing
	  {
	      extern _MSL_IMP_EXP_C long _fcreator, _ftype;
	      _ftype = 'WlfB';
	      _fcreator = 'WlfS';
	  }
	#endif
	  fileOut = FS_SV_FOpenFileWrite("servercache.dat");
	  FS_Write(&cls.numglobalservers, sizeof(int), fileOut);
	  FS_Write(&cls.nummplayerservers, sizeof(int), fileOut);
	  FS_Write(&cls.numfavoriteservers, sizeof(int), fileOut);
	  size = sizeof(cls.globalServers) + sizeof(cls.favoriteServers) + sizeof(cls.mplayerServers);
	  FS_Write(&size, sizeof(int), fileOut);
	  FS_Write(&cls.globalServers, sizeof(cls.globalServers), fileOut);
	  FS_Write(&cls.mplayerServers, sizeof(cls.mplayerServers), fileOut);
	  FS_Write(&cls.favoriteServers, sizeof(cls.favoriteServers), fileOut);
	  FS_FCloseFile(fileOut);
	*/
}


/*
====================
LAN_ResetPings
====================
*/
static void LAN_ResetPings(int source)
{
	int count,i;
	q3serverInfo_t* servers = NULL;
	count = 0;

	switch (source)
	{
	case AS_LOCAL:
		servers = &cls.q3_localServers[0];
		count = MAX_OTHER_SERVERS_Q3;
		break;
	case AS_MPLAYER:
		servers = &cls.q3_mplayerServers[0];
		count = MAX_OTHER_SERVERS_Q3;
		break;
	case AS_GLOBAL:
		servers = &cls.q3_globalServers[0];
		count = MAX_GLOBAL_SERVERS_WS;
		break;
	case AS_FAVORITES:
		servers = &cls.q3_favoriteServers[0];
		count = MAX_OTHER_SERVERS_Q3;
		break;
	}
	if (servers)
	{
		for (i = 0; i < count; i++)
		{
			servers[i].ping = -1;
		}
	}
}

/*
====================
LAN_AddServer
====================
*/
static int LAN_AddServer(int source, const char* name, const char* address)
{
	int max, * count, i;
	netadr_t adr;
	q3serverInfo_t* servers = NULL;
	max = MAX_OTHER_SERVERS_Q3;
	count = 0;

	switch (source)
	{
	case AS_LOCAL:
		count = &cls.q3_numlocalservers;
		servers = &cls.q3_localServers[0];
		break;
	case AS_MPLAYER:
		count = &cls.q3_nummplayerservers;
		servers = &cls.q3_mplayerServers[0];
		break;
	case AS_GLOBAL:
		max = MAX_GLOBAL_SERVERS_WS;
		count = &cls.q3_numglobalservers;
		servers = &cls.q3_globalServers[0];
		break;
	case AS_FAVORITES:
		count = &cls.q3_numfavoriteservers;
		servers = &cls.q3_favoriteServers[0];
		break;
	}
	if (servers && *count < max)
	{
		SOCK_StringToAdr(address, &adr, PORT_SERVER);
		for (i = 0; i < *count; i++)
		{
			if (SOCK_CompareAdr(servers[i].adr, adr))
			{
				break;
			}
		}
		if (i >= *count)
		{
			servers[*count].adr = adr;
			String::NCpyZ(servers[*count].hostName, name, sizeof(servers[*count].hostName));
			servers[*count].visible = qtrue;
			(*count)++;
			return 1;
		}
		return 0;
	}
	return -1;
}

/*
====================
LAN_RemoveServer
====================
*/
static void LAN_RemoveServer(int source, const char* addr)
{
	int* count, i;
	q3serverInfo_t* servers = NULL;
	count = 0;
	switch (source)
	{
	case AS_LOCAL:
		count = &cls.q3_numlocalservers;
		servers = &cls.q3_localServers[0];
		break;
	case AS_MPLAYER:
		count = &cls.q3_nummplayerservers;
		servers = &cls.q3_mplayerServers[0];
		break;
	case AS_GLOBAL:
		count = &cls.q3_numglobalservers;
		servers = &cls.q3_globalServers[0];
		break;
	case AS_FAVORITES:
		count = &cls.q3_numfavoriteservers;
		servers = &cls.q3_favoriteServers[0];
		break;
	}
	if (servers)
	{
		netadr_t comp;
		SOCK_StringToAdr(addr, &comp, PORT_SERVER);
		for (i = 0; i < *count; i++)
		{
			if (SOCK_CompareAdr(comp, servers[i].adr))
			{
				int j = i;
				while (j < *count - 1)
				{
					Com_Memcpy(&servers[j], &servers[j + 1], sizeof(servers[j]));
					j++;
				}
				(*count)--;
				break;
			}
		}
	}
}


/*
====================
LAN_GetServerCount
====================
*/
static int LAN_GetServerCount(int source)
{
	switch (source)
	{
	case AS_LOCAL:
		return cls.q3_numlocalservers;
		break;
	case AS_MPLAYER:
		return cls.q3_nummplayerservers;
		break;
	case AS_GLOBAL:
		return cls.q3_numglobalservers;
		break;
	case AS_FAVORITES:
		return cls.q3_numfavoriteservers;
		break;
	}
	return 0;
}

/*
====================
LAN_GetLocalServerAddressString
====================
*/
static void LAN_GetServerAddressString(int source, int n, char* buf, int buflen)
{
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			String::NCpyZ(buf, SOCK_AdrToString(cls.q3_localServers[n].adr), buflen);
			return;
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			String::NCpyZ(buf, SOCK_AdrToString(cls.q3_mplayerServers[n].adr), buflen);
			return;
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS_WS)
		{
			String::NCpyZ(buf, SOCK_AdrToString(cls.q3_globalServers[n].adr), buflen);
			return;
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			String::NCpyZ(buf, SOCK_AdrToString(cls.q3_favoriteServers[n].adr), buflen);
			return;
		}
		break;
	}
	buf[0] = '\0';
}

/*
====================
LAN_GetServerInfo
====================
*/
static void LAN_GetServerInfo(int source, int n, char* buf, int buflen)
{
	char info[MAX_STRING_CHARS];
	q3serverInfo_t* server = NULL;
	info[0] = '\0';
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			server = &cls.q3_localServers[n];
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			server = &cls.q3_mplayerServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS_WS)
		{
			server = &cls.q3_globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			server = &cls.q3_favoriteServers[n];
		}
		break;
	}
	if (server && buf)
	{
		buf[0] = '\0';
		Info_SetValueForKey(info, "hostname", server->hostName, MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "mapname", server->mapName, MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "clients", va("%i",server->clients), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "sv_maxclients", va("%i",server->maxClients), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "ping", va("%i",server->ping), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "minping", va("%i",server->minPing), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "maxping", va("%i",server->maxPing), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "game", server->game, MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "gametype", va("%i",server->gameType), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "nettype", va("%i",server->netType), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "addr", SOCK_AdrToString(server->adr), MAX_INFO_STRING_Q3);
		Info_SetValueForKey(info, "sv_allowAnonymous", va("%i", server->allowAnonymous), MAX_INFO_STRING_Q3);
		String::NCpyZ(buf, info, buflen);
	}
	else
	{
		if (buf)
		{
			buf[0] = '\0';
		}
	}
}

/*
====================
LAN_GetServerPing
====================
*/
static int LAN_GetServerPing(int source, int n)
{
	q3serverInfo_t* server = NULL;
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			server = &cls.q3_localServers[n];
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			server = &cls.q3_mplayerServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS_WS)
		{
			server = &cls.q3_globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			server = &cls.q3_favoriteServers[n];
		}
		break;
	}
	if (server)
	{
		return server->ping;
	}
	return -1;
}

/*
====================
LAN_GetServerPtr
====================
*/
static q3serverInfo_t* LAN_GetServerPtr(int source, int n)
{
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			return &cls.q3_localServers[n];
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			return &cls.q3_mplayerServers[n];
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS_WS)
		{
			return &cls.q3_globalServers[n];
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			return &cls.q3_favoriteServers[n];
		}
		break;
	}
	return NULL;
}

/*
====================
LAN_CompareServers
====================
*/
static int LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2)
{
	int res;
	q3serverInfo_t* server1, * server2;

	server1 = LAN_GetServerPtr(source, s1);
	server2 = LAN_GetServerPtr(source, s2);
	if (!server1 || !server2)
	{
		return 0;
	}

	res = 0;
	switch (sortKey)
	{
	case SORT_HOST:
		res = String::ICmp(server1->hostName, server2->hostName);
		break;

	case SORT_MAP:
		res = String::ICmp(server1->mapName, server2->mapName);
		break;
	case SORT_CLIENTS:
		if (server1->clients < server2->clients)
		{
			res = -1;
		}
		else if (server1->clients > server2->clients)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_GAME:
		if (server1->gameType < server2->gameType)
		{
			res = -1;
		}
		else if (server1->gameType > server2->gameType)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	case SORT_PING:
		if (server1->ping < server2->ping)
		{
			res = -1;
		}
		else if (server1->ping > server2->ping)
		{
			res = 1;
		}
		else
		{
			res = 0;
		}
		break;
	}

	if (sortDir)
	{
		if (res < 0)
		{
			return 1;
		}
		if (res > 0)
		{
			return -1;
		}
		return 0;
	}
	return res;
}

/*
====================
LAN_GetPingQueueCount
====================
*/
static int LAN_GetPingQueueCount(void)
{
	return (CL_GetPingQueueCount());
}

/*
====================
LAN_ClearPing
====================
*/
static void LAN_ClearPing(int n)
{
	CL_ClearPing(n);
}

/*
====================
LAN_GetPing
====================
*/
static void LAN_GetPing(int n, char* buf, int buflen, int* pingtime)
{
	CL_GetPing(n, buf, buflen, pingtime);
}

/*
====================
LAN_GetPingInfo
====================
*/
static void LAN_GetPingInfo(int n, char* buf, int buflen)
{
	CL_GetPingInfo(n, buf, buflen);
}

/*
====================
LAN_MarkServerVisible
====================
*/
static void LAN_MarkServerVisible(int source, int n, qboolean visible)
{
	if (n == -1)
	{
		int count = MAX_OTHER_SERVERS_Q3;
		q3serverInfo_t* server = NULL;
		switch (source)
		{
		case AS_LOCAL:
			server = &cls.q3_localServers[0];
			break;
		case AS_MPLAYER:
			server = &cls.q3_mplayerServers[0];
			break;
		case AS_GLOBAL:
			server = &cls.q3_globalServers[0];
			count = MAX_GLOBAL_SERVERS_WS;
			break;
		case AS_FAVORITES:
			server = &cls.q3_favoriteServers[0];
			break;
		}
		if (server)
		{
			for (n = 0; n < count; n++)
			{
				server[n].visible = visible;
			}
		}

	}
	else
	{
		switch (source)
		{
		case AS_LOCAL:
			if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
			{
				cls.q3_localServers[n].visible = visible;
			}
			break;
		case AS_MPLAYER:
			if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
			{
				cls.q3_mplayerServers[n].visible = visible;
			}
			break;
		case AS_GLOBAL:
			if (n >= 0 && n < MAX_GLOBAL_SERVERS_WS)
			{
				cls.q3_globalServers[n].visible = visible;
			}
			break;
		case AS_FAVORITES:
			if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
			{
				cls.q3_favoriteServers[n].visible = visible;
			}
			break;
		}
	}
}


/*
=======================
LAN_ServerIsVisible
=======================
*/
static int LAN_ServerIsVisible(int source, int n)
{
	switch (source)
	{
	case AS_LOCAL:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			return cls.q3_localServers[n].visible;
		}
		break;
	case AS_MPLAYER:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			return cls.q3_mplayerServers[n].visible;
		}
		break;
	case AS_GLOBAL:
		if (n >= 0 && n < MAX_GLOBAL_SERVERS_WS)
		{
			return cls.q3_globalServers[n].visible;
		}
		break;
	case AS_FAVORITES:
		if (n >= 0 && n < MAX_OTHER_SERVERS_Q3)
		{
			return cls.q3_favoriteServers[n].visible;
		}
		break;
	}
	return qfalse;
}

/*
=======================
LAN_UpdateVisiblePings
=======================
*/
qboolean LAN_UpdateVisiblePings(int source)
{
	return CL_UpdateVisiblePings_f(source);
}

/*
====================
LAN_GetServerStatus
====================
*/
int LAN_GetServerStatus(char* serverAddress, char* serverStatus, int maxLen)
{
	return CL_ServerStatus(serverAddress, serverStatus, maxLen);
}

/*
====================
GetClipboardData
====================
*/
static void GetClipboardData(char* buf, int buflen)
{
	char* cbd;

	cbd = Sys_GetClipboardData();

	if (!cbd)
	{
		*buf = 0;
		return;
	}

	String::NCpyZ(buf, cbd, buflen);

	delete[] cbd;
}

/*
====================
Key_KeynumToStringBuf
====================
*/
static void Key_KeynumToStringBuf(int keynum, char* buf, int buflen)
{
	String::NCpyZ(buf, Key_KeynumToString(keynum, qtrue), buflen);
}

/*
====================
Key_GetBindingBuf
====================
*/
static void Key_GetBindingBuf(int keynum, char* buf, int buflen)
{
	const char* value;

	value = Key_GetBinding(keynum);
	if (value)
	{
		String::NCpyZ(buf, value, buflen);
	}
	else
	{
		*buf = 0;
	}
}

/*
====================
Key_GetCatcher
====================
*/
int Key_GetCatcher(void)
{
	return in_keyCatchers;
}

/*
====================
Ket_SetCatcher
====================
*/
void Key_SetCatcher(int catcher)
{
	in_keyCatchers = catcher;
}


/*
====================
CLUI_GetCDKey
====================
*/
static void CLUI_GetCDKey(char* buf, int buflen)
{
	Cvar* fs;
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		memcpy(buf, &cl_cdkey[16], 16);
		buf[16] = 0;
	}
	else
	{
		memcpy(buf, cl_cdkey, 16);
		buf[16] = 0;
	}
}


/*
====================
CLUI_SetCDKey
====================
*/
static void CLUI_SetCDKey(char* buf)
{
	Cvar* fs;
	fs = Cvar_Get("fs_game", "", CVAR_INIT | CVAR_SYSTEMINFO);
	if (UI_usesUniqueCDKey() && fs && fs->string[0] != 0)
	{
		memcpy(&cl_cdkey[16], buf, 16);
		cl_cdkey[32] = 0;
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
	else
	{
		memcpy(cl_cdkey, buf, 16);
		// set the flag so the fle will be written at the next opportunity
		cvar_modifiedFlags |= CVAR_ARCHIVE;
	}
}


/*
====================
GetConfigString
====================
*/
static int GetConfigString(int index, char* buf, int size)
{
	int offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS_WS)
	{
		return qfalse;
	}

	offset = cl.ws_gameState.stringOffsets[index];
	if (!offset)
	{
		if (size)
		{
			buf[0] = 0;
		}
		return qfalse;
	}

	String::NCpyZ(buf, cl.ws_gameState.stringData + offset, size);

	return qtrue;
}

/*
====================
FloatAsInt
====================
*/
static int FloatAsInt(float f)
{
	int temp;

	*(float*)&temp = f;

	return temp;
}

/*
====================
CL_UISystemCalls

The ui module is making a system call
====================
*/
qintptr CL_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case UI_ERROR:
		Com_Error(ERR_DROP, "%s", VMA(1));
		return 0;

	case UI_PRINT:
		Com_Printf("%s", VMA(1));
		return 0;

	case UI_MILLISECONDS:
		return Sys_Milliseconds();

	case UI_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;

	case UI_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;

	case UI_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;

	case UI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue((char*)VMA(1)));

	case UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case UI_CVAR_SETVALUE:
		Cvar_SetValue((char*)VMA(1), VMF(2));
		return 0;

	case UI_CVAR_RESET:
		Cvar_Reset((char*)VMA(1));
		return 0;

	case UI_CVAR_CREATE:
		Cvar_Get((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], MAX_INFO_STRING_Q3, (char*)VMA(2), args[3]);
		return 0;

	case UI_ARGC:
		return Cmd_Argc();

	case UI_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;

	case UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case UI_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);

	case UI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

//----(SA)	added
	case UI_FS_SEEK:
		FS_Seek(args[1], args[2], args[3]);
		return 0;
//----(SA)	end

	case UI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case UI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;

	case UI_FS_DELETEFILE:
		return FS_Delete((char*)VMA(1));

	case UI_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case UI_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));

	case UI_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));

	case UI_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));

	case UI_R_CLEARSCENE:
		R_ClearScene();
		return 0;

	case UI_R_ADDREFENTITYTOSCENE:
		CL_AddRefEntityToScene((wsrefEntity_t*)VMA(1));
		return 0;

	case UI_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;

	// Ridah
	case UI_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;
	// done.

	case UI_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6]);
		return 0;

	case UI_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;

	case UI_R_RENDERSCENE:
		CL_RenderScene((wsrefdef_t*)VMA(1));
		return 0;

	case UI_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;

	case UI_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case UI_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;

	case UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case UI_CM_LERPTAG:
		return CL_LerpTag((orientation_t*)VMA(1), (wsrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);

	case UI_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));

	case UI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;

//----(SA)	added
	case UI_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);
		return 0;

	case UI_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], false);
		return 0;
//----(SA)	end

	case UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case UI_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;

	case UI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case UI_KEY_SETCATCHER:
		Key_SetCatcher(args[1]);
		return 0;

	case UI_GETCLIPBOARDDATA:
		GetClipboardData((char*)VMA(1), args[2]);
		return 0;

	case UI_GETCLIENTSTATE:
		GetClientState((uiClientState_t*)VMA(1));
		return 0;

	case UI_GETGLCONFIG:
		CL_GetGlconfig((wsglconfig_t*)VMA(1));
		return 0;

	case UI_GETCONFIGSTRING:
		return GetConfigString(args[1], (char*)VMA(2), args[3]);

	case UI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case UI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case UI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (char*)VMA(2), (char*)VMA(3));

	case UI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (char*)VMA(2));
		return 0;

	case UI_LAN_GETPINGQUEUECOUNT:
		return LAN_GetPingQueueCount();

	case UI_LAN_CLEARPING:
		LAN_ClearPing(args[1]);
		return 0;

	case UI_LAN_GETPING:
		LAN_GetPing(args[1], (char*)VMA(2), args[3], (int*)VMA(4));
		return 0;

	case UI_LAN_GETPINGINFO:
		LAN_GetPingInfo(args[1], (char*)VMA(2), args[3]);
		return 0;

	case UI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case UI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case UI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case UI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);

	case UI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], args[3]);
		return 0;

	case UI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);

	case UI_LAN_UPDATEVISIBLEPINGS:
		return LAN_UpdateVisiblePings(args[1]);

	case UI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;

	case UI_LAN_SERVERSTATUS:
		return LAN_GetServerStatus((char*)VMA(1), (char*)VMA(2), args[3]);

	case UI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);

	case UI_MEMORY_REMAINING:
		return Hunk_MemoryRemaining();

	case UI_GET_CDKEY:
		CLUI_GetCDKey((char*)VMA(1), args[2]);
		return 0;

	case UI_SET_CDKEY:
		CLUI_SetCDKey((char*)VMA(1));
		return 0;

	case UI_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;

	case UI_MEMSET:
		return (qintptr)memset(VMA(1), args[2], args[3]);

	case UI_MEMCPY:
		return (qintptr)memcpy(VMA(1), VMA(2), args[3]);

	case UI_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];

	case UI_SIN:
		return FloatAsInt(sin(VMF(1)));

	case UI_COS:
		return FloatAsInt(cos(VMF(1)));

	case UI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case UI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case UI_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case UI_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case UI_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case UI_PC_LOAD_SOURCE:
		return botlib_export->PC_LoadSourceHandle((char*)VMA(1));
	case UI_PC_FREE_SOURCE:
		return botlib_export->PC_FreeSourceHandle(args[1]);
	case UI_PC_READ_TOKEN:
		return botlib_export->PC_ReadTokenHandle(args[1], (q3pc_token_t*)VMA(2));
	case UI_PC_SOURCE_FILE_AND_LINE:
		return botlib_export->PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), args[3]);			//----(SA)	added fadeup time
		return 0;

	case UI_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));

	case UI_CIN_PLAYCINEMATIC:
		Com_DPrintf("UI_CIN_PlayCinematic\n");
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case UI_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case UI_VERIFY_CDKEY:
		return CL_CDKeyValidate((char*)VMA(1), (char*)VMA(2));

	// NERVE - SMF
	case UI_CL_GETLIMBOSTRING:
		return CL_GetLimboString(args[1], (char*)VMA(2));
	// -NERVE - SMF

	default:
		Com_Error(ERR_DROP, "Bad UI system trap: %i", args[0]);

	}

	return 0;
}

/*
====================
CL_ShutdownUI
====================
*/
void CL_ShutdownUI(void)
{
	in_keyCatchers &= ~KEYCATCH_UI;
	cls.q3_uiStarted = qfalse;
	if (!uivm)
	{
		return;
	}
	VM_Call(uivm, UI_SHUTDOWN);
	VM_Free(uivm);
	uivm = NULL;
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI(void)
{
	int v;
//----(SA)	always dll

	uivm = VM_Create("ui", CL_UISystemCalls, VMI_NATIVE);

	if (!uivm)
	{
		Com_Error(ERR_FATAL, "VM_Create on UI failed");
	}

	// sanity check
	v = VM_Call(uivm, UI_GETAPIVERSION);
	if (v != UI_API_VERSION)
	{
		Com_Error(ERR_FATAL, "User Interface is version %d, expected %d", v, UI_API_VERSION);
		cls.q3_uiStarted = qfalse;
	}

	// init for this gamestate
//	VM_Call( uivm, UI_INIT );
	VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
}


qboolean UI_usesUniqueCDKey()
{
	if (uivm)
	{
		return (VM_Call(uivm, UI_HASUNIQUECDKEY) == qtrue);
	}
	else
	{
		return qfalse;
	}
}

/*
====================
UI_GameCommand

See if the current console command is claimed by the ui
====================
*/
qboolean UI_GameCommand(void)
{
	if (!uivm)
	{
		return qfalse;
	}

	return VM_Call(uivm, UI_CONSOLE_COMMAND, cls.realtime);
}
