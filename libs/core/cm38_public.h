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

#ifndef _CM38_PUBLIC_H
#define _CM38_PUBLIC_H

struct csurface_t
{
	char		name[16];
	int			flags;
	int			value;
};

struct cmodel_t
{
	vec3_t		mins;
	vec3_t		maxs;
	vec3_t		origin;		// for sounds or lights
	int			headnode;
};

#endif
