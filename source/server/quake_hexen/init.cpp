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
//#include "../progsvm/progsvm.h"
#include "local.h"

char qhw_localinfo[QHMAX_LOCALINFO_STRING + 1];	// local game info

//	Moves to the next signon buffer if needed
void SVQH_FlushSignon()
{
	if (sv.qh_signon.cursize < sv.qh_signon.maxsize - (GGameType & GAME_HexenWorld ? 100 : 512))
	{
		return;
	}

	if (sv.qh_num_signon_buffers == MAX_SIGNON_BUFFERS - 1)
	{
		common->Error("sv.qh_num_signon_buffers == MAX_SIGNON_BUFFERS-1");
	}

	sv.qh_signon_buffer_size[sv.qh_num_signon_buffers - 1] = sv.qh_signon.cursize;
	sv.qh_signon._data = sv.qh_signon_buffers[sv.qh_num_signon_buffers];
	sv.qh_num_signon_buffers++;
	sv.qh_signon.cursize = 0;
}

int SVQH_ModelIndex(const char* name)
{
	int i;

	if (!name || !name[0])
	{
		return 0;
	}

	for (i = 0; i < (GGameType & GAME_Hexen2 ? MAX_MODELS_H2 : MAX_MODELS_Q1) && sv.qh_model_precache[i]; i++)
		if (!String::Cmp(sv.qh_model_precache[i], name))
		{
			return i;
		}
	if (i == (GGameType & GAME_Hexen2 ? MAX_MODELS_H2 : MAX_MODELS_Q1) || !sv.qh_model_precache[i])
	{
		if (GGameType & GAME_Hexen2 && !(GGameType & GAME_HexenWorld))
		{
			common->Printf("SVQH_ModelIndex: model %s not precached\n", name);
			return 0;
		}
		common->Error("SVQH_ModelIndex: model %s not precached", name);
	}
	return i;
}
