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

//
//	End of definitions used by Quake 3 vms.
//
//==========================================================================

struct image_t;

image_t* R_PicFromWad(const char* Name);
