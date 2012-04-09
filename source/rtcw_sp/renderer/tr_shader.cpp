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

// tr_shader.c -- this file deals with the parsing and definition of shaders

extern char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
extern shaderStage_t stages[MAX_SHADER_STAGES];
extern shader_t shader;
extern texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static qboolean deferLoad;

#define SHADER_HASH_SIZE      4096

extern shader_t*       ShaderHashTable[SHADER_HASH_SIZE];

// Ridah
// Table containing string indexes for each shader found in the scripts, referenced by their checksum
// values.
typedef struct shaderStringPointer_s
{
	const char *pStr;
	struct shaderStringPointer_s *next;
} shaderStringPointer_t;
//
shaderStringPointer_t shaderChecksumLookup[SHADER_HASH_SIZE];
// done.

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;                          // don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                   // damn path names
		}
		if ( letter == PATH_SEP ) {
			letter = '/';                           // damn path names
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( SHADER_HASH_SIZE - 1 );
	return hash;
}

void R_RemapShader( const char *shaderName, const char *newShaderName, const char *timeOffset ) {
	char strippedName[MAX_QPATH];
	int hash;
	shader_t    *sh, *sh2;
	qhandle_t h;

	sh = R_FindShaderByName( shaderName );
	if ( sh == NULL || sh == tr.defaultShader ) {
		h = RE_RegisterShaderLightMap( shaderName, 0 );
		sh = R_GetShaderByHandle( h );
	}
	if ( sh == NULL || sh == tr.defaultShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if ( sh2 == NULL || sh2 == tr.defaultShader ) {
		h = RE_RegisterShaderLightMap( newShaderName, 0 );
		sh2 = R_GetShaderByHandle( h );
	}

	if ( sh2 == NULL || sh2 == tr.defaultShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	String::StripExtension( shaderName, strippedName );
	hash = generateHashValue( strippedName );
	for ( sh = ShaderHashTable[hash]; sh; sh = sh->next ) {
		if ( String::ICmp( sh->name, strippedName ) == 0 ) {
			if ( sh != sh2 ) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if ( timeOffset ) {
		sh2->timeOffset = String::Atof( timeOffset );
	}
}

bool ParseShader( const char **text );

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void ) {
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky ) {
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		goto done;
	}

	if ( r_ignoreFastPath->integer ) {
		return;
	}

	//
	// see if this can go into the vertex lit fast path
	//
	if ( shader.numUnfoggedPasses == 1 ) {
		if ( stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			if ( stages[0].alphaGen == AGEN_IDENTITY ) {
				if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE ) {
					if ( !shader.polygonOffset ) {
						if ( !shader.multitextureEnv ) {
							if ( !shader.numDeforms ) {
								shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								goto done;
							}
						}
					}
				}
			}
		}
	}

	//
	// see if this can go into an optimized LM, multitextured path
	//
	if ( shader.numUnfoggedPasses == 1 ) {
		if ( ( stages[0].rgbGen == CGEN_IDENTITY ) && ( stages[0].alphaGen == AGEN_IDENTITY ) ) {
			if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE &&
				 stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP ) {
				if ( !shader.polygonOffset ) {
					if ( !shader.numDeforms ) {
						if ( shader.multitextureEnv ) {
							shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
							goto done;
						}
					}
				}
			}
		}
	}

done:
	return;
}

struct collapse_t
{
	int		blendA;
	int		blendB;

	int		multitextureEnv;
	int		multitextureBlend;
};

extern collapse_t collapse[];

