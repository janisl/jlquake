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
//**
//**	This file deals with the parsing and definition of shaders.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"

// MACROS ------------------------------------------------------------------

#define MAX_SHADERTEXT_HASH	2048
#define MAX_SHADER_FILES	4096

// TYPES -------------------------------------------------------------------

struct infoParm_t
{
	const char	*name;
	int			clearSolid;
	int			surfaceFlags;
	int			contents;
};

struct collapse_t
{
	int		blendA;
	int		blendB;

	int		multitextureEnv;
	int		multitextureBlend;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

shader_t*		ShaderHashTable[SHADER_HASH_SIZE];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shader_t			shader;
static shaderStage_t	stages[MAX_SHADER_STAGES];
static texModInfo_t		texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

static char*			s_shaderText;
static const char**		shaderTextHashTable[MAX_SHADERTEXT_HASH];

// this table is also present in q3map

static infoParm_t infoParms[] =
{
	// server relevant contents
	{"water",		1,	0,	BSP46CONTENTS_WATER },
	{"slime",		1,	0,	BSP46CONTENTS_SLIME },		// mildly damaging
	{"lava",		1,	0,	BSP46CONTENTS_LAVA },		// very damaging
	{"playerclip",	1,	0,	BSP46CONTENTS_PLAYERCLIP },
	{"monsterclip",	1,	0,	BSP46CONTENTS_MONSTERCLIP },
	{"nodrop",		1,	0,	BSP46CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid",	1,	BSP46SURF_NONSOLID,	0},						// clears the solid flag

	// utility relevant attributes
	{"origin",		1,	0,	BSP46CONTENTS_ORIGIN },			// center of rotating brushes
	{"trans",		0,	0,	BSP46CONTENTS_TRANSLUCENT },	// don't eat contained surfaces
	{"detail",		0,	0,	BSP46CONTENTS_DETAIL },			// don't include in structural bsp
	{"structural",	0,	0,	BSP46CONTENTS_STRUCTURAL },		// force into structural bsp even if trnas
	{"areaportal",	1,	0,	BSP46CONTENTS_AREAPORTAL },		// divides areas
	{"clusterportal", 1,0,  BSP46CONTENTS_CLUSTERPORTAL },	// for bots
	{"donotenter",  1,  0,  BSP46CONTENTS_DONOTENTER },		// for bots

	{"fog",			1,	0,	BSP46CONTENTS_FOG},			// carves surfaces entering
	{"sky",			0,	BSP46SURF_SKY,		0 },		// emit light from an environment map
	{"lightfilter",	0,	BSP46SURF_LIGHTFILTER, 0 },		// filter light going through it
	{"alphashadow",	0,	BSP46SURF_ALPHASHADOW, 0 },		// test light on a per-pixel basis
	{"hint",		0,	BSP46SURF_HINT,		0 },		// use as a primary splitter

	// server attributes
	{"slick",		0,	BSP46SURF_SLICK,		0 },
	{"noimpact",	0,	BSP46SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nomarks",		0,	BSP46SURF_NOMARKS,	0 },		// don't make impact marks, but still explode
	{"ladder",		0,	BSP46SURF_LADDER,	0 },
	{"nodamage",	0,	BSP46SURF_NODAMAGE,	0 },
	{"metalsteps",	0,	BSP46SURF_METALSTEPS,0 },
	{"flesh",		0,	BSP46SURF_FLESH,		0 },
	{"nosteps",		0,	BSP46SURF_NOSTEPS,	0 },

	// drawsurf attributes
	{"nodraw",		0,	BSP46SURF_NODRAW,	0 },	// don't generate a drawsurface (or a lightmap)
	{"pointlight",	0,	BSP46SURF_POINTLIGHT, 0 },	// sample lighting at vertexes
	{"nolightmap",	0,	BSP46SURF_NOLIGHTMAP,0 },	// don't generate a lightmap
	{"nodlight",	0,	BSP46SURF_NODLIGHT, 0 },	// don't ever add dynamic lights
	{"dust",		0,	BSP46SURF_DUST, 0}			// leave a dust trail when walking on this surface
};

static collapse_t collapse[] =
{
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

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	ParseVector
//
//==========================================================================

static bool ParseVector(const char** text, int count, float* v)
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

static bool ParseStage(shaderStage_t* stage, const char** text)
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
	//JL was CGEN_IDENTITY which is equal to AGEN_ENTITY
	if (stage->alphaGen == AGEN_IDENTITY)
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

static void ParseDeform(const char** text)
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

//==========================================================================
//
//	ParseSkyParms
//
//	skyParms <outerbox> <cloudheight> <innerbox>
//
//==========================================================================

static void ParseSkyParms(const char** text)
{
	static const char* suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

	// outerbox
	char* token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if (QStr::Cmp(token, "-"))
	{
		for (int i = 0; i < 6; i++)
		{
			char pathname[MAX_QPATH];
			QStr::Sprintf(pathname, sizeof(pathname), "%s_%s.tga", token, suf[i]);
			shader.sky.outerbox[i] = R_FindImageFile(pathname, true, true, GL_CLAMP);
			if (!shader.sky.outerbox[i])
			{
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	shader.sky.cloudHeight = QStr::Atof(token);
	if (!shader.sky.cloudHeight)
	{
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords(shader.sky.cloudHeight);

	// innerbox
	token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name);
		return;
	}
	if (QStr::Cmp(token, "-"))
	{
		for (int i = 0; i < 6; i++)
		{
			char pathname[MAX_QPATH];
			QStr::Sprintf(pathname, sizeof(pathname), "%s_%s.tga", token, suf[i]);
			shader.sky.innerbox[i] = R_FindImageFile(pathname, true, true, GL_REPEAT);
			if (!shader.sky.innerbox[i])
			{
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = true;
}

//==========================================================================
//
//	ParseSort
//
//==========================================================================

static void ParseSort(const char** text)
{
	char* token = QStr::ParseExt(text, false);
	if (token[0] == 0)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: missing sort parameter in shader '%s'\n", shader.name);
		return;
	}

	if (!QStr::ICmp( token, "portal"))
	{
		shader.sort = SS_PORTAL;
	}
	else if (!QStr::ICmp(token, "sky"))
	{
		shader.sort = SS_ENVIRONMENT;
	}
	else if (!QStr::ICmp(token, "opaque"))
	{
		shader.sort = SS_OPAQUE;
	}
	else if (!QStr::ICmp(token, "decal"))
	{
		shader.sort = SS_DECAL;
	}
	else if (!QStr::ICmp(token, "seeThrough"))
	{
		shader.sort = SS_SEE_THROUGH;
	}
	else if (!QStr::ICmp(token, "banner"))
	{
		shader.sort = SS_BANNER;
	}
	else if (!QStr::ICmp(token, "additive"))
	{
		shader.sort = SS_BLEND1;
	}
	else if (!QStr::ICmp(token, "nearest"))
	{
		shader.sort = SS_NEAREST;
	}
	else if (!QStr::ICmp(token, "underwater"))
	{
		shader.sort = SS_UNDERWATER;
	}
	else
	{
		shader.sort = QStr::Atof(token);
	}
}

//==========================================================================
//
//	ParseSurfaceParm
//
//	surfaceparm <name>
//
//==========================================================================

static void ParseSurfaceParm(const char** text)
{
	int numInfoParms = sizeof(infoParms) / sizeof(infoParms[0]);

	char* token = QStr::ParseExt(text, false);
	for (int i = 0; i < numInfoParms; i++)
	{
		if (!QStr::ICmp(token, infoParms[i].name))
		{
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
			break;
		}
	}
}

//==========================================================================
//
//	ParseShader
//
//	The current text pointer is at the explicit text definition of the
// shader.  Parse it into the global shader variable.  Later functions
// will optimize it.
//
//==========================================================================

static bool ParseShader(const char** text)
{
	int s;

	s = 0;

	char* token = QStr::ParseExt(text, true);
	if (token[0] != '{')
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name);
		return false;
	}

	while (1)
	{
		token = QStr::ParseExt(text, true);
		if (!token[0])
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: no concluding '}' in shader %s\n", shader.name);
			return false;
		}

		// end of shader definition
		if (token[0] == '}')
		{
			break;
		}
		// stage definition
		else if (token[0] == '{')
		{
			if (!ParseStage(&stages[s], text))
			{
				return false;
			}
			stages[s].active = true;
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if (!QStr::NICmp(token, "qer", 3))
		{
			QStr::SkipRestOfLine(text);
			continue;
		}
		// sun parms
		else if (!QStr::ICmp(token, "q3map_sun"))
		{
			float	a, b;

			token = QStr::ParseExt(text, false);
			tr.sunLight[0] = QStr::Atof(token);
			token = QStr::ParseExt(text, false);
			tr.sunLight[1] = QStr::Atof(token);
			token = QStr::ParseExt(text, false);
			tr.sunLight[2] = QStr::Atof(token);
			
			VectorNormalize(tr.sunLight);

			token = QStr::ParseExt(text, false);
			a = QStr::Atof(token);
			VectorScale(tr.sunLight, a, tr.sunLight);

			token = QStr::ParseExt(text, false);
			a = QStr::Atof(token);
			a = a / 180 * M_PI;

			token = QStr::ParseExt(text, false);
			b = QStr::Atof(token);
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos(a) * cos(b);
			tr.sunDirection[1] = sin(a) * cos(b);
			tr.sunDirection[2] = sin(b);
		}
		else if (!QStr::ICmp(token, "deformVertexes"))
		{
			ParseDeform(text);
			continue;
		}
		else if (!QStr::ICmp(token, "tesssize"))
		{
			QStr::SkipRestOfLine(text);
			continue;
		}
		else if (!QStr::ICmp(token, "clampTime"))
		{
			token = QStr::ParseExt(text, false);
			if (token[0])
			{
				shader.clampTime = QStr::Atof(token);
			}
		}
		// skip stuff that only the q3map needs
		else if (!QStr::NICmp(token, "q3map", 5))
		{
			QStr::SkipRestOfLine(text);
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if (!QStr::ICmp(token, "surfaceParm"))
		{
			ParseSurfaceParm(text);
			continue;
		}
		// no mip maps
		else if (!QStr::ICmp(token, "nomipmaps"))
		{
			shader.noMipMaps = true;
			shader.noPicMip = true;
			continue;
		}
		// no picmip adjustment
		else if (!QStr::ICmp(token, "nopicmip"))
		{
			shader.noPicMip = true;
			continue;
		}
		// polygonOffset
		else if (!QStr::ICmp(token, "polygonOffset"))
		{
			shader.polygonOffset = true;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if (!QStr::ICmp(token, "entityMergable"))
		{
			shader.entityMergable = true;
			continue;
		}
		// fogParms
		else if (!QStr::ICmp(token, "fogParms")) 
		{
			if (!ParseVector(text, 3, shader.fogParms.color))
			{
				return false;
			}

			token = QStr::ParseExt(text, false);
			if (!token[0])
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name);
				continue;
			}
			shader.fogParms.depthForOpaque = QStr::Atof(token);

			// skip any old gradient directions
			QStr::SkipRestOfLine(text);
			continue;
		}
		// portal
		else if (!QStr::ICmp(token, "portal"))
		{
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if (!QStr::ICmp(token, "skyparms"))
		{
			ParseSkyParms(text);
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if (!QStr::ICmp(token, "light"))
		{
			token = QStr::ParseExt(text, false);
			continue;
		}
		// cull <face>
		else if (!QStr::ICmp(token, "cull"))
		{
			token = QStr::ParseExt(text, false);
			if (token[0] == 0)
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: missing cull parms in shader '%s'\n", shader.name);
				continue;
			}

			if (!QStr::ICmp(token, "none") || !QStr::ICmp(token, "twosided") || !QStr::ICmp(token, "disable"))
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if (!QStr::ICmp(token, "back") || !QStr::ICmp(token, "backside") || !QStr::ICmp(token, "backsided"))
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				GLog.Write(S_COLOR_YELLOW "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name);
			}
			continue;
		}
		// sort
		else if (!QStr::ICmp(token, "sort"))
		{
			ParseSort(text);
			continue;
		}
		else
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name);
			return false;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if (s == 0 && !shader.isSky && !(shader.contentFlags & BSP46CONTENTS_FOG))
	{
		return false;
	}

	shader.explicitlyDefined = true;

	return true;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

//==========================================================================
//
//	VertexLightingCollapse
//
//	If vertex lighting is enabled, only render a single pass, trying to guess
// which is the correct one to best aproximate what it is supposed to look like.
//
//==========================================================================

static void VertexLightingCollapse()
{
	// if we aren't opaque, just use the first pass
	if (shader.sort == SS_OPAQUE)
	{
		// pick the best texture for the single pass
		shaderStage_t* bestStage = &stages[0];
		int bestImageRank = -999999;

		for (int stage = 0; stage < MAX_SHADER_STAGES; stage++)
		{
			shaderStage_t* pStage = &stages[stage];

			if (!pStage->active)
			{
				break;
			}
			int rank = 0;

			if (pStage->bundle[0].isLightmap)
			{
				rank -= 100;
			}
			if (pStage->bundle[0].tcGen != TCGEN_TEXTURE)
			{
				rank -= 5;
			}
			if (pStage->bundle[0].numTexMods)
			{
				rank -= 5;
			}
			if (pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING)
			{
				rank -= 3;
			}

			if (rank > bestImageRank)
			{
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if (shader.lightmapIndex == LIGHTMAP_NONE)
		{
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		}
		else
		{
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;		
	}
	else
	{
		// don't use a lightmap (tesla coils)
		if (stages[0].bundle[0].isLightmap)
		{
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if (stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY)
		{
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH) &&
			(stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH))
		{
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ((stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH) &&
			(stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH))
		{
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for (int stage = 1; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t* pStage = &stages[stage];

		if (!pStage->active)
		{
			break;
		}

		Com_Memset(pStage, 0, sizeof(*pStage));
	}
}

//==========================================================================
//
//	CollapseMultitexture
//
//	Attempt to combine two stages into a single multitexture stage
//	FIXME: I think modulated add + modulated add collapses incorrectly
//
//==========================================================================

static bool CollapseMultitexture()
{
	if (!qglActiveTextureARB)
	{
		return false;
	}

	// make sure both stages are active
	if (!stages[0].active || !stages[1].active)
	{
		return false;
	}

	int abits = stages[0].stateBits;
	int bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ((abits & ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE)) !=
		(bbits & ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE)))
	{
		return false;
	}

	abits &= (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
	bbits &= (GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);

	// search for a valid multitexture blend function
	int i;
	for (i = 0; collapse[i].blendA != -1; i++)
	{
		if (abits == collapse[i].blendA && bbits == collapse[i].blendB)
		{
			break;
		}
	}

	// nothing found
	if (collapse[i].blendA == -1)
	{
		return false;
	}

	// GL_ADD is a separate extension
	if (collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable)
	{
		return false;
	}

	// make sure waveforms have identical parameters
	if ((stages[0].rgbGen != stages[1].rgbGen) ||
		(stages[0].alphaGen != stages[1].alphaGen))
	{
		return false;
	}

	// an add collapse can only have identity colors
	if (collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY)
	{
		return false;
	}

	if (stages[0].rgbGen == CGEN_WAVEFORM)
	{
		if (memcmp(&stages[0].rgbWave, &stages[1].rgbWave, sizeof(stages[0].rgbWave)))
		{
			return false;
		}
	}
	if (stages[0].alphaGen == AGEN_WAVEFORM)
	{
		if (memcmp(&stages[0].alphaWave, &stages[1].alphaWave, sizeof(stages[0].alphaWave)))
		{
			return false;
		}
	}

	// make sure that lightmaps are in bundle 1 for 3dfx
	if (stages[0].bundle[0].isLightmap)
	{
		stages[0].bundle[1] = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
	}
	else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits &= ~(GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS);
	stages[0].stateBits |= collapse[i].multitextureBlend;

	//
	// move down subsequent shaders
	//
	memmove(&stages[1], &stages[2], sizeof(stages[0]) * (MAX_SHADER_STAGES - 2));
	Com_Memset(&stages[MAX_SHADER_STAGES - 1], 0, sizeof(stages[0]));

	return true;
}

//==========================================================================
//
//	ComputeStageIteratorFunc
//
//	See if we can use on of the simple fastpath stage functions, otherwise
// set to the generic stage function
//
//==========================================================================

static void ComputeStageIteratorFunc()
{
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if (shader.isSky)
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}

	if (r_ignoreFastPath->integer)
	{
		return;
	}

	//
	// see if this can go into the vertex lit fast path
	//
	if (shader.numUnfoggedPasses == 1)
	{
		if (stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE)
		{
			if (stages[0].alphaGen == AGEN_IDENTITY)
			{
				if (stages[0].bundle[0].tcGen == TCGEN_TEXTURE)
				{
					if (!shader.polygonOffset)
					{
						if (!shader.multitextureEnv)
						{
							if (!shader.numDeforms)
							{
								shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								return;
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
	if (shader.numUnfoggedPasses == 1)
	{
		if ((stages[0].rgbGen == CGEN_IDENTITY) && (stages[0].alphaGen == AGEN_IDENTITY))
		{
			if (stages[0].bundle[0].tcGen == TCGEN_TEXTURE && 
				stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP)
			{
				if (!shader.polygonOffset)
				{
					if (!shader.numDeforms)
					{
						if (shader.multitextureEnv)
						{
							shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
							return;
						}
					}
				}
			}
		}
	}
}

//==========================================================================
//
//	FixRenderCommandList
//
//	https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
//	Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces
// are generated but before the frame is rendered. This will, for the duration
// of one frame, cause drawsurfaces to be rendered with bad shaders. To fix this,
// need to go through all render commands and fix sortedIndex.
//
//==========================================================================

static void FixRenderCommandList(int newShader)
{
	if (!backEndData[tr.smpFrame])
	{
		return;
	}

	const void* curCmd = backEndData[tr.smpFrame]->commands.cmds;

	while (1)
	{
		switch (*(const int*)curCmd)
		{
		case RC_SET_COLOR:
			{
			const setColorCommand_t* sc_cmd = (const setColorCommand_t*)curCmd;
			curCmd = (const void*)(sc_cmd + 1);
			break;
			}
		case RC_STRETCH_PIC:
			{
			const stretchPicCommand_t* sp_cmd = (const stretchPicCommand_t*)curCmd;
			curCmd = (const void*)(sp_cmd + 1);
			break;
			}
		case RC_DRAW_SURFS:
			{
			const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;
			drawSurf_t* drawSurf = ds_cmd->drawSurfs;
			for (int i = 0; i < ds_cmd->numDrawSurfs; i++, drawSurf++)
			{
				int entityNum;
				shader_t* shader;
				int fogNum;
				int dlightMap;
				R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap);
				int sortedIndex = ((drawSurf->sort >> QSORT_SHADERNUM_SHIFT) & (MAX_SHADERS - 1));
				if (sortedIndex >= newShader)
				{
					sortedIndex++;
					drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | entityNum | (fogNum << QSORT_FOGNUM_SHIFT) | (int)dlightMap;
				}
			}
			curCmd = (const void*)(ds_cmd + 1);
			break;
			}
		case RC_DRAW_BUFFER:
			{
			const drawBufferCommand_t* db_cmd = (const drawBufferCommand_t*)curCmd;
			curCmd = (const void*)(db_cmd + 1);
			break;
			}
		case RC_SWAP_BUFFERS:
			{
			const swapBuffersCommand_t* sb_cmd = (const swapBuffersCommand_t*)curCmd;
			curCmd = (const void *)(sb_cmd + 1);
			break;
			}
		case RC_END_OF_LIST:
		default:
			return;
		}
	}
}

//==========================================================================
//
//	SortNewShader
//
//	Positions the most recently created shader in the tr.sortedShaders[]
// array so that the shader->sort key is sorted reletive to the other
// shaders.
//
//	Sets shader->sortedIndex
//
//==========================================================================

static void SortNewShader()
{
	shader_t* newShader = tr.shaders[tr.numShaders - 1];
	float sort = newShader->sort;

	int i;
	for (i = tr.numShaders - 2; i >= 0; i--)
	{
		if (tr.sortedShaders[i]->sort <= sort)
		{
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList(i + 1);

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}

//==========================================================================
//
//	GenerateShaderHashValue
//
//==========================================================================

static int GenerateShaderHashValue(const char* fname, const int size)
{
	int hash = 0;
	int i = 0;
	while (fname[i] != '\0')
	{
		char letter = QStr::ToLower(fname[i]);
		if (letter == '.')
		{
			break;				// don't include extension
		}
		if (letter == '\\')
		{
			letter = '/';		// damn path names
		}
		if (letter == PATH_SEP)
		{
			letter = '/';		// damn path names
		}
		hash += (long)(letter) * (i + 119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size - 1);
	return hash;
}

//==========================================================================
//
//	GeneratePermanentShader
//
//==========================================================================

static shader_t* GeneratePermanentShader()
{
	if (tr.numShaders == MAX_SHADERS)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	shader_t* newShader = new shader_t;

	*newShader = shader;

	if (shader.sort <= SS_OPAQUE)
	{
		newShader->fogPass = FP_EQUAL;
	}
	else if (shader.contentFlags & BSP46CONTENTS_FOG)
	{
		newShader->fogPass = FP_LE;
	}

	tr.shaders[tr.numShaders] = newShader;
	newShader->index = tr.numShaders;
	
	tr.sortedShaders[tr.numShaders] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for (int i = 0; i < newShader->numUnfoggedPasses; i++)
	{
		if (!stages[i].active)
		{
			break;
		}
		newShader->stages[i] = new shaderStage_t;
		*newShader->stages[i] = stages[i];

		for (int b = 0; b < NUM_TEXTURE_BUNDLES; b++)
		{
			int size = newShader->stages[i]->bundle[b].numTexMods * sizeof(texModInfo_t);
			newShader->stages[i]->bundle[b].texMods = new texModInfo_t[newShader->stages[i]->bundle[b].numTexMods];
			Com_Memcpy(newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size);
		}
	}

	SortNewShader();

	int hash = GenerateShaderHashValue(newShader->name, SHADER_HASH_SIZE);
	newShader->next = ShaderHashTable[hash];
	ShaderHashTable[hash] = newShader;

	return newShader;
}

//==========================================================================
//
//	FinishShader
//
//	Returns a freshly allocated shader with all the needed info from the
// current global working shader
//
//==========================================================================

static shader_t* FinishShader()
{
	int stage;
	qboolean		hasLightmapStage;
	qboolean		vertexLightmap;

	hasLightmapStage = false;
	vertexLightmap = false;

	//
	// set sky stuff appropriate
	//
	if (shader.isSky)
	{
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if (shader.polygonOffset && !shader.sort)
	{
		shader.sort = SS_DECAL;
	}

	//
	// set appropriate stage information
	//
	for (stage = 0; stage < MAX_SHADER_STAGES; stage++)
	{
		shaderStage_t* pStage = &stages[stage];

		if (!pStage->active)
		{
			break;
		}

		// check for a missing texture
		if (!pStage->bundle[0].image[0])
		{
			GLog.Write(S_COLOR_YELLOW "Shader %s has a stage with no image\n", shader.name);
			pStage->active = false;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if (pStage->isDetail && !r_detailTextures->integer)
		{
			pStage->active = false;
			if (stage < (MAX_SHADER_STAGES - 1))
			{
				memmove(pStage, pStage + 1, sizeof(*pStage) * (MAX_SHADER_STAGES - stage - 1));
				Com_Memset(pStage + 1, 0, sizeof(*pStage));
			}
			continue;
		}

		//
		// default texture coordinate generation
		//
		if (pStage->bundle[0].isLightmap)
		{
			if (pStage->bundle[0].tcGen == TCGEN_BAD)
			{
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = true;
		}
		else
		{
			if (pStage->bundle[0].tcGen == TCGEN_BAD)
			{
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}

		//
		// determine sort order and fog color adjustment
		//
		if ((pStage->stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) &&
			(stages[0].stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)))
		{
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if (((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE)) ||
				((blendSrcBits == GLS_SRCBLEND_ZERO) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR)))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ((blendSrcBits == GLS_SRCBLEND_SRC_ALPHA) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ((blendSrcBits == GLS_SRCBLEND_ONE) && (blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA))
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			}
			else
			{
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if (!shader.sort)
			{
				// see through item, like a grill or grate
				if (pStage->stateBits & GLS_DEPTHMASK_TRUE)
				{
					shader.sort = SS_SEE_THROUGH;
				}
				else
				{
					shader.sort = SS_BLEND0;
				}
			}
		}
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if (!shader.sort)
	{
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if (stage > 1 && r_vertexLight->integer && !r_uiFullScreen->integer)
	{
		VertexLightingCollapse();
		stage = 1;
		hasLightmapStage = false;
	}

	//
	// look for multitexture potential
	//
	if (stage > 1 && CollapseMultitexture())
	{
		stage--;
	}

	if (shader.lightmapIndex >= 0 && !hasLightmapStage)
	{
		if (vertexLightmap)
		{
			GLog.DWrite(S_COLOR_RED "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name);
		}
		else
		{
			GLog.DWrite(S_COLOR_RED "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name);
  			shader.lightmapIndex = LIGHTMAP_NONE;
		}
	}

	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0)
	{
		shader.sort = SS_FOG;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	return GeneratePermanentShader();
}

//==========================================================================
//
//	FindShaderInShaderText
//
//	Scans the combined text description of all the shader files for the
// given shader name.
//
//	return NULL if not found
//
//	If found, it will return a valid shader
//
//==========================================================================

static const char* FindShaderInShaderText(const char* shadername)
{
	if (!s_shaderText)
	{
		return NULL;
	}

	int hash = GenerateShaderHashValue(shadername, MAX_SHADERTEXT_HASH);

	for (int i = 0; shaderTextHashTable[hash][i]; i++)
	{
		const char* p = shaderTextHashTable[hash][i];
		char* token = QStr::ParseExt(&p, true);
		if (!QStr::ICmp(token, shadername))
		{
			return p;
		}
	}

	const char* p = s_shaderText;

	// look for label
	while (1)
	{
		char* token = QStr::ParseExt(&p, true);
		if (token[0] == 0)
		{
			break;
		}

		if (!QStr::ICmp(token, shadername))
		{
			return p;
		}
		else
		{
			// skip the definition
			QStr::SkipBracedSection(&p);
		}
	}

	return NULL;
}

//==========================================================================
//
//	R_FindShader
//
//	Will always return a valid shader, but it might be the default shader if
// the real one can't be found.
//
//	In the interest of not requiring an explicit shader text entry to be
// defined for every single image used in the game, three default shader
// behaviors can be auto-created for any image:
//
//	If lightmapIndex == LIGHTMAP_NONE, then the image will have dynamic
// diffuse lighting applied to it, as apropriate for most entity skin
// surfaces.
//
//	If lightmapIndex == LIGHTMAP_2D, then the image will be used for 2D
// rendering unless an explicit shader is found
//
//	If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use the
// vertex rgba modulate values, as apropriate for misc_model pre-lit surfaces.
//
//	Other lightmapIndex values will have a lightmap stage created and src*dest
// blending applied with the texture, as apropriate for most world
// construction surfaces.
//
//==========================================================================

shader_t* R_FindShader(const char* name, int lightmapIndex, bool mipRawImage)
{
	if (name[0] == 0)
	{
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if (lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps)
	{
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}

	char strippedName[MAX_QPATH];
	QStr::StripExtension(name, strippedName);

	int hash = GenerateShaderHashValue(strippedName, SHADER_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (shader_t* sh = ShaderHashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ((sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
			!QStr::ICmp(sh->name, strippedName))
		{
			// match found
			return sh;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if (r_smp->integer)
	{
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));
	QStr::NCpyZ(shader.name, strippedName, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;
	for (int i = 0; i < MAX_SHADER_STAGES; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = true;
	shader.needsST1 = true;
	shader.needsST2 = true;
	shader.needsColor = true;

	//
	// attempt to define shader from an explicit parameter file
	//
	const char* shaderText = FindShaderInShaderText(strippedName);
	if (shaderText)
	{
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if (r_printShaders->integer)
		{
			GLog.Write("*SHADER* %s\n", name);
		}

		if (!ParseShader(&shaderText))
		{
			// had errors, so use default shader
			shader.defaultShader = true;
		}
		shader_t* sh = FinishShader();
		return sh;
	}

	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	char fileName[MAX_QPATH];
	QStr::NCpyZ(fileName, name, sizeof(fileName));
	QStr::DefaultExtension(fileName, sizeof(fileName), ".tga");
	image_t* image = R_FindImageFile(fileName, mipRawImage, mipRawImage, mipRawImage ? GL_REPEAT : GL_CLAMP);
	if (!image)
	{
		GLog.DWrite(S_COLOR_RED "Couldn't find image for shader %s\n", name);
		shader.defaultShader = true;
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if (shader.lightmapIndex == LIGHTMAP_NONE)
	{
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex == LIGHTMAP_BY_VERTEX)
	{
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex == LIGHTMAP_2D)
	{
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (shader.lightmapIndex == LIGHTMAP_WHITEIMAGE)
	{
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = true;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
	else
	{
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = true;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = true;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}

//==========================================================================
//
//	R_RegisterShaderFromImage
//
//==========================================================================

qhandle_t R_RegisterShaderFromImage(const char* name, int lightmapIndex, image_t* image, bool mipRawImage)
{
	int hash = GenerateShaderHashValue(name, SHADER_HASH_SIZE);
	
	//
	// see if the shader is already loaded
	//
	for (shader_t* sh = ShaderHashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ((sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
			// index by name
			!QStr::ICmp(sh->name, name))
		{
			// match found
			return sh->index;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if (r_smp->integer)
	{
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));
	QStr::NCpyZ(shader.name, name, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;
	for (int i = 0; i < MAX_SHADER_STAGES; i++)
	{
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = true;
	shader.needsST1 = true;
	shader.needsST2 = true;
	shader.needsColor = true;

	//
	// create the default shading commands
	//
	if (shader.lightmapIndex == LIGHTMAP_NONE)
	{
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex == LIGHTMAP_BY_VERTEX)
	{
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	}
	else if (shader.lightmapIndex == LIGHTMAP_2D)
	{
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if (shader.lightmapIndex == LIGHTMAP_WHITEIMAGE)
	{
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = true;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}
	else
	{
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = true;
		stages[0].active = true;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
											// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = true;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	shader_t* sh = FinishShader();
	return sh->index; 
}

//==========================================================================
//
//	R_RegisterShader
//
//	This is the exported shader entry point for the rest of the system
// It will always return an index that will be valid.
//
//	This should really only be used for explicit shaders, because there is no
// way to ask for different implicit lighting modes (vertex, lightmap, etc)
//
//==========================================================================

qhandle_t R_RegisterShader(const char* name)
{
	if (QStr::Length(name) >= MAX_QPATH)
	{
		GLog.Write("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	shader_t* sh = R_FindShader(name, LIGHTMAP_2D, true);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

//==========================================================================
//
//	R_RegisterShaderNoMip
//
//	For menu graphics that should never be picmiped
//
//==========================================================================

qhandle_t R_RegisterShaderNoMip(const char* name)
{
	if (QStr::Length(name) >= MAX_QPATH)
	{
		GLog.Write("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	shader_t* sh = R_FindShader(name, LIGHTMAP_2D, false);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return 0;
	}

	return sh->index;
}

//==========================================================================
//
//	R_RegisterShaderLightMap
//
//==========================================================================

static shader_t* R_RegisterShaderLightMap(const char* name, int lightmapIndex)
{
	if (QStr::Length(name) >= MAX_QPATH)
	{
		GLog.Write("Shader name exceeds MAX_QPATH\n");
		return 0;
	}

	shader_t* sh = R_FindShader(name, lightmapIndex, true);

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if (sh->defaultShader)
	{
		return tr.defaultShader;
	}

	return sh;
}

//==========================================================================
//
//	R_FindShaderByName
//
//	Will always return a valid shader, but it might be the default shader if
// the real one can't be found.
//
//==========================================================================

static shader_t* R_FindShaderByName(const char* name)
{
	if ((name == NULL) || (name[0] == 0))
	{
		// bk001205
		return tr.defaultShader;
	}

	char strippedName[MAX_QPATH];
	QStr::StripExtension(name, strippedName);

	int hash = GenerateShaderHashValue(strippedName, SHADER_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (shader_t* sh = ShaderHashTable[hash]; sh; sh = sh->next)
	{
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (QStr::ICmp(sh->name, strippedName) == 0)
		{
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}

//==========================================================================
//
//	R_RemapShader
//
//==========================================================================

void R_RemapShader(const char* shaderName, const char* newShaderName, const char* timeOffset)
{
	shader_t* sh = R_FindShaderByName(shaderName);
	if (sh == NULL || sh == tr.defaultShader)
	{
		sh = R_RegisterShaderLightMap(shaderName, 0);
	}
	if (sh == NULL || sh == tr.defaultShader)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: R_RemapShader: shader %s not found\n", shaderName);
		return;
	}

	shader_t* sh2 = R_FindShaderByName(newShaderName);
	if (sh2 == NULL || sh2 == tr.defaultShader)
	{
		sh2 = R_RegisterShaderLightMap(newShaderName, 0);
	}
	if (sh2 == NULL || sh2 == tr.defaultShader)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: R_RemapShader: new shader %s not found\n", newShaderName);
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	char strippedName[MAX_QPATH];
	QStr::StripExtension(shaderName, strippedName);
	int hash = GenerateShaderHashValue(strippedName, SHADER_HASH_SIZE);
	for (sh = ShaderHashTable[hash]; sh; sh = sh->next)
	{
		if (QStr::ICmp(sh->name, strippedName) == 0)
		{
			if (sh != sh2)
			{
				sh->remappedShader = sh2;
			}
			else
			{
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset)
	{
		sh2->timeOffset = QStr::Atof(timeOffset);
	}
}

//==========================================================================
//
//	CreateInternalShaders
//
//==========================================================================

static void CreateInternalShaders()
{
	tr.numShaders = 0;

	// init the default shader
	Com_Memset(&shader, 0, sizeof(shader));
	Com_Memset(&stages, 0, sizeof(stages));

	QStr::NCpyZ(shader.name, "<default>", sizeof(shader.name));

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = true;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	QStr::NCpyZ(shader.name, "<stencil shadow>", sizeof(shader.name));
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

//==========================================================================
//
//	ScanAndLoadShaderFiles
//
//	Finds and loads all .shader files, combining them into a single large
// text block that can be scanned for shader names
//
//==========================================================================

static void ScanAndLoadShaderFiles()
{
	long sum = 0;
	// scan for shader files
	int numShaders;
	char** shaderFiles;
	shaderFiles = FS_ListFiles("scripts", ".shader", &numShaders);

	if (!shaderFiles || !numShaders)
	{
		if (GGameType & GAME_Quake3)
		{
			GLog.Write(S_COLOR_YELLOW "WARNING: no shader files found\n");
		}
		return;
	}

	if (numShaders > MAX_SHADER_FILES)
	{
		numShaders = MAX_SHADER_FILES;
	}

	// load and parse shader files
	char* buffers[MAX_SHADER_FILES];
	for (int i = 0; i < numShaders; i++)
	{
		char filename[MAX_QPATH];
		QStr::Sprintf(filename, sizeof(filename), "scripts/%s", shaderFiles[i]);
		GLog.Write("...loading '%s'\n", filename);
		sum += FS_ReadFile(filename, (void**)&buffers[i]);
		if (!buffers[i])
		{
			throw QDropException(va("Couldn't load %s", filename));
		}
	}

	// build single large buffer
	s_shaderText = new char[sum + numShaders * 2];
	s_shaderText[0] = 0;

	// free in reverse order, so the temp files are all dumped
	for (int i = numShaders - 1; i >= 0 ; i--)
	{
		QStr::Cat(s_shaderText, sum + numShaders * 2, "\n");
		char* p = &s_shaderText[QStr::Length(s_shaderText)];
		QStr::Cat(s_shaderText, sum + numShaders * 2, buffers[i]);
		FS_FreeFile(buffers[i]);
		buffers[i] = p;
		QStr::Compress(p);
	}

	// free up memory
	FS_FreeFileList(shaderFiles);

	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH];
	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	int size = 0;
	//
	for (int i = 0; i < numShaders; i++)
	{
		// pointer to the first shader file
		const char* p = buffers[i];
		// look for label
		while (1)
		{
			char* token = QStr::ParseExt(&p, true);
			if (token[0] == 0)
			{
				break;
			}

			int hash = GenerateShaderHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTableSizes[hash]++;
			size++;
			QStr::SkipBracedSection(&p);
			// if we passed the pointer to the next shader file
			if (i < numShaders - 1)
			{
				if (p > buffers[i + 1])
				{
					break;
				}
			}
		}
	}

	size += MAX_SHADERTEXT_HASH;

	const char** hashMem = new const char*[size];
	Com_Memset(hashMem, 0, sizeof(char*) * size);
	for (int i = 0; i < MAX_SHADERTEXT_HASH; i++)
	{
		shaderTextHashTable[i] = hashMem;
		hashMem = hashMem + (shaderTextHashTableSizes[i] + 1);
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	//
	for (int i = 0; i < numShaders; i++)
	{
		// pointer to the first shader file
		const char* p = buffers[i];
		// look for label
		while (1)
		{
			const char* oldp = p;
			char* token = QStr::ParseExt(&p, true);
			if (token[0] == 0)
			{
				break;
			}

			int hash = GenerateShaderHashValue(token, MAX_SHADERTEXT_HASH);
			shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

			QStr::SkipBracedSection(&p);
			// if we passed the pointer to the next shader file
			if (i < numShaders - 1)
			{
				if (p > buffers[i + 1])
				{
					break;
				}
			}
		}
	}
}

//==========================================================================
//
//	CreateExternalShaders
//
//==========================================================================

static void CreateExternalShaders()
{
	if (GGameType & GAME_Quake3)
	{
		tr.projectionShadowShader = R_FindShader("projectionShadow", LIGHTMAP_NONE, true);
		tr.flareShader = R_FindShader("flareShader", LIGHTMAP_NONE, true);
		tr.sunShader = R_FindShader("sun", LIGHTMAP_NONE, true);
	}
	else
	{
		tr.projectionShadowShader = tr.defaultShader;
		tr.flareShader = tr.defaultShader;
		tr.sunShader = tr.defaultShader;
	}
}

//==========================================================================
//
//	R_InitShaders
//
//==========================================================================

void R_InitShaders()
{
	GLog.Write("Initializing Shaders\n");

	Com_Memset(ShaderHashTable, 0, sizeof(ShaderHashTable));

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();
}

//==========================================================================
//
//	R_GetShaderByHandle
//
//	When a handle is passed in by another module, this range checks it and
// returns a valid (possibly default) shader_t to be used internally.
//
//==========================================================================

shader_t* R_GetShaderByHandle(qhandle_t hShader)
{
	if (hShader < 0)
	{
		GLog.Write(S_COLOR_YELLOW "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	if (hShader >= tr.numShaders)
	{
		GLog.Write(S_COLOR_YELLOW "R_GetShaderByHandle: out of range hShader '%d'\n", hShader);
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

//==========================================================================
//
//	R_ShaderList_f
//
//	Dump information on all valid shaders to the console. A second parameter
// will cause it to print in sorted order
//
//==========================================================================

void R_ShaderList_f()
{
	shader_t	*shader;

	GLog.Write("-----------------------\n");

	int count = 0;
	for (int i = 0; i < tr.numShaders; i++)
	{
		if (Cmd_Argc() > 1)
		{
			shader = tr.sortedShaders[i];
		}
		else
		{
			shader = tr.shaders[i];
		}

		GLog.Write("%i ", shader->numUnfoggedPasses);

		if (shader->lightmapIndex >= 0)
		{
			GLog.Write("L ");
		}
		else
		{
			GLog.Write("  ");
		}
		if (shader->multitextureEnv == GL_ADD)
		{
			GLog.Write("MT(a) ");
		}
		else if (shader->multitextureEnv == GL_MODULATE)
		{
			GLog.Write("MT(m) ");
		}
		else if (shader->multitextureEnv == GL_DECAL)
		{
			GLog.Write("MT(d) ");
		}
		else
		{
			GLog.Write("      ");
		}
		if (shader->explicitlyDefined)
		{
			GLog.Write("E ");
		}
		else
		{
			GLog.Write("  ");
		}

		if (shader->optimalStageIteratorFunc == RB_StageIteratorGeneric)
		{
			GLog.Write("gen ");
		}
		else if (shader->optimalStageIteratorFunc == RB_StageIteratorSky)
		{
			GLog.Write("sky ");
		}
		else if (shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture)
		{
			GLog.Write("lmmt");
		}
		else if (shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture)
		{
			GLog.Write("vlt ");
		}
		else
		{
			GLog.Write("    ");
		}

		if (shader->defaultShader)
		{
			GLog.Write(": %s (DEFAULTED)\n", shader->name);
		}
		else
		{
			GLog.Write(": %s\n", shader->name);
		}
		count++;
	}
	GLog.Write("%i total shaders\n", count);
	GLog.Write("------------------\n");
}
