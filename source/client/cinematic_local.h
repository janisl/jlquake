//**************************************************************************
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

class QCinematic : public QInterface
{
};

class QCinematicCin : public QCinematic
{
public:
	byte*		buf;

	QCinematicCin()
	: buf(NULL)
	{}
	~QCinematicCin();
};

class QCinematicPcx : public QCinematic
{
public:
	byte*		buf;

	QCinematicPcx()
	: buf(NULL)
	{}
	~QCinematicPcx();
};

class QCinematicRoq : public QCinematic
{
};
