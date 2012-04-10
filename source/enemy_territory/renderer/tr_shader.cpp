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

// tr_shader.c -- this file deals with the parsing and definition of shaders

extern char *s_shaderText;

static qboolean deferLoad;

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
//
extern shaderStringPointer_t shaderChecksumLookup[SHADER_HASH_SIZE];
// done.

int GenerateShaderHashValue(const char* fname, const int size);

//========================================================================================

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

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
		ri.Printf( PRINT_DEVELOPER, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader ); // bk: FIXME name
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_DEVELOPER, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void    R_ShaderList_f( void ) {
	int i;
	int count;
	shader_t    *shader;

	ri.Printf( PRINT_ALL, "-----------------------\n" );

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri.Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if ( shader->lightmapIndex >= 0 ) {
			ri.Printf( PRINT_ALL, "L " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}
		if ( shader->multitextureEnv == GL_ADD ) {
			ri.Printf( PRINT_ALL, "MT(a) " );
		} else if ( shader->multitextureEnv == GL_MODULATE ) {
			ri.Printf( PRINT_ALL, "MT(m) " );
		} else if ( shader->multitextureEnv == GL_DECAL ) {
			ri.Printf( PRINT_ALL, "MT(d) " );
		} else {
			ri.Printf( PRINT_ALL, "      " );
		}
		if ( shader->explicitlyDefined ) {
			ri.Printf( PRINT_ALL, "E " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri.Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri.Printf( PRINT_ALL, "sky " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture ) {
			ri.Printf( PRINT_ALL, "lmmt" );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture ) {
			ri.Printf( PRINT_ALL, "vlt " );
		} else {
			ri.Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri.Printf( PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name );
		} else {
			ri.Printf( PRINT_ALL,  ": %s\n", shader->name );
		}
		count++;
	}
	ri.Printf( PRINT_ALL, "%i total shaders\n", count );
	ri.Printf( PRINT_ALL, "------------------\n" );
}

// Ridah, optimized shader loading

#define MAX_SHADER_STRING_POINTERS  100000
shaderStringPointer_t shaderStringPointerList[MAX_SHADER_STRING_POINTERS];

/*
====================
BuildShaderChecksumLookup
====================
*/
static void BuildShaderChecksumLookup( void ) {
	const char *p = s_shaderText, *pOld;
	char *token;
	unsigned short int checksum;
	int numShaderStringPointers = 0;

	// initialize the checksums
	memset( shaderChecksumLookup, 0, sizeof( shaderChecksumLookup ) );

	if ( !p ) {
		return;
	}

	// loop for all labels
	while ( 1 ) {

		pOld = p;

		token = String::ParseExt( &p, qtrue );
		if ( !*token ) {
			break;
		}

		// Gordon: NOTE this is WRONG, need to either unget the {, or as i'm gonna do, assume the shader section follows, if it doesnt, it's b0rked anyway
/*		if (!String::ICmp( token, "{" )) {
			// Gordon: ok, lets try the unget method
			COM_RestoreParseSession( &p );
			// skip braced section
			String::SkipBracedSection( &p );
			continue;
		}*/

		// get it's checksum
		checksum = GenerateShaderHashValue( token, SHADER_HASH_SIZE );

//		Com_Printf( "Shader Found: %s\n", token );

		// if it's not currently used
		if ( !shaderChecksumLookup[checksum].pStr ) {
			shaderChecksumLookup[checksum].pStr = pOld;
		} else {
			// create a new list item
			shaderStringPointer_t *newStrPtr;

			if ( numShaderStringPointers >= MAX_SHADER_STRING_POINTERS ) {
				ri.Error( ERR_DROP, "MAX_SHADER_STRING_POINTERS exceeded, too many shaders" );
			}

			newStrPtr = &shaderStringPointerList[numShaderStringPointers++]; //ri.Hunk_Alloc( sizeof( shaderStringPointer_t ), h_low );
			newStrPtr->pStr = pOld;
			newStrPtr->next = shaderChecksumLookup[checksum].next;
			shaderChecksumLookup[checksum].next = newStrPtr;
		}

		// Gordon: skip the actual shader section
		String::SkipBracedSection( &p );
	}
}
// done.


/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define MAX_SHADER_FILES    4096
static void ScanAndLoadShaderFiles( void ) {
	char **shaderFiles;
	char*   buffers     [MAX_SHADER_FILES];
	int buffersize  [MAX_SHADER_FILES];
	char *p;
	int numShaders;
	int i;

	long sum = 0;
	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "scripts", ".shader", &numShaders );

	if ( !shaderFiles || !numShaders ) {
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaders > MAX_SHADER_FILES ) {
		numShaders = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaders; i++ )
	{
		char filename[MAX_QPATH];

		String::Sprintf( filename, sizeof( filename ), "scripts/%s", shaderFiles[i] );
		ri.Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename ); // JPW NERVE was PRINT_ALL
		buffersize[i] = ri.FS_ReadFile( filename, (void **)&buffers[i] );
		sum += buffersize[i];
		if ( !buffers[i] ) {
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		}
	}

	// build single large buffer
	s_shaderText = (char*)ri.Hunk_Alloc( sum + numShaders * 2, h_low );

	// Gordon: optimised to not use strcat/String::Length which can be VERY slow for the large strings we're using here
	p = s_shaderText;
	// free in reverse order, so the temp files are all dumped
	for ( i = numShaders - 1; i >= 0 ; i-- ) {
		String::Cpy( p++, "\n" );
		String::Cpy( p, buffers[i] );
		ri.FS_FreeFile( buffers[i] );
		buffers[i] = p;
		p += buffersize[i];
	}

	// ydnar: unixify all shaders
	String::FixPath( s_shaderText );

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	// Ridah, optimized shader loading (18ms on a P3-500 for sfm1.bsp)
	if ( r_cacheShaders->integer ) {
		BuildShaderChecksumLookup();
	}
	// done.
}

