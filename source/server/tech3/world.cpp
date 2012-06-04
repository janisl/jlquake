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

void SVT3_UnlinkSvEntity(q3svEntity_t* ent)
{
	q3svEntity_t* scan;
	worldSector_t* ws;

	ws = ent->worldSector;
	if (!ws)
	{
		return;		// not linked in anywhere
	}
	ent->worldSector = NULL;

	if (ws->entities == ent)
	{
		ws->entities = ent->nextEntityInWorldSector;
		return;
	}

	for (scan = ws->entities; scan; scan = scan->nextEntityInWorldSector)
	{
		if (scan->nextEntityInWorldSector == ent)
		{
			scan->nextEntityInWorldSector = ent->nextEntityInWorldSector;
			return;
		}
	}

	common->Printf("WARNING: SV_UnlinkEntity: not found in worldSector\n");
}
