//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../../client.h"
#include "local.h"

h2entity_state_t clh2_baselines[MAX_EDICTS_H2];

static float RTint[256];
static float GTint[256];
static float BTint[256];

qhandle_t clh2_player_models[MAX_PLAYER_CLASS];
image_t* clh2_playertextures[H2BIGGEST_MAX_CLIENTS];		// color translated skins

void CLH2_InitColourShadeTables()
{
	for (int i = 0; i < 16; i++)
	{
		int c = ColorIndex[i];

		int r = r_palette[c][0];
		int g = r_palette[c][1];
		int b = r_palette[c][2];

		for (int p = 0; p < 16; p++)
		{
			RTint[i * 16 + p] = ((float)r) / ((float)ColorPercent[15 - p]) ;
			GTint[i * 16 + p] = ((float)g) / ((float)ColorPercent[15 - p]);
			BTint[i * 16 + p] = ((float)b) / ((float)ColorPercent[15 - p]);
		}
	}
}

int CLH2_GetMaxPlayerClasses()
{
	if (GGameType & GAME_HexenWorld)
	{
		return MAX_PLAYER_CLASS;
	}
	if (GGameType & GAME_H2Portals)
	{
		return NUM_CLASSES_H2MP;
	}
	return NUM_CLASSES_H2;
}

void CLH2_SetRefEntAxis(refEntity_t* entity, vec3_t entityAngles, vec3_t angleAdd, int scale, int colourShade, int absoluteLight, int drawFlags)
{
	if (drawFlags & H2DRF_TRANSLUCENT)
	{
		entity->renderfx |= RF_WATERTRANS;
	}

	vec3_t angles;
	if (R_IsMeshModel(entity->hModel))
	{
		if (R_ModelFlags(entity->hModel) & H2MDLEF_FACE_VIEW)
		{
			//	yaw and pitch must be 0 so that renderer can safely multply matrices.
			angles[PITCH] = 0;
			angles[YAW] = 0;
			angles[ROLL] = entityAngles[ROLL];

			AnglesToAxis(angles, entity->axis);
		}
		else 
		{
			if (R_ModelFlags(entity->hModel) & H2MDLEF_ROTATE)
			{
				angles[YAW] = AngleMod((entity->origin[0] + entity->origin[1]) * 0.8 + (108 * cl_common->serverTime * 0.001));
			}
			else
			{
				angles[YAW] = entityAngles[YAW];
			}
			angles[ROLL] = entityAngles[ROLL];
			// stupid quake bug
			angles[PITCH] = -entityAngles[PITCH];

			if (angleAdd[0] || angleAdd[1] || angleAdd[2])
			{
				float BaseAxis[3][3];
				AnglesToAxis(angles, BaseAxis);

				// For clientside rotation stuff
				float AddAxis[3][3];
				AnglesToAxis(angleAdd, AddAxis);

				MatrixMultiply(AddAxis, BaseAxis, entity->axis);
			}
			else
			{
				AnglesToAxis(angles, entity->axis);
			}
		}

		if ((R_ModelFlags(entity->hModel) & H2MDLEF_ROTATE) || (scale != 0 && scale != 100))
		{
			entity->renderfx |= RF_LIGHTING_ORIGIN;
			VectorCopy(entity->origin, entity->lightingOrigin);
		}

		if (R_ModelFlags(entity->hModel) & H2MDLEF_ROTATE)
		{
			// Floating motion
			float delta = sin(entity->origin[0] + entity->origin[1] + (cl_common->serverTime * 0.001 * 3)) * 5.5;
			VectorMA(entity->origin, delta, entity->axis[2], entity->origin);
			absoluteLight = 60 + 34 + sin(entity->origin[0] + entity->origin[1] + (cl_common->serverTime * 0.001 * 3.8)) * 34;
			drawFlags |= H2MLS_ABSLIGHT;
		}

		if (scale != 0 && scale != 100)
		{
			float entScale = (float)scale / 100.0;
			float esx;
			float esy;
			float esz;
			switch (drawFlags & H2SCALE_TYPE_MASKIN)
			{
			case H2SCALE_TYPE_UNIFORM:
				esx = entScale;
				esy = entScale;
				esz = entScale;
				break;
			case H2SCALE_TYPE_XYONLY:
				esx = entScale;
				esy = entScale;
				esz = 1;
				break;
			case H2SCALE_TYPE_ZONLY:
				esx = 1;
				esy = 1;
				esz = entScale;
				break;
			}
			float etz;
			switch (drawFlags & H2SCALE_ORIGIN_MASKIN)
			{
			case H2SCALE_ORIGIN_CENTER:
				etz = 0.5;
				break;
			case H2SCALE_ORIGIN_BOTTOM:
				etz = 0;
				break;
			case H2SCALE_ORIGIN_TOP:
				etz = 1.0;
				break;
			}

			vec3_t Out;
			R_CalculateModelScaleOffset(entity->hModel, esx, esy, esz, etz, Out);
			VectorMA(entity->origin, Out[0], entity->axis[0], entity->origin);
			VectorMA(entity->origin, Out[1], entity->axis[1], entity->origin);
			VectorMA(entity->origin, Out[2], entity->axis[2], entity->origin);
			VectorScale(entity->axis[0], esx, entity->axis[0]);
			VectorScale(entity->axis[1], esy, entity->axis[1]);
			VectorScale(entity->axis[2], esz, entity->axis[2]);
			entity->nonNormalizedAxes = true;
		}
	}
	else
	{
		angles[YAW] = entityAngles[YAW];
		angles[ROLL] = entityAngles[ROLL];
		angles[PITCH] = entityAngles[PITCH];

		AnglesToAxis(angles, entity->axis);
	}

	if (colourShade)
	{
		entity->renderfx |= RF_COLORSHADE;
		entity->shaderRGBA[0] = (int)(RTint[colourShade] * 255);
		entity->shaderRGBA[1] = (int)(GTint[colourShade] * 255);
		entity->shaderRGBA[2] = (int)(BTint[colourShade] * 255);
	}

	int mls = drawFlags & H2MLS_MASKIN;
	if (mls == H2MLS_ABSLIGHT)
	{
		entity->renderfx |= RF_ABSOLUTE_LIGHT;
		entity->radius = absoluteLight / 256.0;
	}
	else if (mls != H2MLS_NONE)
	{
		// Use a model light style (25-30)
		entity->renderfx |= RF_ABSOLUTE_LIGHT;
		entity->radius = cl_lightstyle[24 + mls].value[0] / 2;
	}
}
