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

#include "local.h"
#include "../../client_main.h"
#include "../../../common/common_defs.h"

Cvar* clh2_playerclass;
Cvar* clhw_teamcolor;
Cvar* clhw_talksounds;

void CLH2_ClearState()
{
	// clear other arrays
	Com_Memset(h2cl_entities, 0, sizeof(h2cl_entities));
	Com_Memset(clh2_baselines, 0, sizeof(clh2_baselines));
	CLH2_ClearTEnts();
	CLH2_ClearEffects();

	if (!(GGameType & GAME_HexenWorld))
	{
		cl.h2_current_frame = cl.h2_current_sequence = 99;
		cl.h2_reference_frame = cl.h2_last_sequence = 199;
		cl.h2_need_build = 2;
	}

	clh2_plaquemessage = "";

	SbarH2_InvReset();
}
