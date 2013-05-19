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

#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/command_buffer.h"

struct vidmode_t {
	const char* description;
	int width;
	int height;
	float pixelAspect;				// pixel width / height
};

Cvar* r_logFile;

Cvar* r_mode;
Cvar* r_fullscreen;
Cvar* r_customwidth;
Cvar* r_customheight;
Cvar* r_customaspect;

Cvar* r_allowSoftwareGL;			// don't abort out if the pixelformat claims software
Cvar* r_stencilbits;
Cvar* r_depthbits;
Cvar* r_colorbits;
Cvar* r_stereo;
Cvar* r_displayRefresh;
Cvar* r_swapInterval;
Cvar* r_drawBuffer;

Cvar* r_verbose;

Cvar* r_ext_compressed_textures;
Cvar* r_ext_multitexture;
Cvar* r_ext_texture_env_add;
Cvar* r_ext_gamma_control;
Cvar* r_ext_compiled_vertex_array;
Cvar* r_ext_point_parameters;
Cvar* r_ext_NV_fog_dist;
Cvar* r_ext_texture_filter_anisotropic;
Cvar* r_ext_ATI_pntriangles;

Cvar* r_gamma;
Cvar* r_ignorehwgamma;
Cvar* r_intensity;
Cvar* r_overBrightBits;

Cvar* r_wateralpha;
Cvar* r_roundImagesDown;
Cvar* r_picmip;
Cvar* r_texturebits;
Cvar* r_colorMipLevels;
Cvar* r_simpleMipMaps;
Cvar* r_textureMode;

Cvar* r_ignoreGLErrors;

Cvar* r_nobind;

Cvar* r_mapOverBrightBits;
Cvar* r_vertexLight;
Cvar* r_lightmap;
Cvar* r_fullbright;
Cvar* r_singleShader;

Cvar* r_subdivisions;

Cvar* r_ignoreFastPath;
Cvar* r_detailTextures;
Cvar* r_uiFullScreen;
Cvar* r_printShaders;

Cvar* r_saveFontData;

Cvar* r_smp;
Cvar* r_maxpolys;
Cvar* r_maxpolyverts;

Cvar* r_dynamiclight;
Cvar* r_znear;

Cvar* r_nocull;

Cvar* r_primitives;
Cvar* r_lerpmodels;
Cvar* r_shadows;
Cvar* r_debugSort;
Cvar* r_showtris;
Cvar* r_shownormals;
Cvar* r_offsetFactor;
Cvar* r_offsetUnits;
Cvar* r_clear;

Cvar* r_modulate;
Cvar* r_ambientScale;
Cvar* r_directedScale;
Cvar* r_debugLight;

Cvar* r_lightlevel;			// FIXME: This is a HACK to get the client's light level

Cvar* r_lodbias;
Cvar* r_lodscale;

Cvar* r_skymip;
Cvar* r_fastsky;
Cvar* r_showsky;
Cvar* r_drawSun;

Cvar* r_lodCurveError;

Cvar* r_ignore;

Cvar* r_keeptjunctions;
Cvar* r_dynamic;
Cvar* r_fullBrightColours;
Cvar* r_drawOverBrights;

Cvar* r_nocurves;
Cvar* r_facePlaneCull;
Cvar* r_novis;
Cvar* r_lockpvs;
Cvar* r_showcluster;
Cvar* r_drawworld;
Cvar* r_measureOverdraw;
Cvar* r_finish;
Cvar* r_showImages;
Cvar* r_speeds;
Cvar* r_showSmp;
Cvar* r_skipBackEnd;
Cvar* r_drawentities;
Cvar* r_debugSurface;
Cvar* r_norefresh;

Cvar* r_railWidth;
Cvar* r_railCoreWidth;
Cvar* r_railSegmentLength;

Cvar* r_flares;
Cvar* r_flareSize;
Cvar* r_flareFade;

Cvar* r_particle_size;
Cvar* r_particle_min_size;
Cvar* r_particle_max_size;
Cvar* r_particle_att_a;
Cvar* r_particle_att_b;
Cvar* r_particle_att_c;

Cvar* r_noportals;
Cvar* r_portalOnly;

Cvar* r_nv_fogdist_mode;

Cvar* r_textureAnisotropy;

