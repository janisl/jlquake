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
#include "../server/public.h"

#ifdef  ERR_FATAL
#undef  ERR_FATAL				// this is be defined in malloc.h
#endif

// parameters to the main Error routine
enum
{
	ERR_FATAL,					// exit the entire game with a popup window
	ERR_DROP,					// print to console and disconnect from game
	ERR_SERVERDISCONNECT,		// don't kill server
	ERR_DISCONNECT,				// client disconnected from the server
	ERR_ENDGAME					// not an error.  just clean up properly, exit to the menu, and start up the "endgame" menu  //----(SA)	added
};

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Disconnect(const char* message, ...) id_attribute((format(printf, 2, 3)));

private:
	void VError(int code, const char* format, va_list argPtr);
};

static idCommonLocal commonLocal;
idCommon* common = &commonLocal;

char com_errorMessage[MAXPRINTMSG];
jmp_buf abortframe;		// an ERR_DROP occured, exit the entire frame

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
	va_start(argPtr, format);
	VError(ERR_DROP, format, argPtr);
	va_end(argPtr);
}

void idCommonLocal::FatalError(const char* format, ...)
{
	va_list argPtr;
	va_start(argPtr, format);
	VError(ERR_FATAL, format, argPtr);
	va_end(argPtr);
}

void idCommonLocal::EndGame(const char* format, ...)
{
	va_list argPtr;
	va_start(argPtr, format);
	VError(ERR_ENDGAME, format, argPtr);
	va_end(argPtr);
}

void idCommonLocal::ServerDisconnected(const char* format, ...)
{
	va_list argPtr;
	va_start(argPtr, format);
	VError(ERR_SERVERDISCONNECT, format, argPtr);
	va_end(argPtr);
}

void idCommonLocal::Disconnect(const char* format, ...)
{
	va_list argPtr;
	va_start(argPtr, format);
	VError(ERR_DISCONNECT, format, argPtr);
	va_end(argPtr);
}

//	Both client and server can use this, and it will
// do the apropriate things.
void idCommonLocal::VError(int code, const char* format, va_list argPtr)
{
#if defined(_WIN32) && defined(_DEBUG) && !defined(_WIN64)
	if (code != ERR_DISCONNECT)
	{
		if (!com_noErrorInterrupt->integer)
		{
			__asm
			{
				int 0x03
			}
		}
	}
#endif

	// when we are running automated scripts, make sure we
	// know if anything failed
	if (com_buildScript && com_buildScript->integer)
	{

		// ERR_ENDGAME is not really an error, don't die if building a script
		if (code != ERR_ENDGAME)
		{
			code = ERR_FATAL;
		}
	}

	// make sure we can get at our local stuff
	FS_PureServerSetLoadedPaks("", "");

	// if we are getting a solid stream of ERR_DROP, do an ERR_FATAL
	static int lastErrorTime;
	static int errorCount;
	int currentTime = Sys_Milliseconds();
	if (currentTime - lastErrorTime < 100)
	{
		if (++errorCount > 3)
		{
			code = ERR_FATAL;
		}
	}
	else
	{
		errorCount = 0;
	}
	lastErrorTime = currentTime;

	if (com_errorEntered)
	{
		char buf[4096];

		Q_vsnprintf(buf, sizeof(buf), format, argPtr);

		Sys_Error("recursive error '%s' after: %s", buf, com_errorMessage);
	}
	com_errorEntered = true;

	Q_vsnprintf(com_errorMessage, MAXPRINTMSG, format, argPtr);

	if (GGameType & GAME_Tech3 && code != ERR_DISCONNECT && code != ERR_ENDGAME)
	{
		Cvar_Set("com_errorMessage", com_errorMessage);
	}

	if (code == ERR_SERVERDISCONNECT)
	{
		if (GGameType & GAME_Quake2)
		{
			CLQ2_Drop();
		}
		else
		{
			CL_Disconnect(true);
			CLT3_FlushMemory();
		}
		com_errorEntered = false;
		longjmp(abortframe, -1);
	}

	if (code == ERR_DROP || code == ERR_DISCONNECT)
	{
		if (GGameType & GAME_QuakeHexen)
		{
			// reenable screen updates
			SCR_EndLoadingPlaque();
		}
		common->Printf("********************\nERROR: %s\n********************\n", com_errorMessage);
		SV_Shutdown(va("Server crashed: %s\n", com_errorMessage));
		if (GGameType & GAME_Quake2)
		{
			CLQ2_Drop();
		}
		else
		{
			CL_Disconnect(true);
		}
		if (GGameType & GAME_Tech3)
		{
			CLT3_FlushMemory();
		}
		if (GGameType & GAME_QuakeHexen)
		{
			CLQH_StopDemoLoop();
		}
		com_errorEntered = false;
		if (GGameType & GAME_QuakeHexen && com_dedicated->integer)
		{
			// dedicated servers exit
			Sys_Error("Error: %s\n", com_errorMessage);
		}
		longjmp(abortframe, -1);
	}

	if (code == ERR_ENDGAME)
	{
		SV_Shutdown("endgame");
		if (GGameType & GAME_WolfSP)
		{
			if (com_cl_running && com_cl_running->integer)
			{
				CL_Disconnect(true);
				CLT3_FlushMemory();
				com_errorEntered = false;
				CLWS_EndgameMenu();
			}
		}
		else
		{
			if (com_dedicated->integer)
			{
				Sys_Error("Host_EndGame: %s\n", com_errorMessage);		// dedicated servers exit
			}
			CLQH_OnEndGame();
		}
		longjmp(abortframe, -1);
	}

	if (GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		CL_Disconnect(true);
	}
	else if (!(GGameType & GAME_QuakeHexen))
	{
		CL_Shutdown();
	}
	if (!(GGameType & GAME_QuakeHexen) || GGameType & (GAME_QuakeWorld | GAME_HexenWorld))
	{
		SV_Shutdown(va("Server fatal crashed: %s\n", com_errorMessage));
	}
	if (!(GGameType & GAME_QuakeHexen))
	{
		Com_Shutdown();
	}

	Sys_Error("%s", com_errorMessage);
}
