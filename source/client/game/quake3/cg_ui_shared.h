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

#ifndef _QUAKE3_CG_UI_SHARED_H
#define _QUAKE3_CG_UI_SHARED_H

struct q3refEntity_t {
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;				// opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;		// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;		// projection shadows go here, stencils go slightly lower

	vec3_t axis[ 3 ];			// rotation vectors
	qboolean nonNormalizedAxes;	// axis are not normalized, i.e. they have scale
	vec3_t origin;				// also used as MODEL_BEAM's "from"
	int frame;				// also used as MODEL_BEAM's diameter

	// previous data for frame interpolation
	vec3_t oldorigin;			// also used as MODEL_BEAM's "to"
	int oldframe;
	float backlerp;			// 0.0 = current, 1.0 = old

	// texturing
	int skinNum;			// inline skin index
	qhandle_t customSkin;			// NULL for default skin
	qhandle_t customShader;		// use one image for the entire thing

	// misc
	byte shaderRGBA[ 4 ];		// colors used by rgbgen entity shaders
	float shaderTexCoord[ 2 ];		// texture coordinates used by tcMod entity modifiers
	float shaderTime;			// subtracted from refdef time to control effect start times
	// Also used for synctime

	// extra sprite information
	float radius;
	float rotation;
};

struct q3refdef_t {
	int x;
	int y;
	int width;
	int height;
	float fov_x;
	float fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[ 3 ];		// transformation matrix

	// time in milliseconds for shader effects and other time dependent rendering issues
	int time;

	int rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[ MAX_MAP_AREA_BYTES ];

	// text messages for deform text shaders
	char text[ MAX_RENDER_STRINGS ][ MAX_RENDER_STRING_LENGTH ];
};

struct q3glconfig_t {
	char renderer_string[ MAX_STRING_CHARS ];
	char vendor_string[ MAX_STRING_CHARS ];
	char version_string[ MAX_STRING_CHARS ];
	char extensions_string[ BIG_INFO_STRING ];

	int maxTextureSize;			// queried from GL
	int maxActiveTextures;		// multitexture ability

	int colorBits, depthBits, stencilBits;

	int driverType;
	int hardwareType;

	qboolean deviceSupportsGamma;
	int textureCompression;
	qboolean textureEnvAddAvailable;

	int vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float windowAspect;

	int displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Win32 ICD that used CDS will have this set to TRUE.
	qboolean isFullscreen;
	qboolean stereoEnabled;
	qboolean smpActive;		// dual processor
};

void CLQ3_GetGlconfig( q3glconfig_t* glconfig );
void CLQ3_AddRefEntityToScene( const q3refEntity_t* ent );
void CLQ3_RenderScene( const q3refdef_t* refdef );

#endif
