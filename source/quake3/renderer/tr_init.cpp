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

QCvar	*r_inGameVideo;
QCvar	*r_dlightBacks;

QCvar	*r_norefresh;

QCvar  *r_glDriver;

refimport_t	ri;

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
	r_norefresh = Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
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

	R_CommonInit2();
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow )
{
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

void BotDrawDebugPolygons(void (*drawPoly)(int color, int numPoints, float *points), int value);


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
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.RenderScene = RE_RenderScene;

	re.DrawStretchRaw = RE_StretchRaw;

	re.inPVS = R_inPVS;

	BotDrawDebugPolygonsFunc = BotDrawDebugPolygons;

	return &re;
}
