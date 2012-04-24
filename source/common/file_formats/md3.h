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
//**
//**	.MD3 triangle model file format
//**
//**************************************************************************

#ifndef _MD3FILE_H
#define _MD3FILE_H

#define MD3_IDENT			(('3' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		3
#define MD3_MAX_SURFACES	32		// per model

// vertex scales
#define MD3_XYZ_SCALE		(1.0/64)

struct md3Frame_t
{
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
};

struct md3Tag_t
{
	char		name[MAX_QPATH];	// tag name
	vec3_t		origin;
	vec3_t		axis[3];
};

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
struct md3Surface_t
{
	int			ident;				// 

	char		name[MAX_QPATH];	// polyset name

	int			flags;
	int			numFrames;			// all surfaces in a model should have the same

	int			numShaders;			// all surfaces in a model should have the same
	int			numVerts;

	int			numTriangles;
	int			ofsTriangles;

	int			ofsShaders;			// offset from start of md3Surface_t
	int			ofsSt;				// texture coords are common for all frames
	int			ofsXyzNormals;		// numVerts * numFrames

	int			ofsEnd;				// next surface follows
};

struct md3Shader_t
{
	char		name[MAX_QPATH];
	int			shaderIndex;	// for in-game use
};

struct md3Triangle_t
{
	int			indexes[3];
};

struct md3St_t
{
	float		st[2];
};

struct md3XyzNormal_t
{
	short		xyz[3];
	short		normal;
};

struct md3Header_t
{
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;			
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
};

#endif
