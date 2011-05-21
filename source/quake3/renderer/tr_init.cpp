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

QCvar	*r_flareSize;
QCvar	*r_flareFade;

QCvar	*r_railWidth;
QCvar	*r_railCoreWidth;
QCvar	*r_railSegmentLength;

QCvar	*r_ignoreFastPath;

QCvar	*r_ignore;

QCvar	*r_detailTextures;

QCvar	*r_znear;

QCvar	*r_smp;
QCvar	*r_showSmp;
QCvar	*r_skipBackEnd;

QCvar	*r_measureOverdraw;

QCvar	*r_inGameVideo;
QCvar	*r_fastsky;
QCvar	*r_drawSun;
QCvar	*r_dynamiclight;
QCvar	*r_dlightBacks;

QCvar	*r_lodbias;
QCvar	*r_lodscale;

QCvar	*r_norefresh;
QCvar	*r_drawentities;
QCvar	*r_drawworld;
QCvar	*r_speeds;
QCvar	*r_fullbright;
QCvar	*r_novis;
QCvar	*r_nocull;
QCvar	*r_facePlaneCull;
QCvar	*r_showcluster;
QCvar	*r_nocurves;

QCvar	*r_primitives;

QCvar	*r_drawBuffer;
QCvar  *r_glDriver;
QCvar	*r_lightmap;
QCvar	*r_vertexLight;
QCvar	*r_uiFullScreen;
QCvar	*r_shadows;
QCvar	*r_flares;
QCvar	*r_singleShader;
QCvar	*r_showtris;
QCvar	*r_showsky;
QCvar	*r_shownormals;
QCvar	*r_finish;
QCvar	*r_clear;
QCvar	*r_offsetFactor;
QCvar	*r_offsetUnits;
QCvar	*r_lockpvs;
QCvar	*r_noportals;
QCvar	*r_portalOnly;

QCvar	*r_subdivisions;
QCvar	*r_lodCurveError;

QCvar	*r_mapOverBrightBits;

QCvar	*r_debugSurface;

QCvar	*r_showImages;

QCvar	*r_ambientScale;
QCvar	*r_directedScale;
QCvar	*r_debugLight;
QCvar	*r_debugSort;
QCvar	*r_printShaders;
QCvar	*r_saveFontData;

QCvar	*r_maxpolys;
int		max_polys;
QCvar	*r_maxpolyverts;
int		max_polyverts;

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
	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_glDriver
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//
	
	if ( glConfig.vidWidth == 0 )
	{
		R_CommonInit();
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

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

============================================================================== 
*/ 

/* 
================== 
RB_TakeScreenshot
================== 
*/  
void RB_TakeScreenshot( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;
		
	buffer = (byte*)ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*3);

	qglReadPixels( x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer);

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 3 );
	}

	R_SaveTGA(fileName, buffer, width, height, false);

	ri.Hunk_FreeTempMemory( buffer );
}

/* 
================== 
RB_TakeScreenshotJPEG
================== 
*/  
void RB_TakeScreenshotJPEG( int x, int y, int width, int height, char *fileName ) {
	byte		*buffer;

	buffer = (byte*)ri.Hunk_AllocateTempMemory(glConfig.vidWidth*glConfig.vidHeight*4);

	qglReadPixels( x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer ); 

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, glConfig.vidWidth * glConfig.vidHeight * 4 );
	}

	FS_WriteFile( fileName, buffer, 1 );		// create path
	R_SaveJPG( fileName, 95, glConfig.vidWidth, glConfig.vidHeight, buffer);

	ri.Hunk_FreeTempMemory( buffer );
}

