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

class idCommon : public Interface
{
public:
	// Prints message to the console, which may cause a screen update.
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3))) = 0;

	// Prints message that only shows up if the "developer" cvar is set,
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3))) = 0;

	// Issues a C++ throw. Normal errors just abort to the game loop,
	// which is appropriate for media or dynamic logic errors.
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3))) = 0;

	// Fatal errors quit all the way to a system dialog box, which is appropriate for
	// static internal errors or cases where the system may be corrupted.
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3))) = 0;

	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3))) = 0;

	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3))) = 0;

	virtual void Disconnect(const char* message) = 0;
};

extern idCommon* common;

class idCommonLocal : public idCommon
{
public:
	virtual void Printf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void DPrintf(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Error(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void FatalError(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void EndGame(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void ServerDisconnected(const char* format, ...) id_attribute((format(printf, 2, 3)));
	virtual void Disconnect(const char* message);
};

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

void Com_Error(int code, const char* fmt, ...);
