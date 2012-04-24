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

LogListener* Log::listeners[Log::MAX_LISTENERS];

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

void Log::write(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	sendStringToListeners(string);
}

void Log::writeLine(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);
	String::Cat(string, MAXPRINTMSG, "\n");

	sendStringToListeners(string);
}

void Log::sendStringToListeners(const char* string)
{
	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i])
		{
			try
			{
				listeners[i]->serialise(string);
			}
			catch (...)
			{
			}
		}
	}
}

void Log::develWrite(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	sendDevelStringToListeners(string);
}

void Log::develWriteLine(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);
	String::Cat(string, MAXPRINTMSG, "\n");

	sendDevelStringToListeners(string);
}

void Log::sendDevelStringToListeners(const char* string)
{
	for (int i = 0; i < MAX_LISTENERS; i++)
	{
		if (listeners[i])
		{
			try
			{
				listeners[i]->develSerialise(string);
			}
			catch (...)
			{
			}
		}
	}
}
