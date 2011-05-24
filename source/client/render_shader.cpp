//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

shader_t		shader;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	ParseVector
//
//==========================================================================

bool ParseVector(const char** text, int count, float* v)
{
	// FIXME: spaces are currently required after parens, should change parseext...
	char* token = QStr::ParseExt(text, false);
	if (QStr::Cmp(token, "("))
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return false;
	}

	for (int i = 0; i < count; i++)
	{
		token = QStr::ParseExt(text, false);
		if (!token[0])
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing vector element in shader '%s'\n", shader.name);
			return false;
		}
		v[i] = QStr::Atof(token);
	}

	token = QStr::ParseExt(text, false);
	if (QStr::Cmp(token, ")"))
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing parenthesis in shader '%s'\n", shader.name);
		return false;
	}

	return true;
}

//==========================================================================
//
//	NameToAFunc
//
//==========================================================================

static unsigned NameToAFunc(const char* funcname)
{	
	if (!QStr::ICmp(funcname, "GT0"))
	{
		return GLS_ATEST_GT_0;
	}
	else if (!QStr::ICmp(funcname, "LT128"))
	{
		return GLS_ATEST_LT_80;
	}
	else if (!QStr::ICmp(funcname, "GE128"))
	{
		return GLS_ATEST_GE_80;
	}

	GLog.Write(S_COLOR_YELLOW "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name);
	return 0;
}

//==========================================================================
//
//	NameToSrcBlendMode
//
//==========================================================================

static int NameToSrcBlendMode(const char* name)
{
	if (!QStr::ICmp(name, "GL_ONE"))
	{
		return GLS_SRCBLEND_ONE;
	}
	else if (!QStr::ICmp(name, "GL_ZERO"))
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if (!QStr::ICmp(name, "GL_DST_COLOR"))
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if (!QStr::ICmp(name, "GL_ONE_MINUS_DST_COLOR"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if (!QStr::ICmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_DST_ALPHA"))
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_SRC_ALPHA_SATURATE"))
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	GLog.Write(S_COLOR_YELLOW "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_SRCBLEND_ONE;
}

//==========================================================================
//
//	NameToDstBlendMode
//
//==========================================================================

static int NameToDstBlendMode(const char* name)
{
	if (!QStr::ICmp(name, "GL_ONE"))
	{
		return GLS_DSTBLEND_ONE;
	}
	else if (!QStr::ICmp(name, "GL_ZERO"))
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if (!QStr::ICmp(name, "GL_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_ONE_MINUS_SRC_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_DST_ALPHA"))
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_ONE_MINUS_DST_ALPHA"))
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if (!QStr::ICmp(name, "GL_SRC_COLOR"))
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if (!QStr::ICmp(name, "GL_ONE_MINUS_SRC_COLOR"))
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	GLog.Write(S_COLOR_YELLOW "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name);
	return GLS_DSTBLEND_ONE;
}

//==========================================================================
//
//	NameToGenFunc
//
//==========================================================================

static genFunc_t NameToGenFunc(const char* funcname)
{
	if (!QStr::ICmp(funcname, "sin"))
	{
		return GF_SIN;
	}
	else if (!QStr::ICmp(funcname, "square"))
	{
		return GF_SQUARE;
	}
	else if (!QStr::ICmp(funcname, "triangle"))
	{
		return GF_TRIANGLE;
	}
	else if (!QStr::ICmp(funcname, "sawtooth"))
	{
		return GF_SAWTOOTH;
	}
	else if (!QStr::ICmp(funcname, "inversesawtooth"))
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if (!QStr::ICmp(funcname, "noise"))
	{
		return GF_NOISE;
	}

	GLog.Write(S_COLOR_YELLOW "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name);
	return GF_SIN;
}

//==========================================================================
//
//	ParseWaveForm
//
//==========================================================================

static void ParseWaveForm(const char** text, waveForm_t* wave)
{
	char* token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->func = NameToGenFunc(token);

	// BASE, AMP, PHASE, FREQ
	token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->base = QStr::Atof(token);

	token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->amplitude = QStr::Atof(token);

	token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->phase = QStr::Atof(token);

	token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing waveform parm in shader '%s'\n", shader.name);
		return;
	}
	wave->frequency = QStr::Atof(token);
}

//==========================================================================
//
//	ParseTexMod
//
//==========================================================================

static void ParseTexMod(const char* _text, shaderStage_t* stage)
{
	const char** text = &_text;

	if (stage->bundle[0].numTexMods == TR_MAX_TEXMODS)
	{
		throw QDropException(va("ERROR: too many tcMod stages in shader '%s'\n", shader.name));
	}

	texModInfo_t* tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	const char* token = QStr::ParseExt(text, false);

	//
	// turb
	//
	if (!QStr::ICmp(token, "turb"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = QStr::Atof(token);
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = QStr::Atof(token);
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = QStr::Atof(token);
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing tcMod turb in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = QStr::Atof(token);

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if (!QStr::ICmp(token, "scale"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scale[0] = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing scale parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scale[1] = QStr::Atof(token);
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if (!QStr::ICmp(token, "scroll"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scroll[0] = QStr::Atof(token);
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing scale scroll parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->scroll[1] = QStr::Atof(token);
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if (!QStr::ICmp( token, "stretch"))
	{
		token = QStr::ParseExt( text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.func = NameToGenFunc(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.base = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.amplitude = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.phase = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing stretch parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->wave.frequency = QStr::Atof(token);
		
		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if (!QStr::ICmp(token, "transform"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][0] = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[0][1] = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][0] = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->matrix[1][1] = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[0] = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing transform parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->translate[1] = QStr::Atof(token);

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if (!QStr::ICmp(token, "rotate"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name);
			return;
		}
		tmi->rotateSpeed = QStr::Atof(token);
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if (!QStr::ICmp(token, "entityTranslate"))
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name);
	}
}

//==========================================================================
//
//	ParseStage
//
//==========================================================================

bool ParseStage(shaderStage_t* stage, const char** text)
{
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = false;

	stage->active = true;

	while (1)
	{
		char* token = QStr::ParseExt(text, true);
		if (!token[0])
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: no matching '}' found\n");
			return false;
		}

		if (token[0] == '}')
		{
			break;
		}
		//
		// map <name>
		//
		else if (!QStr::ICmp(token, "map"))
		{
			token = QStr::ParseExt(text, false);
			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name);
				return false;
			}

			if (!QStr::ICmp( token, "$whiteimage"))
			{
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
			else if (!QStr::ICmp(token, "$lightmap"))
			{
				stage->bundle[0].isLightmap = true;
				if (shader.lightmapIndex < 0)
				{
					stage->bundle[0].image[0] = tr.whiteImage;
				}
				else
				{
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				stage->bundle[0].image[0] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
				if (!stage->bundle[0].image[0])
				{
					GLog.Write(S_COLOR_YELLOW "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
					return false;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if (!QStr::ICmp(token, "clampmap"))
		{
			token = QStr::ParseExt(text, false);
			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name);
				return false;
			}

			stage->bundle[0].image[0] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_CLAMP);
			if (!stage->bundle[0].image[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
				return false;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if (!QStr::ICmp(token, "animMap"))
		{
			token = QStr::ParseExt(text, false);
			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name);
				return false;
			}
			stage->bundle[0].imageAnimationSpeed = QStr::Atof(token);

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while (1)
			{
				token = QStr::ParseExt(text, false);
				if (!token[0])
				{
					break;
				}
				int num = stage->bundle[0].numImageAnimations;
				if (num < MAX_IMAGE_ANIMATIONS)
				{
					stage->bundle[0].image[num] = R_FindImageFile(token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT);
					if (!stage->bundle[0].image[num])
					{
						GLog.Write(S_COLOR_YELLOW "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name);
						return false;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		}
		else if (!QStr::ICmp(token, "videoMap"))
		{
			token = QStr::ParseExt(text, false);
			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n", shader.name);
				return false;
			}
			stage->bundle[0].videoMapHandle = CIN_PlayCinematic(token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1)
			{
				stage->bundle[0].isVideoMap = true;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if (!QStr::ICmp(token, "alphaFunc"))
		{
			token = QStr::ParseExt( text, false);
			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name);
				return false;
			}

			atestBits = NameToAFunc(token);
		}
		//
		// depthFunc <func>
		//
		else if (!QStr::ICmp(token, "depthfunc"))
		{
			token = QStr::ParseExt(text, false);

			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name);
				return false;
			}

			if (!QStr::ICmp(token, "lequal"))
			{
				depthFuncBits = 0;
			}
			else if (!QStr::ICmp(token, "equal"))
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		//
		// detail
		//
		else if (!QStr::ICmp(token, "detail"))
		{
			stage->isDetail = true;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if (!QStr::ICmp(token, "blendfunc"))
		{
			token = QStr::ParseExt(text, false);
			if (token[0] == 0)
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
				continue;
			}
			// check for "simple" blends first
			if (!QStr::ICmp( token, "add"))
			{
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			}
			else if (!QStr::ICmp( token, "filter"))
			{
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			}
			else if (!QStr::ICmp( token, "blend"))
			{
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			}
			else
			{
				// complex double blends
				blendSrcBits = NameToSrcBlendMode(token);

				token = QStr::ParseExt(text, false);
				if (token[0] == 0)
				{
					GLog.Write(S_COLOR_YELLOW "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name);
					continue;
				}
				blendDstBits = NameToDstBlendMode(token);
			}

			// clear depth mask for blended surfaces
			if (!depthMaskExplicit)
			{
				depthMaskBits = 0;
			}
		}
		//
		// rgbGen
		//
		else if (!QStr::ICmp(token, "rgbGen"))
		{
			token = QStr::ParseExt(text, false);
			if (token[0] == 0)
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!QStr::ICmp( token, "wave"))
			{
				ParseWaveForm(text, &stage->rgbWave);
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if (!QStr::ICmp(token, "const"))
			{
				vec3_t	color;

				ParseVector(text, 3, color);
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if (!QStr::ICmp(token, "identity"))
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if (!QStr::ICmp(token, "identityLighting"))
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if (!QStr::ICmp(token, "entity"))
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if (!QStr::ICmp(token, "oneMinusEntity"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if (!QStr::ICmp(token, "vertex"))
			{
				stage->rgbGen = CGEN_VERTEX;
				if (stage->alphaGen == 0)
				{
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if (!QStr::ICmp(token, "exactVertex"))
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if (!QStr::ICmp(token, "lightingDiffuse"))
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if (!QStr::ICmp(token, "oneMinusVertex"))
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		//
		// alphaGen 
		//
		else if (!QStr::ICmp(token, "alphaGen"))
		{
			token = QStr::ParseExt(text, false);
			if (token[0] == 0)
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name);
				continue;
			}

			if (!QStr::ICmp(token, "wave"))
			{
				ParseWaveForm(text, &stage->alphaWave);
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if (!QStr::ICmp(token, "const"))
			{
				token = QStr::ParseExt(text, false);
				stage->constantColor[3] = 255 * QStr::Atof(token);
				stage->alphaGen = AGEN_CONST;
			}
			else if (!QStr::ICmp(token, "identity"))
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if (!QStr::ICmp(token, "entity"))
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if (!QStr::ICmp(token, "oneMinusEntity"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if (!QStr::ICmp(token, "vertex"))
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if (!QStr::ICmp(token, "lightingSpecular"))
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if (!QStr::ICmp(token, "oneMinusVertex"))
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if (!QStr::ICmp(token, "portal"))
			{
				stage->alphaGen = AGEN_PORTAL;
				token = QStr::ParseExt(text, false);
				if (token[0] == 0)
				{
					shader.portalRange = 256;
					GLog.Write(S_COLOR_YELLOW "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name);
				}
				else
				{
					shader.portalRange = QStr::Atof(token);
				}
			}
			else
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name);
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if (!QStr::ICmp(token, "texgen") || !QStr::ICmp(token, "tcGen")) 
		{
			token = QStr::ParseExt(text, false);
			if (token[0] == 0)
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing texgen parm in shader '%s'\n", shader.name);
				continue;
			}

			if (!QStr::ICmp( token, "environment"))
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if (!QStr::ICmp( token, "lightmap"))
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if (!QStr::ICmp(token, "texture") || !QStr::ICmp(token, "base"))
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if (!QStr::ICmp( token, "vector"))
			{
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[0]);
				ParseVector(text, 3, stage->bundle[0].tcGenVectors[1]);

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else 
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: unknown texgen parm in shader '%s'\n", shader.name);
			}
		}
		//
		// tcMod <type> <...>
		//
		else if (!QStr::ICmp(token, "tcMod"))
		{
			char buffer[1024] = "";

			while (1)
			{
				token = QStr::ParseExt(text, false);
				if (token[0] == 0)
				{
					break;
				}
				QStr::Cat(buffer, sizeof(buffer), token);
				QStr::Cat(buffer, sizeof(buffer), " ");
			}

			ParseTexMod(buffer, stage);

			continue;
		}
		//
		// depthmask
		//
		else if (!QStr::ICmp(token, "depthwrite"))
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = true;

			continue;
		}
		else
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name);
			return false;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if (stage->rgbGen == CGEN_BAD)
	{
		if (blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE || 
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA)
		{
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		else
		{
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if (blendSrcBits == GLS_SRCBLEND_ONE &&  blendDstBits == GLS_DSTBLEND_ZERO)
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if (stage->alphaGen == CGEN_IDENTITY)
	{
		if (stage->rgbGen == CGEN_IDENTITY || stage->rgbGen == CGEN_LIGHTING_DIFFUSE)
		{
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

	return true;
}

//==========================================================================
//
//	ParseDeform
//
//	deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
//	deformVertexes normal <frequency> <amplitude>
//	deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
//	deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
//	deformVertexes projectionShadow
//	deformVertexes autoSprite
//	deformVertexes autoSprite2
//	deformVertexes text[0-7]
//
//==========================================================================

void ParseDeform(const char** text)
{
	char* token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing deform parm in shader '%s'\n", shader.name);
		return;
	}

	if (shader.numDeforms == MAX_SHADER_DEFORMS)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name);
		return;
	}

	deformStage_t* ds = &shader.deforms[shader.numDeforms];
	shader.numDeforms++;

	if (!QStr::ICmp(token, "projectionShadow"))
	{
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if (!QStr::ICmp(token, "autosprite"))
	{
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if (!QStr::ICmp(token, "autosprite2"))
	{
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if (!QStr::NICmp(token, "text", 4))
	{
		int n = token[4] - '0';
		if (n < 0 || n > 7)
		{
			n = 0;
		}
		ds->deformation = (deform_t)(DEFORM_TEXT0 + n);
		return;
	}

	if (!QStr::ICmp(token, "bulge"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeWidth = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeHeight = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name);
			return;
		}
		ds->bulgeSpeed = QStr::Atof(token);

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if (!QStr::ICmp(token, "wave"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}

		if (QStr::Atof( token ) != 0)
		{
			ds->deformationSpread = 1.0f / QStr::Atof(token);
		}
		else
		{
			ds->deformationSpread = 100.0f;
			GLog.Write(S_COLOR_YELLOW "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if (!QStr::ICmp(token, "normal"))
	{
		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.amplitude = QStr::Atof(token);

		token = QStr::ParseExt(text, false);
		if (token[0] == 0)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
			return;
		}
		ds->deformationWave.frequency = QStr::Atof(token);

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if (!QStr::ICmp(token, "move"))
	{
		for (int i = 0; i < 3; i++)
		{
			token = QStr::ParseExt(text, false);
			if (token[0] == 0)
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name);
				return;
			}
			ds->moveVector[i] = QStr::Atof(token);
		}

		ParseWaveForm(text, &ds->deformationWave);
		ds->deformation = DEFORM_MOVE;
		return;
	}

	GLog.Write(S_COLOR_YELLOW "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name);
}
