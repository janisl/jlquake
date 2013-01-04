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

#include "parse.h"
#include "../ui/ui.h"

void CL_ParseCenterPrint(QMsg& message)
{
	SCR_CenterPrint(message.ReadString2());
}

void CL_ParseStuffText(QMsg& message)
{
	const char* s = message.ReadString2();
	common->DPrintf("stufftext: %s\n", s);
	Cbuf_AddText(s);
}
