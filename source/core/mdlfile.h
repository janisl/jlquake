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

#ifndef _MDLFILE_H
#define _MDLFILE_H

//	Little-endian "IDPO"
#define IDPOLYHEADER		(('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define RAPOLYHEADER		(('O' << 24) + ('P' << 16) + ('A' << 8) + 'R')

#define ALIAS_VERSION		6
#define ALIAS_NEWVERSION	50

#define ALIAS_ONSEAM		0x0020

#define DT_FACES_FRONT		0x0010

// must match definition in spritegn.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
enum synctype_t
{
	ST_SYNC = 0,
	ST_RAND
};
#endif

enum mdl_skintype_t
{
	ALIAS_SKIN_SINGLE = 0,
	ALIAS_SKIN_GROUP
};

enum mdl_frametype_t
{
	ALIAS_SINGLE = 0,
	ALIAS_GROUP
};

struct mdl_t
{
	qint32		ident;
	qint32		version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	qint32		numskins;
	qint32		skinwidth;
	qint32		skinheight;
	qint32		numverts;
	qint32		numtris;
	qint32		numframes;
	qint32		synctype;
	qint32		flags;
	float		size;
};

struct newmdl_t
{
	qint32		ident;
	qint32		version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	qint32		numskins;
	qint32		skinwidth;
	qint32		skinheight;
	qint32		numverts;
	qint32		numtris;
	qint32		numframes;
	qint32		synctype;
	qint32		flags;
	float		size;
	qint32		num_st_verts;
};

struct dmdl_skintype_t
{
	qint32		type;
};

struct dmdl_skingroup_t
{
	qint32		numskins;
};

struct dmdl_skininterval_t
{
	float		interval;
};

struct dmdl_stvert_t
{
	qint32		onseam;
	qint32		s;
	qint32		t;
};

struct dmdl_triangle_t
{
	qint32		facesfront;
	qint32		vertindex[3];
};

struct dmdl_newtriangle_t
{
	qint32		facesfront;
	quint16		vertindex[3];
	quint16		stindex[3];
};

struct dmdl_frametype_t
{
	qint32		type;
};

struct dmdl_trivertx_t
{
	quint8		v[3];
	quint8		lightnormalindex;
};

struct dmdl_frame_t
{
	dmdl_trivertx_t	bboxmin;	// lightnormal isn't used
	dmdl_trivertx_t	bboxmax;	// lightnormal isn't used
	char		name[16];	// frame name from grabbing
};

struct dmdl_group_t
{
	qint32		numframes;
	dmdl_trivertx_t	bboxmin;	// lightnormal isn't used
	dmdl_trivertx_t	bboxmax;	// lightnormal isn't used
};

struct dmdl_interval_t
{
	float		interval;
};

#endif
