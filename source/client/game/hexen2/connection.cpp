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

//	An h2svc_signonnum has been received, perform a client side setup
void CLH2_SignonReply()
{
	char str[8192];

	common->DPrintf("CLH2_SignonReply: %i\n", clc.qh_signon);

	switch (clc.qh_signon)
	{
	case 1:
		CL_AddReliableCommand("prespawn");
		break;

	case 2:
		CL_AddReliableCommand(va("name \"%s\"\n", clqh_name->string));
		CL_AddReliableCommand(va("playerclass %i\n", (int)clh2_playerclass->value));
		CL_AddReliableCommand(va("color %i %i\n", clqh_color->integer >> 4, clqh_color->integer & 15));
		sprintf(str, "spawn %s", cls.qh_spawnparms);
		CL_AddReliableCommand(str);
		break;

	case 3:
		CL_AddReliableCommand("begin");
		break;

	case 4:
		SCR_EndLoadingPlaque();			// allow normal screen updates
		break;
	}
}
