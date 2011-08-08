//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	.MD4 file format
//**
//**************************************************************************

#ifndef _MD4FILE_H
#define _MD4FILE_H

#define MD4_IDENT			(('4' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MD4_VERSION			1

#define MD4_MAX_BONES		128

struct md4Weight_t
{
	int			boneIndex;		// these are indexes into the boneReferences,
	float		boneWeight;		// not the global per-frame bone list
	vec3_t		offset;
};

struct md4Vertex_t
{
	vec3_t		normal;
	vec2_t		texCoords;
	int			numWeights;
	md4Weight_t	weights[1];		// variable sized
};

struct md4Triangle_t
{
	int			indexes[3];
};

struct md4Surface_t
{
	int			ident;

	char		name[MAX_QPATH];	// polyset name
	char		shader[MAX_QPATH];
	int			shaderIndex;		// for in-game use

	int			ofsHeader;			// this will be a negative number

	int			numVerts;
	int			ofsVerts;

	int			numTriangles;
	int			ofsTriangles;

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int			numBoneReferences;
	int			ofsBoneReferences;

	int			ofsEnd;				// next surface follows
};

struct md4Bone_t
{
	float		matrix[3][4];
};

struct md4Frame_t
{
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;				// dist from localOrigin to corner
	md4Bone_t	bones[1];			// [numBones]
};

struct md4LOD_t
{
	int			numSurfaces;
	int			ofsSurfaces;		// first surface, others follow
	int			ofsEnd;				// next lod follows
};

struct md4Header_t
{
	int			ident;
	int			version;

	char		name[MAX_QPATH];	// model name

	// frames and bones are shared by all levels of detail
	int			numFrames;
	int			numBones;
	int			ofsBoneNames;		// char	name[ MAX_QPATH ]
	int			ofsFrames;			// md4Frame_t[numFrames]

	// each level of detail has completely separate sets of surfaces
	int			numLODs;
	int			ofsLODs;

	int			ofsEnd;				// end of file
};

#endif
