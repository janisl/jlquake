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

#include "../../client.h"
#include "local.h"

q2entity_state_t clq2_parse_entities[MAX_PARSE_ENTITIES_Q2];

void CLQ2_AddPacketEntities(q2frame_t* frame)
{
	// bonus items rotate at a fixed rate
	float autorotate = AngleMod(cl.serverTime / 10);

	// brush models can auto animate their frames
	int autoanim = 2 * cl.serverTime / 1000;

	for (int pnum = 0; pnum < frame->num_entities; pnum++)
	{
		q2entity_state_t* s1 = &clq2_parse_entities[(frame->parse_entities + pnum) & (MAX_PARSE_ENTITIES_Q2 - 1)];

		q2centity_t* cent = &clq2_entities[s1->number];

		unsigned int effects = s1->effects;
		unsigned int renderfx_old = s1->renderfx;

		refEntity_t ent;
		Com_Memset(&ent, 0, sizeof(ent));
		ent.reType = RT_MODEL;

		//	Convert Quake 2 render flags to new ones.
		unsigned int renderfx = 0;
		if (renderfx_old & Q2RF_MINLIGHT)
		{
			renderfx |= RF_MINLIGHT;
		}
		if (renderfx_old & Q2RF_VIEWERMODEL)
		{
			renderfx |= RF_THIRD_PERSON;
		}
		if (renderfx_old & Q2RF_WEAPONMODEL)
		{
			renderfx |= RF_FIRST_PERSON;
		}
		if (renderfx_old & Q2RF_DEPTHHACK)
		{
			renderfx |= RF_DEPTHHACK;
		}
		if (renderfx_old & Q2RF_TRANSLUCENT)
		{
			renderfx |= RF_TRANSLUCENT;
		}
		if (renderfx_old & Q2RF_GLOW)
		{
			renderfx |= RF_GLOW;
		}
		if (renderfx_old & Q2RF_IR_VISIBLE)
		{
			renderfx |= RF_IR_VISIBLE;
		}

		// set frame
		if (effects & Q2EF_ANIM01)
		{
			ent.frame = autoanim & 1;
		}
		else if (effects & Q2EF_ANIM23)
		{
			ent.frame = 2 + (autoanim & 1);
		}
		else if (effects & Q2EF_ANIM_ALL)
		{
			ent.frame = autoanim;
		}
		else if (effects & Q2EF_ANIM_ALLFAST)
		{
			ent.frame = cl.serverTime / 100;
		}
		else
		{
			ent.frame = s1->frame;
		}

		// quad and pent can do different things on client
		if (effects & Q2EF_PENT)
		{
			effects &= ~Q2EF_PENT;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_RED;
		}

		if (effects & Q2EF_QUAD)
		{
			effects &= ~Q2EF_QUAD;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_BLUE;
		}
//======
// PMM
		if (effects & Q2EF_DOUBLE)
		{
			effects &= ~Q2EF_DOUBLE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_DOUBLE;
		}

		if (effects & Q2EF_HALF_DAMAGE)
		{
			effects &= ~Q2EF_HALF_DAMAGE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx_old |= Q2RF_SHELL_HALF_DAM;
		}
// pmm
//======
		ent.oldframe = cent->prev.frame;
		ent.backlerp = 1.0 - cl.q2_lerpfrac;

		if (renderfx_old & (Q2RF_FRAMELERP | Q2RF_BEAM))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy(cent->current.origin, ent.origin);
			VectorCopy(cent->current.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (int i = 0; i < 3; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.q2_lerpfrac *
												   (cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// create a new entity

		// tweak the color of beams
		if (renderfx_old & Q2RF_BEAM)
		{
			// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.reType = RT_BEAM;
			ent.shaderRGBA[3] = 76;
			ent.skinNum = (s1->skinnum >> ((rand() % 4) * 8)) & 0xff;
			ent.hModel = 0;
		}
		else
		{
			// set skin
			if (s1->modelindex == 255)
			{	// use custom player skin
				ent.skinNum = 0;
				q2clientinfo_t* ci = &cl.q2_clientinfo[s1->skinnum & 0xff];
				ent.customSkin = R_GetImageHandle(ci->skin);
				ent.hModel = ci->model;
				if (!ent.customSkin || !ent.hModel)
				{
					ent.customSkin = R_GetImageHandle(cl.q2_baseclientinfo.skin);
					ent.hModel = cl.q2_baseclientinfo.model;
				}

//============
//PGM
				if (renderfx_old & Q2RF_USE_DISGUISE)
				{
					if (!String::NCmp(R_GetImageName(ent.customSkin), "players/male", 12))
					{
						ent.customSkin = R_GetImageHandle(R_RegisterSkinQ2("players/male/disguise.pcx"));
						ent.hModel = R_RegisterModel("players/male/tris.md2");
					}
					else if (!String::NCmp(R_GetImageName(ent.customSkin), "players/female", 14))
					{
						ent.customSkin = R_GetImageHandle(R_RegisterSkinQ2("players/female/disguise.pcx"));
						ent.hModel = R_RegisterModel("players/female/tris.md2");
					}
					else if (!String::NCmp(R_GetImageName(ent.customSkin), "players/cyborg", 14))
					{
						ent.customSkin = R_GetImageHandle(R_RegisterSkinQ2("players/cyborg/disguise.pcx"));
						ent.hModel = R_RegisterModel("players/cyborg/tris.md2");
					}
				}
//PGM
//============
			}
			else
			{
				ent.skinNum = s1->skinnum;
				ent.hModel = cl.model_draw[s1->modelindex];
			}
		}

		// only used for black hole model right now, FIXME: do better
		if (renderfx_old == Q2RF_TRANSLUCENT)
		{
			ent.shaderRGBA[3] = 178;
		}

		// render effects (fullbright, translucent, etc)
		if ((effects & Q2EF_COLOR_SHELL))
		{
			// renderfx go on color shell entity
			ent.renderfx = 0;
		}
		else
		{
			ent.renderfx = renderfx;
		}

		// calculate angles
		vec3_t angles;
		if (effects & Q2EF_ROTATE)
		{	// some bonus items auto-rotate
			angles[0] = 0;
			angles[1] = autorotate;
			angles[2] = 0;
		}
		// RAFAEL
		else if (effects & Q2EF_SPINNINGLIGHTS)
		{
			angles[0] = 0;
			angles[1] = AngleMod(cl.serverTime / 2) + s1->angles[1];
			angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors(angles, forward, NULL, NULL);
				VectorMA(ent.origin, 64, forward, start);
				R_AddLightToScene(start, 100, 1, 0, 0);
			}
		}
		else
		{	// interpolate angles
			for (int i = 0; i < 3; i++)
			{
				float a1 = cent->current.angles[i];
				float a2 = cent->prev.angles[i];
				angles[i] = LerpAngle(a2, a1, cl.q2_lerpfrac);
			}
		}
		AnglesToAxis(angles, ent.axis);

		if (s1->number == cl.playernum + 1)
		{
			ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
			// FIXME: still pass to refresh

			if (effects & Q2EF_FLAG1)
			{
				R_AddLightToScene(ent.origin, 225, 1.0, 0.1, 0.1);
			}
			else if (effects & Q2EF_FLAG2)
			{
				R_AddLightToScene(ent.origin, 225, 0.1, 0.1, 1.0);
			}
			else if (effects & Q2EF_TAGTRAIL)						//PGM
			{
				R_AddLightToScene(ent.origin, 225, 1.0, 1.0, 0.0);	//PGM
			}
			else if (effects & Q2EF_TRACKERTRAIL)					//PGM
			{
				R_AddLightToScene(ent.origin, 225, -1.0, -1.0, -1.0);	//PGM

			}
			continue;
		}

		// if set to invisible, skip
		if (!s1->modelindex)
		{
			continue;
		}

		if (effects & Q2EF_BFG)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			ent.shaderRGBA[3] = 76;
		}

		// RAFAEL
		if (effects & Q2EF_PLASMA)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			ent.shaderRGBA[3] = 153;
		}

		if (effects & Q2EF_SPHERETRANS)
		{
			ent.renderfx |= RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if (effects & Q2EF_TRACKERTRAIL)
			{
				ent.shaderRGBA[3] = 153;
			}
			else
			{
				ent.shaderRGBA[3] = 76;
			}
		}
//pmm

		// add to refresh list
		R_AddRefEntityToScene(&ent);

		// color shells generate a seperate entity for the main model
		if (effects & Q2EF_COLOR_SHELL)
		{
			ent.renderfx = renderfx | RF_TRANSLUCENT | RF_COLOUR_SHELL;
			// PMM - rewrote, reordered to handle new shells & mixing
			// PMM -special case for godmode
			if ((renderfx_old & Q2RF_SHELL_RED) &&
				(renderfx_old & Q2RF_SHELL_BLUE) &&
				(renderfx_old & Q2RF_SHELL_GREEN))
			{
				ent.shaderRGBA[0] = 255;
				ent.shaderRGBA[1] = 255;
				ent.shaderRGBA[2] = 255;
			}
			else if (renderfx_old & (Q2RF_SHELL_RED | Q2RF_SHELL_BLUE | Q2RF_SHELL_DOUBLE))
			{
				ent.shaderRGBA[0] = 0;
				ent.shaderRGBA[1] = 0;
				ent.shaderRGBA[2] = 0;

				if (renderfx_old & Q2RF_SHELL_RED)
				{
					ent.shaderRGBA[0] = 255;
					if (renderfx_old & (Q2RF_SHELL_BLUE | Q2RF_SHELL_DOUBLE))
					{
						ent.shaderRGBA[2] = 255;
					}
				}
				else if (renderfx_old & Q2RF_SHELL_BLUE)
				{
					if (renderfx_old & Q2RF_SHELL_DOUBLE)
					{
						ent.shaderRGBA[1] = 255;
						ent.shaderRGBA[2] = 255;
					}
					else
					{
						ent.shaderRGBA[2] = 255;
					}
				}
				else if (renderfx_old & Q2RF_SHELL_DOUBLE)
				{
					ent.shaderRGBA[0] = 230;
					ent.shaderRGBA[1] = 178;
				}
			}
			else if (renderfx_old & (Q2RF_SHELL_HALF_DAM | Q2RF_SHELL_GREEN))
			{
				ent.shaderRGBA[0] = 0;
				ent.shaderRGBA[1] = 0;
				ent.shaderRGBA[2] = 0;
				// PMM - new colors
				if (renderfx_old & Q2RF_SHELL_HALF_DAM)
				{
					ent.shaderRGBA[0] = 143;
					ent.shaderRGBA[1] = 150;
					ent.shaderRGBA[2] = 115;
				}
				if (renderfx_old & Q2RF_SHELL_GREEN)
				{
					ent.shaderRGBA[1] = 255;
				}
			}
			ent.shaderRGBA[3] = 76;
			R_AddRefEntityToScene(&ent);
		}

		ent.skinNum = 0;
		ent.renderfx = 0;
		ent.shaderRGBA[3] = 0;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				q2clientinfo_t* ci = &cl.q2_clientinfo[s1->skinnum & 0xff];
				int i = (s1->skinnum >> 8);	// 0 is default weapon model
				if (!clq2_vwep->value || i > MAX_CLIENTWEAPONMODELS_Q2 - 1)
				{
					i = 0;
				}
				ent.hModel = ci->weaponmodel[i];
				if (!ent.hModel)
				{
					if (i != 0)
					{
						ent.hModel = ci->weaponmodel[0];
					}
					if (!ent.hModel)
					{
						ent.hModel = cl.q2_baseclientinfo.weaponmodel[0];
					}
				}
			}
			//PGM - hack to allow translucent linked models (defender sphere's shell)
			//		set the high bit 0x80 on modelindex2 to enable translucency
			else if (s1->modelindex2 & 0x80)
			{
				ent.hModel = cl.model_draw[s1->modelindex2 & 0x7F];
				ent.shaderRGBA[3] = 82;
				ent.renderfx = RF_TRANSLUCENT;
			}
			//PGM
			else
			{
				ent.hModel = cl.model_draw[s1->modelindex2];
			}
			R_AddRefEntityToScene(&ent);

			//PGM - make sure these get reset.
			ent.renderfx = 0;
			ent.shaderRGBA[3] = 0;
			//PGM
		}
		if (s1->modelindex3)
		{
			ent.hModel = cl.model_draw[s1->modelindex3];
			R_AddRefEntityToScene(&ent);
		}
		if (s1->modelindex4)
		{
			ent.hModel = cl.model_draw[s1->modelindex4];
			R_AddRefEntityToScene(&ent);
		}

		if (effects & Q2EF_POWERSCREEN)
		{
			ent.hModel = clq2_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.renderfx |= RF_TRANSLUCENT | RF_COLOUR_SHELL;
			ent.shaderRGBA[0] = 0;
			ent.shaderRGBA[1] = 255;
			ent.shaderRGBA[2] = 0;
			ent.shaderRGBA[3] = 76;
			R_AddRefEntityToScene(&ent);
		}

		// add automatic particle trails
		if ((effects & ~Q2EF_ROTATE))
		{
			if (effects & Q2EF_ROCKET)
			{
				CLQ2_RocketTrail(cent->lerp_origin, ent.origin, cent);
				R_AddLightToScene(ent.origin, 200, 1, 1, 0);
			}
			// PGM - Do not reorder Q2EF_BLASTER and Q2EF_HYPERBLASTER.
			// Q2EF_BLASTER | Q2EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & Q2EF_BLASTER)
			{
//				CLQ2_BlasterTrail (cent->lerp_origin, ent.origin);
//PGM
				if (effects & Q2EF_TRACKER)	// lame... problematic?
				{
					CLQ2_BlasterTrail2(cent->lerp_origin, ent.origin);
					R_AddLightToScene(ent.origin, 200, 0, 1, 0);
				}
				else
				{
					CLQ2_BlasterTrail(cent->lerp_origin, ent.origin);
					R_AddLightToScene(ent.origin, 200, 1, 1, 0);
				}
//PGM
			}
			else if (effects & Q2EF_HYPERBLASTER)
			{
				if (effects & Q2EF_TRACKER)						// PGM	overloaded for blaster2.
				{
					R_AddLightToScene(ent.origin, 200, 0, 1, 0);		// PGM
				}
				else											// PGM
				{
					R_AddLightToScene(ent.origin, 200, 1, 1, 0);
				}
			}
			else if (effects & Q2EF_GIB)
			{
				CLQ2_DiminishingTrail(cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & Q2EF_GRENADE)
			{
				CLQ2_DiminishingTrail(cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & Q2EF_FLIES)
			{
				CLQ2_FlyEffect(cent, ent.origin);
			}
			else if (effects & Q2EF_BFG)
			{
				static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

				int i;
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BfgParticles(ent.origin);
					i = 200;
				}
				else
				{
					i = bfg_lightramp[s1->frame];
				}
				R_AddLightToScene(ent.origin, i, 0, 1, 0);
			}
			// RAFAEL
			else if (effects & Q2EF_TRAP)
			{
				ent.origin[2] += 32;
				CLQ2_TrapParticles(ent.origin);
				int i = (rand() % 100) + 100;
				R_AddLightToScene(ent.origin, i, 1, 0.8, 0.1);
			}
			else if (effects & Q2EF_FLAG1)
			{
				CLQ2_FlagTrail(cent->lerp_origin, ent.origin, 242);
				R_AddLightToScene(ent.origin, 225, 1, 0.1, 0.1);
			}
			else if (effects & Q2EF_FLAG2)
			{
				CLQ2_FlagTrail(cent->lerp_origin, ent.origin, 115);
				R_AddLightToScene(ent.origin, 225, 0.1, 0.1, 1);
			}
//======
//ROGUE
			else if (effects & Q2EF_TAGTRAIL)
			{
				CLQ2_TagTrail(cent->lerp_origin, ent.origin, 220);
				R_AddLightToScene(ent.origin, 225, 1.0, 1.0, 0.0);
			}
			else if (effects & Q2EF_TRACKERTRAIL)
			{
				if (effects & Q2EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.serverTime / 500.0) + 1.0));
					R_AddLightToScene(ent.origin, intensity, -1.0, -1.0, -1.0);
				}
				else
				{
					CLQ2_Tracker_Shell(cent->lerp_origin);
					R_AddLightToScene(ent.origin, 155, -1.0, -1.0, -1.0);
				}
			}
			else if (effects & Q2EF_TRACKER)
			{
				CLQ2_TrackerTrail(cent->lerp_origin, ent.origin, 0);
				R_AddLightToScene(ent.origin, 200, -1, -1, -1);
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & Q2EF_GREENGIB)
			{
				CLQ2_DiminishingTrail(cent->lerp_origin, ent.origin, cent, effects);
			}
			// RAFAEL
			else if (effects & Q2EF_IONRIPPER)
			{
				CLQ2_IonripperTrail(cent->lerp_origin, ent.origin);
				R_AddLightToScene(ent.origin, 100, 1, 0.5, 0.5);
			}
			// RAFAEL
			else if (effects & Q2EF_BLUEHYPERBLASTER)
			{
				R_AddLightToScene(ent.origin, 200, 0, 0, 1);
			}
			// RAFAEL
			else if (effects & Q2EF_PLASMA)
			{
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BlasterTrail(cent->lerp_origin, ent.origin);
				}
				R_AddLightToScene(ent.origin, 130, 1, 0.5, 0.5);
			}
		}

		VectorCopy(ent.origin, cent->lerp_origin);
	}
}
