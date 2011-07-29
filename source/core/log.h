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

//==========================================================================
//
//	QLogListener
//
//==========================================================================

class QLogListener : QInterface
{
public:
	virtual void Serialise(const char* Text, bool Devel) = 0;
};

//==========================================================================
//
//	QLog
//
//==========================================================================

class QLog
{
private:
	enum { MAX_LISTENERS	= 8 };

	QLogListener*	Listeners[MAX_LISTENERS];

public:
	QLog();

	void AddListener(QLogListener* Listener);
	void RemoveListener(QLogListener* Listener);

	void Write(const char* Fmt, ...);
	void WriteLine(const char* Fmt, ...);

	void DWrite(const char* Fmt, ...);
	void DWriteLine(const char* Fmt, ...);
};

extern QLog			GLog;
