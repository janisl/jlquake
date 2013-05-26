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

#ifndef _R_DRIVER_H
#define _R_DRIVER_H

#include "../../common/qcommon.h"

enum rserr_t
{
	RSERR_OK,

	RSERR_INVALID_FULLSCREEN,
	RSERR_INVALID_MODE,

	RSERR_UNKNOWN
};

rserr_t GLimp_SetMode( int mode, int colorbits, bool fullscreen );
void GLimp_Shutdown();
const char* GLimp_GetSystemExtensionsString();
void* GLimp_GetProcAddress( const char* Name );

// NOTE TTimo linux works with float gamma value, not the gamma table
//   the params won't be used, getting the r_gamma cvar directly
void GLimp_SetGamma( unsigned char red[ 256 ], unsigned char green[ 256 ], unsigned char blue[ 256 ] );
void GLimp_SwapBuffers();

bool GLimp_SpawnRenderThread( void ( * function )() );
void* GLimp_RendererSleep();
void GLimp_FrontEndSleep();
void GLimp_WakeRenderer( void* data );

#endif
