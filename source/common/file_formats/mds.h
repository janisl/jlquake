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
//**	MDS file format (Wolfenstein Skeletal Format)
//**
//**************************************************************************

#include "../mathlib.h"
#include "../files.h"

#define MDS_IDENT           ( ( 'W' << 24 ) + ( 'S' << 16 ) + ( 'D' << 8 ) + 'M' )
#define MDS_VERSION         4
#define MDS_MAX_VERTS       6000
#define MDS_MAX_BONES       128

#define MDS_TRANSLATION_SCALE   ( 1.0 / 64 )

#define MDS_BONEFLAG_TAG        1		// this bone is actually a tag

struct mdsWeight_t {
	int boneIndex;				// these are indexes into the boneReferences,
	float boneWeight;			// not the global per-frame bone list
	vec3_t offset;
};

struct mdsVertex_t {
	vec3_t normal;
	vec2_t texCoords;
	int numWeights;
	int fixedParent;			// stay equi-distant from this parent
	float fixedDist;
	mdsWeight_t weights[ 1 ];		// variable sized
};

struct mdsTriangle_t {
	int indexes[ 3 ];
};

struct mdsSurface_t {
	int ident;

	char name[ MAX_QPATH ];				// polyset name
	char shader[ MAX_QPATH ];
	int shaderIndex;				// for in-game use

	int minLod;

	int ofsHeader;					// this will be a negative number

	int numVerts;
	int ofsVerts;

	int numTriangles;
	int ofsTriangles;

	int ofsCollapseMap;				// numVerts * int

	// Bone references are a set of ints representing all the bones
	// present in any vertex weights for this surface.  This is
	// needed because a model may have surfaces that need to be
	// drawn at different sort times, and we don't want to have
	// to re-interpolate all the bones for each surface.
	int numBoneReferences;
	int ofsBoneReferences;

	int ofsEnd;						// next surface follows
};

struct mdsBoneFrameCompressed_t {
	//float		angles[3];
	//float		ofsAngles[2];
	short angles[ 4 ];				// to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[ 2 ];			// PITCH/YAW, head in this direction from parent to go to the offset position
};

struct mdsFrame_t {
	vec3_t bounds[ 2 ];					// bounds of all surfaces of all LOD's for this frame
	vec3_t localOrigin;				// midpoint of bounds, used for sphere cull
	float radius;					// dist from localOrigin to corner
	vec3_t parentOffset;			// one bone is an ascendant of all other bones, it starts the hierachy at this position
	mdsBoneFrameCompressed_t bones[ 1 ];				// [numBones]
};

struct mdsLOD_t {
	int numSurfaces;
	int ofsSurfaces;				// first surface, others follow
	int ofsEnd;						// next lod follows
};

struct mdsTag_t {
	char name[ MAX_QPATH ];				// name of tag
	float torsoWeight;
	int boneIndex;					// our index in the bones
};

struct mdsBoneInfo_t {
	char name[ MAX_QPATH ];				// name of bone
	int parent;						// not sure if this is required, no harm throwing it in
	float torsoWeight;				// scale torso rotation about torsoParent by this
	float parentDist;
	int flags;
};

struct mdsHeader_t {
	int ident;
	int version;

	char name[ MAX_QPATH ];				// model name

	float lodScale;
	float lodBias;

	// frames and bones are shared by all levels of detail
	int numFrames;
	int numBones;
	int ofsFrames;					// md4Frame_t[numFrames]
	int ofsBones;					// mdsBoneInfo_t[numBones]
	int torsoParent;				// index of bone that is the parent of the torso

	int numSurfaces;
	int ofsSurfaces;

	// tag data
	int numTags;
	int ofsTags;					// mdsTag_t[numTags]

	int ofsEnd;						// end of file
};
