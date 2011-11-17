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

#include "../../client.h"
#include "local.h"

h2entity_state_t clh2_baselines[MAX_EDICTS_H2];

float RTint[256];
float GTint[256];
float BTint[256];

void CLH2_InitColourShadeTables()
{
	for (int i = 0; i < 16; i++)
	{
		int c = ColorIndex[i];

		int r = r_palette[c][0];
		int g = r_palette[c][1];
		int b = r_palette[c][2];

		for (int p = 0; p < 16; p++)
		{
			RTint[i * 16 + p] = ((float)r) / ((float)ColorPercent[15 - p]) ;
			GTint[i * 16 + p] = ((float)g) / ((float)ColorPercent[15 - p]);
			BTint[i * 16 + p] = ((float)b) / ((float)ColorPercent[15 - p]);
		}
	}
}