/*
==================
RB_TakeScreenshotCmd
==================
*/
const void *RB_TakeScreenshotCmd( const void *data ) {
	const screenshotCommand_t	*cmd;
	
	cmd = (const screenshotCommand_t *)data;
	
	if (cmd->jpeg)
		RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	else
		RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	
	return (const void *)(cmd + 1);	
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg ) {
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = (screenshotCommand_t*)R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	QStr::NCpyZ( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilename( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		QStr::Sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	QStr::Sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
		, a, b, c, d );
}

/* 
================== 
R_ScreenshotFilename
================== 
*/  
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName ) {
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		QStr::Sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	QStr::Sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
		, a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void ) {
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source;
	byte		*src, *dst;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	sprintf( checkname, "levelshots/%s.tga", tr.world->baseName );

	source = (byte*)ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight * 3 );

	buffer = (byte*)ri.Hunk_AllocateTempMemory(128 * 128 * 3);

	qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_RGB, GL_UNSIGNED_BYTE, source ); 

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + 3 * ( glConfig.vidWidth * (int)( (y*3+yy)*yScale ) + (int)( (x*4+xx)*xScale ) );
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 3 * (y * 128 + x);
			dst[0] = r / 12;
			dst[1] = g / 12;
			dst[2] = b / 12;
		}
	}

	// gamma correct
	if ( ( tr.overbrightBits > 0 ) && glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer, 128 * 128 * 3 );
	}

	R_SaveTGA(checkname, buffer, 128, 128, false);

	ri.Hunk_FreeTempMemory( buffer );
	ri.Hunk_FreeTempMemory( source );

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

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
void R_ScreenShot_f (void) {
	char	checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !QStr::Cmp( Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !QStr::Cmp( Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		QStr::Sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

      if (!FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber >= 9999 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
} 

void R_ScreenShotJPEG_f (void) {
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !QStr::Cmp( Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !QStr::Cmp( Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	} else {
		silent = qfalse;
	}

	if ( Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		QStr::Sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", Cmd_Argv( 1 ) );
	} else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

      if (!FS_FileExists( checkname ))
      {
        break; // file doesn't exist
      }
		}

		if ( lastNumber == 10000 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n"); 
			return;
 		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qtrue );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
} 

//============================================================================

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

	r_detailTextures = Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_vertexLight = Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH2 );
	r_uiFullScreen = Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH2);
#if defined(MACOS_X) || defined(__linux__)
  // Default to using SMP on Mac OS X or Linux if we have multiple processors
	r_smp = Cvar_Get( "r_smp", Sys_ProcessorCount() > 1 ? "1" : "0", CVAR_ARCHIVE | CVAR_LATCH2);
#else        
	r_smp = Cvar_Get( "r_smp", "0", CVAR_ARCHIVE | CVAR_LATCH2);
#endif
	r_ignoreFastPath = Cvar_Get( "r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH2 );

	//
	// temporary latched variables that can only change over a restart
	//
	r_fullbright = Cvar_Get ("r_fullbright", "0", CVAR_LATCH2|CVAR_CHEAT );
	r_mapOverBrightBits = Cvar_Get ("r_mapOverBrightBits", "2", CVAR_LATCH2 );
	r_singleShader = Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH2 );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
	r_lodbias = Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_flares = Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_znear = Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	AssertCvarRange( r_znear, 0.001f, 200, qtrue );
	r_fastsky = Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_inGameVideo = Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = Cvar_Get( "r_drawSun", "0", CVAR_ARCHIVE );
	r_dynamiclight = Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_dlightBacks = Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );
	r_finish = Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_facePlaneCull = Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_railWidth = Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
	r_railCoreWidth = Cvar_Get( "r_railCoreWidth", "6", CVAR_ARCHIVE );
	r_railSegmentLength = Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );

	r_primitives = Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

	r_ambientScale = Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	//
	// temporary variables that can change at any time
	//
	r_showImages = Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = Cvar_Get( "r_saveFontData", "0", 0 );

	r_nocurves = Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_lightmap = Cvar_Get ("r_lightmap", "0", 0 );
	r_portalOnly = Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );

	r_flareSize = Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);

	r_showSmp = Cvar_Get ("r_showSmp", "0", CVAR_CHEAT);
	r_skipBackEnd = Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_debugSurface = Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_showtris = Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_drawBuffer = Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = Cvar_Get( "cg_shadows", "1", 0 );

	r_maxpolys = Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);

	Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	Cmd_AddCommand( "skinlist", R_SkinList_f );
	Cmd_AddCommand( "modellist", R_Modellist_f );
	Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	Cmd_AddCommand( "gfxinfo", GfxInfo_f );
}

/*
===============
R_Init
===============
*/
void R_Init( void ) {	
	int	err;
	int i;
	byte *ptr;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );
	tr_shared = &tr;

	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );
	Com_Memset( &backEnd, 0, sizeof( backEnd ) );
	Com_Memset( &tess, 0, sizeof( tess ) );

//	Swap_Init();

	if ( (qintptr)tess.xyz & 15 ) {
		Com_Printf( "WARNING: tess.xyz not 16 byte aligned\n" );
	}
	Com_Memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ )
	{
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 )
		{
			if ( i < FUNCTABLE_SIZE / 4 )
			{
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else
			{
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else
		{
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = (byte*)ri.Hunk_Alloc( sizeof( *backEndData[0] ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData[0] = (backEndData_t *) ptr;
	backEndData[0]->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData[0] ));
	backEndData[0]->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData[0] ) + sizeof(srfPoly_t) * max_polys);
	if ( r_smp->integer ) {
		ptr = (byte*)ri.Hunk_Alloc( sizeof( *backEndData[1] ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
		backEndData[1] = (backEndData_t *) ptr;
		backEndData[1]->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData[1] ));
		backEndData[1]->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData[1] ) + sizeof(srfPoly_t) * max_polys);
	} else {
		backEndData[1] = NULL;
	}
	R_ToggleSmpFrame();

	InitOpenGL();

	R_InitImages();

	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();


	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow ) {	

	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("screenshotJPEG");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("shaderlist");
	Cmd_RemoveCommand ("skinlist");
	Cmd_RemoveCommand ("gfxinfo");
	Cmd_RemoveCommand( "modelist" );
	Cmd_RemoveCommand( "shaderstate" );


	if ( tr.registered ) {
		R_SyncRenderThread();
		R_ShutdownCommandBuffers();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		// shutdown QGL subsystem
		QGL_Shutdown();
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
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;

	re.RegisterFont = RE_RegisterFont;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	return &re;
}
