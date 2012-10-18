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

#ifndef _SERVER_H
#define _SERVER_H

#include "../common/qcommon.h"
#include "../common/file_formats/bsp38.h"
#include "../common/file_formats/md3.h"
#include "public.h"
#include "link.h"
#include "progsvm/edict.h"
#include "quakeserverdefs.h"
#include "hexen2serverdefs.h"
#include "quake2serverdefs.h"
#include "quake3serverdefs.h"
#include "wolfserverdefs.h"
#include "tech3/Entity3.h"
#include "tech3/PlayerState3.h"
#include "global.h"
#include "worldsector.h"

struct ucmd_t
{
	const char* name;
	void (*func)(client_t* cl);
	bool allowedpostmapchange;
};

#define MAX_MASTERS 8				// max recipients for heartbeat packets

extern Cvar* sv_maxclients;

extern netadr_t master_adr[MAX_MASTERS];		// address of the master server

void SV_ExecuteClientCommand(client_t* cl, const char* s, bool clientOK, bool preMapRestart);

//
// Must be provided
//
void CL_MapLoading();
void CL_Drop();

#endif
