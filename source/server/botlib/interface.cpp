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

void BotImport_Print(int type, const char* fmt, ...)
{
	char str[2048];
	va_list ap;

	va_start(ap, fmt);
	Q_vsnprintf(str, sizeof(str), fmt, ap);
	va_end(ap);

	switch (type)
	{
	case PRT_MESSAGE:
		common->Printf("%s", str);
		break;

	case PRT_WARNING:
		common->Printf(S_COLOR_YELLOW "Warning: %s", str);
		break;

	case PRT_ERROR:
		common->Printf(S_COLOR_RED "Error: %s", str);
		break;

	case PRT_FATAL:
		common->Printf(S_COLOR_RED "Fatal: %s", str);
		break;

	case PRT_EXIT:
		common->Error(S_COLOR_RED "Exit: %s", str);
		break;

	default:
		common->Printf("unknown print type\n");
		break;
	}
}
