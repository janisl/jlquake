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

q2game_export_t* ge;

void* svq2_gameLibrary;

//	Sends the contents of the mutlicast buffer to a single client
void SVQ2_Unicast(q2edict_t* ent, qboolean reliable)
{
	if (!ent)
	{
		return;
	}

	int p = Q2_NUM_FOR_EDICT(ent);
	if (p < 1 || p > sv_maxclients->value)
	{
		return;
	}

	client_t* client = svs.clients + (p - 1);

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

//	Debug print to server console
void SVQ2_dprintf(const char* fmt, ...)
{
	char msg[1024];
	va_list argptr;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	common->Printf("%s", msg);
}

//	Print to a single client
void SVQ2_cprintf(q2edict_t* ent, int level, const char* fmt, ...)
{
	int n;
	if (ent)
	{
		n = Q2_NUM_FOR_EDICT(ent);
		if (n < 1 || n > sv_maxclients->value)
		{
			common->Error("cprintf to a non-client");
		}
	}

	char msg[1024];
	va_list argptr;
	va_start(argptr, fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	if (ent)
	{
		SVQ2_ClientPrintf(svs.clients + (n - 1), level, "%s", msg);
	}
	else
	{
		common->Printf("%s", msg);
	}
}

//	centerprint to a single client
void SVQ2_centerprintf(q2edict_t* ent, const char* fmt, ...)
{
	int n = Q2_NUM_FOR_EDICT(ent);
	if (n < 1 || n > sv_maxclients->value)
	{
		return;
	}

	char msg[1024];
	va_list argptr;
	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	sv.multicast.WriteByte(q2svc_centerprint);
	sv.multicast.WriteString2(msg);
	SVQ2_Unicast(ent, true);
}

//	Abort the server with a game error
void SVQ2_error(const char* fmt, ...)
{
	char msg[1024];
	va_list argptr;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, 1024, fmt, argptr);
	va_end(argptr);

	common->Error("Game Error: %s", msg);
}

//	Also sets mins and maxs for inline bmodels
void SVQ2_setmodel(q2edict_t* ent, const char* name)
{
	if (!name)
	{
		common->Error("SVQ2_setmodel: NULL");
	}

	int i = SVQ2_ModelIndex(name);

	ent->s.modelindex = i;

	// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		clipHandle_t mod = CM_InlineModel(String::Atoi(name + 1));
		CM_ModelBounds(mod, ent->mins, ent->maxs);
		SVQ2_LinkEdict(ent);
	}
}

void SVQ2_Configstring(int index, const char* val)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS_Q2)
	{
		common->Error("configstring: bad index %i\n", index);
	}

	if (!val)
	{
		val = "";
	}

	// change the string in sv
	String::Cpy(sv.q2_configstrings[index], val);

	if (sv.state != SS_LOADING)
	{
		// send the update to everyone
		sv.multicast.Clear();
		sv.multicast.WriteChar(q2svc_configstring);
		sv.multicast.WriteShort(index);
		sv.multicast.WriteString2(val);

		SVQ2_Multicast(vec3_origin, Q2MULTICAST_ALL_R);
	}
}

void SVQ2_WriteChar(int c)
{
	sv.multicast.WriteChar(c);
}

void SVQ2_WriteByte(int c)
{
	sv.multicast.WriteByte(c);
}

void SVQ2_WriteShort(int c)
{
	sv.multicast.WriteShort(c);
}

void SVQ2_WriteLong(int c)
{
	sv.multicast.WriteLong(c);
}

void SVQ2_WriteFloat(float f)
{
	sv.multicast.WriteFloat(f);
}

void SVQ2_WriteString(const char* s)
{
	sv.multicast.WriteString2(s);
}

void SVQ2_WritePos(const vec3_t pos)
{
	sv.multicast.WritePos(pos);
}

void SVQ2_WriteDir(const vec3_t dir)
{
	sv.multicast.WriteDir(dir);
}

void SVQ2_WriteAngle(float f)
{
	sv.multicast.WriteAngle(f);
}

//	Also checks portalareas so that doors block sight
qboolean SVQ2_inPVS(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	int area1 = CM_LeafArea(leafnum);
	byte* mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	int area2 = CM_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return false;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		// a door blocks sight
		return false;
	}
	return true;
}

//	Also checks portalareas so that doors block sound
qboolean SVQ2_inPHS(const vec3_t p1, const vec3_t p2)
{
	int leafnum = CM_PointLeafnum(p1);
	int cluster = CM_LeafCluster(leafnum);
	int area1 = CM_LeafArea(leafnum);
	byte* mask = CM_ClusterPHS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	int area2 = CM_LeafArea(leafnum);
	if (mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		// more than one bounce away
		return false;
	}
	if (!CM_AreasConnected(area1, area2))
	{
		// a door blocks hearing
		return false;
	}
	return true;
}

void SVQ2_sound(q2edict_t* entity, int channel, int sound_num, float volume,
	float attenuation, float timeofs)
{
	if (!entity)
	{
		return;
	}
	SVQ2_StartSound(NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}

static void SVQ2_UnloadGame()
{
	Sys_UnloadDll(svq2_gameLibrary);
	svq2_gameLibrary = NULL;
}

//	Called when either the entire server is being killed, or
// it is changing to a different game directory.
void SVQ2_ShutdownGameProgs()
{
	if (!ge)
	{
		return;
	}
	ge->Shutdown();
	SVQ2_UnloadGame();
	ge = NULL;
}

//	Loads the game dll
void* SVQ2_GetGameAPI(void* parms)
{
	void*(*GetGameAPI)(void*);
	char name[MAX_OSPATH];
	char* path;
	const char* gamename = Sys_GetDllName("game");

	if (svq2_gameLibrary)
	{
		common->FatalError("SVQ2_GetGameAPI without SVQ2_UnloadGame");
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
		svq2_gameLibrary = Sys_LoadDll(name);
		if (svq2_gameLibrary)
		{
			common->DPrintf("LoadLibrary (%s)\n",name);
			break;
		}
	}

	GetGameAPI = (void*(*)(void*))Sys_GetDllFunction(svq2_gameLibrary, "GetGameAPI");
	if (!GetGameAPI)
	{
		SVQ2_UnloadGame();
		return NULL;
	}

	return GetGameAPI(parms);
}
