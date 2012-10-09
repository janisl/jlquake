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
#include "../quake3/local.h"
#include "../wolfsp/local.h"
#include "../wolfmp/local.h"
#include "../et/local.h"

Cvar* clet_profile;
Cvar* clt3_showServerCommands;
Cvar* clt3_showTimeDelta;
Cvar* clt3_activeAction;
Cvar* clwm_shownuments;			// DHM - Nerve
Cvar* clet_autorecord;

void CLET_PurgeCache()
{
	cls.et_doCachePurge = true;
}

void CLET_DoPurgeCache()
{
	if (!cls.et_doCachePurge)
	{
		return;
	}

	cls.et_doCachePurge = false;

	if (!com_cl_running)
	{
		return;
	}

	if (!com_cl_running->integer)
	{
		return;
	}

	if (!cls.q3_rendererStarted)
	{
		return;
	}

	R_PurgeCache();
}
