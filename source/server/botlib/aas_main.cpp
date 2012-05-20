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

aas_t aasworlds[MAX_AAS_WORLDS];
aas_t* aasworld;

void AAS_Error(const char* fmt, ...)
{
	char str[1024];
	va_list arglist;

	va_start(arglist, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, arglist);
	va_end(arglist);
	BotImport_Print(PRT_FATAL, "%s", str);
}

bool AAS_Loaded()
{
	return aasworld->loaded;
}

bool AAS_Initialized()
{
	return aasworld->initialized;
}

float AAS_Time()
{
	return aasworld->time;
}

void AAS_SetCurrentWorld(int index)
{
	if (index >= MAX_AAS_WORLDS || index < 0)
	{
		AAS_Error("AAS_SetCurrentWorld: index out of range\n");
		return;
	}

	// set the current world pointer
	aasworld = &aasworlds[index];
}
