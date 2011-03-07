//**************************************************************************
//**
//**	$Id$
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

//==========================================================================
//
//	Exceptions
//
//==========================================================================

class QException : public QInterface
{
private:
	enum { MAX_ERROR_TEXT_SIZE		= 1024 };

	char message[MAX_ERROR_TEXT_SIZE];

public:
	explicit QException(const char *text);
	virtual const char* What() const;
};

class QDropException : public QException
{
public:
	QDropException(const char* text) : QException(text)
	{}
};

#define qassert(x)		if (x) {} else throw QException("Assertion failed");
