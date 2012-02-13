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

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

glstate_t	glState;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	GL_Bind
//
//==========================================================================

void GL_Bind(image_t* image)
{
	int texnum;
	if (!image)
	{
		Log::write(S_COLOR_YELLOW "GL_Bind: NULL image\n");
		texnum = tr.defaultImage->texnum;
	}
	else
	{
		texnum = image->texnum;
	}

	if (r_nobind->integer && tr.dlightImage)
	{
		// performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if (glState.currenttextures[glState.currenttmu] == texnum)
	{
		return;
	}

	image->frameUsed = tr.frameCount;
	glState.currenttextures[glState.currenttmu] = texnum;
	qglBindTexture(GL_TEXTURE_2D, texnum);
}

//==========================================================================
//
//	GL_SelectTexture
//
//==========================================================================

void GL_SelectTexture(int unit)
{
	if (glState.currenttmu == unit)
	{
		return;
	}

	if (unit == 0)
	{
		qglActiveTextureARB(GL_TEXTURE0_ARB);
		QGL_LogComment("glActiveTextureARB( GL_TEXTURE0_ARB )\n");
		qglClientActiveTextureARB(GL_TEXTURE0_ARB);
		QGL_LogComment("glClientActiveTextureARB( GL_TEXTURE0_ARB )\n");
	}
	else if (unit == 1)
	{
		qglActiveTextureARB(GL_TEXTURE1_ARB);
		QGL_LogComment("glActiveTextureARB( GL_TEXTURE1_ARB )\n");
		qglClientActiveTextureARB(GL_TEXTURE1_ARB);
		QGL_LogComment("glClientActiveTextureARB( GL_TEXTURE1_ARB )\n");
	}
	else
	{
		throw DropException(va("GL_SelectTexture: unit = %i", unit));
	}

	glState.currenttmu = unit;
}

//==========================================================================
//
//	GL_TexEnv
//
//==========================================================================

void GL_TexEnv(GLenum env)
{
	if (env == glState.texEnv[glState.currenttmu])
	{
		return;
	}

	glState.texEnv[glState.currenttmu] = env;

	switch (env)
	{
	case GL_MODULATE:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case GL_REPLACE:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	case GL_DECAL:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		break;
	case GL_ADD:
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		break;
	default:
		throw DropException(va("GL_TexEnv: invalid env '%d' passed\n", env));
	}
}

//==========================================================================
//
//	GL_State
//
//	This routine is responsible for setting the most commonly changed state.
//
//==========================================================================

void GL_State(unsigned long stateBits)
{
	unsigned long diff = stateBits ^ glState.glStateBits;

	if (!diff)
	{
		return;
	}

	//
	// check depthFunc bits
	//
	if (diff & GLS_DEPTHFUNC_EQUAL)
	{
		if (stateBits & GLS_DEPTHFUNC_EQUAL)
		{
			qglDepthFunc(GL_EQUAL);
		}
		else
		{
			qglDepthFunc(GL_LEQUAL);
		}
	}

	//
	// check blend bits
	//
	if (diff & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
	{
		if (stateBits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS))
		{
			GLenum srcFactor;
			switch (stateBits & GLS_SRCBLEND_BITS)
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				throw DropException("GL_State: invalid src blend state bits\n");
			}

			GLenum dstFactor;
			switch (stateBits & GLS_DSTBLEND_BITS)
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				throw DropException("GL_State: invalid dst blend state bits\n");
			}

			qglEnable(GL_BLEND);
			qglBlendFunc(srcFactor, dstFactor);
		}
		else
		{
			qglDisable(GL_BLEND);
		}
	}

	//
	// check depthmask
	//
	if (diff & GLS_DEPTHMASK_TRUE)
	{
		if (stateBits & GLS_DEPTHMASK_TRUE)
		{
			qglDepthMask(GL_TRUE);
		}
		else
		{
			qglDepthMask(GL_FALSE);
		}
	}

	//
	// fill/line mode
	//
	if (diff & GLS_POLYMODE_LINE)
	{
		if (stateBits & GLS_POLYMODE_LINE)
		{
			qglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			qglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	//
	// depthtest
	//
	if (diff & GLS_DEPTHTEST_DISABLE)
	{
		if (stateBits & GLS_DEPTHTEST_DISABLE)
		{
			qglDisable(GL_DEPTH_TEST);
		}
		else
		{
			qglEnable(GL_DEPTH_TEST);
		}
	}

	//
	// alpha test
	//
	if (diff & GLS_ATEST_BITS)
	{
		switch (stateBits & GLS_ATEST_BITS)
		{
		case 0:
			qglDisable(GL_ALPHA_TEST);
			break;
		case GLS_ATEST_GT_0:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GREATER, 0.0f);
			break;
		case GLS_ATEST_LT_80:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_LESS, 0.5f);
			break;
		case GLS_ATEST_GE_80:
			qglEnable(GL_ALPHA_TEST);
			qglAlphaFunc(GL_GEQUAL, 0.5f);
			break;
		default:
			qassert(0);
			break;
		}
	}

	glState.glStateBits = stateBits;
}

//==========================================================================
//
//	GL_Cull
//
//==========================================================================

void GL_Cull(int cullType)
{
	if (glState.faceCulling == cullType)
	{
		return;
	}

	glState.faceCulling = cullType;

	if (cullType == CT_TWO_SIDED)
	{
		qglDisable(GL_CULL_FACE);
	} 
	else 
	{
		qglEnable(GL_CULL_FACE);

		if (cullType == CT_BACK_SIDED)
		{
			if (backEnd.viewParms.isMirror)
			{
				qglCullFace(GL_FRONT);
			}
			else
			{
				qglCullFace(GL_BACK);
			}
		}
		else
		{
			if (backEnd.viewParms.isMirror)
			{
				qglCullFace(GL_BACK);
			}
			else
			{
				qglCullFace(GL_FRONT);
			}
		}
	}
}
