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

typedef int qhandle_t;

//==========================================================================
//
//	Definitions starting here are also used by Quake 3 vms and should not
// be changed.
//

/*
** glconfig_t
**
** Contains variables specific to the OpenGL configuration
** being run right now.  These are constant once the OpenGL
** subsystem is initialized.
*/
enum textureCompression_t
{
	TC_NONE,
	TC_S3TC
};

enum glDriverType_t
{
	GLDRV_ICD,					// driver is integrated with window system
};

enum glHardwareType_t
{
	GLHW_GENERIC,			// where everthing works the way it should
};

struct glconfig_t
{
	char					renderer_string[MAX_STRING_CHARS];
	char					vendor_string[MAX_STRING_CHARS];
	char					version_string[MAX_STRING_CHARS];
	char					extensions_string[BIG_INFO_STRING];

	int						maxTextureSize;			// queried from GL
	int						maxActiveTextures;		// multitexture ability

	int						colorBits, depthBits, stencilBits;

	glDriverType_t			driverType;
	glHardwareType_t		hardwareType;

	qboolean				deviceSupportsGamma;
	textureCompression_t	textureCompression;
	qboolean				textureEnvAddAvailable;

	int						vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float					windowAspect;

	int						displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Win32 ICD that used CDS will have this set to TRUE.
	qboolean				isFullscreen;
	qboolean				stereoEnabled;
	qboolean				smpActive;		// dual processor
};

enum refEntityType_t
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
};

// renderfx flags
#define RF_MINLIGHT			1		// allways have some light (viewmodel, some items)
#define RF_THIRD_PERSON		2		// don't draw through eyes, only mirrors (player bodies, chat sprites)
#define RF_FIRST_PERSON		4		// only draw through eyes (view weapon, damage blood blob)
#define RF_DEPTHHACK		8		// for view weapon Z crunching
#define RF_NOSHADOW			64		// don't add stencil shadows
#define RF_LIGHTING_ORIGIN	128		// use refEntity->lightingOrigin instead of refEntity->origin
									// for lighting.  This allows entities to sink into the floor
									// with their origin going solid, and allows all parts of a
									// player to get the same lighting
#define RF_SHADOW_PLANE		256		// use refEntity->shadowPlane
#define	RF_WRAP_FRAMES		512		// mod the model frames by the maxframes to allow continuous
									// animation without needing to know the frame count
//	New flags
#define RF_WATERTRANS		16		// translucent, alpha from r_wateralpha cvar.

struct refEntity_base_t
{
	refEntityType_t	reType;
	int				renderfx;

	qhandle_t		hModel;				// opaque type outside refresh

	// most recent data
	vec3_t			lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float			shadowPlane;		// projection shadows go here, stencils go slightly lower

	vec3_t			axis[3];			// rotation vectors
	qboolean		nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	vec3_t			origin;				// also used as MODEL_BEAM's "from"
	int				frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	vec3_t			oldorigin;			// also used as MODEL_BEAM's "to"
	int				oldframe;
	float			backlerp;			// 0.0 = current, 1.0 = old

	// texturing
	int				skinNum;			// inline skin index
	qhandle_t		customSkin;			// NULL for default skin
	qhandle_t		customShader;		// use one image for the entire thing

	// misc
	byte			shaderRGBA[4];		// colors used by rgbgen entity shaders
	float			shaderTexCoord[2];	// texture coordinates used by tcMod entity modifiers
	float			shaderTime;			// subtracted from refdef time to control effect start times
										// Also used for synctime

	// extra sprite information
	float			radius;
	float			rotation;
};

//
//	End of definitions used by Quake 3 vms.
//
//==========================================================================

struct image_t;

image_t* R_PicFromWad(const char* Name);
qhandle_t R_GetImageHandle(image_t* Image);
const char* R_GetImageName(qhandle_t Handle);
