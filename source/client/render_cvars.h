//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#ifndef _RENDER_CVARS_H
#define _RENDER_CVARS_H

extern QCvar*	r_logFile;				// number of frames to emit GL logs
extern QCvar*	r_verbose;				// used for verbose debug spew

extern QCvar*	r_mode;					// video mode
extern QCvar*	r_fullscreen;
extern QCvar*	r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
extern QCvar*	r_stencilbits;			// number of desired stencil bits
extern QCvar*	r_depthbits;			// number of desired depth bits
extern QCvar*	r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern QCvar*	r_stereo;				// desired pixelformat stereo flag
extern QCvar*	r_displayRefresh;		// optional display refresh option
extern QCvar*	r_swapInterval;
extern QCvar*	r_drawBuffer;

extern QCvar*	r_ext_compressed_textures;	// these control use of specific extensions
extern QCvar*	r_ext_multitexture;
extern QCvar*	r_ext_texture_env_add;
extern QCvar*	r_ext_gamma_control;
extern QCvar*	r_ext_compiled_vertex_array;
extern QCvar*	r_ext_point_parameters;

extern QCvar*	r_gamma;
extern QCvar*	r_ignorehwgamma;		// overrides hardware gamma capabilities
extern QCvar*	r_intensity;
extern QCvar*	r_overBrightBits;

extern QCvar*	r_wateralpha;
extern QCvar*	r_roundImagesDown;
extern QCvar*	r_picmip;				// controls picmip values
extern QCvar*	r_texturebits;			// number of desired texture bits
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error
extern QCvar*	r_colorMipLevels;		// development aid to see texture mip usage
extern QCvar*	r_simpleMipMaps;
extern QCvar*	r_textureMode;

extern QCvar*	r_ignoreGLErrors;

extern QCvar*	r_nobind;				// turns off binding to appropriate textures

extern QCvar*	r_mapOverBrightBits;
extern QCvar*	r_vertexLight;			// vertex lighting mode for better performance
extern QCvar*	r_lightmap;				// render lightmaps only
extern QCvar*	r_fullbright;			// avoid lightmap pass
extern QCvar*	r_singleShader;			// make most world faces use default shader

extern QCvar*	r_subdivisions;

extern QCvar*	r_ignoreFastPath;		// allows us to ignore our Tess fast paths
extern QCvar*	r_detailTextures;		// enables/disables detail texturing stages
extern QCvar*	r_uiFullScreen;			// ui is running fullscreen
extern QCvar*	r_printShaders;

extern QCvar*	r_saveFontData;

extern QCvar*	r_smp;
extern QCvar*	r_maxpolys;
extern QCvar*	r_maxpolyverts;

extern QCvar*	r_dynamiclight;			// dynamic lights enabled/disabled
extern QCvar*	r_znear;				// near Z clip plane

extern QCvar*	r_nocull;

extern QCvar*	r_primitives;			// "0" = based on compiled vertex array existance
										// "1" = glDrawElemet tristrips
										// "2" = glDrawElements triangles
										// "-1" = no drawing
extern QCvar*	r_vertex_arrays;
extern QCvar*	r_lerpmodels;
extern QCvar*	r_shadows;				// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern QCvar*	r_debugSort;
extern QCvar*	r_showtris;				// enables wireframe rendering of the world
extern QCvar*	r_shownormals;			// draws wireframe normals
extern QCvar*	r_offsetFactor;
extern QCvar*	r_offsetUnits;
extern QCvar*	r_clear;				// force screen clear every frame

extern QCvar*	r_modulate;
extern QCvar*	r_ambientScale;
extern QCvar*	r_directedScale;
extern QCvar*	r_debugLight;

extern QCvar*	r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern QCvar*	r_lodbias;				// push/pull LOD transitions
extern QCvar*	r_lodscale;

extern QCvar*	r_skymip;
extern QCvar*	r_fastsky;				// controls whether sky should be cleared or drawn
extern QCvar*	r_showsky;				// forces sky in front of all surfaces
extern QCvar*	r_drawSun;				// controls drawing of sun quad

extern QCvar*	r_lodCurveError;

extern QCvar*	r_ignore;				// used for debugging anything

extern QCvar*	r_keeptjunctions;
extern QCvar*	r_texsort;
extern QCvar*	r_dynamic;
extern QCvar*	r_saturatelighting;

extern QCvar*	r_nocurves;
extern QCvar*	r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern QCvar*	r_novis;				// disable/enable usage of PVS
extern QCvar*	r_lockpvs;
extern QCvar*	r_showcluster;
extern QCvar*	r_drawworld;			// disable/enable world rendering
extern QCvar*	r_measureOverdraw;		// enables stencil buffer overdraw measurement
extern QCvar*	r_finish;
extern QCvar*	r_showImages;
extern QCvar*	r_speeds;				// various levels of information display
extern QCvar*	r_showSmp;
extern QCvar*	r_skipBackEnd;
extern QCvar*	r_drawentities;			// disable/enable entity rendering
extern QCvar*	r_debugSurface;
extern QCvar*	r_norefresh;			// bypasses the ref rendering

extern QCvar*	r_railWidth;
extern QCvar*	r_railCoreWidth;
extern QCvar*	r_railSegmentLength;

extern QCvar*	r_flares;				// light flares
extern QCvar*	r_flareSize;
extern QCvar*	r_flareFade;

extern QCvar*	r_particle_size;
extern QCvar*	r_particle_min_size;
extern QCvar*	r_particle_max_size;
extern QCvar*	r_particle_att_a;
extern QCvar*	r_particle_att_b;
extern QCvar*	r_particle_att_c;

extern QCvar*	r_noportals;
extern QCvar*	r_portalOnly;

#endif
