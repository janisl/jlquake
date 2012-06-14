/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sv_game.c -- interface to the game dll

#include "server.h"

game_export_t* ge;

void* game_library;


/*
===============
PF_Unicast

Sends the contents of the mutlicast buffer to a single client
===============
*/
void PF_Unicast(q2edict_t* ent, qboolean reliable)
{
	int p;
	client_t* client;

	if (!ent)
	{
		return;
	}

	p = Q2_NUM_FOR_EDICT(ent);
	if (p < 1 || p > maxclients->value)
	{
		return;
	}

	client = svs.clients + (p - 1);

	if (reliable)
	{
		client->netchan.message.WriteData(sv.multicast._data, sv.multicast.cursize);
	}
	else
	{
		client->datagram.WriteData(sv.multicast._data, sv.multicast.cursize);
	}

	sv.multicast.Clear();
}


/*
===============
PF_dprintf

Debug print to server console
===============
*/
void PF_dprintf(const char* fmt, ...)
{
	char msg[1024];
	va_list argptr;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	Com_Printf("%s", msg);
}


/*
===============
PF_cprintf

Print to a single client
===============
*/
void PF_cprintf(q2edict_t* ent, int level, const char* fmt, ...)
{
	char msg[1024];
	va_list argptr;
	int n;

	if (ent)
	{
		n = Q2_NUM_FOR_EDICT(ent);
		if (n < 1 || n > maxclients->value)
		{
			Com_Error(ERR_DROP, "cprintf to a non-client");
		}
	}

	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	if (ent)
	{
		SV_ClientPrintf(svs.clients + (n - 1), level, "%s", msg);
	}
	else
	{
		Com_Printf("%s", msg);
	}
}


/*
===============
PF_centerprintf

centerprint to a single client
===============
*/
void PF_centerprintf(q2edict_t* ent, const char* fmt, ...)
{
	char msg[1024];
	va_list argptr;
	int n;

	n = Q2_NUM_FOR_EDICT(ent);
	if (n < 1 || n > maxclients->value)
	{
		return;	// Com_Error (ERR_DROP, "centerprintf to a non-client");

	}
	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	sv.multicast.WriteByte(q2svc_centerprint);
	sv.multicast.WriteString2(msg);
	PF_Unicast(ent, true);
}


/*
===============
PF_error

Abort the server with a game error
===============
*/
void PF_error(const char* fmt, ...)
{
	char msg[1024];
	va_list argptr;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	Com_Error(ERR_DROP, "Game Error: %s", msg);
}


/*
=================
PF_setmodel

Also sets mins and maxs for inline bmodels
=================
*/
void PF_setmodel(q2edict_t* ent, char* name)
{
	int i;
	clipHandle_t mod;

	if (!name)
	{
		Com_Error(ERR_DROP, "PF_setmodel: NULL");
	}

	i = SV_ModelIndex(name);

//	ent->model = name;
	ent->s.modelindex = i;

// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		mod = CM_InlineModel(String::Atoi(name + 1));
		CM_ModelBounds(mod, ent->mins, ent->maxs);
		SV_LinkEdict(ent);
	}

}

/*
===============
PF_Configstring

===============
*/
void PF_Configstring(int index, const char* val)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS_Q2)
	{
		Com_Error(ERR_DROP, "configstring: bad index %i\n", index);
	}

	if (!val)
	{
		val = "";
	}

	// change the string in sv
	String::Cpy(sv.q2_configstrings[index], val);

	if (sv.state != SS_LOADING)
	{	// send the update to everyone
		sv.multicast.Clear();
		sv.multicast.WriteChar(q2svc_configstring);
		sv.multicast.WriteShort(index);
		sv.multicast.WriteString2(val);

		SV_Multicast(vec3_origin, MULTICAST_ALL_R);
	}
}



void PF_WriteChar(int c) {sv.multicast.WriteChar(c); }
void PF_WriteByte(int c) {sv.multicast.WriteByte(c); }
void PF_WriteShort(int c) {sv.multicast.WriteShort(c); }
void PF_WriteLong(int c) {sv.multicast.WriteLong(c); }
void PF_WriteFloat(float f) {sv.multicast.WriteFloat(f); }
void PF_WriteString(char* s) {sv.multicast.WriteString2(s); }
void PF_WritePos(vec3_t pos) {sv.multicast.WritePos(pos); }
void PF_WriteDir(vec3_t dir) {sv.multicast.WriteDir(dir); }
void PF_WriteAngle(float f) {sv.multicast.WriteAngle(f); }


