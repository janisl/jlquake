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

#define MAX_IMAGE_ANIMATIONS	8
#define TR_MAX_TEXMODS			4
#define NUM_TEXTURE_BUNDLES		2
#define MAX_SHADER_STAGES		8
#define MAX_SHADER_DEFORMS		3
#define MAX_STATES_PER_SHADER	32

#define LIGHTMAP_2D				-4		// shader is for 2D rendering
#define LIGHTMAP_BY_VERTEX		-3		// pre-lit triangle models
#define LIGHTMAP_WHITEIMAGE		-2
#define LIGHTMAP_NONE			-1

enum shaderSort_t
{
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
};

enum genFunc_t
{
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE
};

enum deform_t
{
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7
};

enum texCoordGen_t
{
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
};

enum colorGen_t
{
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_FOG,				// standard fog
	CGEN_CONST				// fixed color
};

enum alphaGen_t
{
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
};

enum texMod_t
{
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
};

enum acff_t
{
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
};

enum cullType_t
{
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
};

enum fogPass_t
{
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
};

struct waveForm_t
{
	genFunc_t	func;

	float		base;
	float		amplitude;
	float		phase;
	float		frequency;
};

struct texModInfo_t
{
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;
};

struct textureBundle_t
{
	image_t*		image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t*	texMods;

	int				videoMapHandle;
	bool			isLightmap;
	bool			vertexLightmap;
	bool			isVideoMap;
};

struct shaderStage_t
{
	bool			active;
	
	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	byte			constantColor[4];			// for CGEN_CONST and AGEN_CONST

	unsigned		stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	bool			isDetail;
};

struct deformStage_t
{
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;
};

struct skyParms_t
{
	float		cloudHeight;
	image_t*	outerbox[6];
	image_t*	innerbox[6];
};

struct fogParms_t
{
	vec3_t	color;
	float	depthForOpaque;
};

struct shader_t
{
	char		name[MAX_QPATH];		// game path, including extension
	int			lightmapIndex;			// for a shader to match, both name and lightmapIndex must match

	int			index;					// this shader == tr.shaders[index]
	int			sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]

	float		sort;					// lower numbered shaders draw before higher numbered

	bool		defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	bool		explicitlyDefined;		// found in a .shader file

	int			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	int			contentFlags;

	bool		entityMergable;			// merge across entites optimizable (smoke, blood)

	bool		isSky;
	skyParms_t	sky;
	fogParms_t	fogParms;

	float		portalRange;			// distance to fog out at

	int			multitextureEnv;		// 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	bool		polygonOffset;			// set for decals and other items that must be offset 
	bool		noMipMaps;				// for console fonts, 2D elements, etc.
	bool		noPicMip;				// for images that must always be full resolution

	fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals

	bool		needsNormal;			// not all shaders will need all data to be gathered
	bool		needsST1;
	bool		needsST2;
	bool		needsColor;

	int			numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	int			numUnfoggedPasses;
	shaderStage_t	*stages[MAX_SHADER_STAGES];		

	void		(*optimalStageIteratorFunc)();

	float		clampTime;				// time this shader is clamped to
	float		timeOffset;				// current time offset for this shader

	int			numStates;				// if non-zero this is a state shader
	shader_t*	currentShader;			// current state if this is a state shader
	shader_t*	parentShader;			// current state if this is a state shader
	int			currentState;			// current state index for cycle purposes
	long		expireTime;				// time in milliseconds this expires

	shader_t*	remappedShader;			// current shader this one is remapped too

	int			shaderStates[MAX_STATES_PER_SHADER];	// index to valid shader states

	shader_t*	next;
};

bool ParseShader(const char** text);

extern shader_t		shader;
extern shaderStage_t	stages[MAX_SHADER_STAGES];		