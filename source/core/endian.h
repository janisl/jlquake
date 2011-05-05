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

//	Endianess handling
extern bool			GBigEndian;

extern qint16		(*LittleShort)(qint16);
extern qint16		(*BigShort)(qint16);
extern qint32		(*LittleLong)(qint32);
extern qint32		(*BigLong)(qint32);
extern float		(*LittleFloat)(float);
extern float		(*BigFloat)(float);

void Com_InitByteOrder();
