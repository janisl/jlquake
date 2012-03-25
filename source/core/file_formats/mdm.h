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
//**	MDM file format (Wolfenstein Skeletal Mesh)
//**
//**************************************************************************

#define MDM_IDENT			(('W' << 24) + ('M' << 16) + ('D' << 8) + 'M')
#define MDM_VERSION			3
#define MDM_MAX_VERTS		6000
#define MDM_MAX_TRIANGLES	8192
#define MDM_MAX_SURFACES	32
#define MDM_MAX_TAGS		128

#define MDM_TRANSLATION_SCALE	(1.0 / 64)

struct mdmWeight_t
{
	int boneIndex;              // these are indexes into the boneReferences,
	float boneWeight;           // not the global per-frame bone list
	vec3_t offset;
};

struct mdmVertex_t
{
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	mdmWeight_t weights[1];     // variable sized
};

struct mdmTriangle_t
{
	int indexes[3];
};

struct mdmSurface_t
{
	int ident;

	char name[MAX_QPATH];           // polyset name
	char shader[MAX_QPATH];
	int shaderIndex;                // for in-game use

	int minLod;

	int ofsHeader;                  // this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsCollapseMap;           // numVerts * int

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next surface follows
};

typedef struct {
	int numSurfaces;
	int ofsSurfaces;                // first surface, others follow
	int ofsEnd;                     // next lod follows
} mdmLOD_t;

// Tags always only have one parent bone
struct mdmTag_t
{
	char name[MAX_QPATH];           // name of tag
	vec3_t axis[3];

	int boneIndex;
	vec3_t offset;

	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;                     // next tag follows
};

struct mdmHeader_t
{
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
	int numSurfaces;
	int ofsSurfaces;

	// tag data
	int numTags;
	int ofsTags;

	int ofsEnd;                     // end of file
};
