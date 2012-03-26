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

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t shader;
static texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static qboolean deferLoad;

// ydnar: these are here because they are only referenced while parsing a shader
static char implicitMap[ MAX_QPATH ];
static unsigned implicitStateBits;
static cullType_t implicitCullType;

#define FILE_HASH_SIZE      4096
static shader_t*       hashTable[FILE_HASH_SIZE];

// Ridah
// Table containing string indexes for each shader found in the scripts, referenced by their checksum
// values.
typedef struct shaderStringPointer_s
{
	const char *pStr;
	struct shaderStringPointer_s *next;
} shaderStringPointer_t;
//
shaderStringPointer_t shaderChecksumLookup[FILE_HASH_SIZE];
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
	hash &= ( FILE_HASH_SIZE - 1 );
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
	COM_StripExtension2( shaderName, strippedName, sizeof( strippedName ) );
	hash = generateHashValue( strippedName );
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
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

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( const char **text, int count, float *v ) {
	char    *token;
	int i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = String::ParseExt( text, qfalse );
	if ( String::Cmp( token, "(" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = String::ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = String::Atof( token );
	}

	token = String::ParseExt( text, qfalse );
	if ( String::Cmp( token, ")" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname ) {
	if ( !String::ICmp( funcname, "GT0" ) ) {
		return GLS_ATEST_GT_0;
	} else if ( !String::ICmp( funcname, "LT128" ) )    {
		return GLS_ATEST_LT_80;
	} else if ( !String::ICmp( funcname, "GE128" ) )    {
		return GLS_ATEST_GE_80;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name ) {
	if ( !String::ICmp( name, "GL_ONE" ) ) {
		return GLS_SRCBLEND_ONE;
	} else if ( !String::ICmp( name, "GL_ZERO" ) )    {
		return GLS_SRCBLEND_ZERO;
	} else if ( !String::ICmp( name, "GL_DST_COLOR" ) )    {
		return GLS_SRCBLEND_DST_COLOR;
	} else if ( !String::ICmp( name, "GL_ONE_MINUS_DST_COLOR" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	} else if ( !String::ICmp( name, "GL_SRC_ALPHA" ) )    {
		return GLS_SRCBLEND_SRC_ALPHA;
	} else if ( !String::ICmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !String::ICmp( name, "GL_DST_ALPHA" ) )    {
		return GLS_SRCBLEND_DST_ALPHA;
	} else if ( !String::ICmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !String::ICmp( name, "GL_SRC_ALPHA_SATURATE" ) )    {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name ) {
	if ( !String::ICmp( name, "GL_ONE" ) ) {
		return GLS_DSTBLEND_ONE;
	} else if ( !String::ICmp( name, "GL_ZERO" ) )    {
		return GLS_DSTBLEND_ZERO;
	} else if ( !String::ICmp( name, "GL_SRC_ALPHA" ) )    {
		return GLS_DSTBLEND_SRC_ALPHA;
	} else if ( !String::ICmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !String::ICmp( name, "GL_DST_ALPHA" ) )    {
		return GLS_DSTBLEND_DST_ALPHA;
	} else if ( !String::ICmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !String::ICmp( name, "GL_SRC_COLOR" ) )    {
		return GLS_DSTBLEND_SRC_COLOR;
	} else if ( !String::ICmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname ) {
	if ( !String::ICmp( funcname, "sin" ) ) {
		return GF_SIN;
	} else if ( !String::ICmp( funcname, "square" ) )    {
		return GF_SQUARE;
	} else if ( !String::ICmp( funcname, "triangle" ) )    {
		return GF_TRIANGLE;
	} else if ( !String::ICmp( funcname, "sawtooth" ) )    {
		return GF_SAWTOOTH;
	} else if ( !String::ICmp( funcname, "inversesawtooth" ) )    {
		return GF_INVERSE_SAWTOOTH;
	} else if ( !String::ICmp( funcname, "noise" ) )    {
		return GF_NOISE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( const char **text, waveForm_t *wave ) {
	char *token;

	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = String::Atof( token );

	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = String::Atof( token );

	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = String::Atof( token );

	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = String::Atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( const char *_text, shaderStage_t *stage ) {
	const char *token;
	const char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri.Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'\n", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = String::ParseExt( text, qfalse );

	//
	// swap
	//
	if ( !String::ICmp( token, "swap" ) ) { // swap S/T coords (rotate 90d)
		tmi->type = TMOD_SWAP;
	}
	//
	// turb
	//
	// (SA) added 'else' so it wouldn't claim 'swap' was unknown.
	else if ( !String::ICmp( token, "turb" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = String::Atof( token );
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = String::Atof( token );
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = String::Atof( token );
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = String::Atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !String::ICmp( token, "scale" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = String::Atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !String::ICmp( token, "scroll" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = String::Atof( token );
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = String::Atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !String::ICmp( token, "stretch" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = String::Atof( token );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !String::ICmp( token, "transform" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = String::Atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !String::ICmp( token, "rotate" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = String::Atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !String::ICmp( token, "entityTranslate" ) ) {
		tmi->type = TMOD_ENTITY_TRANSLATE;
	} else
	{
		ri.Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
}


/*
===================
ParseStage
===================
*/
static qboolean ParseStage( shaderStage_t *stage, const char **text ) {
	char *token;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	while ( 1 )
	{
		token = String::ParseExt( text, qtrue );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' ) {
			break;
		}
		//
		// check special case for map16/map32/mapcomp/mapnocomp (compression enabled)
		if ( !String::ICmp( token, "map16" ) ) {    // only use this texture if 16 bit color depth
			if ( glConfig.colorBits <= 16 ) {
				token = "map";   // use this map
			} else {
				String::ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !String::ICmp( token, "map32" ) )    { // only use this texture if 16 bit color depth
			if ( glConfig.colorBits > 16 ) {
				token = "map";   // use this map
			} else {
				String::ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !String::ICmp( token, "mapcomp" ) )    { // only use this texture if compression is enabled
			if ( glConfig.textureCompression && r_ext_compressed_textures->integer ) {
				token = "map";   // use this map
			} else {
				String::ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !String::ICmp( token, "mapnocomp" ) )    { // only use this texture if compression is not available or disabled
			if ( !glConfig.textureCompression ) {
				token = "map";   // use this map
			} else {
				String::ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !String::ICmp( token, "animmapcomp" ) )    { // only use this texture if compression is enabled
			if ( glConfig.textureCompression && r_ext_compressed_textures->integer ) {
				token = "animmap";   // use this map
			} else {
				while ( token[0] )
					String::ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !String::ICmp( token, "animmapnocomp" ) )    { // only use this texture if compression is not available or disabled
			if ( !glConfig.textureCompression ) {
				token = "animmap";   // use this map
			} else {
				while ( token[0] )
					String::ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		}
		//
		// map <name>
		//
		if ( !String::ICmp( token, "map" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

//----(SA)	fixes startup error and allows polygon shadows to work again
			if ( !String::ICmp( token, "$whiteimage" ) || !String::ICmp( token, "*white" ) ) {
//----(SA)	end
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
//----(SA) added
			else if ( !String::ICmp( token, "$dlight" ) ) {
				stage->bundle[0].image[0] = tr.dlightImage;
				continue;
			}
//----(SA) end
			else if ( !String::ICmp( token, "$lightmap" ) ) {
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			} else
			{
				stage->bundle[0].image[0] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT, qfalse );
				if ( !stage->bundle[0].image[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !String::ICmp( token, "clampmap" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			stage->bundle[0].image[0] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, GL_CLAMP, qfalse );
			if ( !stage->bundle[0].image[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// lightmap <name>
		//
		else if ( !String::ICmp( token, "lightmap" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'lightmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

//----(SA)	fixes startup error and allows polygon shadows to work again
			if ( !String::ICmp( token, "$whiteimage" ) || !String::ICmp( token, "*white" ) ) {
//----(SA)	end
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
//----(SA) added
			else if ( !String::ICmp( token, "$dlight" ) ) {
				stage->bundle[0].image[0] = tr.dlightImage;
				continue;
			}
//----(SA) end
			else if ( !String::ICmp( token, "$lightmap" ) ) {
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			} else {
				stage->bundle[0].image[0] = R_FindImageFile( token, qfalse, qfalse, GL_CLAMP, qtrue );
				if ( !stage->bundle[0].image[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
				stage->bundle[0].isLightmap = qtrue;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !String::ICmp( token, "animMap" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = String::Atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int num;

				token = String::ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					stage->bundle[0].image[num] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT, qfalse );
					if ( !stage->bundle[0].image[num] ) {
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		} else if ( !String::ICmp( token, "videoMap" ) )    {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, ( CIN_loop | CIN_silent | CIN_shader ) );
			if ( stage->bundle[0].videoMapHandle != -1 ) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !String::ICmp( token, "alphaFunc" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
		}
		//
		// depthFunc <func>
		//
		else if ( !String::ICmp( token, "depthfunc" ) ) {
			token = String::ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !String::ICmp( token, "lequal" ) ) {
				depthFuncBits = 0;
			} else if ( !String::ICmp( token, "equal" ) )    {
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// detail
		//
		else if ( !String::ICmp( token, "detail" ) ) {
			stage->isDetail = qtrue;
		}
		//
		// fog
		//
		else if ( !String::ICmp( token, "fog" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for fog in shader '%s'\n", shader.name );
				continue;
			}
			if ( !String::ICmp( token, "on" ) ) {
				stage->isFogged = qtrue;
			} else {
				stage->isFogged = qfalse;
			}
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !String::ICmp( token, "blendfunc" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !String::ICmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !String::ICmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !String::ICmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = String::ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit ) {
				depthMaskBits = 0;
			}
		}
		//
		// rgbGen
		//
		else if ( !String::ICmp( token, "rgbGen" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !String::ICmp( token, "wave" ) ) {
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			} else if ( !String::ICmp( token, "const" ) )    {
				vec3_t color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			} else if ( !String::ICmp( token, "identity" ) )    {
				stage->rgbGen = CGEN_IDENTITY;
			} else if ( !String::ICmp( token, "identityLighting" ) )    {
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			} else if ( !String::ICmp( token, "entity" ) )    {
				stage->rgbGen = CGEN_ENTITY;
			} else if ( !String::ICmp( token, "oneMinusEntity" ) )    {
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			} else if ( !String::ICmp( token, "vertex" ) )    {
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			} else if ( !String::ICmp( token, "exactVertex" ) )    {
				stage->rgbGen = CGEN_EXACT_VERTEX;
			} else if ( !String::ICmp( token, "lightingDiffuse" ) )    {
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			} else if ( !String::ICmp( token, "oneMinusVertex" ) )    {
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// alphaGen
		//
		else if ( !String::ICmp( token, "alphaGen" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !String::ICmp( token, "wave" ) ) {
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			} else if ( !String::ICmp( token, "const" ) )    {
				token = String::ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * String::Atof( token );
				stage->alphaGen = AGEN_CONST;
			} else if ( !String::ICmp( token, "identity" ) )    {
				stage->alphaGen = AGEN_IDENTITY;
			} else if ( !String::ICmp( token, "entity" ) )    {
				stage->alphaGen = AGEN_ENTITY;
			} else if ( !String::ICmp( token, "oneMinusEntity" ) )    {
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			// Ridah
			else if ( !String::ICmp( token, "normalzfade" ) ) {
				stage->alphaGen = AGEN_NORMALZFADE;
				token = String::ParseExt( text, qfalse );
				if ( token[0] ) {
					stage->constantColor[3] = 255 * String::Atof( token );
				} else {
					stage->constantColor[3] = 255;
				}

				token = String::ParseExt( text, qfalse );
				if ( token[0] ) {
					stage->zFadeBounds[0] = String::Atof( token );    // lower range
					token = String::ParseExt( text, qfalse );
					stage->zFadeBounds[1] = String::Atof( token );    // upper range
				} else {
					stage->zFadeBounds[0] = -1.0;   // lower range
					stage->zFadeBounds[1] =  1.0;   // upper range
				}

			}
			// done.
			else if ( !String::ICmp( token, "vertex" ) ) {
				stage->alphaGen = AGEN_VERTEX;
			} else if ( !String::ICmp( token, "lightingSpecular" ) )    {
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			} else if ( !String::ICmp( token, "oneMinusVertex" ) )    {
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			} else if ( !String::ICmp( token, "portal" ) )    {
				stage->alphaGen = AGEN_PORTAL;
				token = String::ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					shader.portalRange = 256;
					ri.Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				} else
				{
					shader.portalRange = String::Atof( token );
				}
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !String::ICmp( token, "texgen" ) || !String::ICmp( token, "tcGen" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !String::ICmp( token, "environment" ) ) {
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			} else if ( !String::ICmp( token, "firerisenv" ) )    {
				stage->bundle[0].tcGen = TCGEN_FIRERISEENV_MAPPED;
			} else if ( !String::ICmp( token, "lightmap" ) )    {
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			} else if ( !String::ICmp( token, "texture" ) || !String::ICmp( token, "base" ) )     {
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			} else if ( !String::ICmp( token, "vector" ) )    {
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !String::ICmp( token, "tcMod" ) ) {
			char buffer[1024] = "";

			while ( 1 )
			{
				token = String::ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					break;
				}
				strcat( buffer, token );
				strcat( buffer, " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !String::ICmp( token, "depthwrite" ) ) {
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		} else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			 blendSrcBits == GLS_SRCBLEND_ONE ||
			 blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	// ydnar: if shader stage references a lightmap, but no lightmap is present
	// (vertex-approximated surfaces), then set cgen to vertex
	if ( stage->bundle[ 0 ].isLightmap && shader.lightmapIndex < 0 &&
		 stage->bundle[ 0 ].image[ 0 ] == tr.whiteImage ) {
		stage->rgbGen = CGEN_EXACT_VERTEX;
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) ) {
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == CGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			 || stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
					   blendSrcBits | blendDstBits |
					   atestBits |
					   depthFuncBits;

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( const char **text ) {
	char    *token;
	deformStage_t   *ds;

	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !String::ICmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if ( !String::ICmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !String::ICmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !String::NICmp( token, "text", 4 ) ) {
		int n;

		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if ( !String::ICmp( token, "bulge" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = String::Atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !String::ICmp( token, "wave" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( String::Atof( token ) != 0 ) {
			ds->deformationSpread = 1.0f / String::Atof( token );
		} else
		{
			ds->deformationSpread = 100.0f;
			ri.Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !String::ICmp( token, "normal" ) ) {
		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = String::Atof( token );

		token = String::ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = String::Atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !String::ICmp( token, "move" ) ) {
		int i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = String::Atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( const char **text ) {
	char        *token;
	static char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	char pathname[MAX_QPATH];
	int i;

	// outerbox
	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( String::Cmp( token, "-" ) ) {
		for ( i = 0 ; i < 6 ; i++ ) {
			String::Sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
						 , token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, GL_CLAMP, qfalse );
			if ( !shader.sky.outerbox[i] ) {
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = String::Atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );


	// innerbox
	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( String::Cmp( token, "-" ) ) {
		for ( i = 0 ; i < 6 ; i++ ) {
			String::Sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
						 , token, suf[i] );
			shader.sky.innerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, GL_REPEAT, qfalse );
			if ( !shader.sky.innerbox[i] ) {
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( const char **text ) {
	char    *token;

	token = String::ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !String::ICmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !String::ICmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !String::ICmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	} else if ( !String::ICmp( token, "decal" ) )   {
		shader.sort = SS_DECAL;
	} else if ( !String::ICmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !String::ICmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !String::ICmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !String::ICmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !String::ICmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else {
		shader.sort = String::Atof( token );
	}
}



// this table is also present in q3map

typedef struct {
	char    *name;
	int clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t infoParms[] = {
	// server relevant contents

//----(SA)	modified
	{"clipmissile",  1,  0, BSP47CONTENTS_MISSILECLIP},       // impact only specific weapons (rl, gl)
//----(SA)	end

	{"water",        1,  0,  BSP46CONTENTS_WATER },
	{"slag",     1,  0,  BSP46CONTENTS_SLIME },       // uses the BSP46CONTENTS_SLIME flag, but the shader reference is changed to 'slag'
	// to idendify that this doesn't work the same as 'slime' did.
	// (slime hurts instantly, slag doesn't)
//	{"slime",		1,	0,	BSP46CONTENTS_SLIME },		// mildly damaging
	{"lava",     1,  0,  BSP46CONTENTS_LAVA },        // very damaging
	{"playerclip",   1,  0,  BSP46CONTENTS_PLAYERCLIP },
	{"monsterclip",  1,  0,  BSP46CONTENTS_MONSTERCLIP },
	{"nodrop",       1,  0,  BSP46CONTENTS_NODROP },      // don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid", 1,  BSP46SURF_NONSOLID,  0},                     // clears the solid flag

	// utility relevant attributes
	{"origin",       1,  0,  BSP46CONTENTS_ORIGIN },      // center of rotating brushes
	{"trans",        0,  0,  BSP46CONTENTS_TRANSLUCENT }, // don't eat contained surfaces
	{"detail",       0,  0,  BSP46CONTENTS_DETAIL },      // don't include in structural bsp
	{"structural",   0,  0,  BSP46CONTENTS_STRUCTURAL },  // force into structural bsp even if trnas
	{"areaportal",   1,  0,  BSP46CONTENTS_AREAPORTAL },  // divides areas
	{"clusterportal", 1,0,  BSP46CONTENTS_CLUSTERPORTAL },    // for bots
	{"donotenter",  1,  0,  BSP46CONTENTS_DONOTENTER },       // for bots

	// Rafael - nopass
	{"donotenterlarge", 1, 0,    BSP47CONTENTS_DONOTENTER_LARGE }, // for larger bots

	{"fog",          1,  0,  BSP46CONTENTS_FOG},          // carves surfaces entering
	{"sky",          0,  BSP46SURF_SKY,       0 },        // emit light from an environment map
	{"lightfilter",  0,  BSP46SURF_LIGHTFILTER, 0 },      // filter light going through it
	{"alphashadow",  0,  BSP46SURF_ALPHASHADOW, 0 },      // test light on a per-pixel basis
	{"hint",     0,  BSP46SURF_HINT,      0 },        // use as a primary splitter

	// server attributes
	{"slick",            0,  BSP46SURF_SLICK,     0 },
	{"noimpact",     0,  BSP46SURF_NOIMPACT,  0 },        // don't make impact explosions or marks
	{"nomarks",          0,  BSP46SURF_NOMARKS,   0 },        // don't make impact marks, but still explode
	{"ladder",           0,  BSP46SURF_LADDER,    0 },
	{"nodamage",     0,  BSP46SURF_NODAMAGE,  0 },

	{"monsterslick", 0,  BSP47SURF_MONSTERSLICK,  0},     // surf only slick for monsters

//	{"flesh",		0,	BSP46SURF_FLESH,		0 },
	{"glass",        0,  BSP47SURF_GLASS,     0 },    //----(SA)	added
	{"splash",       0,  BSP47SURF_SPLASH,    0 },    //----(SA)	added

	// steps
	{"metal",        0,  BSP46SURF_METALSTEPS,     0 },
	{"metalsteps",   0,  BSP46SURF_METALSTEPS,     0 },    // retain bw compatibility with Q3A metal shaders... (SA)
	{"nosteps",      0,  BSP46SURF_NOSTEPS,   0 },
	{"woodsteps",    0,  BSP47SURF_WOOD,      0 },
	{"grasssteps",   0,  BSP47SURF_GRASS,     0 },
	{"gravelsteps",  0,  BSP47SURF_GRAVEL,    0 },
	{"carpetsteps",  0,  BSP47SURF_CARPET,    0 },
	{"snowsteps",    0,  BSP47SURF_SNOW,      0 },
	{"roofsteps",    0,  BSP47SURF_ROOF,      0 },    // tile roof

	{"rubble", 0, BSP47SURF_RUBBLE, 0 },

	// drawsurf attributes
	{"nodraw",       0,  BSP46SURF_NODRAW,    0 },    // don't generate a drawsurface (or a lightmap)
	{"pointlight",   0,  BSP46SURF_POINTLIGHT, 0 },   // sample lighting at vertexes
	{"nolightmap",   0,  BSP46SURF_NOLIGHTMAP,0 },        // don't generate a lightmap
	{"nodlight", 0,  BSP46SURF_NODLIGHT, 0 },     // don't ever add dynamic lights

	{"monsterslicknorth",    0, BSP47SURF_MONSLICK_N,0},
	{"monsterslickeast", 0, BSP47SURF_MONSLICK_E,0},
	{"monsterslicksouth",    0, BSP47SURF_MONSLICK_S,0},
	{"monsterslickwest", 0, BSP47SURF_MONSLICK_W,0}

};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( const char **text ) {
	char    *token;
	int numInfoParms = sizeof( infoParms ) / sizeof( infoParms[0] );
	int i;

	token = String::ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !String::ICmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if ( infoParms[i].clearSolid ) {
				si->contents &= ~BSP46CONTENTS_SOLID;
			}
#endif
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( const char **text ) {
	char *token;
	int s;


	s = 0;

	tr.allowCompress = qtrue;   // Arnout: allow compression by default

	token = String::ParseExt( text, qtrue );
	if ( token[0] != '{' ) {
		ri.Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = String::ParseExt( text, qtrue );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' ) {
			tr.allowCompress = qtrue;   // Arnout: allow compression by default
			break;
		}
		// stage definition
		else if ( token[0] == '{' ) {
			if ( !ParseStage( &stages[s], text ) ) {
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !String::NICmp( token, "qer", 3 ) ) {
			String::SkipRestOfLine( text );
			continue;
		}
		// sun parms
		else if ( !String::ICmp( token, "q3map_sun" ) ) {
			float a, b;

			token = String::ParseExt( text, qfalse );
			tr.sunLight[0] = String::Atof( token );
			token = String::ParseExt( text, qfalse );
			tr.sunLight[1] = String::Atof( token );
			token = String::ParseExt( text, qfalse );
			tr.sunLight[2] = String::Atof( token );

			VectorNormalize( tr.sunLight );

			token = String::ParseExt( text, qfalse );
			a = String::Atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight );

			token = String::ParseExt( text, qfalse );
			a = String::Atof( token );
			a = a / 180 * M_PI;

			token = String::ParseExt( text, qfalse );
			b = String::Atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );
		} else if ( !String::ICmp( token, "deformVertexes" ) )    {
			ParseDeform( text );
			continue;
		} else if ( !String::ICmp( token, "tesssize" ) )    {
			String::SkipRestOfLine( text );
			continue;
		} else if ( !String::ICmp( token, "clampTime" ) )    {
			token = String::ParseExt( text, qfalse );
			if ( token[0] ) {
				shader.clampTime = String::Atof( token );
			}
		}
		// skip stuff that only the q3map needs
		else if ( !String::NICmp( token, "q3map", 5 ) ) {
			String::SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !String::ICmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( ( !String::ICmp( token, "nomipmaps" ) ) || ( !String::ICmp( token,"nomipmap" ) ) ) {
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !String::ICmp( token, "nopicmip" ) ) {
			shader.noPicMip = qtrue;
			continue;
		}
		// polygonOffset
		else if ( !String::ICmp( token, "polygonOffset" ) ) {
			shader.polygonOffset = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !String::ICmp( token, "entityMergable" ) ) {
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !String::ICmp( token, "fogParms" ) ) {
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			shader.fogParms.colorInt = ColorBytes4( shader.fogParms.color[0] * tr.identityLight,
													shader.fogParms.color[1] * tr.identityLight,
													shader.fogParms.color[2] * tr.identityLight, 1.0 );

			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = String::Atof( token );
			shader.fogParms.depthForOpaque = shader.fogParms.depthForOpaque < 1 ? 1 : shader.fogParms.depthForOpaque;
			shader.fogParms.tcScale = 1.0f / shader.fogParms.depthForOpaque;

			// skip any old gradient directions
			String::SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !String::ICmp( token, "portal" ) ) {
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !String::ICmp( token, "skyparms" ) ) {
			ParseSkyParms( text );
			continue;
		}
		// This is fixed fog for the skybox/clouds determined solely by the shader
		// it will not change in a level and will not be necessary
		// to force clients to use a sky fog the server says to.
		// skyfogvars <(r,g,b)> <dist>
		else if ( !String::ICmp( token, "skyfogvars" ) ) {
			vec3_t fogColor;

			if ( !ParseVector( text, 3, fogColor ) ) {
				return qfalse;
			}
			token = String::ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density value for sky fog\n" );
				continue;
			}

			if ( String::Atof( token ) > 1 ) {
				ri.Printf( PRINT_WARNING, "WARNING: last value for skyfogvars is 'density' which needs to be 0.0-1.0\n" );
				continue;
			}

			R_SetFog( FOG_SKY, 0, 5, fogColor[0], fogColor[1], fogColor[2], String::Atof( token ) );
			continue;
		} else if ( !String::ICmp( token, "sunshader" ) )        {
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing shader name for 'sunshader'\n" );
				continue;
			}
			tr.sunShaderName = CopyString( token );
		}
//----(SA)	added
		else if ( !String::ICmp( token, "lightgridmulamb" ) ) { // ambient multiplier for lightgrid
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing value for 'lightgrid ambient multiplier'\n" );
				continue;
			}
			if ( String::Atof( token ) > 0 ) {
				tr.lightGridMulAmbient = String::Atof( token );
			}
		} else if ( !String::ICmp( token, "lightgridmuldir" ) )        { // directional multiplier for lightgrid
			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing value for 'lightgrid directional multiplier'\n" );
				continue;
			}
			if ( String::Atof( token ) > 0 ) {
				tr.lightGridMulDirected = String::Atof( token );
			}
		}
//----(SA)	end
		else if ( !String::ICmp( token, "waterfogvars" ) ) {
			vec3_t watercolor;
			float fogvar;

			if ( !ParseVector( text, 3, watercolor ) ) {
				return qfalse;
			}
			token = String::ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density/distance value for water fog\n" );
				continue;
			}

			fogvar = String::Atof( token );

			//----(SA)	right now allow one water color per map.  I'm sure this will need
			//			to change at some point, but I'm not sure how to track fog parameters
			//			on a "per-water volume" basis yet.

			if ( fogvar == 0 ) {       // '0' specifies "use the map values for everything except the fog color
				// TODO
			} else if ( fogvar > 1 )      { // distance "linear" fog
				R_SetFog( FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1 );
			} else {                      // density "exp" fog
				R_SetFog( FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar );
			}

			continue;
		}
		// fogvars
		else if ( !String::ICmp( token, "fogvars" ) ) {
			vec3_t fogColor;
			float fogDensity;
			int fogFar;

			if ( !ParseVector( text, 3, fogColor ) ) {
				return qfalse;
			}

			token = String::ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density value for the fog\n" );
				continue;
			}


			//----(SA)	NOTE:	fogFar > 1 means the shader is setting the farclip, < 1 means setting
			//					density (so old maps or maps that just need softening fog don't have to care about farclip)

			fogDensity = String::Atof( token );
			if ( fogDensity > 1 ) {  // linear
				fogFar      = fogDensity;
			} else {
				fogFar      = 5;
			}

			R_SetFog( FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity );
			R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0 );

			continue;
		}
		// done.
		// Ridah, allow disable fog for some shaders
		else if ( !String::ICmp( token, "nofog" ) ) {
			shader.noFog = qtrue;
			continue;
		}
		// done.
		// RF, allow each shader to permit compression if available
		else if ( !String::ICmp( token, "allowcompress" ) ) {
			tr.allowCompress = qtrue;
			continue;
		} else if ( !String::ICmp( token, "nocompress" ) )   {
			tr.allowCompress = -1;
			continue;
		}
		// done.
		// light <value> determines flaring in q3map, not needed here
		else if ( !String::ICmp( token, "light" ) ) {
			token = String::ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !String::ICmp( token, "cull" ) ) {
			token = String::ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !String::ICmp( token, "none" ) || !String::ICmp( token, "twosided" ) || !String::ICmp( token, "disable" ) ) {
				shader.cullType = CT_TWO_SIDED;
			} else if ( !String::ICmp( token, "back" ) || !String::ICmp( token, "backside" ) || !String::ICmp( token, "backsided" ) )      {
				shader.cullType = CT_BACK_SIDED;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// ydnar: distancecull <opaque distance> <transparent distance> <alpha threshold>
		else if ( !String::ICmp( token, "distancecull" ) ) {
			int i;


			for ( i = 0; i < 3; i++ )
			{
				token = String::ParseExt( text, qfalse );
				if ( token[ 0 ] == 0 ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing distancecull parms in shader '%s'\n", shader.name );
				} else {
					shader.distanceCull[ i ] = String::Atof( token );
				}
			}

			if ( shader.distanceCull[ 1 ] - shader.distanceCull[ 0 ] > 0 ) {
				// distanceCull[ 3 ] is an optimization
				shader.distanceCull[ 3 ] = 1.0 / ( shader.distanceCull[ 1 ] - shader.distanceCull[ 0 ] );
			} else
			{
				shader.distanceCull[ 0 ] = 0;
				shader.distanceCull[ 1 ] = 0;
				shader.distanceCull[ 2 ] = 0;
				shader.distanceCull[ 3 ] = 0;
			}
			continue;
		}
		// sort
		else if ( !String::ICmp( token, "sort" ) ) {
			ParseSort( text );
			continue;
		}
		// ydnar: implicit default mapping to eliminate redundant/incorrect explicit shader stages
		else if ( !String::NICmp( token, "implicit", 8 ) ) {
			// set implicit mapping state
			if ( !String::ICmp( token, "implicitBlend" ) ) {
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
				implicitCullType = CT_TWO_SIDED;
			} else if ( !String::ICmp( token, "implicitMask" ) )     {
				implicitStateBits = GLS_DEPTHMASK_TRUE | GLS_ATEST_GE_80;
				implicitCullType = CT_TWO_SIDED;
			} else    // "implicitMap"
			{
				implicitStateBits = GLS_DEPTHMASK_TRUE;
				implicitCullType = CT_FRONT_SIDED;
			}

			// get image
			token = String::ParseExt( text, qfalse );
			if ( token[ 0 ] != '\0' ) {
				String::NCpyZ( implicitMap, token, sizeof( implicitMap ) );
			} else
			{
				implicitMap[ 0 ] = '-';
				implicitMap[ 1 ] = '\0';
			}

			continue;
		}
		// unknown directive
		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	// ydnar: or have implicit mapping
	//
	if ( s == 0 && !shader.isSky && !( shader.contentFlags & BSP46CONTENTS_FOG ) && implicitMap[ 0 ] == '\0' ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	return qtrue;
}

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

	// ydnar: stages with tcMods can't be fast-pathed
	if ( stages[ 0 ].bundle[ 0 ].numTexMods != 0 ||
		 stages[ 0 ].bundle[ 1 ].numTexMods != 0 ) {
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

typedef struct {
	int blendA;
	int blendB;

	int multitextureEnv;
	int multitextureBlend;
} collapse_t;

static collapse_t collapse[] = {
	{ 0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, 0 },

	{ 0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },
#if 0
	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
	  GL_DECAL, 0 },
#endif
	{ -1 }
};

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
=============
FixRenderCommandList

Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList( int newShader ) {
	renderCommandList_t *cmdList = &backEndData[tr.smpFrame]->commands;

	if ( cmdList ) {
		const void *curCmd = cmdList->cmds;

		while ( 1 ) {
			switch ( *(const int *)curCmd ) {
			case RC_SET_COLOR:
			{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
				curCmd = (const void *)( sc_cmd + 1 );
				break;
			}
			case RC_STRETCH_PIC:
			case RC_ROTATED_PIC:
			case RC_STRETCH_PIC_GRADIENT:
			{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
				curCmd = (const void *)( sp_cmd + 1 );
				break;
			}
			case RC_2DPOLYS:
			{
				const poly2dCommand_t *sp_cmd = (const poly2dCommand_t *)curCmd;
				curCmd = (const void *)( sp_cmd + 1 );
				break;
			}
			break;
			case RC_DRAW_SURFS:
			{
				int i;
				drawSurf_t  *drawSurf;
				shader_t    *shader;
				int fogNum;
				int frontFace;
				int entityNum;
				int dlightMap;
				int sortedIndex;
				const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

				for ( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
					R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &frontFace, &dlightMap );
					sortedIndex = ( ( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & ( MAX_SHADERS - 1 ) );
					if ( sortedIndex >= newShader ) {
						sortedIndex++;
						drawSurf->sort = ( sortedIndex << QSORT_SHADERNUM_SHIFT )
										 | ( entityNum << QSORT_ENTITYNUM_SHIFT ) | ( fogNum << QSORT_FOGNUM_SHIFT ) | ( frontFace << QSORT_FRONTFACE_SHIFT ) | dlightMap;
					}
					curCmd = (const void *)( ds_cmd + 1 );
					break;
				}
			}
			case RC_DRAW_BUFFER:
			{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
				curCmd = (const void *)( db_cmd + 1 );
				break;
			}
			case RC_SWAP_BUFFERS:
			{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
				curCmd = (const void *)( sb_cmd + 1 );
				break;
			}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
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

	// Arnout: fix rendercommandlist
	FixRenderCommandList( i + 1 );

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
	newShader = (shader_t*)R_CacheShaderAlloc( shader.lightmapIndex < 0 ? va( "%s lm: %i", shader.name, shader.lightmapIndex ) : NULL, sizeof( shader_t ) );

	*newShader = shader;

	if ( shader.sort <= SS_SEE_THROUGH ) {  // ydnar: was SS_DECAL, this allows grates to be fogged
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
		newShader->stages[i] = (shaderStage_t*)R_CacheShaderAlloc( NULL, sizeof( stages[i] ) );

		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			if ( !newShader->stages[i]->bundle[b].numTexMods ) {
				// make sure unalloc'd texMods aren't pointing to some random point in memory
				newShader->stages[i]->bundle[b].texMods = NULL;
				continue;
			}
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			// Ridah, caching system
			newShader->stages[i]->bundle[b].texMods = (texModInfo_t*)R_CacheShaderAlloc( NULL, size );

			memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue( newShader->name );
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

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
SetImplicitShaderStages() - ydnar
sets a shader's stages to one of several defaults
*/

static void SetImplicitShaderStages( image_t *image ) {
	// set implicit cull type
	if ( implicitCullType && !shader.cullType ) {
		shader.cullType = implicitCullType;
	}

	// set shader stages
	switch ( shader.lightmapIndex )
	{
		// dynamic colors at vertexes
	case LIGHTMAP_NONE:
		stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
		stages[ 0 ].active = qtrue;
		stages[ 0 ].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[ 0 ].stateBits = implicitStateBits;
		break;

		// gui elements (note state bits are overridden)
	case LIGHTMAP_2D:
		stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
		stages[ 0 ].active = qtrue;
		stages[ 0 ].rgbGen = CGEN_VERTEX;
		stages[ 0 ].alphaGen = AGEN_SKIP;
		stages[ 0 ].stateBits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
		break;

		// fullbright is disabled per atvi request
	case LIGHTMAP_WHITEIMAGE:

		// explicit colors at vertexes
	case LIGHTMAP_BY_VERTEX:
		stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
		stages[ 0 ].active = qtrue;
		stages[ 0 ].rgbGen = CGEN_EXACT_VERTEX;
		stages[ 0 ].alphaGen = AGEN_SKIP;
		stages[ 0 ].stateBits = implicitStateBits;
		break;

		// use lightmap pass
	default:
		// masked or blended implicit shaders need texture first
		if ( implicitStateBits & ( GLS_ATEST_BITS | GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
			stages[ 0 ].bundle[ 0 ].image[ 0 ] = image;
			stages[ 0 ].active = qtrue;
			stages[ 0 ].rgbGen = CGEN_IDENTITY;
			stages[ 0 ].stateBits = implicitStateBits;

			stages[ 1 ].bundle[ 0 ].image[ 0 ] = tr.lightmaps[ shader.lightmapIndex ];
			stages[ 1 ].bundle[ 0 ].isLightmap = qtrue;
			stages[ 1 ].active = qtrue;
			stages[ 1 ].rgbGen = CGEN_IDENTITY;
			stages[ 1 ].stateBits = GLS_DEFAULT | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO | GLS_DEPTHFUNC_EQUAL;
		}
		// otherwise do standard lightmap + texture
		else
		{
			stages[ 0 ].bundle[ 0 ].image[ 0 ] = tr.lightmaps[ shader.lightmapIndex ];
			stages[ 0 ].bundle[ 0 ].isLightmap = qtrue;
			stages[ 0 ].active = qtrue;
			stages[ 0 ].rgbGen = CGEN_IDENTITY;
			stages[ 0 ].stateBits = GLS_DEFAULT;

			stages[ 1 ].bundle[ 0 ].image[ 0 ] = image;
			stages[ 1 ].active = qtrue;
			stages[ 1 ].rgbGen = CGEN_IDENTITY;
			stages[ 1 ].stateBits = GLS_DEFAULT | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		}
		break;
	}

	#if 0
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
		stages[0].rgbGen = CGEN_IDENTITY;       // lightmaps are scaled on creation for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
	#endif
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

	hasLightmapStage = qfalse;

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
			}
			// ydnar: zero + blended alpha, one + blended alpha
			else if ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA || blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
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
	// look for multitexture potential
	//
	if ( stage > 1 && CollapseMultitexture() ) {
		stage--;
	}

	if ( shader.lightmapIndex >= 0 && !hasLightmapStage ) {
		ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
		shader.lightmapIndex = LIGHTMAP_NONE;
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

//bani - dynamic shader list
typedef struct dynamicshader dynamicshader_t;
struct dynamicshader {
	char *shadertext;
	dynamicshader_t *next;
};
static dynamicshader_t *dshader = NULL;

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
#ifdef SH_LOADTIMING
	static int total = 0;

	int start = Sys_Milliseconds();
#endif // _DEBUG

	if ( !p ) {
		return NULL;
	}

	//bani - if we have any dynamic shaders loaded, check them first
	if ( dshader ) {
		dynamicshader_t *dptr;
		const char    *q;
		int i;

		dptr = dshader;
		i = 0;
		while ( dptr ) {
			if ( !dptr->shadertext || !String::Length( dptr->shadertext ) ) {
				ri.Printf( PRINT_WARNING, "WARNING: dynamic shader %s(%d) has no shadertext\n", shadername, i );
			} else {
				q = dptr->shadertext;

				token = String::ParseExt( &q, qtrue );

				if ( ( token[0] != 0 ) && !String::ICmp( token, shadername ) ) {
#ifdef SH_LOADTIMING
					total += Sys_Milliseconds() - start;
					Com_Printf( "Shader lookup: %i, total: %i\n", Sys_Milliseconds() - start, total );
#endif // _DEBUG
//					ri.Printf( PRINT_ALL, "Found dynamic shader [%s] with shadertext [%s]\n", shadername, dptr->shadertext );
					return q;
				}
			}
			i++;
			dptr = dptr->next;
		}
	}

	// Ridah, optimized shader loading
	if ( r_cacheShaders->integer ) {
		/*if (strstr( shadername, "/" ) && !strstr( shadername, "." ))*/ {
			unsigned short int checksum;
			shaderStringPointer_t *pShaderString;

			checksum = generateHashValue( shadername );

			// if it's known, skip straight to it's position
			pShaderString = &shaderChecksumLookup[checksum];
			while ( pShaderString && pShaderString->pStr ) {
				p = pShaderString->pStr;

				token = String::ParseExt( &p, qtrue );

				if ( ( token[0] != 0 ) && !String::ICmp( token, shadername ) ) {
#ifdef SH_LOADTIMING
					total += Sys_Milliseconds() - start;
					Com_Printf( "Shader lookup: %i, total: %i\n", Sys_Milliseconds() - start, total );
#endif // _DEBUG
					return p;
				}

				pShaderString = pShaderString->next;
			}

			// it's not even in our list, so it mustn't exist
			return NULL;
		}
	}
	// done.

	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while ( 1 ) {
		token = String::ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !String::ICmp( token, shadername ) ) {
#ifdef SH_LOADTIMING
			total += Sys_Milliseconds() - start;
			Com_Printf( "Shader lookup: %i, total: %i\n", Sys_Milliseconds() - start, total );
#endif // _DEBUG
			return p;
		}

		String::SkipBracedSection( &p );
	}

#ifdef SH_LOADTIMING
	total += Sys_Milliseconds() - start;
	Com_Printf( "Shader lookup: %i, total: %i\n", Sys_Milliseconds() - start, total );
#endif // _DEBUG
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

	COM_StripExtension2( name, strippedName, sizeof( strippedName ) );
	COM_FixPath( strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
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
R_FindLightmap - ydnar
given a (potentially erroneous) lightmap index, attempts to load
an external lightmap image and/or sets the index to a valid number
===============
*/

#define EXTERNAL_LIGHTMAP   "lm_%04d.tga"    // THIS MUST BE IN SYNC WITH Q3MAP2

void R_FindLightmap( int *lightmapIndex ) {
	image_t     *image;
	char fileName[ MAX_QPATH ];


	// don't fool with bogus lightmap indexes
	if ( *lightmapIndex < 0 ) {
		return;
	}

	// does this lightmap already exist?
	if ( *lightmapIndex < tr.numLightmaps && tr.lightmaps[ *lightmapIndex ] != NULL ) {
		return;
	}

	// bail if no world dir
	if ( tr.worldDir == NULL ) {
		*lightmapIndex = LIGHTMAP_BY_VERTEX;
		return;
	}

	// sync up render thread, because we're going to have to load an image
	R_SyncRenderThread();

	// attempt to load an external lightmap
	sprintf( fileName, "%s/" EXTERNAL_LIGHTMAP, tr.worldDir, *lightmapIndex );
	image = R_FindImageFile( fileName, qfalse, qfalse, GL_CLAMP, qtrue );
	if ( image == NULL ) {
		*lightmapIndex = LIGHTMAP_BY_VERTEX;
		return;
	}

	// add it to the lightmap list
	if ( *lightmapIndex >= tr.numLightmaps ) {
		tr.numLightmaps = *lightmapIndex + 1;
	}
	tr.lightmaps[ *lightmapIndex ] = image;
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

	// ydnar: validate lightmap index
	R_FindLightmap( &lightmapIndex );

	COM_StripExtension2( name, strippedName, sizeof( strippedName ) );
	COM_FixPath( strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
#if 1
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		// index by name

		// ydnar: the original way was correct
		if ( sh->lightmapIndex == lightmapIndex &&
			 !String::ICmp( sh->name, strippedName ) ) {
			// match found
			return sh;
		}

		// Ridah, modified this so we don't keep trying to load an invalid lightmap shader
		#if 0
		if ( ( ( sh->lightmapIndex == lightmapIndex ) || ( sh->lightmapIndex < 0 && lightmapIndex >= 0 ) ) &&
			 !String::ICmp( sh->name, strippedName ) ) {
			// match found
			return sh;
		}
		#endif
	}
#else
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( ( sh->lightmapIndex == lightmapIndex || sh->defaultShader ) &&
			 !String::ICmp( sh->name, strippedName ) ) {
			// match found
			return sh;
		}
	}
#endif

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// Ridah, check the cache
	// assignment used as truth value
	// ydnar: don't cache shaders using lightmaps
	if ( lightmapIndex < 0 ) {
		sh = R_FindCachedShader( strippedName, lightmapIndex, hash );
		if ( sh != NULL ) {
			return sh;
		}
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

	// ydnar: default to no implicit mappings
	implicitMap[ 0 ] = '\0';
	implicitStateBits = GLS_DEFAULT;
	implicitCullType = CT_FRONT_SIDED;

	// attempt to define shader from an explicit parameter file
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
			sh = FinishShader();
			return sh;
		}

		// ydnar: allow implicit mappings
		if ( implicitMap[ 0 ] == '\0' ) {
			sh = FinishShader();
			return sh;
		}
	}


	// ydnar: allow implicit mapping ('-' = use shader name)
	if ( implicitMap[ 0 ] == '\0' || implicitMap[ 0 ] == '-' ) {
		String::NCpyZ( fileName, name, sizeof( fileName ) );
	} else {
		String::NCpyZ( fileName, implicitMap, sizeof( fileName ) );
	}
	String::DefaultExtension( fileName, sizeof( fileName ), ".tga" );

	// ydnar: implicit shaders were breaking nopicmip/nomipmaps
	if ( !mipRawImage ) {
		shader.noMipMaps = qtrue;
		shader.noPicMip = qtrue;
	}

	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	image = R_FindImageFile( fileName, !shader.noMipMaps, !shader.noPicMip, mipRawImage ? GL_REPEAT : GL_CLAMP, qfalse );
	if ( !image ) {
		//ri.Printf( PRINT_DEVELOPER, "Couldn't find image for shader %s\n", name );
		ri.Printf( PRINT_WARNING, "WARNING: Couldn't find image for shader %s\n", name );
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	// ydnar: set default stages (removing redundant code)
	SetImplicitShaderStages( image );

	return FinishShader();
}


qhandle_t RE_RegisterShaderFromImage( const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage ) {
	int i, hash;
	shader_t    *sh;

	hash = generateHashValue( name );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
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

	// ydnar: set default stages (removing redundant code)
	SetImplicitShaderStages( image );

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
		checksum = generateHashValue( token );

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
	COM_FixPath( s_shaderText );

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	// Ridah, optimized shader loading (18ms on a P3-500 for sfm1.bsp)
	if ( r_cacheShaders->integer ) {
		BuildShaderChecksumLookup();
	}
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
	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
	tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, qtrue );
//	tr.sunShader = R_FindShader( "sun", LIGHTMAP_NONE, qtrue );	//----(SA)	let sky shader set this
	tr.sunflareShader = R_FindShader( "sunflare1", LIGHTMAP_NONE, qtrue );
}

//=============================================================================
// Ridah, shader caching
static int numBackupShaders = 0;
static shader_t *backupShaders[MAX_SHADERS];
static shader_t *backupHashTable[FILE_HASH_SIZE];

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
	// now backup the hashTable
	memcpy( backupHashTable, hashTable, sizeof( hashTable ) );

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
			if ( tr.numShaders == MAX_SHADERS ) {
				ri.Printf( PRINT_WARNING, "WARNING: R_FindCachedShader - MAX_SHADERS hit\n" );
				return NULL;
			}

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

			sh->next = hashTable[hash];
			hashTable[hash] = sh;

			backupShaders[sh->index] = NULL;    // make sure we don't try and free it

			// set the index up, and add it to the current list
			tr.shaders[ tr.numShaders ] = sh;
			sh->index = tr.numShaders;

			tr.sortedShaders[ tr.numShaders ] = sh;
			sh->sortedIndex = tr.numShaders;

			tr.numShaders++;

			numBackupShaders--;

			sh->remappedShader = NULL;  // Arnout: remove any remaps

			SortNewShader();    // make sure it renders in the right order

//			Com_Printf( "Removing %s from the cache: lm: %i\n", sh->name, sh->lightmapIndex );

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

	memset( hashTable, 0, sizeof( hashTable ) );
	deferLoad = qfalse;

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();

	// Ridah
	R_LoadCacheShaders();
}
