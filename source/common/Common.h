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
