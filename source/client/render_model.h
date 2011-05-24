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

QUAKE SPRITE MODELS

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

/*
==============================================================================

QUAKE MESH MODELS

==============================================================================
*/

#define MAX_MESH1_SKINS		32

struct mmesh1framedesc_t
{
	int					firstpose;
	int					numposes;
	float				interval;
	dmdl_trivertx_t		bboxmin;
	dmdl_trivertx_t		bboxmax;
	int					frame;
	char				name[16];
};

struct mmesh1groupframedesc_t
{
	dmdl_trivertx_t		bboxmin;
	dmdl_trivertx_t		bboxmax;
	int					frame;
};

struct mmesh1group_t
{
	int						numframes;
	int						intervals;
	mmesh1groupframedesc_t	frames[1];
};

struct mmesh1triangle_t
{
	int					facesfront;
	int					vertindex[3];
	int					stindex[3];
};

struct mesh1hdr_t
{
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;

	int					numposes;
	int					poseverts;
	int					posedata;	// numposes*poseverts trivert_t
	int					commands;	// gl command list with embedded s/t
	image_t*			gl_texture[MAX_MESH1_SKINS][4];
	int					texels[MAX_MESH1_SKINS];	// only for player skins
	mmesh1framedesc_t	frames[1];	// variable sized
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
