/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "tr_local.h"

#define SHADER_HASH_SIZE      4096

extern shader_t*       ShaderHashTable[SHADER_HASH_SIZE];

// Ridah
// Table containing string indexes for each shader found in the scripts, referenced by their checksum
// values.
struct shaderStringPointer_t
{
	const char* pStr;
	shaderStringPointer_t* next;
};

int GenerateShaderHashValue(const char* fname, const int size);

void ScanAndLoadShaderFiles( void );
void CreateInternalShaders( void );
void CreateExternalShaders( void );

//=============================================================================
// Ridah, shader caching
extern int numBackupShaders;
extern shader_t *backupShaders[MAX_SHADERS];
extern shader_t *backupHashTable[SHADER_HASH_SIZE];

/*
===============
R_PurgeShaders
===============
*/
void R_PurgeShaders( int count ) {
	int i, j, c, b;
	shader_t **sh;
	static int lastPurged = 0;

	if ( !numBackupShaders ) {
		lastPurged = 0;
		return;
	}

	// find the first shader still in memory
	c = 0;
	sh = (shader_t **)&backupShaders;
	for ( i = lastPurged; i < numBackupShaders; i++, sh++ ) {
		if ( *sh ) {
			// free all memory associated with this shader
			for ( j = 0 ; j < ( *sh )->numUnfoggedPasses ; j++ ) {
				if ( !( *sh )->stages[j] ) {
					break;
				}
				for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
					if ( ( *sh )->stages[j]->bundle[b].texMods ) {
						delete[] ( *sh )->stages[j]->bundle[b].texMods;
					}
				}
				delete ( *sh )->stages[j];
			}
			delete *sh;
			*sh = NULL;

			if ( ++c >= count ) {
				lastPurged = i;
				return;
			}
		}
	}
	lastPurged = 0;
	numBackupShaders = 0;
}

/*
===============
R_BackupShaders
===============
*/
void R_BackupShaders( void ) {

	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheShaders->integer ) {
		return;
	}

	// copy each model in memory across to the backupModels
	memcpy( backupShaders, tr.shaders, sizeof( backupShaders ) );
	// now backup the ShaderHashTable
	memcpy( backupHashTable, ShaderHashTable, sizeof( ShaderHashTable ) );

	numBackupShaders = tr.numShaders;
}

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
