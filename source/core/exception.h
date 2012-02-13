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

class Exception : public Interface
{
private:
	enum { MAX_ERROR_TEXT_SIZE		= 1024 };

	char message[MAX_ERROR_TEXT_SIZE];

public:
	explicit Exception(const char *text);
	virtual const char* What() const;
};

class DropException : public Exception
{
public:
	DropException(const char* text) : Exception(text)
	{}
};

#define qassert(x)		if (x) {} else throw Exception("Assertion failed");