void CreateInternalShaders( void );

static void CreateExternalShaders( void ) {
	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
	tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, qtrue );
//	tr.sunShader = R_FindShader( "sun", LIGHTMAP_NONE, qtrue );	//----(SA)	let sky shader set this
	tr.sunflareShader = R_FindShader( "sunflare1", LIGHTMAP_NONE, qtrue );
}

//=============================================================================
// Ridah, shader caching
extern int numBackupShaders;
extern shader_t *backupShaders[MAX_SHADERS];
extern shader_t *backupHashTable[SHADER_HASH_SIZE];

/*
===============
R_CacheShaderAlloc
===============
*/
//int g_numshaderallocs = 0;
//void *R_CacheShaderAlloc( int size ) {
void* R_CacheShaderAllocExt( const char* name, int size, const char* file, int line ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		void* ptr = ri.Z_Malloc( size );

//		g_numshaderallocs++;

//		if( name ) {
//			Com_Printf( "Zone Malloc from %s: size %i: pointer %p: %i in use\n", name, size, ptr, g_numshaderallocs );
//		}

		//return malloc( size );
		return ptr;
	} else {
		return ri.Hunk_Alloc( size, h_low );
	}
}

/*
===============
R_CacheShaderFree
===============
*/
//void R_CacheShaderFree( void *ptr ) {
void R_CacheShaderFreeExt( const char* name, void *ptr, const char* file, int line ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
//		g_numshaderallocs--;

//		if( name ) {
//			Com_Printf( "Zone Free from %s: pointer %p: %i in use\n", name, ptr, g_numshaderallocs );
//		}

		//free( ptr );
		ri.Free( ptr );
	}
}

/*
===============
R_PurgeShaders
===============
*/