Cvar* r_ati_truform_tess;
Cvar* r_ati_truform_normalmode;	// linear/quadratic
Cvar* r_ati_truform_pointmode;	// linear/cubic

Cvar* r_zfar;

Cvar* r_dlightScale;

Cvar* r_picmip2;

Cvar* r_portalsky;

Cvar* r_cache;
Cvar* r_cacheShaders;
Cvar* r_cacheModels;
Cvar* r_cacheGathering;

Cvar* r_bonesDebug;

Cvar* r_wolffog;

Cvar* r_drawfoliage;

Cvar* r_clampToEdge;

Cvar* r_trisColor;
Cvar* r_normallength;

static vidmode_t r_vidModes[] =
{
	{ "Mode  0: 320x240",       320,    240,    1 },
	{ "Mode  1: 400x300",       400,    300,    1 },
	{ "Mode  2: 512x384",       512,    384,    1 },
	{ "Mode  3: 640x480",       640,    480,    1 },
	{ "Mode  4: 800x600",       800,    600,    1 },
	{ "Mode  5: 960x720",       960,    720,    1 },
	{ "Mode  6: 1024x768",      1024,   768,    1 },
	{ "Mode  7: 1152x864",      1152,   864,    1 },
	{ "Mode  8: 1280x1024",     1280,   1024,   1 },
	{ "Mode  9: 1600x1200",     1600,   1200,   1 },
	{ "Mode 10: 2048x1536",     2048,   1536,   1 },
	{ "Mode 11: 856x480 (wide)",856,    480,    1 }
};
static int s_numVidModes = sizeof ( r_vidModes ) / sizeof ( r_vidModes[ 0 ] );

const char* gl_system_extensions_string;

bool R_GetModeInfo( int* width, int* height, float* windowAspect, int mode ) {
	if ( mode < -1 ) {
		return false;
	}
	if ( mode >= s_numVidModes ) {
		return false;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		*windowAspect = r_customaspect->value;
		return true;
	}

	vidmode_t* vm = &r_vidModes[ mode ];

	*width  = vm->width;
	*height = vm->height;
	*windowAspect = ( float )vm->width / ( vm->height * vm->pixelAspect );

	return true;
}

static void R_ModeList_f() {
	common->Printf( "\n" );
	for ( int i = 0; i < s_numVidModes; i++ ) {
		common->Printf( "%s\n", r_vidModes[ i ].description );
	}
	common->Printf( "\n" );
}

static void AssertCvarRange( Cvar* cv, float minVal, float maxVal, bool shouldBeIntegral ) {
	if ( shouldBeIntegral ) {
		if ( ( int )cv->value != cv->integer ) {
			common->Printf( S_COLOR_YELLOW "WARNING: cvar '%s' must be integral (%f)\n", cv->name, cv->value );
			Cvar_Set( cv->name, va( "%d", cv->integer ) );
		}
	}

	if ( cv->value < minVal ) {
		common->Printf( S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f < %f)\n", cv->name, cv->value, minVal );
		Cvar_Set( cv->name, va( "%f", minVal ) );
	} else if ( cv->value > maxVal ) {
		common->Printf( S_COLOR_YELLOW "WARNING: cvar '%s' out of range (%f > %f)\n", cv->name, cv->value, maxVal );
		Cvar_Set( cv->name, va( "%f", maxVal ) );
	}
}

