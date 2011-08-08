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

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

Log			GLog;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	Log::Log
//
//==========================================================================

Log::Log()
{
	Com_Memset(listeners, 0, sizeof(listeners));
}

//==========================================================================
//
//	Log::addListener
//
//==========================================================================

void Log::addListener(LogListener* listener)
{
	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (!listeners[i])
		{
			listeners[i] = listener;
			return;
		}
	}
}

//==========================================================================
//
//	Log::removeListener
//
//==========================================================================

void Log::removeListener(LogListener* listener)
{
	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i] == listener)
		{
			listeners[i] = NULL;
		}
	}
}

//==========================================================================
//
//	Log::Write
//
//==========================================================================

void Log::Write(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i])
		{
			try
			{
				listeners[i]->serialise(string, false);
			}
			catch (...)
			{
			}
		}
	}
}

//==========================================================================
//
//	Log::WriteLine
//
//==========================================================================

void Log::WriteLine(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);
	strcat(string, "\n");

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i])
		{
			try
			{
				listeners[i]->serialise(string, false);
			}
			catch (...)
			{
			}
		}
	}
}

//==========================================================================
//
//	Log::DWrite
//
//==========================================================================

void Log::DWrite(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i])
		{
			try
			{
				listeners[i]->serialise(string, true);
			}
			catch (...)
			{
			}
		}
	}
}

//==========================================================================
//
//	Log::DWriteLine
//
//==========================================================================

void Log::DWriteLine(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);
	strcat(string, "\n");

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i])
		{
			try
			{
				listeners[i]->serialise(string, true);
			}
			catch (...)
			{
			}
		}
	}
}
