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

static int SVQ2_FindIndex(const char* name, int start, int max)
{
	if (!name || !name[0])
	{
		return 0;
	}

	int i;
	for (i = 1; i < max && sv.q2_configstrings[start + i][0]; i++)
	{
		if (!String::Cmp(sv.q2_configstrings[start + i], name))
		{
			return i;
		}
	}

	if (i == max)
	{
		common->Error("*Index: overflow");
	}

	String::NCpy(sv.q2_configstrings[start + i], name, sizeof(sv.q2_configstrings[i]));

	if (sv.state != SS_LOADING)
	{
		// send the update to everyone
		sv.multicast.Clear();
		sv.multicast.WriteChar(q2svc_configstring);
		sv.multicast.WriteShort(start + i);
		sv.multicast.WriteString2(name);
		SVQ2_Multicast(vec3_origin, Q2MULTICAST_ALL_R);
	}

	return i;
}

int SVQ2_ModelIndex(const char* name)
{
	return SVQ2_FindIndex(name, Q2CS_MODELS, MAX_MODELS_Q2);
}

int SVQ2_SoundIndex(const char* name)
{
	return SVQ2_FindIndex(name, Q2CS_SOUNDS, MAX_SOUNDS_Q2);
}

int SVQ2_ImageIndex(const char* name)
{
	return SVQ2_FindIndex(name, Q2CS_IMAGES, MAX_IMAGES_Q2);
}

//	Entity baselines are used to compress the update messages
// to the clients -- only the fields that differ from the
// baseline will be transmitted
void SVQ2_CreateBaseline()
{
	for (int entnum = 1; entnum < ge->num_edicts; entnum++)
	{
		q2edict_t* svent = Q2_EDICT_NUM(entnum);
		if (!svent->inuse)
		{
			continue;
		}
		if (!svent->s.modelindex && !svent->s.sound && !svent->s.effects)
		{
			continue;
		}
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		VectorCopy(svent->s.origin, svent->s.old_origin);
		sv.q2_baselines[entnum] = svent->s;
	}
}

void SVQ2_CheckForSavegame()
{
	if (svq2_noreload->value)
	{
		return;
	}

	if (Cvar_VariableValue("deathmatch"))
	{
		return;
	}

	char name[MAX_OSPATH];
	String::Sprintf(name, sizeof(name), "save/current/%s.sav", sv.name);
	if (!FS_FileExists(name))
	{
		// no savegame
		return;

	}
	SV_ClearWorld();

	// get configstrings and areaportals
	SVQ2_ReadLevelFile();

	if (!sv.loadgame)
	{
		// coming back to a level after being in a different
		// level, so run it for ten seconds

		// rlava2 was sending too many lightstyles, and overflowing the
		// reliable data. temporarily changing the server state to loading
		// prevents these from being passed down.
		serverState_t previousState = sv.state;

		sv.state = SS_LOADING;
		for (int i = 0; i < 100; i++)
		{
			ge->RunFrame();
		}

		sv.state = previousState;
	}
}
