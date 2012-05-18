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

#define MAX_LOGFILENAMESIZE     1024

struct logfile_t
{
	char filename[MAX_LOGFILENAMESIZE];
	FILE* fp;
};

static logfile_t logfile;

void Log_Open(const char* filename)
{
	if (!LibVarValue("log", "0"))
	{
		return;
	}
	if (!filename || !String::Length(filename))
	{
		BotImport_Print(PRT_MESSAGE, "openlog <filename>\n");
		return;
	}
	if (logfile.fp)
	{
		BotImport_Print(PRT_ERROR, "log file %s is already opened\n", logfile.filename);
		return;
	}
	logfile.fp = fopen(filename, "wb");
	if (!logfile.fp)
	{
		BotImport_Print(PRT_ERROR, "can't open the log file %s\n", filename);
		return;
	}
	String::NCpy(logfile.filename, filename, MAX_LOGFILENAMESIZE);
	BotImport_Print(PRT_MESSAGE, "Opened log %s\n", logfile.filename);
}

void Log_Shutdown()
{
	if (!logfile.fp)
	{
		return;
	}
	if (fclose(logfile.fp))
	{
		BotImport_Print(PRT_ERROR, "can't close log file %s\n", logfile.filename);
		return;
	}
	logfile.fp = NULL;
	BotImport_Print(PRT_MESSAGE, "Closed log %s\n", logfile.filename);
}

void Log_Write(const char* fmt, ...)
{
	va_list ap;

	if (!logfile.fp)
	{
		return;
	}
	va_start(ap, fmt);
	vfprintf(logfile.fp, fmt, ap);
	va_end(ap);
	fflush(logfile.fp);
}
