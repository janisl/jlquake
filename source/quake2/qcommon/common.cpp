/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// common.c -- misc functions used in client and server
#include "qcommon.h"
#include "../client/client.h"
#include "../../server/public.h"
#include <setjmp.h>


int curtime;

int realtime;

jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame


fileHandle_t log_stats_file;

Cvar* log_stats;
Cvar* timescale;
Cvar* fixedtime;
Cvar* logfile_active;		// 1 = buffer log, 2 = flush after each print
Cvar* showtrace;

static fileHandle_t logfile;

// host_speeds times
int time_before_ref;
int time_after_ref;

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

void idCommonLocal::Printf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Printf("%s", string);
}

void idCommonLocal::DPrintf(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_DPrintf("%s", string);
}

void idCommonLocal::Error(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw DropException(string);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw Exception(string);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	throw EndGameException(string);
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
	va_list argPtr;
	char string[MAXPRINTMSG];

	va_start(argPtr, format);
	Q_vsnprintf(string, MAXPRINTMSG, format, argPtr);
	va_end(argPtr);

	Com_Error(ERR_DISCONNECT, string);
}

/*
============================================================================

CLIENT / SERVER interactions

============================================================================
*/

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.
=============
*/
void Com_Printf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	if (rd_buffer)
	{
		if ((String::Length(msg) + String::Length(rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		String::Cat(rd_buffer, rd_buffersize, msg);
		return;
	}

	Con_ConsolePrint(msg);

	// also echo to debugging console
	Sys_Print(msg);

	// logfile
	if (logfile_active && logfile_active->value)
	{
		if (!logfile)
		{
			logfile = FS_FOpenFileWrite("qconsole.log");
		}
		if (logfile)
		{
			FS_Printf(logfile, "%s", msg);
		}
		if (logfile_active->value > 1)
		{
			FS_Flush(logfile);		// force it to save every time
		}
	}
}


/*
================
Com_DPrintf

A common->Printf that only shows up if the "developer" cvar is set
================
*/
void Com_DPrintf(const char* fmt, ...)
{
	va_list argptr;
	char msg[MAXPRINTMSG];

	if (!com_developer || !com_developer->value)
	{
		return;			// don't confuse non-developers with techie stuff...

	}
	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	common->Printf("%s", msg);
}


/*
=============
Com_Error

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Error(int code, const char* fmt, ...)
{
	va_list argptr;
	static char msg[MAXPRINTMSG];

	if (com_errorEntered)
	{
		Sys_Error("recursive error after: %s", msg);
	}
	com_errorEntered = true;

	va_start(argptr,fmt);
	Q_vsnprintf(msg, MAXPRINTMSG, fmt, argptr);
	va_end(argptr);

	if (code == ERR_DISCONNECT)
	{
		CL_Drop();
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}
	else if (code == ERR_DROP)
	{
		common->Printf("********************\nERROR: %s\n********************\n", msg);
		SV_Shutdown(va("Server crashed: %s\n", msg));
		CL_Drop();
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}
	else
	{
		SV_Shutdown(va("Server fatal crashed: %s\n", msg));
		CL_Shutdown();
	}

	if (logfile)
	{
		FS_FCloseFile(logfile);
		logfile = 0;
	}

	Sys_Error("%s", msg);
}


/*
=============
Com_Quit_f

Both client and server can use this, and it will
do the apropriate things.
=============
*/
void Com_Quit_f(void)
{
	SV_Shutdown("Server quit\n");
	CL_Shutdown();

	if (logfile)
	{
		FS_FCloseFile(logfile);
		logfile = 0;
	}

	Sys_Quit();
}

/// just for debugging
int memsearch(byte* start, int count, int search)
{
	int i;

	for (i = 0; i < count; i++)
		if (start[i] == search)
		{
			return i;
		}
	return -1;
}

//============================================================================


/*
====================
COM_BlockSequenceCheckByte

For proxy protecting

// THIS IS MASSIVELY BROKEN!  CHALLENGE MAY BE NEGATIVE
// DON'T USE THIS FUNCTION!!!!!

====================
*/
byte    COM_BlockSequenceCheckByte(byte* base, int length, int sequence, int challenge)
{
	Sys_Error("COM_BlockSequenceCheckByte called\n");

#if 0
	int checksum;
	byte buf[68];
	byte* p;
	float temp;
	byte c;

	temp = bytedirs[(sequence / 3) % NUMVERTEXNORMALS][sequence % 3];
	temp = LittleFloat(temp);
	p = ((byte*)&temp);

	if (length > 60)
	{
		length = 60;
	}
	Com_Memcpy(buf, base, length);

	buf[length] = (sequence & 0xff) ^ p[0];
	buf[length + 1] = p[1];
	buf[length + 2] = ((sequence >> 8) & 0xff) ^ p[2];
	buf[length + 3] = p[3];

	temp = bytedirs[((sequence + challenge) / 3) % NUMVERTEXNORMALS][(sequence + challenge) % 3];
	temp = LittleFloat(temp);
	p = ((byte*)&temp);

	buf[length + 4] = (sequence & 0xff) ^ p[3];
	buf[length + 5] = (challenge & 0xff) ^ p[2];
	buf[length + 6] = ((sequence >> 8) & 0xff) ^ p[1];
	buf[length + 7] = ((challenge >> 7) & 0xff) ^ p[0];

	length += 8;

	checksum = LittleLong(Com_BlockChecksum(buf, length));

	checksum &= 0xff;

	return checksum;
#endif
	return 0;
}

//========================================================

void SCR_EndLoadingPlaque(void);

/*
=============
Com_Error_f

Just throw a fatal error to
test error shutdown procedures
=============
*/
void Com_Error_f(void)
{
	common->FatalError("%s", Cmd_Argv(1));
}


/*
=================
Qcommon_Init
=================
*/
void Qcommon_Init(int argc, char** argv)
{
	try
	{
		char* s;

		if (setjmp(abortframe))
		{
			Sys_Error("Error during initialization");
		}

		GGameType = GAME_Quake2;
		Sys_SetHomePathSuffix("jlquake2");

		// prepare enough of the subsystems to handle
		// cvar and command buffer management
		COM_InitArgv(argc, const_cast<const char**>(argv));

		Com_InitByteOrder();
		Cbuf_Init();

		Cmd_Init();
		Cvar_Init();

		CL_InitKeyCommands();

		// we need to add the early commands twice, because
		// a basedir or cddir needs to be set before execing
		// config files, but we want other parms to override
		// the settings of the config files
		Cbuf_AddEarlyCommands(false);
		Cbuf_Execute();

		FS_InitFilesystem();

		Cbuf_AddText("exec default.cfg\n");
		Cbuf_AddText("exec config.cfg\n");

		Cbuf_AddEarlyCommands(true);
		Cbuf_Execute();

		//
		// init commands and vars
		//
		Cmd_AddCommand("error", Com_Error_f);

		COM_InitCommonCvars();

		com_speeds = Cvar_Get("host_speeds", "0", 0);
		log_stats = Cvar_Get("log_stats", "0", 0);
		timescale = Cvar_Get("timescale", "1", 0);
		fixedtime = Cvar_Get("fixedtime", "0", 0);
		logfile_active = Cvar_Get("logfile", "0", 0);
		showtrace = Cvar_Get("showtrace", "0", 0);
#ifdef DEDICATED_ONLY
		com_dedicated = Cvar_Get("dedicated", "1", CVAR_INIT);
#else
		com_dedicated = Cvar_Get("dedicated", "0", CVAR_INIT);
#endif

		s = va("%4.2f %s %s %s", VERSION, CPUSTRING, __DATE__, BUILDSTRING);
		Cvar_Get("version", s, CVAR_SERVERINFO | CVAR_INIT);


		Cmd_AddCommand("quit", Com_Quit_f);

		Sys_Init();

		NETQ23_Init();
		// pick a port value that should be nice and random
		Netchan_Init(Sys_Milliseconds_());

		SV_Init();
		CL_Init();

		// add + commands from command line
		if (!Cbuf_AddLateCommands())
		{	// if the user didn't give any commands, run default action
			if (!com_dedicated->value)
			{
				Cbuf_AddText("d1\n");
			}
			else
			{
				Cbuf_AddText("dedicated_start\n");
			}
			Cbuf_Execute();
		}
		else
		{	// the user asked for something explicit
			// so drop the loading plaque
			SCR_EndLoadingPlaque();
		}

		common->Printf("====== Quake2 Initialized ======\n\n");
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
=================
Qcommon_Frame
=================
*/
void Qcommon_Frame(int msec)
{
	try
	{
		int time_before, time_between, time_after;

		if (setjmp(abortframe))
		{
			return;		// an ERR_DROP was thrown

		}
		if (log_stats->modified)
		{
			log_stats->modified = false;
			if (log_stats->value)
			{
				if (log_stats_file)
				{
					FS_FCloseFile(log_stats_file);
					log_stats_file = 0;
				}
				log_stats_file = FS_FOpenFileWrite("stats.log");
				if (log_stats_file)
				{
					FS_Printf(log_stats_file, "entities,dlights,parts,frame time\n");
				}
			}
			else
			{
				if (log_stats_file)
				{
					FS_FCloseFile(log_stats_file);
					log_stats_file = 0;
				}
			}
		}

		if (fixedtime->value)
		{
			msec = fixedtime->value;
		}
		else if (timescale->value)
		{
			msec *= timescale->value;
			if (msec < 1)
			{
				msec = 1;
			}
		}

		if (showtrace->value)
		{
			common->Printf("%4i traces  %4i points\n", c_traces, c_pointcontents);
			c_traces = 0;
			c_brush_traces = 0;
			c_pointcontents = 0;
		}

		Sys_MessageLoop();

		Com_EventLoop();
		Cbuf_Execute();

		if (com_speeds->value)
		{
			time_before = Sys_Milliseconds_();
		}

		SV_Frame(msec);

		if (com_speeds->value)
		{
			time_between = Sys_Milliseconds_();
		}

#ifndef DEDICATED_ONLY
		// let the mouse activate or deactivate
		IN_Frame();
#endif

		// get new key events
		Sys_MessageLoop();
		Sys_SendKeyEvents();

		Com_EventLoop();

		// process console commands
		Cbuf_Execute();

		CL_Frame(msec);

		if (com_speeds->value)
		{
			time_after = Sys_Milliseconds_();
		}


		if (com_speeds->value)
		{
			int all, sv, gm, cl, rf;

			all = time_after - time_before;
			sv = time_between - time_before;
			cl = time_after - time_between;
			gm = time_after_game - time_before_game;
			rf = time_after_ref - time_before_ref;
			sv -= gm;
			cl -= rf;
			common->Printf("all:%3i sv:%3i gm:%3i cl:%3i rf:%3i\n",
				all, sv, gm, cl, rf);
		}
	}
	catch (DropException& e)
	{
		Com_Error(ERR_DROP, e.What());
	}
	catch (Exception& e)
	{
		Sys_Error("%s", e.What());
	}
}

/*
=================
Qcommon_Shutdown
=================
*/
void Qcommon_Shutdown(void)
{
}

int Com_Milliseconds()
{
	return Sys_Milliseconds();
}

void CL_EstablishConnection(const char* name)
{
}
void Host_Reconnect_f()
{
}

void CL_MapLoading(void)
{
}
void CL_ShutdownAll(void)
{
}
void CL_Disconnect(qboolean showMainMenu)
{
}
