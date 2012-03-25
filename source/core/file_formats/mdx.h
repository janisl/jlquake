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
//**	MDX file format (Wolfenstein Skeletal Data)
//**
//**************************************************************************

#define MDX_IDENT			(('W' << 24) + ('X' << 16) + ('D' << 8) + 'M')
#define MDX_VERSION			2
#define MDX_MAX_BONES		128

struct mdxFrame_t
{
	vec3_t bounds[2];               // bounds of this frame
	vec3_t localOrigin;             // midpoint of bounds, used for sphere cull
	float radius;                   // dist from localOrigin to corner
	vec3_t parentOffset;            // one bone is an ascendant of all other bones, it starts the hierachy at this position
};

struct mdxBoneFrameCompressed_t
{
	short angles[4];                // to be converted to axis at run-time (this is also better for lerping)
	short ofsAngles[2];             // PITCH/YAW, head in this direction from parent to go to the offset position
};

struct mdxBoneInfo_t
{
	char name[MAX_QPATH];           // name of bone
	int parent;                     // not sure if this is required, no harm throwing it in
	float torsoWeight;              // scale torso rotation about torsoParent by this
	float parentDist;
	int flags;
};

struct mdxHeader_t
{
	int ident;
	int version;

	char name[MAX_QPATH];           // model name

	// bones are shared by all levels of detail
	int numFrames;
	int numBones;
	int ofsFrames;                  // (mdxFrame_t + mdxBoneFrameCompressed_t[numBones]) * numframes
	int ofsBones;                   // mdxBoneInfo_t[numBones]
	int torsoParent;                // index of bone that is the parent of the torso

	int ofsEnd;                     // end of file
};