/*
=================
PF_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean PF_inPVS(vec3_t p1, vec3_t p2)
{
	int leafnum;
	int cluster;
	int area1, area2;
	byte* mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		return false;		// a door blocks sight
	}
	return true;
}


/*
=================
PF_inPHS

Also checks portalareas so that doors block sound
=================
*/
qboolean PF_inPHS(vec3_t p1, vec3_t p2)
{
	int leafnum;
	int cluster;
	int area1, area2;
	byte* mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPHS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;		// more than one bounce away
	}
	if (!CM_AreasConnected(area1, area2))
	{
		return false;		// a door blocks hearing

	}
	return true;
}

void PF_StartSound(q2edict_t* entity, int channel, int sound_num, float volume,
	float attenuation, float timeofs)
{
	if (!entity)
	{
		return;
	}
	SV_StartSound(NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}

//==============================================

static void SV_UnloadGame()
{
	Sys_UnloadDll(game_library);
	game_library = NULL;
}

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs(void)
{
	if (!ge)
	{
		return;
	}
	ge->Shutdown();
	SV_UnloadGame();
	ge = NULL;
}

//	Loads the game dll
static void* Sys_GetGameAPI(void* parms)
{
	void*(*GetGameAPI)(void*);
	char name[MAX_OSPATH];
	char* path;
	const char* gamename = Sys_GetDllName("game");

	if (game_library)
	{
		Com_Error(ERR_FATAL, "Sys_GetGameAPI without SV_UnloadGame");
	}

	// run through the search paths
	path = NULL;
	while (1)
	{
		path = FS_NextPath(path);
		if (!path)
		{
			return NULL;		// couldn't find one anywhere
		}
		String::Sprintf(name, sizeof(name), "%s/%s", path, gamename);
		game_library = Sys_LoadDll(name);
		if (game_library)
		{
			Com_DPrintf("LoadLibrary (%s)\n",name);
			break;
		}
	}

	GetGameAPI = (void*(*)(void*))Sys_GetDllFunction(game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		SV_UnloadGame();
		return NULL;
	}

	return GetGameAPI(parms);
}

/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/
void SCR_DebugGraph(float value, int color);

void SV_InitGameProgs(void)
{
	game_import_t import;

	// unload anything we have now
	if (ge)
	{
		SV_ShutdownGameProgs();
	}


	// load a new game dll
	import.multicast = SV_Multicast;
	import.unicast = PF_Unicast;
	import.bprintf = SV_BroadcastPrintf;
	import.dprintf = PF_dprintf;
	import.cprintf = PF_cprintf;
	import.centerprintf = PF_centerprintf;
	import.error = PF_error;

	import.linkentity = SV_LinkEdict;
	import.unlinkentity = SV_UnlinkEdict;
	import.BoxEdicts = SV_AreaEdicts;
	import.trace = SV_Trace;
	import.pointcontents = SV_PointContents;
	import.setmodel = PF_setmodel;
	import.inPVS = PF_inPVS;
	import.inPHS = PF_inPHS;
	import.Pmove = Pmove;

	import.modelindex = SV_ModelIndex;
	import.soundindex = SV_SoundIndex;
	import.imageindex = SV_ImageIndex;

	import.configstring = PF_Configstring;
	import.sound = PF_StartSound;
	import.positioned_sound = SV_StartSound;

	import.WriteChar = PF_WriteChar;
	import.WriteByte = PF_WriteByte;
	import.WriteShort = PF_WriteShort;
	import.WriteLong = PF_WriteLong;
	import.WriteFloat = PF_WriteFloat;
	import.WriteString = PF_WriteString;
	import.WritePosition = PF_WritePos;
	import.WriteDir = PF_WriteDir;
	import.WriteAngle = PF_WriteAngle;

	import.TagMalloc = Z_TagMalloc;
	import.TagFree = Z_Free;
	import.FreeTags = Z_FreeTags;

	import.cvar = Cvar_Get;
	import.cvar_set = Cvar_SetLatched;
	import.cvar_forceset = Cvar_Set;

	import.argc = Cmd_Argc;
	import.argv = Cmd_Argv;
	import.args = Cmd_ArgsUnmodified;
	import.AddCommandString = Cbuf_AddText;

	import.DebugGraph = SCR_DebugGraph;
	import.SetAreaPortalState = CM_SetAreaPortalState;
	import.AreasConnected = CM_AreasConnected;

	ge = (game_export_t*)Sys_GetGameAPI(&import);

	if (!ge)
	{
		Com_Error(ERR_DROP, "failed to load game DLL");
	}
	if (ge->apiversion != GAME_API_VERSION)
	{
		Com_Error(ERR_DROP, "game is version %i, not %i", ge->apiversion,
			GAME_API_VERSION);
	}

	ge->Init();
}
