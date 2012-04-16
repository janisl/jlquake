/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

void R_Register_();

static void GfxInfo_f( void );

/*
The tessellation level and normal generation mode are specified with:

	void qglPNTriangles{if}ATI(enum pname, T param)

	If <pname> is:
		GL_PN_TRIANGLES_NORMAL_MODE_ATI -
			<param> must be one of the symbolic constants:
				- GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI or
				- GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI
			which will select linear or quadratic normal interpolation respectively.
		GL_PN_TRIANGLES_POINT_MODE_ATI -
			<param> must be one of the symbolic  constants:
				- GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI or
				- GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI
			which will select linear or cubic interpolation respectively.
		GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI -
			<param> should be a value specifying the number of evaluation points on each edge.  This value must be
			greater than 0 and less than or equal to the value given by GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI.

	An INVALID_VALUE error will be generated if the value for <param> is less than zero or greater than the max value.

Associated 'gets':
Get Value                               Get Command Type     Minimum Value								Attribute
---------                               ----------- ----     ------------								---------
PN_TRIANGLES_ATI						IsEnabled   B		False                                       PN Triangles/enable
PN_TRIANGLES_NORMAL_MODE_ATI			GetIntegerv Z2		PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI		PN Triangles
PN_TRIANGLES_POINT_MODE_ATI				GetIntegerv Z2		PN_TRIANGLES_POINT_MODE_CUBIC_ATI			PN Triangles
PN_TRIANGLES_TESSELATION_LEVEL_ATI		GetIntegerv Z+		1											PN Triangles
MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI	GetIntegerv Z+		1											-




*/
//----(SA)	end


/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
void InitOpenGLSubsystem();
static void InitOpenGL( void ) {
	if ( glConfig.vidWidth == 0 ) {
		InitOpenGLSubsystem();
	}

	// init command buffers and SMP
	R_InitCommandBuffers();

	// print info
	GfxInfo_f();

	// set default state
	GL_SetDefaultState();
}

/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

void RB_TakeScreenshot(int x, int y, int width, int height, const char* fileName, bool IsJpeg);
bool R_FindAvailableScreenshotFilename(int& lastNumber, char* fileName, const char* Extension);
void R_LevelShot( void );

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f( void ) {
	char checkname[MAX_OSPATH];
	int len;
	static int lastNumber = -1;
	qboolean silent;

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		String::Sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename
		if (!R_FindAvailableScreenshotFilename(lastNumber, checkname, "tga"))
		{
			return;
		}

		lastNumber++;
	}


	RB_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, false );

	if ( !silent ) {
		ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
	}
}

void R_ScreenShotJPEG_f( void ) {
	char checkname[MAX_OSPATH];
	int len;
	static int lastNumber = -1;
	qboolean silent;

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !String::Cmp( ri.Cmd_Argv( 1 ), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		String::Sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename
		if (!R_FindAvailableScreenshotFilename(lastNumber, checkname, "jpg"))
		{
			return;
		}

		lastNumber++;
	}


	RB_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, true );

	if ( !silent ) {
		ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
	}
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void ) {
	qglClearDepth( 1.0f );

	qglCullFace( GL_FRONT );

	qglColor4f( 1,1,1,1 );

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable( GL_TEXTURE_2D );
	GL_TextureMode( r_textureMode->string );
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

	qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );

//----(SA)	added.
	// ATI pn_triangles
	if ( qglPNTrianglesiATI ) {
		int maxtess;
		// get max supported tesselation
		qglGetIntegerv( GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI, (GLint*)&maxtess );
#ifdef __MACOS__
		glConfig.ATIMaxTruformTess = 7;
#else
		glConfig.ATIMaxTruformTess = maxtess;
#endif
		// cap if necessary
		if ( r_ati_truform_tess->value > maxtess ) {
			ri.Cvar_Set( "r_ati_truform_tess", va( "%d", maxtess ) );
		}

		// set Wolf defaults
		qglPNTrianglesiATI( GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, r_ati_truform_tess->value );
	}

	if ( glConfig.anisotropicAvailable ) {
		float maxAnisotropy;

		qglGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
		glConfig.maxAnisotropy = maxAnisotropy;

		// set when rendering
//	   qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, glConfig.maxAnisotropy);
	}

