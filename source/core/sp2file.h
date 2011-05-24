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
//**
//**	.SP2 sprite file format
//**
//**************************************************************************

#ifndef _SP2FILE_H
#define _SP2FILE_H

#define IDSPRITE2HEADER		(('2' << 24) + ('S' << 16) + ('D' << 8) + 'I')
		// little-endian "IDS2"
#define SPRITE2_VERSION		2

#define MAX_SP2_SKINNAME	64

struct dsp2_frame_t
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[MAX_SP2_SKINNAME];		// name of pcx file
};

struct dsprite2_t
{
	int				ident;
	int				version;
	int				numframes;
	dsp2_frame_t	frames[1];			// variable sized
};

#endif
