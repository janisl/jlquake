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
#include "../tech3/local.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SVQ3_NumForGentity(q3sharedEntity_t* ent)
{
	return ((byte*)ent - (byte*)sv.q3_gentities) / sv.q3_gentitySize;
}

q3sharedEntity_t* SVQ3_GentityNum(int num)
{
	return (q3sharedEntity_t*)((byte*)sv.q3_gentities + sv.q3_gentitySize * num);
}
