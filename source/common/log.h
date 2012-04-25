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

class LogListener : Interface
{
public:
	virtual void serialise(const char* text) = 0;
	virtual void develSerialise(const char* text) = 0;
};

class Log
{
public:
	static void addListener(LogListener* listener);
	static void removeListener(LogListener* listener);

	static void write(const char* format, ...);
	static void writeLine(const char* format, ...);

	static void develWrite(const char* format, ...);
	static void develWriteLine(const char* format, ...);

private:
	enum { MAX_LISTENERS    = 8 };

	static LogListener* listeners[MAX_LISTENERS];

	static void sendStringToListeners(const char* string);
	static void sendDevelStringToListeners(const char* string);
};
