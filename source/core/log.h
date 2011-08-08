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

class LogListener : Interface
{
public:
	virtual void serialise(const char* text, bool devel) = 0;
};

class Log
{
private:
	enum { MAX_LISTENERS	= 8 };

	LogListener* listeners[MAX_LISTENERS];

public:
	Log();

	void addListener(LogListener* listener);
	void removeListener(LogListener* listener);

	void write(const char* format, ...);
	void writeLine(const char* format, ...);

	void develWrite(const char* format, ...);
	void develWriteLine(const char* format, ...);
};

extern Log			GLog;
