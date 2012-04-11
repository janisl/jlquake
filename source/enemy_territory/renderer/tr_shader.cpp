/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "tr_local.h"

#define SHADER_HASH_SIZE      4096
extern shader_t*       ShaderHashTable[SHADER_HASH_SIZE];

//bani - dynamic shader list
struct dynamicshader_t
{
	char *shadertext;
	dynamicshader_t *next;
};
extern dynamicshader_t *dshader;

/*
====================
RE_LoadDynamicShader

bani - load a new dynamic shader

if shadertext is NULL, looks for matching shadername and removes it

returns qtrue if request was successful, qfalse if the gods were angered
====================
*/
qboolean RE_LoadDynamicShader( const char *shadername, const char *shadertext ) {
	const char *func_err = "WARNING: RE_LoadDynamicShader";
	dynamicshader_t *dptr, *lastdptr;
	const char *q;
	char *token;

	if ( !shadername && shadertext ) {
		ri.Printf( PRINT_WARNING, "%s called with NULL shadername and non-NULL shadertext:\n%s\n", func_err, shadertext );
		return qfalse;
	}

	if ( shadername && String::Length( shadername ) >= MAX_QPATH ) {
		ri.Printf( PRINT_WARNING, "%s shadername %s exceeds MAX_QPATH\n", func_err, shadername );
		return qfalse;
	}

	//empty the whole list
	if ( !shadername && !shadertext ) {
		dptr = dshader;
		while ( dptr ) {
			lastdptr = dptr->next;
			Z_Free( dptr->shadertext );
			Z_Free( dptr );
			dptr = lastdptr;
		}
		dshader = NULL;
		return qtrue;
	}

	//walk list for existing shader to delete, or end of the list
	dptr = dshader;
	lastdptr = NULL;
	while ( dptr ) {
		q = dptr->shadertext;

		token = String::ParseExt( &q, qtrue );

		if ( ( token[0] != 0 ) && !String::ICmp( token, shadername ) ) {
			//request to nuke this dynamic shader
			if ( !shadertext ) {
				if ( !lastdptr ) {
					dshader = NULL;
				} else {
					lastdptr->next = dptr->next;
				}
				Z_Free( dptr->shadertext );
				Z_Free( dptr );
				return qtrue;
			}
			ri.Printf( PRINT_WARNING, "%s shader %s already exists!\n", func_err, shadername );
			return qfalse;
		}
		lastdptr = dptr;
		dptr = dptr->next;
	}

	//cant add a new one with empty shadertext
	if ( !shadertext || !String::Length( shadertext ) ) {
		ri.Printf( PRINT_WARNING, "%s new shader %s has NULL shadertext!\n", func_err, shadername );
		return qfalse;
	}

	//create a new shader
	dptr = (dynamicshader_t *)Z_Malloc( sizeof( *dptr ) );
	if ( !dptr ) {
		Com_Error( ERR_FATAL, "Couldn't allocate struct for dynamic shader %s\n", shadername );
	}
	if ( lastdptr ) {
		lastdptr->next = dptr;
	}
	dptr->shadertext = (char*)Z_Malloc( String::Length( shadertext ) + 1 );
	if ( !dptr->shadertext ) {
		Com_Error( ERR_FATAL, "Couldn't allocate buffer for dynamic shader %s\n", shadername );
	}
	String::NCpyZ( dptr->shadertext, shadertext, String::Length( shadertext ) + 1 );
	dptr->next = NULL;
	if ( !dshader ) {
		dshader = dptr;
	}

//	ri.Printf( PRINT_ALL, "Loaded dynamic shader [%s] with shadertext [%s]\n", shadername, shadertext );

	return qtrue;
}

void ScanAndLoadShaderFiles( void );
void CreateInternalShaders( void );
void CreateExternalShaders( void );

//=============================================================================
// Ridah, shader caching
extern int numBackupShaders;

/*
===============
R_LoadCacheShaders
===============
*/
void R_LoadCacheShaders( void ) {
	int len;
	char *buf;
	char    *token;
	const char *pString;
	char name[MAX_QPATH];

	if ( !r_cacheShaders->integer ) {
		return;
	}

	// don't load the cache list in between level loads, only on startup, or after a vid_restart
	if ( numBackupShaders > 0 ) {
		return;
	}

	len = ri.FS_ReadFile( "shader.cache", NULL );

	if ( len <= 0 ) {
		return;
	}

	buf = (char *)ri.Hunk_AllocateTempMemory( len );
	ri.FS_ReadFile( "shader.cache", (void **)&buf );
	pString = buf;

	while ( ( token = String::ParseExt( &pString, qtrue ) ) && token[0] ) {
		String::NCpyZ( name, token, sizeof( name ) );
		RE_RegisterModel( name );
	}

	ri.Hunk_FreeTempMemory( buf );
}
// done.
//=============================================================================

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void ) {

	glfogNum = FOG_NONE;

	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	memset( ShaderHashTable, 0, sizeof( ShaderHashTable ) );

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();

	// Ridah
	R_LoadCacheShaders();
}
