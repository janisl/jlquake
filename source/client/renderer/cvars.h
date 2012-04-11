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

#ifndef _RENDER_CVARS_H
#define _RENDER_CVARS_H

extern Cvar*	r_logFile;				// number of frames to emit GL logs
extern Cvar*	r_verbose;				// used for verbose debug spew

extern Cvar*	r_mode;					// video mode
extern Cvar*	r_fullscreen;
extern Cvar*	r_allowSoftwareGL;		// don't abort out if the pixelformat claims software
extern Cvar*	r_stencilbits;			// number of desired stencil bits
extern Cvar*	r_depthbits;			// number of desired depth bits
extern Cvar*	r_colorbits;			// number of desired color bits, only relevant for fullscreen
extern Cvar*	r_stereo;				// desired pixelformat stereo flag
extern Cvar*	r_displayRefresh;		// optional display refresh option
extern Cvar*	r_swapInterval;
extern Cvar*	r_drawBuffer;

extern Cvar*	r_ext_compressed_textures;	// these control use of specific extensions
extern Cvar*	r_ext_multitexture;
extern Cvar*	r_ext_texture_env_add;
extern Cvar*	r_ext_gamma_control;
extern Cvar*	r_ext_compiled_vertex_array;
extern Cvar*	r_ext_point_parameters;
extern Cvar* r_ext_NV_fog_dist;
extern Cvar* r_ext_texture_filter_anisotropic;
extern Cvar* r_ext_ATI_pntriangles;

extern Cvar*	r_gamma;
extern Cvar*	r_ignorehwgamma;		// overrides hardware gamma capabilities
extern Cvar*	r_intensity;
extern Cvar*	r_overBrightBits;

extern Cvar*	r_wateralpha;
extern Cvar*	r_roundImagesDown;
extern Cvar*	r_picmip;				// controls picmip values
extern Cvar*	r_texturebits;			// number of desired texture bits
										// 0 = use framebuffer depth
										// 16 = use 16-bit textures
										// 32 = use 32-bit textures
										// all else = error
extern Cvar*	r_colorMipLevels;		// development aid to see texture mip usage
extern Cvar*	r_simpleMipMaps;
extern Cvar*	r_textureMode;

extern Cvar*	r_ignoreGLErrors;

extern Cvar*	r_nobind;				// turns off binding to appropriate textures

extern Cvar*	r_mapOverBrightBits;
extern Cvar*	r_vertexLight;			// vertex lighting mode for better performance
extern Cvar*	r_lightmap;				// render lightmaps only
extern Cvar*	r_fullbright;			// avoid lightmap pass
extern Cvar*	r_singleShader;			// make most world faces use default shader

extern Cvar*	r_subdivisions;

extern Cvar*	r_ignoreFastPath;		// allows us to ignore our Tess fast paths
extern Cvar*	r_detailTextures;		// enables/disables detail texturing stages
extern Cvar*	r_uiFullScreen;			// ui is running fullscreen
extern Cvar*	r_printShaders;

extern Cvar*	r_saveFontData;

extern Cvar*	r_smp;
extern Cvar*	r_maxpolys;
extern Cvar*	r_maxpolyverts;

extern Cvar*	r_dynamiclight;			// dynamic lights enabled/disabled
extern Cvar*	r_znear;				// near Z clip plane

extern Cvar*	r_nocull;

extern Cvar*	r_primitives;			// "0" = based on compiled vertex array existance
										// "1" = glDrawElemet tristrips
										// "2" = glDrawElements triangles
										// "-1" = no drawing
extern Cvar*	r_vertex_arrays;
extern Cvar*	r_lerpmodels;
extern Cvar*	r_shadows;				// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern Cvar*	r_debugSort;
extern Cvar*	r_showtris;				// enables wireframe rendering of the world
extern Cvar*	r_shownormals;			// draws wireframe normals
extern Cvar*	r_offsetFactor;
extern Cvar*	r_offsetUnits;
extern Cvar*	r_clear;				// force screen clear every frame

extern Cvar*	r_modulate;
extern Cvar*	r_ambientScale;
extern Cvar*	r_directedScale;
extern Cvar*	r_debugLight;

extern Cvar*	r_lightlevel;	// FIXME: This is a HACK to get the client's light level

extern Cvar*	r_lodbias;				// push/pull LOD transitions
extern Cvar*	r_lodscale;

extern Cvar*	r_skymip;
extern Cvar*	r_fastsky;				// controls whether sky should be cleared or drawn
extern Cvar*	r_showsky;				// forces sky in front of all surfaces
extern Cvar*	r_drawSun;				// controls drawing of sun quad

extern Cvar*	r_lodCurveError;

extern Cvar*	r_ignore;				// used for debugging anything

extern Cvar*	r_keeptjunctions;
extern Cvar*	r_texsort;
extern Cvar*	r_dynamic;
extern Cvar*	r_saturatelighting;

extern Cvar*	r_nocurves;
extern Cvar*	r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern Cvar*	r_novis;				// disable/enable usage of PVS
extern Cvar*	r_lockpvs;
extern Cvar*	r_showcluster;
extern Cvar*	r_drawworld;			// disable/enable world rendering
extern Cvar*	r_measureOverdraw;		// enables stencil buffer overdraw measurement
extern Cvar*	r_finish;
extern Cvar*	r_showImages;
extern Cvar*	r_speeds;				// various levels of information display
extern Cvar*	r_showSmp;
extern Cvar*	r_skipBackEnd;
extern Cvar*	r_drawentities;			// disable/enable entity rendering
extern Cvar*	r_debugSurface;
extern Cvar*	r_norefresh;			// bypasses the ref rendering

extern Cvar*	r_railWidth;
extern Cvar*	r_railCoreWidth;
extern Cvar*	r_railSegmentLength;

extern Cvar*	r_flares;				// light flares
extern Cvar*	r_flareSize;
extern Cvar*	r_flareFade;

extern Cvar*	r_particle_size;
extern Cvar*	r_particle_min_size;
extern Cvar*	r_particle_max_size;
extern Cvar*	r_particle_att_a;
extern Cvar*	r_particle_att_b;
extern Cvar*	r_particle_att_c;

extern Cvar*	r_noportals;
extern Cvar*	r_portalOnly;

extern Cvar* r_nv_fogdist_mode;

extern Cvar* r_textureAnisotropy;

extern Cvar* r_ati_truform_tess;
extern Cvar* r_ati_truform_pointmode;
extern Cvar* r_ati_truform_normalmode;

extern Cvar* r_zfar;					// far Z clip plane

extern Cvar* r_dlightScale;				// global user attenuation of dlights

extern Cvar* r_picmip2;					// controls picmip values for designated (character skin) textures

extern Cvar* r_portalsky;

extern Cvar* r_cache;
extern Cvar* r_cacheShaders;
extern Cvar* r_cacheModels;
extern Cvar* r_cacheGathering;

extern Cvar* r_bonesDebug;

extern Cvar* r_wolffog;

extern Cvar* r_drawfoliage;				// ydnar: disable/enable foliage rendering

extern Cvar* r_clampToEdge;				// ydnar: opengl 1.2 GL_CLAMP_TO_EDGE support

extern Cvar* r_trisColor;				// enables modifying of the wireframe colour (in 0xRRGGBB[AA] format, alpha defaults to FF)
extern Cvar* r_normallength;			// length of the normals

extern Cvar* r_buildScript;

#endif