static void GfxInfo_f() {
	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};
	const char* enablestrings[] =
	{
		"disabled",
		"enabled"
	};

	common->Printf( "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	common->Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	common->Printf( "GL_VERSION: %s\n", glConfig.version_string );

	common->Printf( "GL_EXTENSIONS:\n" );
	idList<idStr> Exts;
	idStr( glConfig.extensions_string ).Split( ' ', Exts );
	for ( int i = 0; i < Exts.Num(); i++ ) {
		common->Printf( " %s\n", Exts[ i ].CStr() );
	}
#ifdef _WIN32
	common->Printf( "WGL_EXTENSIONS:\n" );
#else
	common->Printf( "GLX_EXTENSIONS:\n" );
#endif
	idStr( gl_system_extensions_string ).Split( ' ', Exts );
	for ( int i = 0; i < Exts.Num(); i++ ) {
		common->Printf( " %s\n", Exts[ i ].CStr() );
	}

	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	common->Printf( "GL_MAX_ACTIVE_TEXTURES: %d\n", glConfig.maxActiveTextures );
	common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	common->Printf( "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[ r_fullscreen->integer == 1 ] );
	if ( glConfig.displayFrequency ) {
		common->Printf( "%d\n", glConfig.displayFrequency );
	} else {
		common->Printf( "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma ) {
		common->Printf( "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	} else {
		common->Printf( "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	// rendering primitives
	// default is to use triangles if compiled vertex arrays are present
	common->Printf( "rendering primitives: " );
	int primitives = r_primitives->integer;
	if ( primitives == 0 ) {
		if ( qglLockArraysEXT ) {
			primitives = 2;
		} else {
			primitives = 1;
		}
	}
	if ( primitives == -1 ) {
		common->Printf( "none\n" );
	} else if ( primitives == 2 ) {
		common->Printf( "single glDrawElements\n" );
	} else if ( primitives == 1 ) {
		common->Printf( "multiple glArrayElement\n" );
	} else if ( primitives == 3 ) {
		common->Printf( "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
	}

	common->Printf( "texturemode: %s\n", r_textureMode->string );
	common->Printf( "picmip: %d\n", r_picmip->integer );
	common->Printf( "picmip2: %d\n", r_picmip2->integer );
	common->Printf( "texture bits: %d\n", r_texturebits->integer );
	common->Printf( "multitexture: %s\n", enablestrings[ qglActiveTextureARB != 0 ] );
	common->Printf( "compiled vertex arrays: %s\n", enablestrings[ qglLockArraysEXT != 0 ] );
	common->Printf( "texenv add: %s\n", enablestrings[ glConfig.textureEnvAddAvailable != 0 ] );
	common->Printf( "compressed textures: %s\n", enablestrings[ glConfig.textureCompression != TC_NONE ] );
	common->Printf( "anisotropy: %s\n", r_textureAnisotropy->string );
	common->Printf( "ATI truform: %s\n", enablestrings[ qglPNTrianglesiATI != 0 ] );
	if ( qglPNTrianglesiATI ) {
		common->Printf( "Truform Tess: %d\n", r_ati_truform_tess->integer );
		common->Printf( "Truform Point Mode: %s\n", r_ati_truform_pointmode->string );
		common->Printf( "Truform Normal Mode: %s\n", r_ati_truform_normalmode->string );
	}
	common->Printf( "NV distance fog: %s\n", enablestrings[ glConfig.NVFogAvailable != 0 ] );
	if ( glConfig.NVFogAvailable ) {
		common->Printf( "Fog Mode: %s\n", r_nv_fogdist_mode->string );
	}
	if ( r_vertexLight->integer ) {
		common->Printf( "HACK: using vertex lightmap approximation\n" );
	}
	if ( glConfig.smpActive ) {
		common->Printf( "Using dual processor acceleration\n" );
	}
	if ( r_finish->integer ) {
		common->Printf( "Forcing glFinish\n" );
	}
}

static void R_Register() {
	//
	// latched and archived variables
	//
	r_mode = Cvar_Get( "r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_customwidth = Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_customheight = Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_customaspect = Cvar_Get( "r_customaspect", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_colorbits = Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_stencilbits = Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_depthbits = Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_stereo = Cvar_Get( "r_stereo", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_compressed_textures = Cvar_Get( "r_ext_compressed_textures", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_multitexture = Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_texture_env_add = Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_gamma_control = Cvar_Get( "r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_compiled_vertex_array = Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_point_parameters = Cvar_Get( "r_ext_point_parameters", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_ext_NV_fog_dist = Cvar_Get( "r_ext_NV_fog_dist", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_texture_filter_anisotropic = Cvar_Get( "r_ext_texture_filter_anisotropic", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ext_ATI_pntriangles = Cvar_Get( "r_ext_ATI_pntriangles", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_ignorehwgamma = Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_overBrightBits = Cvar_Get( "r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	if ( GGameType & GAME_ET ) {
		AssertCvarRange( r_overBrightBits, 0, 1, true );	// ydnar: limit to overbrightbits 1 (sorry 1337 players)
	}
	r_roundImagesDown = Cvar_Get( "r_roundImagesDown", "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_picmip = Cvar_Get( "r_picmip", ( GGameType & GAME_Tech3 ) ? "1" : "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	AssertCvarRange( r_picmip, 0, 16, true );
	r_texturebits = Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_simpleMipMaps = Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_vertexLight = Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_subdivisions = Cvar_Get( "r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_ignoreFastPath = Cvar_Get( "r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_detailTextures = Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	//	Disabled until I fix it on Linux.
	r_smp = Cvar_Get( "r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );
	r_picmip2 = Cvar_Get( "r_picmip2", "2", CVAR_ARCHIVE | CVAR_LATCH2 );		// used for character skins picmipping at a different level from the rest of the game
	AssertCvarRange( r_picmip2, 0, 16, true );
	r_clampToEdge = Cvar_Get( "r_clampToEdge", "1", CVAR_ARCHIVE | CVAR_LATCH2 | CVAR_UNSAFE );		// ydnar: opengl 1.2 GL_CLAMP_TO_EDGE support

	//
	// temporary latched variables that can only change over a restart
	//
	r_allowSoftwareGL = Cvar_Get( "r_allowSoftwareGL", "0", CVAR_LATCH2 );
	r_displayRefresh = Cvar_Get( "r_displayRefresh", "0", CVAR_LATCH2 );
	AssertCvarRange( r_displayRefresh, 0, 200, true );
	r_intensity = Cvar_Get( "r_intensity", "1", CVAR_LATCH2 );
	if ( GGameType & GAME_ET ) {
		AssertCvarRange( r_intensity, 0, 1.5, false );
	}
	r_colorMipLevels = Cvar_Get( "r_colorMipLevels", "0", CVAR_LATCH2 );
	r_mapOverBrightBits = Cvar_Get( "r_mapOverBrightBits", "2", CVAR_LATCH2 );
	if ( GGameType & GAME_ET ) {
		AssertCvarRange( r_mapOverBrightBits, 0, 3, true );
	}
	r_fullbright = Cvar_Get( "r_fullbright", "0", CVAR_LATCH2 | CVAR_CHEAT );
	r_singleShader = Cvar_Get( "r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH2 );
	if ( GGameType & ( GAME_QuakeHexen | GAME_Quake2 | GAME_WolfSP ) ) {
		// NOTE TTimo: r_cache is disabled by default in SP
		Cvar_Set( "r_cache", "0" );
		// (SA) disabling cacheshaders
		Cvar_Set( "r_cacheShaders", "0" );
	}
	// TTimo show_bug.cgi?id=440
	//   with r_cache enabled, non-win32 OSes were leaking 24Mb per R_Init..
	r_cache = Cvar_Get( "r_cache", "1", CVAR_LATCH2 );		// leaving it as this for backwards compability. but it caches models and shaders also
	r_cacheShaders = Cvar_Get( "r_cacheShaders", "1", CVAR_LATCH2 );
	r_cacheModels = Cvar_Get( "r_cacheModels", "1", CVAR_LATCH2 );

	//
	// archived variables that can change at any time
	//
	r_swapInterval = Cvar_Get( "r_swapInterval", "0", CVAR_ARCHIVE );
	r_gamma = Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	r_wateralpha = Cvar_Get( "r_wateralpha","1", CVAR_ARCHIVE );
	r_ignoreGLErrors = Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_textureMode = Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	r_dynamiclight = Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_primitives = Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );
	r_modulate = Cvar_Get( "r_modulate", "1", CVAR_ARCHIVE );
	if ( GGameType & GAME_Tech3 ) {
		r_shadows = Cvar_Get( "cg_shadows", "1", 0 );
	} else {
		r_shadows = Cvar_Get( "r_shadows", "0", CVAR_ARCHIVE );
	}
	r_lodbias = Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_drawSun = Cvar_Get( "r_drawSun", GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ? "1" : "0", CVAR_ARCHIVE );
	r_lodCurveError = Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE | CVAR_CHEAT );
	r_keeptjunctions = Cvar_Get( "r_keeptjunctions", "1", CVAR_ARCHIVE );
	r_facePlaneCull = Cvar_Get( "r_facePlaneCull", "1", CVAR_ARCHIVE );
	r_railWidth = Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
	r_railCoreWidth = Cvar_Get( "r_railCoreWidth", GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ? "1" : "6", CVAR_ARCHIVE );
	r_railSegmentLength = Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );
	r_flares = Cvar_Get( "r_flares", "1", CVAR_ARCHIVE );
	r_finish = Cvar_Get( "r_finish", "0", CVAR_ARCHIVE );
	r_particle_size = Cvar_Get( "r_particle_size", "40", CVAR_ARCHIVE );
	r_particle_min_size = Cvar_Get( "r_particle_min_size", "2", CVAR_ARCHIVE );
	r_particle_max_size = Cvar_Get( "r_particle_max_size", "40", CVAR_ARCHIVE );
	r_particle_att_a = Cvar_Get( "r_particle_att_a", "0.01", CVAR_ARCHIVE );
	r_particle_att_b = Cvar_Get( "r_particle_att_b", "0.0", CVAR_ARCHIVE );
	r_particle_att_c = Cvar_Get( "r_particle_att_c", "0.01", CVAR_ARCHIVE );
	r_nv_fogdist_mode = Cvar_Get( "r_nv_fogdist_mode", "GL_EYE_RADIAL_NV", CVAR_ARCHIVE | CVAR_UNSAFE );	// default to 'looking good'
	r_textureAnisotropy = Cvar_Get( "r_textureAnisotropy", "1.0", CVAR_ARCHIVE );
	r_ati_truform_tess = Cvar_Get( "r_ati_truform_tess", "1", CVAR_ARCHIVE );
	r_ati_truform_normalmode = Cvar_Get( "r_ati_truform_normalmode", "QUADRATIC", CVAR_ARCHIVE );
	r_ati_truform_pointmode = Cvar_Get( "r_ati_truform_pointmode", "CUBIC", CVAR_ARCHIVE );
	r_dlightScale = Cvar_Get( "r_dlightScale", "1.0", CVAR_ARCHIVE );
	r_trisColor = Cvar_Get( "r_trisColor", "1.0 1.0 1.0 1.0", CVAR_ARCHIVE );
	r_normallength = Cvar_Get( "r_normallength", "0.5", CVAR_ARCHIVE );

	//
	// temporary variables that can change at any time
	//
	r_logFile = Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_verbose = Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_nobind = Cvar_Get( "r_nobind", "0", CVAR_CHEAT );
	r_lightmap = Cvar_Get( "r_lightmap", "0", CVAR_CHEAT );
	r_uiFullScreen = Cvar_Get( "r_uifullscreen", "0", 0 );
	r_printShaders = Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = Cvar_Get( "r_saveFontData", "0", 0 );
	r_maxpolys = Cvar_Get( "r_maxpolys", va( "%d", MAX_POLYS ), 0 );
	r_maxpolyverts = Cvar_Get( "r_maxpolyverts", va( "%d", MAX_POLYVERTS ), 0 );
	r_znear = Cvar_Get( "r_znear", GGameType & GAME_ET ? "3" : "4", CVAR_CHEAT );
	AssertCvarRange( r_znear, 0.001f, 200, true );
	r_nocull = Cvar_Get( "r_nocull", "0", CVAR_CHEAT );
	r_lightlevel = Cvar_Get( "r_lightlevel", "0", 0 );
	r_lerpmodels = Cvar_Get( "r_lerpmodels", "1", 0 );
	r_ambientScale = Cvar_Get( "r_ambientScale", GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ? "0.5" : "0.6", CVAR_CHEAT );
	r_directedScale = Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );
	r_debugLight = Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_showtris = Cvar_Get( "r_showtris", "0", CVAR_CHEAT );
	r_shownormals = Cvar_Get( "r_shownormals", "0", CVAR_CHEAT );
	r_offsetFactor = Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_lodscale = Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_clear = Cvar_Get( "r_clear", "0", CVAR_CHEAT );
	r_skymip = Cvar_Get( "r_skymip", "0", 0 );
	r_showsky = Cvar_Get( "r_showsky", "0", CVAR_CHEAT );
	r_ignore = Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_dynamic = Cvar_Get( "r_dynamic", "1", 0 );
	r_fullBrightColours = Cvar_Get( "r_fullBrightColours", "1", 0 );
	r_drawOverBrights = Cvar_Get( "r_drawOverBrights", "1", 0 );
	r_nocurves = Cvar_Get( "r_nocurves", "0", CVAR_CHEAT );
	r_novis = Cvar_Get( "r_novis", "0", CVAR_CHEAT );
	r_lockpvs = Cvar_Get( "r_lockpvs", "0", CVAR_CHEAT );
	r_showcluster = Cvar_Get( "r_showcluster", "0", CVAR_CHEAT );
	r_drawworld = Cvar_Get( "r_drawworld", "1", CVAR_CHEAT );
	r_flareSize = Cvar_Get( "r_flareSize", "40", CVAR_CHEAT );
	if ( GGameType & ( GAME_WolfMP | GAME_ET ) ) {
		Cvar_Set( "r_flareFade", "5" );		// to force this when people already have "7" in their config
	}
	r_flareFade = Cvar_Get( "r_flareFade", GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ? "5" : "7", CVAR_CHEAT );
	r_measureOverdraw = Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_showImages = Cvar_Get( "r_showImages", "0", CVAR_TEMP );
	r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_speeds = Cvar_Get( "r_speeds", "0", CVAR_CHEAT );
	r_showSmp = Cvar_Get( "r_showSmp", "0", CVAR_CHEAT );
	r_skipBackEnd = Cvar_Get( "r_skipBackEnd", "0", CVAR_CHEAT );
	r_drawentities = Cvar_Get( "r_drawentities", "1", CVAR_CHEAT );
	r_noportals = Cvar_Get( "r_noportals", "0", CVAR_CHEAT );
	r_portalOnly = Cvar_Get( "r_portalOnly", "0", CVAR_CHEAT );
	r_debugSurface = Cvar_Get( "r_debugSurface", "0", CVAR_CHEAT );
	r_norefresh = Cvar_Get( "r_norefresh", "0", CVAR_CHEAT );
	r_zfar = Cvar_Get( "r_zfar", "0", CVAR_CHEAT );
	r_portalsky = Cvar_Get( "cg_skybox", "1", 0 );
	r_cacheGathering = Cvar_Get( "cl_cacheGathering", "0", 0 );
	r_bonesDebug = Cvar_Get( "r_bonesDebug", "0", CVAR_CHEAT );
	r_wolffog = Cvar_Get( "r_wolffog", "1", 0 );
	r_drawfoliage = Cvar_Get( "r_drawfoliage", "1", CVAR_CHEAT );

	// make sure all the commands added here are also
	// removed in R_Shutdown
	Cmd_AddCommand( "modelist", R_ModeList_f );
	Cmd_AddCommand( "imagelist", R_ImageList_f );
	Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	Cmd_AddCommand( "skinlist", R_SkinList_f );
	Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	Cmd_AddCommand( "modellist", R_Modellist_f );
}

const char* R_GetTitleForWindow() {
	if ( GGameType & GAME_QuakeWorld ) {
		return "QuakeWorld";
	}
	if ( GGameType & GAME_Quake ) {
		return "Quake";
	}
	if ( GGameType & GAME_HexenWorld ) {
		return "HexenWorld";
	}
	if ( GGameType & GAME_Hexen2 ) {
		return "Hexen II";
	}
	if ( GGameType & GAME_Quake2 ) {
		return "Quake 2";
	}
	if ( GGameType & GAME_Quake3 ) {
		return "Quake 3: Arena";
	}
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP ) ) {
		return "Return to Castle Wolfenstein";
	}
	if ( GGameType & GAME_ET ) {
		return "Enemy Territory";
	}
	return "Unknown";
}

static void R_SetMode() {
	rserr_t err = GLimp_SetMode( r_mode->integer, r_colorbits->integer, !!r_fullscreen->integer );
	if ( err == RSERR_OK ) {
		return;
	}

	if ( err == RSERR_INVALID_FULLSCREEN ) {
		common->Printf( "...WARNING: fullscreen unavailable in this mode\n" );

		Cvar_SetValue( "r_fullscreen", 0 );
		r_fullscreen->modified = false;

		err = GLimp_SetMode( r_mode->integer, r_colorbits->integer, false );
		if ( err == RSERR_OK ) {
			return;
		}
	}

	common->Printf( "...WARNING: could not set the given mode (%d)\n", r_mode->integer );

	// if we're on a 24/32-bit desktop and we're going fullscreen on an ICD,
	// try it again but with a 16-bit desktop
	if ( r_colorbits->integer != 16 || r_fullscreen->integer == 0 || r_mode->integer != 3 ) {
		err = GLimp_SetMode( 3, 16, true );
		if ( err == RSERR_OK ) {
			return;
		}
		common->Printf( "...WARNING: could not set default 16-bit fullscreen mode\n" );
	}

	// try setting it back to something safe
	err = GLimp_SetMode( 3, r_colorbits->integer, false );
	if ( err == RSERR_OK ) {
		return;
	}

	common->Printf( "...WARNING: could not revert to safe mode\n" );
	common->FatalError( "R_SetMode() - could not initialise OpenGL subsystem\n" );
}

//	This is the OpenGL initialization function.  It is responsible for
// initialising OpenGL, setting extensions, creating a window of the
// appropriate size, doing fullscreen manipulations, etc.  Its overall
// responsibility is to make sure that a functional OpenGL subsystem is
// operating when it returns.
static void InitOpenGLSubsystem() {
	common->Printf( "Initializing OpenGL subsystem\n" );

	//	Ceate the window and set up the context.
	R_SetMode();

	//  Initialise our QGL dynamic bindings
	QGL_Init();

	//	Needed for Quake 3 UI vm.
	glConfig.driverType = GLDRV_ICD;
	glConfig.hardwareType = GLHW_GENERIC;

	//	Get our config strings.
	String::NCpyZ( glConfig.vendor_string, ( char* )qglGetString( GL_VENDOR ), sizeof ( glConfig.vendor_string ) );
	String::NCpyZ( glConfig.renderer_string, ( char* )qglGetString( GL_RENDERER ), sizeof ( glConfig.renderer_string ) );
	if ( *glConfig.renderer_string && glConfig.renderer_string[ String::Length( glConfig.renderer_string ) - 1 ] == '\n' ) {
		glConfig.renderer_string[ String::Length( glConfig.renderer_string ) - 1 ] = 0;
	}
	String::NCpyZ( glConfig.version_string, ( char* )qglGetString( GL_VERSION ), sizeof ( glConfig.version_string ) );
	String::NCpyZ( glConfig.extensions_string, ( char* )qglGetString( GL_EXTENSIONS ), sizeof ( glConfig.extensions_string ) );

	//bani - glx extensions string
	gl_system_extensions_string = GLimp_GetSystemExtensionsString();

	// OpenGL driver constants
	GLint temp;
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;

	//	Load palette used by 8-bit graphics files.
	if ( GGameType & GAME_QuakeHexen ) {
		R_InitQ1Palette();
	} else if ( GGameType & GAME_Quake2 ) {
		R_InitQ2Palette();
	}
}

static void GL_SetDefaultState() {
	qglClearDepth( 1.0f );

	qglColor4f( 1, 1, 1, 1 );

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TextureAnisotropy( r_textureAnisotropy->value );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable( GL_TEXTURE_2D );
	GL_TextureMode( r_textureMode->string );
	GL_TextureAnisotropy( r_textureAnisotropy->value );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState( GL_VERTEX_ARRAY );

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.faceCulling = CT_TWO_SIDED;

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

	if ( qglPointParameterfEXT ) {
		float attenuations[ 3 ];

		attenuations[ 0 ] = r_particle_att_a->value;
		attenuations[ 1 ] = r_particle_att_b->value;
		attenuations[ 2 ] = r_particle_att_c->value;

		qglEnable( GL_POINT_SMOOTH );
		qglPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, r_particle_min_size->value );
		qglPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, r_particle_max_size->value );
		qglPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}

	// ATI pn_triangles
	if ( qglPNTrianglesiATI ) {
		int maxtess;
		// get max supported tesselation
		qglGetIntegerv( GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI, ( GLint* )&maxtess );
		glConfig.ATIMaxTruformTess = maxtess;
		// cap if necessary
		if ( r_ati_truform_tess->value > maxtess ) {
			Cvar_Set( "r_ati_truform_tess", va( "%d", maxtess ) );
		}

		// set Wolf defaults
		qglPNTrianglesiATI( GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, r_ati_truform_tess->value );
	}

	if ( glConfig.anisotropicAvailable ) {
		float maxAnisotropy;
		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
		glConfig.maxAnisotropy = maxAnisotropy;
	}
}

//	This function is responsible for initializing a valid OpenGL subsystem.
// This is done by calling InitOpenGLSubsystem (which gives us a working OGL
// subsystem) then setting variables, checking GL constants, and reporting
// the gfx system config to the user.
static void InitOpenGL() {
	//
	// initialize OS specific portions of the renderer
	//
	// InitOpenGLSubsystem directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if ( glConfig.vidWidth == 0 ) {
		InitOpenGLSubsystem();

		// init command buffers and SMP
		R_InitCommandBuffers();

		// print info
		GfxInfo_f();
	} else {
		// init command buffers and SMP
		R_InitCommandBuffers();
	}

	// set default state
	GL_SetDefaultState();
}

static void R_InitFunctionTables() {
	//
	// init function tables
	//
	for ( int i = 0; i < FUNCTABLE_SIZE; i++ ) {
		tr.sinTable[ i ] = sin( DEG2RAD( i * 360.0f / ( ( float )FUNCTABLE_SIZE ) ) );
		tr.squareTable[ i ] = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[ i ] = ( float )i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[ i ] = 1.0f - tr.sawToothTable[ i ];

		if ( i < FUNCTABLE_SIZE / 2 ) {
			if ( i < FUNCTABLE_SIZE / 4 ) {
				tr.triangleTable[ i ] = ( float )i / ( FUNCTABLE_SIZE / 4 );
			} else {
				tr.triangleTable[ i ] = 1.0f - tr.triangleTable[ i - FUNCTABLE_SIZE / 4 ];
			}
		} else {
			tr.triangleTable[ i ] = -tr.triangleTable[ i - FUNCTABLE_SIZE / 2 ];
		}
	}
}

static void R_Init() {
	common->Printf( "----- R_Init -----\n" );

	// clear all our internal state
	Com_Memset( &tr, 0, sizeof ( tr ) );
	Com_Memset( &backEnd, 0, sizeof ( backEnd ) );
	Com_Memset( &tess, 0, sizeof ( tess ) );

	if ( ( qintptr )tess.xyz & 15 ) {
		common->Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}
	Com_Memset( tess.constantColor255, 255, sizeof ( tess.constantColor255 ) );

	R_InitFunctionTables();

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	R_InitBackEndData();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();

	int err = qglGetError();
	if ( err != GL_NO_ERROR ) {
		common->Printf( "glGetError() = 0x%x\n", err );
	}

	common->Printf( "----- finished R_Init -----\n" );
}

void R_BeginRegistration( glconfig_t* glconfigOut ) {
	R_Init();

	*glconfigOut = glConfig;

	R_SyncRenderThread();

	r_oldviewleaf = NULL;
	r_viewleaf = NULL;
	r_oldviewcluster = -1;
	r_viewcluster = -1;
	tr.viewCluster = -1;		// force markleafs to regenerate
	R_ClearFlares();
	R_ClearScene();

	tr.registered = true;

	if ( GGameType & GAME_Tech3 ) {
		// NOTE: this sucks, for some reason the first stretch pic is never drawn
		// without this we'd see a white flash on a level load because the very
		// first time the level shot would not be drawn
		R_StretchPic( 0, 0, 0, 0, 0, 0, 1, 1, 0 );
	}
}

//	Touch all images to make sure they are resident
void R_EndRegistration() {
	if ( GGameType & GAME_QuakeHexen ) {
		GL_BuildLightmaps();
	}
	R_SyncRenderThread();
	RB_ShowImages();
}

void R_PurgeCache() {
	R_PurgeShaders();
	R_PurgeBackupImages( 9999999 );
	R_PurgeModels( 9999999 );
}

void R_Shutdown( bool destroyWindow ) {
	common->Printf( "R_Shutdown( %i )\n", destroyWindow );

	Cmd_RemoveCommand( "modellist" );
	Cmd_RemoveCommand( "imagelist" );
	Cmd_RemoveCommand( "shaderlist" );
	Cmd_RemoveCommand( "skinlist" );
	Cmd_RemoveCommand( "screenshot" );
	Cmd_RemoveCommand( "screenshotJPEG" );
	Cmd_RemoveCommand( "gfxinfo" );
	Cmd_RemoveCommand( "modelist" );

	// Ridah, keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeCache();
	if ( r_cache->integer && tr.registered && !destroyWindow ) {
		R_BackupModels();
		R_BackupShaders();
		R_BackupImages();
	}

	if ( tr.registered ) {
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();

		R_FreeModels();

		R_FreeShaders();

		R_DeleteTextures();

		R_FreeBackEndData();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		// shutdown QGL subsystem
		QGL_Shutdown();
	}

	tr.registered = false;
}
