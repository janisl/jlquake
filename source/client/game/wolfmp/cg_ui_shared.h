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

#ifndef _WOLFMP_CG_UI_SHARED_H
#define _WOLFMP_CG_UI_SHARED_H

//	Overlaps with RF_WRAP_FRAMES
#define WMRF_BLINK            (1 << 9)		// eyes in 'blink' state

struct wmrefEntity_t
{
	refEntityType_t reType;
	int renderfx;

	qhandle_t hModel;				// opaque type outside refresh

	// most recent data
	vec3_t lightingOrigin;			// so multi-part models can be lit identically (RF_LIGHTING_ORIGIN)
	float shadowPlane;				// projection shadows go here, stencils go slightly lower

	vec3_t axis[3];					// rotation vectors
	vec3_t torsoAxis[3];			// rotation vectors for torso section of skeletal animation
	qboolean nonNormalizedAxes;		// axis are not normalized, i.e. they have scale
	float origin[3];				// also used as MODEL_BEAM's "from"
	int frame;						// also used as MODEL_BEAM's diameter
	int torsoFrame;					// skeletal torso can have frame independant of legs frame

	// previous data for frame interpolation
	float oldorigin[3];				// also used as MODEL_BEAM's "to"
	int oldframe;
	int oldTorsoFrame;
	float backlerp;					// 0.0 = current, 1.0 = old
	float torsoBacklerp;

	// texturing
	int skinNum;					// inline skin index
	qhandle_t customSkin;			// NULL for default skin
	qhandle_t customShader;			// use one image for the entire thing

	// misc
	byte shaderRGBA[4];				// colors used by rgbgen entity shaders
	float shaderTexCoord[2];		// texture coordinates used by tcMod entity modifiers
	float shaderTime;				// subtracted from refdef time to control effect start times

	// extra sprite information
	float radius;
	float rotation;

	// Ridah
	vec3_t fireRiseDir;

	// Ridah, entity fading (gibs, debris, etc)
	int fadeStartTime, fadeEndTime;

	float hilightIntensity;			//----(SA)	added

	int reFlags;

	int entityNum;					// currentState.number, so we can attach rendering effects to specific entities (Zombie)
};

struct wmglfog_t
{
	int mode;					// GL_LINEAR, GL_EXP
	int hint;					// GL_DONT_CARE
	int startTime;				// in ms
	int finishTime;				// in ms
	float color[4];
	float start;				// near
	float end;					// far
	qboolean useEndForClip;		// use the 'far' value for the far clipping plane
	float density;				// 0.0-1.0
	qboolean registered;		// has this fog been set up?
	qboolean drawsky;			// draw skybox
	qboolean clearscreen;		// clear the GL color buffer
};

struct wmrefdef_t
{
	int x, y, width, height;
	float fov_x, fov_y;
	vec3_t vieworg;
	vec3_t viewaxis[3];				// transformation matrix

	int time;			// time in milliseconds for shader effects and other time dependent rendering issues
	int rdflags;					// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte areamask[MAX_MAP_AREA_BYTES];

	// text messages for deform text shaders
	char text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	//	added (needed to pass fog infos into the portal sky scene)
	wmglfog_t glfog;
};

struct wmglconfig_t
{
	char renderer_string[MAX_STRING_CHARS];
	char vendor_string[MAX_STRING_CHARS];
	char version_string[MAX_STRING_CHARS];
	char extensions_string[MAX_STRING_CHARS * 4];					// TTimo - bumping, some cards have a big extension string

	int maxTextureSize;								// queried from GL
	int maxActiveTextures;							// multitexture ability

	int colorBits, depthBits, stencilBits;

	glDriverType_t driverType;
	glHardwareType_t hardwareType;

	qboolean deviceSupportsGamma;
	textureCompression_t textureCompression;
	qboolean textureEnvAddAvailable;
	qboolean anisotropicAvailable;					//----(SA)	added
	float maxAnisotropy;							//----(SA)	added

	// vendor-specific support
	// NVidia
	qboolean NVFogAvailable;						//----(SA)	added
	int NVFogMode;									//----(SA)	added
	// ATI
	int ATIMaxTruformTess;							// for truform support
	int ATINormalMode;							// for truform support
	int ATIPointMode;							// for truform support

	int vidWidth, vidHeight;
	// aspect is the screen's physical width / height, which may be different
	// than scrWidth / scrHeight if the pixels are non-square
	// normal screens should be 4/3, but wide aspect monitors may be 16/9
	float windowAspect;

	int displayFrequency;

	// synonymous with "does rendering consume the entire screen?", therefore
	// a Voodoo or Voodoo2 will have this set to TRUE, as will a Win32 ICD that
	// used CDS.
	qboolean isFullscreen;
	qboolean stereoEnabled;
	qboolean smpActive;						// dual processor

	qboolean textureFilterAnisotropicAvailable;					//DAJ
};

#endif
