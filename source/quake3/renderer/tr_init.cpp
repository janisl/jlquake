/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

static void GfxInfo_f( void );

QCvar	*r_inGameVideo;
QCvar	*r_dlightBacks;

QCvar	*r_norefresh;
QCvar	*r_drawentities;

QCvar  *r_glDriver;
QCvar	*r_noportals;
QCvar	*r_portalOnly;

QCvar	*r_debugSurface;

/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	R_CommonInitOpenGL();

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearDepth( 1.0f );

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState (GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;
	glState.faceCulling = CT_TWO_SIDED;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );
}


/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void ) 
{
	QCvar *sys_cpustring = Cvar_Get( "sys_cpustring", "", 0 );
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};

	CommonGfxInfo_f();
	if ( glConfig.deviceSupportsGamma )
	{
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else
	{
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}
	ri.Printf( PRINT_ALL, "CPU: %s\n", sys_cpustring->string );

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			} else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			ri.Printf( PRINT_ALL, "none\n" );
		} else if ( primitives == 2 ) {
			ri.Printf( PRINT_ALL, "single glDrawElements\n" );
		} else if ( primitives == 1 ) {
			ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
		} else if ( primitives == 3 ) {
			ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
	if (r_vertexLight->integer)
	{
		ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( glConfig.smpActive ) {
		ri.Printf( PRINT_ALL, "Using dual processor acceleration\n" );
	}
	if ( r_finish->integer ) {
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
}

/*
===============
R_Register
===============
*/
void R_Register( void ) 
{
	R_SharedRegister();
	//
	// latched and archived variables
	//
	r_glDriver = Cvar_Get( "r_glDriver", OPENGL_DRIVER_NAME, CVAR_ARCHIVE | CVAR_LATCH2 );

	//
	// archived variables that can change at any time
	//
	r_inGameVideo = Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_dlightBacks = Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );

	//
	// temporary variables that can change at any time
	//
	r_portalOnly = Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_norefresh = Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_debugSurface = Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_noportals = Cvar_Get ("r_noportals", "0", CVAR_CHEAT);

	Cmd_AddCommand( "modellist", R_Modellist_f );
	Cmd_AddCommand( "gfxinfo", GfxInfo_f );
}

/*
===============
R_Init
===============
*/
void R_Init()
{	
	R_CommonInit1();

	R_Register();

	R_InitBackEndData();

	InitOpenGL();

	R_CommonInit2();
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

	Cmd_RemoveCommand ("gfxinfo");
	Cmd_RemoveCommand( "modelist" );
	Cmd_RemoveCommand( "shaderstate" );

	R_CommonShutdown(destroyWindow);
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if (!Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp ) {
	static refexport_t	re;

	ri = *rimp;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n", 
			REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.LoadWorld = RE_LoadWorldMap;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.LightForPoint = R_LightForPoint;
	re.RenderScene = RE_RenderScene;

	re.DrawStretchRaw = RE_StretchRaw;

	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	return &re;
}
