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

#include "core.h"

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char *format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char *format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char *format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char *format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char *format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Log::write("%s", string);
}

void idCommonLocal::DPrintf(const char *format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Log::develWrite("%s", string);
}

void idCommonLocal::Error(const char *format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char *format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];
	
	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

