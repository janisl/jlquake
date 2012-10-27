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

#include "qcommon.h"
#include "../client/public.h"

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

//	Both client and server can use this, and it will output
// to the apropriate place.
//	A raw string should NEVER be passed as fmt, because of "%f" type crashers.
void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, sizeof(string), format, argPtr);
	va_end(argPtr);

	// add to redirected message
	if (rd_buffer)
	{
		if (String::Length(string) + String::Length(rd_buffer) > rd_buffersize - 1)
		{
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		String::Cat(rd_buffer, rd_buffersize, string);
		return;
	}

	// echo to console if we're not a dedicated server
	if (com_dedicated && !com_dedicated->integer)
	{
		Con_ConsolePrint(string);
	}

	// echo to dedicated console and early console
	Sys_Print(string);

	// logfile
	Com_LogToFile(string);
}

//	A Printf that only shows up if the "developer" cvar is set
void idCommonLocal::DPrintf(const char* format, ...)
{
	if (!com_developer || !com_developer->integer)
	{
		return;			// don't confuse non-developers with techie stuff...
	}

	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, sizeof(string), format, argPtr);
	va_end(argPtr);

	Printf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Error(ERR_DROP, "%s", string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Error(ERR_FATAL, "%s", string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Error(ERR_ENDGAME, "%s", string);
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Error(ERR_SERVERDISCONNECT, "%s", string);
}

void idCommonLocal::Disconnect(const char* message)
{
	Com_Error(ERR_DISCONNECT, message);
}
