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

QLog			GLog;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QLog::QLog
//
//==========================================================================

QLog::QLog()
{
	Com_Memset(Listeners, 0, sizeof(Listeners));
}

//==========================================================================
//
//	QLog::AddListener
//
//==========================================================================

void QLog::AddListener(QLogListener* Listener)
{
	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (!Listeners[i])
		{
			Listeners[i] = Listener;
			return;
		}
	}
}

//==========================================================================
//
//	QLog::RemoveListener
//
//==========================================================================

void QLog::RemoveListener(QLogListener* Listener)
{
	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (Listeners[i] == Listener)
		{
			Listeners[i] = NULL;
		}
	}
}

//==========================================================================
//
//	QLog::Write
//
//==========================================================================

void QLog::Write(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (Listeners[i])
		{
			try
			{
				Listeners[i]->Serialise(string, false);
			}
			catch (...)
			{
			}
		}
	}
}

//==========================================================================
//
//	QLog::WriteLine
//
//==========================================================================

void QLog::WriteLine(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);
	strcat(string, "\n");

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (Listeners[i])
		{
			try
			{
				Listeners[i]->Serialise(string, false);
			}
			catch (...)
			{
			}
		}
	}
}

//==========================================================================
//
//	QLog::DWrite
//
//==========================================================================

void QLog::DWrite(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (Listeners[i])
		{
			try
			{
				Listeners[i]->Serialise(string, true);
			}
			catch (...)
			{
			}
		}
	}
}

//==========================================================================
//
//	QLog::DWriteLine
//
//==========================================================================

void QLog::DWriteLine(const char* Fmt, ...)
{
	va_list		ArgPtr;
	char		string[MAXPRINTMSG];
	
	va_start(ArgPtr, Fmt);
	Q_vsnprintf(string, MAXPRINTMSG, Fmt, ArgPtr);
	va_end(ArgPtr);
	strcat(string, "\n");

	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (Listeners[i])
		{
			try
			{
				Listeners[i]->Serialise(string, true);
			}
			catch (...)
			{
			}
		}
	}
}
