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

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

struct msprite1frame_t
{
	int			width;
	int			height;
	float		up, down, left, right;
	image_t*	gl_texture;
};

struct msprite1group_t
{
	int					numframes;
	float*				intervals;
	msprite1frame_t*	frames[1];
};

struct msprite1framedesc_t
{
	sprite1frametype_t	type;
	msprite1frame_t		*frameptr;
};

struct msprite1_t
{
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	msprite1framedesc_t	frames[1];
};

enum modtype_t
{
	MOD_BAD,
	MOD_BRUSH29,
	MOD_BRUSH38,
	MOD_BRUSH46,
	MOD_SPRITE,
	MOD_SPRITE2,
	MOD_MESH1,
	MOD_MESH2,
	MOD_MESH3,
	MOD_MD4
};

struct model_common_t
{
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;				// model = tr.models[model->index]

};
