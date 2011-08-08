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

LogListener* Log::listeners[Log::MAX_LISTENERS];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

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
//	Log::write
//
//==========================================================================

void Log::write(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

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
//	Log::writeLine
//
//==========================================================================

void Log::writeLine(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);
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
//	Log::develWrite
//
//==========================================================================

void Log::develWrite(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

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
//	Log::develWriteLine
//
//==========================================================================

void Log::develWriteLine(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);
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