/*
================
CollapseMultitexture

Attempt to combine two stages into a single multitexture stage
FIXME: I think modulated add + modulated add collapses incorrectly
=================
*/
static qboolean CollapseMultitexture( void ) {
	int abits, bbits;
	int i;
	textureBundle_t tmpBundle;

	if ( !qglActiveTextureARB ) {
		return qfalse;
	}

	// make sure both stages are active
	if ( !stages[0].active || !stages[1].active ) {
		return qfalse;
	}

	abits = stages[0].stateBits;
	bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ( ( abits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) !=
		 ( bbits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) ) {
		return qfalse;
	}

	abits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	bbits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

	// search for a valid multitexture blend function
	for ( i = 0; collapse[i].blendA != -1 ; i++ ) {
		if ( abits == collapse[i].blendA
			 && bbits == collapse[i].blendB ) {
			break;
		}
	}

	// nothing found
	if ( collapse[i].blendA == -1 ) {
		return qfalse;
	}

	// GL_ADD is a separate extension
	if ( collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable ) {
		return qfalse;
	}

	// make sure waveforms have identical parameters
	if ( ( stages[0].rgbGen != stages[1].rgbGen ) ||
		 ( stages[0].alphaGen != stages[1].alphaGen ) ) {
		return qfalse;
	}

	// an add collapse can only have identity colors
	if ( collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY ) {
		return qfalse;
	}

	if ( stages[0].rgbGen == CGEN_WAVEFORM ) {
		if ( memcmp( &stages[0].rgbWave,
					 &stages[1].rgbWave,
					 sizeof( stages[0].rgbWave ) ) ) {
			return qfalse;
		}
	}
	if ( stages[0].alphaGen == CGEN_WAVEFORM ) {
		if ( memcmp( &stages[0].alphaWave,
					 &stages[1].alphaWave,
					 sizeof( stages[0].alphaWave ) ) ) {
			return qfalse;
		}
	}


	// make sure that lightmaps are in bundle 1 for 3dfx
	if ( stages[0].bundle[0].isLightmap ) {
		tmpBundle = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
		stages[0].bundle[1] = tmpBundle;
	} else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	stages[0].stateBits |= collapse[i].multitextureBlend;

	//
	// move down subsequent shaders
	//
	memmove( &stages[1], &stages[2], sizeof( stages[0] ) * ( MAX_SHADER_STAGES - 2 ) );
	memset( &stages[MAX_SHADER_STAGES - 1], 0, sizeof( stages[0] ) );

	return qtrue;
}


