/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
#ifndef __QFILES_H__
#define __QFILES_H__

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

// surface geometry should not exceed these limits
#define	SHADER_MAX_VERTEXES	1000
#define	SHADER_MAX_INDEXES	(6*SHADER_MAX_VERTEXES)


// the maximum size of game relative pathnames
#define	MAX_QPATH		64

/*
========================================================================

QVM files

========================================================================
*/

#define	VM_MAGIC	0x12721444
typedef struct {
	int		vmMagic;

	int		instructionCount;

	int		codeOffset;
	int		codeLength;

	int		dataOffset;
	int		dataLength;
	int		litLength;			// ( dataLength - litLength ) should be byteswapped on load
	int		bssLength;			// zero filled memory appended to datalength
} vmHeader_t;

/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

#include "../../core/md3file.h"

/*
==============================================================================

MD4 file format

==============================================================================
*/

#define MD4_IDENT			(('4'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD4_VERSION			1
#define	MD4_MAX_BONES		128

typedef struct {
	int			boneIndex;		// these are indexes into the boneReferences,
	float		   boneWeight;		// not the global per-frame bone list
	vec3_t		offset;
} md4Weight_t;

typedef struct {
	vec3_t		normal;
	vec2_t		texCoords;
	int			numWeights;
	md4Weight_t	weights[1];		// variable sized
} md4Vertex_t;

typedef struct {
	int			indexes[3];
} md4Triangle_t;

typedef struct {
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
} md4Surface_t;

typedef struct {
	float		matrix[3][4];
} md4Bone_t;

typedef struct {
	vec3_t		bounds[2];			// bounds of all surfaces of all LOD's for this frame
	vec3_t		localOrigin;		// midpoint of bounds, used for sphere cull
	float		radius;				// dist from localOrigin to corner
	md4Bone_t	bones[1];			// [numBones]
} md4Frame_t;

typedef struct {
	int			numSurfaces;
	int			ofsSurfaces;		// first surface, others follow
	int			ofsEnd;				// next lod follows
} md4LOD_t;

typedef struct {
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
} md4Header_t;


/*
==============================================================================

  .BSP file format

==============================================================================
*/

#include "../../core/bsp46file.h"

#endif
