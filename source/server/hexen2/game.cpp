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
#include "../progsvm/progsvm.h"
#include "../quake_hexen/local.h"
#include "local.h"

void PF_setpuzzlemodel()
{
	qhedict_t* e = G_EDICT(OFS_PARM0);
	const char* m = G_STRING(OFS_PARM1);

	char NewName[256];
	String::Sprintf(NewName, sizeof(NewName), "models/puzzle/%s.mdl", m);
	// check to see if model was properly precached
	int i;
	const char** check = sv.qh_model_precache;
	for (i = 0; *check; i++, check++)
	{
		if (!String::Cmp(*check, NewName))
		{
			break;
		}
	}

	e->SetModel(PR_SetString(ED_NewString(NewName)));

	if (!*check)
	{
		common->Printf("**** NO PRECACHE FOR PUZZLE PIECE:");
		common->Printf("**** %s\n",NewName);

		sv.qh_model_precache[i] = PR_GetString(e->GetModel());
		if (!(GGameType & GAME_HexenWorld))
		{
			sv.models[i] = CM_PrecacheModel(NewName);
		}
	}

	e->v.modelindex = i;

	clipHandle_t mod = sv.models[(int)e->v.modelindex];
	if (mod)
	{
		vec3_t mins;
		vec3_t maxs;
		CM_ModelBounds(mod, mins, maxs);
		SetMinMaxSize(e, mins, maxs);
	}
	else
	{
		SetMinMaxSize(e, vec3_origin, vec3_origin);
	}
}

void PF_precache_sound2()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_sound3()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_sound4()
{
	//mission pack only
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_sound();
}

void PF_precache_model2()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_model();
}

void PF_precache_model3()
{
	if (!qh_registered->value)
	{
		return;
	}

	PF_precache_model();
}

void PF_precache_model4()
{
	if (!qh_registered->value)
	{
		return;
	}
	PF_precache_model();
}