/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int i;
	float sort;
	shader_t    *newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t    *newShader;
	int i, b;
	int size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n" );
		return tr.defaultShader;
	}

	// Ridah, caching system
	newShader = (shader_t*)R_CacheShaderAlloc( sizeof( shader_t ) );

	*newShader = shader;

	if ( shader.sort <= SS_OPAQUE ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & BSP46CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			newShader->stages[i] = NULL;    // Ridah, make sure it's null
			break;
		}
		// Ridah, caching system
		newShader->stages[i] = (shaderStage_t*)R_CacheShaderAlloc( sizeof( stages[i] ) );

		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			if ( !newShader->stages[i]->bundle[b].numTexMods ) {
				// make sure unalloc'd texMods aren't pointing to some random point in memory
				newShader->stages[i]->bundle[b].texMods = NULL;
				continue;
			}
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			// Ridah, caching system
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t*)R_CacheShaderAlloc( size );

			memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue( newShader->name );
	newShader->next = ShaderHashTable[hash];
	ShaderHashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int stage;
	shaderStage_t   *bestStage;
	int bestImageRank;
	int rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			 && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			 && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		memset( pStage, 0, sizeof( *pStage ) );
	}
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage, i;
	qboolean hasLightmapStage;
	qboolean vertexLightmap;

	hasLightmapStage = qfalse;
	vertexLightmap = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		// check for a missing texture
		if ( !pStage->bundle[0].image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer ) {
			if ( stage < ( MAX_SHADER_STAGES - 1 ) ) {
				memmove( pStage, pStage + 1, sizeof( *pStage ) * ( MAX_SHADER_STAGES - stage - 1 ) );
				// kill the last stage, since it's now a duplicate
				for ( i = MAX_SHADER_STAGES - 1; i > stage; i-- ) {
					if ( stages[i].active ) {
						memset(  &stages[i], 0, sizeof( *pStage ) );
						break;
					}
				}
				stage--;    // the next stage is now the current stage, so check it again
			} else {
				memset( pStage, 0, sizeof( *pStage ) );
			}
			continue;
		}

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		} else {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}


		// not a true lightmap but we want to leave existing
		// behaviour in place and not print out a warning
		//if (pStage->rgbGen == CGEN_VERTEX) {
		//  vertexLightmap = qtrue;
		//}



		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if ( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
				 ( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			} else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
					shader.sort = SS_BLEND0;
				}
			}
		}
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( stage > 1 && r_vertexLight->integer && !r_uiFullScreen->integer ) {
		VertexLightingCollapse();
		stage = 1;
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if ( stage > 1 && CollapseMultitexture() ) {
		stage--;
	}

	if ( shader.lightmapIndex >= 0 && !hasLightmapStage ) {
		if ( vertexLightmap ) {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name );
		} else {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
			shader.lightmapIndex = LIGHTMAP_NONE;
		}
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if ( stage == 0 ) {
		shader.sort = SS_FOG;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// RF default back to no compression for next shader
	if ( r_ext_compressed_textures->integer == 2 ) {
		tr.allowCompress = qfalse;
	}

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static const char *FindShaderInShaderText( const char *shadername ) {
	const char *p = s_shaderText;
	char *token;

	if ( !p ) {
		return NULL;
	}

	// Ridah, optimized shader loading
	{
		shaderStringPointer_t *pShaderString;
		unsigned short int checksum;

		checksum = generateHashValue( shadername );

		// if it's known, skip straight to it's position
		pShaderString = &shaderChecksumLookup[checksum];
		while ( pShaderString && pShaderString->pStr ) {
			p = pShaderString->pStr;

			token = String::ParseExt( &p, qtrue );

			if ( ( token[0] != 0 ) && !String::ICmp( token, shadername ) ) {
				return p;
			}

			pShaderString = pShaderString->next;
		}
	}

	// done.

	/*
	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while ( 1 ) {
		token = String::ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( token[0] == '{' ) {
			// skip the definition
			String::SkipBracedSection( &p );
		} else if ( !String::ICmp( token, shadername ) ) {
			return p;
		} else {
			// skip to end of line
			String::SkipRestOfLine( &p );
		}
	}
	*/

	return NULL;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char strippedName[MAX_QPATH];
	int hash;
	shader_t    *sh;

	if ( ( name == NULL ) || ( name[0] == 0 ) ) {  // bk001205
		return tr.defaultShader;
	}

	String::StripExtension( name, strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
	for ( sh = ShaderHashTable[hash]; sh; sh = sh->next ) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( String::ICmp( sh->name, strippedName ) == 0 ) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}


/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
shader_t *R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage ) {
	char strippedName[MAX_QPATH];
	char fileName[MAX_QPATH];
	int i, hash;
	const char        *shaderText;
	image_t     *image;
	shader_t    *sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps ) {
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}

	String::StripExtension( name, strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
	for ( sh = ShaderHashTable[hash]; sh; sh = sh->next ) {
		// index by name

		// Ridah, modified this so we don't keep trying to load an invalid lightmap shader
/*
		if ( sh->lightmapIndex == lightmapIndex &&
			!String::ICmp(sh->name, strippedName)) {
			// match found
			return sh;
		}
*/
		if ( ( ( sh->lightmapIndex == lightmapIndex ) || ( sh->lightmapIndex < 0 && lightmapIndex >= 0 ) ) &&
			 !String::ICmp( sh->name, strippedName ) ) {
			// match found
			return sh;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// Ridah, check the cache
	sh = R_FindCachedShader( strippedName, lightmapIndex, hash );
	if ( sh ) {
		return sh;
	}
	// done.

	// clear the global shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	String::NCpyZ( shader.name, strippedName, sizeof( shader.name ) );
	shader.lightmapIndex = lightmapIndex;
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}

		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
		sh = FinishShader();
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	String::NCpyZ( fileName, name, sizeof( fileName ) );
	String::DefaultExtension( fileName, sizeof( fileName ), ".tga" );
	image = R_FindImageFile( fileName, mipRawImage, mipRawImage, mipRawImage ? GL_REPEAT : GL_CLAMP );
	if ( !image ) {
		ri.Printf( PRINT_DEVELOPER, "Couldn't find image for shader %s\n", name );
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
							  GLS_SRCBLEND_SRC_ALPHA |
							  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;   // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}


qhandle_t RE_RegisterShaderFromImage( const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage ) {
	int i, hash;
	shader_t    *sh;

	hash = generateHashValue( name );

	//
	// see if the shader is already loaded
	//
	for ( sh = ShaderHashTable[hash]; sh; sh = sh->next ) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( ( sh->lightmapIndex == lightmapIndex || sh->defaultShader ) &&
			 // index by name
			 !String::ICmp( sh->name, name ) ) {
			// match found
			return sh->index;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	String::NCpyZ( shader.name, name, sizeof( shader.name ) );
	shader.lightmapIndex = lightmapIndex;
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
							  GLS_SRCBLEND_SRC_ALPHA |
							  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;   // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}


/*
====================
RE_RegisterShaderLightMap

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, int lightmapIndex ) {
	shader_t    *sh;

	if ( String::Length( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndex, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t    *sh;

	if ( String::Length( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t    *sh;

	if ( String::Length( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
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
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader ); // bk: FIXME name
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
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
	const char *p = s_shaderText;
	const char *pOld;
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
		if ( token[0] == 0 ) {
			break;
		}

		if ( !String::ICmp( token, "{" ) ) {
			// skip braced section
			String::SkipBracedSection( &p );
			continue;
		}

		// get it's checksum
		checksum = generateHashValue( token );

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
	char *buffers[MAX_SHADER_FILES];
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
		ri.Printf( PRINT_ALL, "...loading '%s'\n", filename );
		sum += ri.FS_ReadFile( filename, (void **)&buffers[i] );
		if ( !buffers[i] ) {
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		}
	}

	// build single large buffer
	s_shaderText = (char*)ri.Hunk_Alloc( sum + numShaders * 2, h_low );

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaders - 1; i >= 0 ; i-- ) {
		strcat( s_shaderText, "\n" );
		p = &s_shaderText[String::Length( s_shaderText )];
		strcat( s_shaderText, buffers[i] );
		ri.FS_FreeFile( buffers[i] );
		buffers[i] = p;
//		String::Compress(p);
	}

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	// Ridah, optimized shader loading (18ms on a P3-500 for sfm1.bsp)
	BuildShaderChecksumLookup();
	// done.
}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );

	String::NCpyZ( shader.name, "<default>", sizeof( shader.name ) );

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	String::NCpyZ( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void CreateExternalShaders( void ) {
//	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
	tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, qtrue );
	tr.spotFlareShader = R_FindShader( "spotLight", LIGHTMAP_NONE, qtrue );
//	tr.sunShader = R_FindShader( "sun", LIGHTMAP_NONE, qtrue );	//----(SA)	let sky shader set this
	tr.sunflareShader = R_FindShader( "sunflare1", LIGHTMAP_NONE, qtrue );
}

//=============================================================================
// Ridah, shader caching
static int numBackupShaders = 0;
static shader_t *backupShaders[MAX_SHADERS];
static shader_t *backupHashTable[SHADER_HASH_SIZE];

/*
===============
R_CacheShaderAlloc
===============
*/
void *R_CacheShaderAlloc( int size ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		//return malloc( size );
		return malloc( size );
	} else {
		return ri.Hunk_Alloc( size, h_low );
	}
}

/*
===============
R_CacheShaderFree
===============
*/
void R_CacheShaderFree( void *ptr ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		//free( ptr );
		free( ptr );
	}
}

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
						R_CacheShaderFree( ( *sh )->stages[j]->bundle[b].texMods );
					}
				}
				R_CacheShaderFree( ( *sh )->stages[j] );
			}
			R_CacheShaderFree( *sh );
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
=================
R_RegisterShaderImages

  Make sure all images that belong to this shader remain valid
=================
*/
static qboolean R_RegisterShaderImages( shader_t *sh ) {
	int i,j,b;

	if ( sh->isSky ) {
		return qfalse;
	}

	for ( i = 0; i < sh->numUnfoggedPasses; i++ ) {
		if ( sh->stages[i] && sh->stages[i]->active ) {
			for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
				for ( j = 0; sh->stages[i]->bundle[b].image[j] && j < MAX_IMAGE_ANIMATIONS; j++ ) {
					if ( !R_TouchImage( sh->stages[i]->bundle[b].image[j] ) ) {
						return qfalse;
					}
				}
			}
		}
	}
	return qtrue;
}

/*
===============
R_FindCachedShader

  look for the given shader in the list of backupShaders
===============
*/
shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash ) {
	shader_t *sh, *shPrev;

	if ( !r_cacheShaders->integer ) {
		return NULL;
	}

	if ( !numBackupShaders ) {
		return NULL;
	}

	if ( !name ) {
		return NULL;
	}

	sh = backupHashTable[hash];
	shPrev = NULL;
	while ( sh ) {
		if ( sh->lightmapIndex == lightmapIndex && !String::ICmp( sh->name, name ) ) {

			// make sure the images stay valid
			if ( !R_RegisterShaderImages( sh ) ) {
				return NULL;
			}

			// this is the one, so move this shader into the current list

			if ( !shPrev ) {
				backupHashTable[hash] = sh->next;
			} else {
				shPrev->next = sh->next;
			}

			sh->next = ShaderHashTable[hash];
			ShaderHashTable[hash] = sh;

			backupShaders[sh->index] = NULL;    // make sure we don't try and free it

			// set the index up, and add it to the current list
			tr.shaders[ tr.numShaders ] = sh;
			sh->index = tr.numShaders;

			tr.sortedShaders[ tr.numShaders ] = sh;
			sh->sortedIndex = tr.numShaders;

			tr.numShaders++;

			SortNewShader();    // make sure it renders in the right order

			return sh;
		}

		shPrev = sh;
		sh = sh->next;
	}

	return NULL;
}

/*
===============
R_LoadCacheShaders
===============
*/
void R_LoadCacheShaders( void ) {
	int len;
	byte *buf;
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

	buf = (byte *)ri.Hunk_AllocateTempMemory( len );
	ri.FS_ReadFile( "shader.cache", (void **)&buf );
	pString = (char*)buf;   //DAJ added (char*)

	while ( ( token = String::ParseExt( &pString, qtrue ) ) != NULL && token[0] ) {
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