qboolean purgeallshaders = qfalse;
void R_PurgeShaders( int count ) {
	/*int i, j, c, b;
	shader_t **sh;
	static int lastPurged = 0;

	if (!numBackupShaders) {
		lastPurged = 0;
		return;
	}

	// find the first shader still in memory
	c = 0;
	sh = (shader_t **)&backupShaders;
	for (i = lastPurged; i < numBackupShaders; i++, sh++) {
		if (*sh) {
			// free all memory associated with this shader
			for ( j = 0 ; j < (*sh)->numUnfoggedPasses ; j++ ) {
				if ( !(*sh)->stages[j] ) {
					break;
				}
				for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
					if ((*sh)->stages[j]->bundle[b].texMods)
						R_CacheShaderFree( NULL, (*sh)->stages[j]->bundle[b].texMods );
				}
				R_CacheShaderFree( NULL, (*sh)->stages[j] );
			}
			R_CacheShaderFree( (*sh)->lightmapIndex ? va( "%s lm: %i", (*sh)->name, (*sh)->lightmapIndex) : NULL, *sh );
			*sh = NULL;

			if (++c >= count) {
				lastPurged = i;
				return;
			}
		}
	}
	lastPurged = 0;
	numBackupShaders = 0;*/

	if ( !numBackupShaders ) {
		return;
	}

	purgeallshaders = qtrue;

	R_PurgeLightmapShaders();

	purgeallshaders = qfalse;
}

qboolean R_ShaderCanBeCached( shader_t *sh ) {
	int i,j,b;

	if ( purgeallshaders ) {
		return qfalse;
	}

	if ( sh->isSky ) {
		return qfalse;
	}

	for ( i = 0; i < sh->numUnfoggedPasses; i++ ) {
		if ( sh->stages[i] && sh->stages[i]->active ) {
			for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
				// rain - swapped order of for() comparisons so that
				// image[16] (out of bounds) isn't dereferenced
				//for (j=0; sh->stages[i]->bundle[b].image[j] && j < MAX_IMAGE_ANIMATIONS; j++) {
				for ( j = 0; j < MAX_IMAGE_ANIMATIONS && sh->stages[i]->bundle[b].image[j]; j++ ) {
					if ( sh->stages[i]->bundle[b].image[j]->imgName[0] == '*' ) {
						return qfalse;
					}
				}
			}
		}
	}
	return qtrue;
}

void R_PurgeLightmapShaders( void ) {
	int j, b, i = 0;
	shader_t *sh, *shPrev, *next;

	for ( i = 0; i < sizeof( backupHashTable ) / sizeof( backupHashTable[0] ); i++ ) {
		sh = backupHashTable[i];

		shPrev = NULL;
		next = NULL;

		while ( sh ) {
			if ( sh->lightmapIndex >= 0 || !R_ShaderCanBeCached( sh ) ) {
				next = sh->next;

				if ( !shPrev ) {
					backupHashTable[i] = sh->next;
				} else {
					shPrev->next = sh->next;
				}

				backupShaders[sh->index] = NULL;    // make sure we don't try and free it

				numBackupShaders--;

				for ( j = 0 ; j < sh->numUnfoggedPasses ; j++ ) {
					if ( !sh->stages[j] ) {
						break;
					}
					for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
						if ( sh->stages[j]->bundle[b].texMods ) {
							R_CacheShaderFree( NULL, sh->stages[j]->bundle[b].texMods );
						}
					}
					R_CacheShaderFree( NULL, sh->stages[j] );
				}
				R_CacheShaderFree( sh->lightmapIndex < 0 ? va( "%s lm: %i", sh->name, sh->lightmapIndex ) : NULL, sh );

				sh = next;

				continue;
			}

			shPrev = sh;
			sh = sh->next;
		}
	}
}

/*
===============
R_BackupShaders
===============
*/
void R_BackupShaders( void ) {
//	int i;
//	long hash;

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

	// Gordon: ditch all lightmapped shaders
	R_PurgeLightmapShaders();

//	Com_Printf( "Backing up %i images\n", numBackupShaders );

//	for( i = 0; i < tr.numShaders; i++ ) {
//		if( backupShaders[ i ] ) {
//			Com_Printf( "Shader: %s: lm %i\n", backupShaders[ i ]->name, backupShaders[i]->lightmapIndex );
//		}
//	}

//	Com_Printf( "=======================================\n" );
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
	deferLoad = qfalse;

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();

	// Ridah
	R_LoadCacheShaders();
}
