//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../client.h"

cdlight_t cl_dlights[MAX_DLIGHTS];

void CL_ClearDlights()
{
	Com_Memset(cl_dlights, 0, sizeof(cl_dlights));
}

cdlight_t* CL_AllocDlight(int key)
{
	// first look for an exact key match
	if (key)
	{
		cdlight_t* dl = cl_dlights;
		for (int i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				Com_Memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	cdlight_t* dl = cl_dlights;
	for (int i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < cl.serverTime)
		{
			Com_Memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	Com_Memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

void CL_RunDLights()
{
	float time = cls.frametime * 0.001;

	cdlight_t* dl = cl_dlights;
	for (int i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
		{
			continue;
		}
		if (dl->die < cl.serverTime)
		{
			dl->radius = 0;
			continue;
		}
		dl->radius -= time * dl->decay;
		if (dl->radius < 0)
		{
			dl->radius = 0;
		}
	}
}

void CL_AddDLights()
{
	cdlight_t* dl = cl_dlights;
	for (int i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius)
		{
			continue;
		}
		R_AddLightToScene(dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}