//----(SA)	end
}


/*
================
GfxInfo_f
================
*/
void GfxInfo_f( void ) {
	Cvar *sys_cpustring = ri.Cvar_Get( "sys_cpustring", "", 0 );
	const char *enablestrings[] =
	{
		"disabled",
		"enabled"
	};
	const char *fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri.Printf( PRINT_ALL, "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.maxActiveTextures );
	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency ) {
		ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	} else
	{
		ri.Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma ) {
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	} else
	{
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}
	ri.Printf( PRINT_ALL, "CPU: %s\n", sys_cpustring->string );

	// rendering primitives
	{
		int primitives;

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
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression != TC_NONE] );

	ri.Printf( PRINT_ALL, "NV distance fog: %s\n", enablestrings[glConfig.NVFogAvailable != 0] );
	if ( glConfig.NVFogAvailable ) {
		ri.Printf( PRINT_ALL, "Fog Mode: %s\n", r_nv_fogdist_mode->string );
	}

	if ( r_vertexLight->integer ) {
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
void R_Register( void ) {
	R_Register_();

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {
	int err;
	int i;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	memset( &tr, 0, sizeof( tr ) );
	memset( &backEnd, 0, sizeof( backEnd ) );
	memset( &tess, 0, sizeof( tess ) );

	if ( (qintptr)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}
	memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]      = sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]   = ( i < FUNCTABLE_SIZE / 2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 ) {
			if ( i < FUNCTABLE_SIZE / 4 ) {
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			} else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i - FUNCTABLE_SIZE / 4];
			}
		} else
		{
			tr.triangleTable[i] = -tr.triangleTable[i - FUNCTABLE_SIZE / 2];
		}
	}

	// Ridah, init the virtual memory
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

	err = qglGetError();
	if ( err != GL_NO_ERROR ) {
		ri.Printf( PRINT_ALL, "glGetError() = 0x%x\n", err );
	}

	ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {

	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri.Cmd_RemoveCommand( "modellist" );
	ri.Cmd_RemoveCommand( "screenshotJPEG" );
	ri.Cmd_RemoveCommand( "screenshot" );
	ri.Cmd_RemoveCommand( "imagelist" );
	ri.Cmd_RemoveCommand( "shaderlist" );
	ri.Cmd_RemoveCommand( "skinlist" );
	ri.Cmd_RemoveCommand( "gfxinfo" );
	ri.Cmd_RemoveCommand( "modelist" );
	ri.Cmd_RemoveCommand( "shaderstate" );

	// Ridah
	ri.Cmd_RemoveCommand( "cropimages" );
	// done.

	R_ShutdownCommandBuffers();

	// Ridah, keep a backup of the current images if possible
	// clean out any remaining unused media from the last backup
	R_PurgeShaders();
	R_PurgeBackupImages( 9999999 );
	R_PurgeModels( 9999999 );

	if (r_cache->integer && tr.registered && !destroyWindow)
	{
		R_BackupModels();
		R_BackupShaders();
		R_BackupImages();
	}
	if (tr.registered)
	{
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

		ri.Tag_Free();  // wipe all render alloc'd zone memory
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void ) {
	R_SyncRenderThread();
	if ( !Sys_LowPhysicalMemory() ) {
		RB_ShowImages();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
refexport_t *GetRefAPI( int apiVersion, refimport_t *rimp ) {
	static refexport_t re;

	ri = *rimp;

	memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf( PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
				   REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.EndRegistration  = RE_EndRegistration;

	re.MarkFragments    = R_MarkFragments;

	re.RegisterFont     = RE_RegisterFont;

	return &re;
}

