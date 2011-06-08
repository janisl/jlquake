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

#define EF_TRANSPARENT		4096	// Transparent sprite
#define EF_HOLEY			16384	// Solid model with color 0
#define EF_SPECIAL_TRANS	32768	// Translucency through the particle table

#define	MAX_LBM_HEIGHT		480

#define ALIAS_BASE_SIZE_RATIO		(1.0 / 11.0)
					// normalizing factor so player model works out to about
					//  1 pixel per triangle

#define MAXALIASFRAMES		256
#define MAXALIASVERTS		2000
#define MAXALIASTRIS		2048

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

byte					q1_player_8bit_texels[320 * 200];
byte					h2_player_8bit_texels[6][620 * 245];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static float			aliastransform[3][4];

static vec3_t			mins;
static vec3_t			maxs;

static int				posenum;
static mesh1hdr_t*		pheader;

// a pose is a single set of vertexes.  a frame may be
// an animating sequence of poses
static const dmdl_trivertx_t*	poseverts[MAXALIASFRAMES];
static dmdl_stvert_t			stverts[MAXALIASVERTS];
static mmesh1triangle_t			triangles[MAXALIASTRIS];

static model_t*			aliasmodel;
static mesh1hdr_t*		paliashdr;

static qboolean			used[8192];

// the command list holds counts and s/t values that are valid for
// every frame
static int				commands[8192];
static int				numcommands;

// all frames will have their vertexes rearranged and expanded
// so they are in the order expected by the command list
static int				vertexorder[8192];
static int				numorder;

static int				stripverts[128];
static int				striptris[128];
static int				stripstverts[128];
static int				stripcount;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	R_AliasTransformVector
//
//==========================================================================

static void R_AliasTransformVector(const vec3_t in, vec3_t out)
{
	out[0] = DotProduct(in, aliastransform[0]) + aliastransform[0][3];
	out[1] = DotProduct(in, aliastransform[1]) + aliastransform[1][3];
	out[2] = DotProduct(in, aliastransform[2]) + aliastransform[2][3];
}

//==========================================================================
//
//	Mod_LoadAliasFrame
//
//==========================================================================

static const void* Mod_LoadAliasFrame(const void* pin, mmesh1framedesc_t* frame)
{
	const dmdl_frame_t* pdaliasframe = (const dmdl_frame_t*)pin;

	QStr::Cpy(frame->name, pdaliasframe->name);
	frame->firstpose = posenum;
	frame->numposes = 1;

	for (int i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about
		// endianness
		frame->bboxmin.v[i] = pdaliasframe->bboxmin.v[i];
		frame->bboxmin.v[i] = pdaliasframe->bboxmax.v[i];
	}

	const dmdl_trivertx_t* pinframe = (const dmdl_trivertx_t*)(pdaliasframe + 1);

	aliastransform[0][0] = pheader->scale[0];
	aliastransform[1][1] = pheader->scale[1];
	aliastransform[2][2] = pheader->scale[2];
	aliastransform[0][3] = pheader->scale_origin[0];
	aliastransform[1][3] = pheader->scale_origin[1];
	aliastransform[2][3] = pheader->scale_origin[2];

	for (int j = 0; j < pheader->numverts; j++)
	{
		vec3_t in;
		in[0] = pinframe[j].v[0];
		in[1] = pinframe[j].v[1];
		in[2] = pinframe[j].v[2];
		vec3_t out;
		R_AliasTransformVector(in, out);
		for (int i = 0; i < 3; i++)
		{
			if (mins[i] > out[i])
			{
				mins[i] = out[i];
			}
			if (maxs[i] < out[i])
			{
				maxs[i] = out[i];
			}
		}
	}
	poseverts[posenum] = pinframe;
	posenum++;

	pinframe += pheader->numverts;

	return (const void *)pinframe;
}

//==========================================================================
//
//	Mod_LoadAliasGroup
//
//==========================================================================

static const void* Mod_LoadAliasGroup(const void* pin, mmesh1framedesc_t* frame)
{
	const dmdl_group_t* pingroup = (const dmdl_group_t*)pin;

	int numframes = LittleLong(pingroup->numframes);

	frame->firstpose = posenum;
	frame->numposes = numframes;

	for (int i = 0; i < 3; i++)
	{
		// these are byte values, so we don't have to worry about endianness
		frame->bboxmin.v[i] = pingroup->bboxmin.v[i];
		frame->bboxmax.v[i] = pingroup->bboxmax.v[i];
	}

	const dmdl_interval_t* pin_intervals = (const dmdl_interval_t*)(pingroup + 1);

	frame->interval = LittleFloat(pin_intervals->interval);

	pin_intervals += numframes;

	const void* ptemp = (const void*)pin_intervals;

	aliastransform[0][0] = pheader->scale[0];
	aliastransform[1][1] = pheader->scale[1];
	aliastransform[2][2] = pheader->scale[2];
	aliastransform[0][3] = pheader->scale_origin[0];
	aliastransform[1][3] = pheader->scale_origin[1];
	aliastransform[2][3] = pheader->scale_origin[2];

	for (int i = 0; i < numframes; i++)
	{
		poseverts[posenum] = (const dmdl_trivertx_t*)((const dmdl_frame_t*)ptemp + 1);

		for (int j = 0; j < pheader->numverts; j++)
		{
			vec3_t in;
			in[0] = poseverts[posenum][j].v[0];
			in[1] = poseverts[posenum][j].v[1];
			in[2] = poseverts[posenum][j].v[2];
			vec3_t out;
			R_AliasTransformVector(in, out);
			for (int k = 0; k < 3; k++)
			{
				if (mins[k] > out[k])
				{
					mins[k] = out[k];
				}
				if (maxs[k] < out[k])
				{
					maxs[k] = out[k];
				}
			}
		}

		posenum++;

		ptemp = (const dmdl_trivertx_t*)((const dmdl_frame_t*)ptemp + 1) + pheader->numverts;
	}

	return ptemp;
}

//==========================================================================
//
//	Mod_LoadAllSkins
//
//==========================================================================

static void* Mod_LoadAllSkins(int numskins, dmdl_skintype_t* pskintype, int mdl_flags)
{
	if (numskins < 1 || numskins > MAX_MESH1_SKINS)
	{
		throw QException(va("Mod_LoadMdlModel: Invalid # of skins: %d\n", numskins));
	}

	int s = pheader->skinwidth * pheader->skinheight;

	int texture_mode = IMG8MODE_Skin;
	if (GGameType & GAME_Hexen2)
	{
		if (mdl_flags & EF_HOLEY)
		{
			texture_mode = IMG8MODE_SkinHoley;
		}
		else if (mdl_flags & EF_TRANSPARENT)
		{
			texture_mode = IMG8MODE_SkinTransparent;
		}
		else if (mdl_flags & EF_SPECIAL_TRANS)
		{
			texture_mode = IMG8MODE_SkinSpecialTrans;
		}
	}

	for (int i = 0; i < numskins; i++)
	{
		if (pskintype->type == ALIAS_SKIN_SINGLE)
		{
			byte* pic32 = R_ConvertImage8To32((byte *)(pskintype + 1), pheader->skinwidth, pheader->skinheight, texture_mode);

			// save 8 bit texels for the player model to remap
			if ((GGameType & GAME_Quake) && !QStr::Cmp(loadmodel->name,"progs/player.mdl"))
			{
				if (s > (int)sizeof(q1_player_8bit_texels))
					throw QException("Player skin too large");
				Com_Memcpy(q1_player_8bit_texels, (byte *)(pskintype + 1), s);
			}
			else if (GGameType & GAME_Hexen2)
			{
				if (!QStr::Cmp(loadmodel->name,"models/paladin.mdl"))
				{
					if (s > (int)sizeof(h2_player_8bit_texels[0]))
						throw QException("Player skin too large");
					Com_Memcpy(h2_player_8bit_texels[0], (byte *)(pskintype + 1), s);
				}
				else if (!QStr::Cmp(loadmodel->name,"models/crusader.mdl"))
				{
					if (s > (int)sizeof(h2_player_8bit_texels[1]))
						throw QException("Player skin too large");
					Com_Memcpy(h2_player_8bit_texels[1], (byte *)(pskintype + 1), s);
				}
				else if (!QStr::Cmp(loadmodel->name,"models/necro.mdl"))
				{
					if (s > (int)sizeof(h2_player_8bit_texels[2]))
						throw QException("Player skin too large");
					Com_Memcpy(h2_player_8bit_texels[2], (byte *)(pskintype + 1), s);
				}
				else if (!QStr::Cmp(loadmodel->name,"models/assassin.mdl"))
				{
					if (s > (int)sizeof(h2_player_8bit_texels[3]))
						throw QException("Player skin too large");
					Com_Memcpy(h2_player_8bit_texels[3], (byte *)(pskintype + 1), s);
				}
				else if (!QStr::Cmp(loadmodel->name,"models/succubus.mdl"))
				{
					if (s > (int)sizeof(h2_player_8bit_texels[4]))
						throw QException("Player skin too large");
					Com_Memcpy(h2_player_8bit_texels[4], (byte *)(pskintype + 1), s);
				}
				else if (!QStr::Cmp(loadmodel->name,"models/hank.mdl"))
				{
					if (s > (int)sizeof(h2_player_8bit_texels[5]))
						throw QException("Player skin too large");
					Com_Memcpy(h2_player_8bit_texels[5], (byte *)(pskintype + 1), s);
				}
			}

			char name[32];
			sprintf(name, "%s_%i", loadmodel->name, i);

			pheader->gl_texture[i][0] =
			pheader->gl_texture[i][1] =
			pheader->gl_texture[i][2] =
			pheader->gl_texture[i][3] =
				R_CreateImage(name, pic32, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false);
			delete[] pic32;
			pskintype = (dmdl_skintype_t *)((byte *)(pskintype+1) + s);
		}
		else
		{
			// animating skin group.  yuck.
			pskintype++;
			dmdl_skingroup_t* pinskingroup = (dmdl_skingroup_t*)pskintype;
			int groupskins = LittleLong(pinskingroup->numskins);
			dmdl_skininterval_t* pinskinintervals = (dmdl_skininterval_t*)(pinskingroup + 1);

			pskintype = (dmdl_skintype_t*)(pinskinintervals + groupskins);

			int j;
			for (j = 0; j < groupskins; j++)
			{
				char name[32];
				sprintf(name, "%s_%i_%i", loadmodel->name, i, j);
				
				byte* pic32 = R_ConvertImage8To32((byte*)pskintype, pheader->skinwidth, pheader->skinheight, texture_mode);
				pheader->gl_texture[i][j&3] = R_CreateImage(name, pic32, pheader->skinwidth, pheader->skinheight, true, true, GL_REPEAT, false);
				delete[] pic32;
				pskintype = (dmdl_skintype_t*)((byte*)pskintype + s);
			}
			int k = j;
			for (/* */; j < 4; j++)
			{
				pheader->gl_texture[i][j & 3] = pheader->gl_texture[i][j - k]; 
			}
		}
	}

	return (void *)pskintype;
}

//==========================================================================
//
//	StripLength
//
//==========================================================================

static int StripLength(int starttri, int startv)
{
	used[starttri] = 2;

	mmesh1triangle_t* last = &triangles[starttri];

	stripverts[0] = last->vertindex[startv % 3];
	stripstverts[0] = last->stindex[startv % 3];

	stripverts[1] = last->vertindex[(startv + 1) % 3];
	stripstverts[1] = last->stindex[(startv + 1) % 3];

	stripverts[2] = last->vertindex[(startv + 2) % 3];
	stripstverts[2] = last->stindex[(startv + 2) % 3];

	striptris[0] = starttri;
	stripcount = 1;

	int m1 = last->vertindex[(startv + 2) % 3];
	int st1 = last->stindex[(startv + 2) % 3];
	int m2 = last->vertindex[(startv + 1) % 3];
	int st2 = last->stindex[(startv + 1) % 3];

	// look for a matching triangle
nexttri:
	mmesh1triangle_t* check = &triangles[starttri + 1];
	for (int j = starttri + 1; j < pheader->numtris; j++, check++)
	{
		if (check->facesfront != last->facesfront)
		{
			continue;
		}
		for (int k = 0; k < 3; k++)
		{
			if (check->vertindex[k] != m1)
			{
				continue;
			}
			if (check->stindex[k] != st1)
			{
				continue;
			}
			if (check->vertindex[(k + 1) % 3] != m2)
			{
				continue;
			}
			if (check->stindex[(k + 1) % 3] != st2)
			{
				continue;
			}

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
			{
				goto done;
			}

			// the new edge
			if (stripcount & 1)
			{
				m2 = check->vertindex[(k + 2) % 3];
				st2 = check->stindex[(k + 2) % 3];
			}
			else
			{
				m1 = check->vertindex[(k + 2) % 3];
				st1 = check->stindex[(k + 2) % 3];
			}

			stripverts[stripcount + 2] = check->vertindex[(k + 2) % 3];
			stripstverts[stripcount + 2] = check->stindex[(k + 2) % 3];
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (int j = starttri + 1; j < pheader->numtris; j++)
	{
		if (used[j] == 2)
		{
			used[j] = 0;
		}
	}

	return stripcount;
}

//==========================================================================
//
//	FanLength
//
//==========================================================================

static int FanLength(int starttri, int startv)
{
	used[starttri] = 2;

	mmesh1triangle_t* last = &triangles[starttri];

	stripverts[0] = last->vertindex[startv % 3];
	stripstverts[0] = last->stindex[startv % 3];

	stripverts[1] = last->vertindex[(startv + 1) % 3];
	stripstverts[1] = last->stindex[(startv + 1) % 3];

	stripverts[2] = last->vertindex[(startv + 2) % 3];
	stripstverts[2] = last->stindex[(startv + 2) % 3];

	striptris[0] = starttri;
	stripcount = 1;

	int m1 = last->vertindex[(startv + 0) % 3];
	int st1 = last->stindex[(startv + 2) % 3];
	int m2 = last->vertindex[(startv + 2) % 3];
	int st2 = last->stindex[(startv + 1) % 3];


	// look for a matching triangle
nexttri:
	mmesh1triangle_t* check = &triangles[starttri + 1];
	for (int j = starttri + 1; j < pheader->numtris; j++, check++)
	{
		if (check->facesfront != last->facesfront)
		{
			continue;
		}
		for (int k = 0; k < 3; k++)
		{
			if (check->vertindex[k] != m1)
			{
				continue;
			}
			if (check->stindex[k] != st1)
			{
				continue;
			}
			if (check->vertindex[(k + 1) % 3] != m2)
			{
				continue;
			}
			if (check->stindex[(k + 1) % 3] != st2)
			{
				continue;
			}

			// this is the next part of the fan

			// if we can't use this triangle, this tristrip is done
			if (used[j])
			{
				goto done;
			}

			// the new edge
			m2 = check->vertindex[(k + 2) % 3];
			st2 = check->stindex[(k + 2) % 3];

			stripverts[stripcount + 2] = m2;
			stripstverts[stripcount + 2] = st2;
			striptris[stripcount] = j;
			stripcount++;

			used[j] = 2;
			goto nexttri;
		}
	}
done:

	// clear the temp used flags
	for (int j = starttri + 1; j < pheader->numtris; j++)
	{
		if (used[j] == 2)
		{
			used[j] = 0;
		}
	}

	return stripcount;
}

//==========================================================================
//
//	BuildTris
//
//	Generate a list of trifans or strips for the model, which holds for all
// frames
//
//==========================================================================

static void BuildTris()
{
	//
	// build tristrips
	//
	numorder = 0;
	numcommands = 0;
	Com_Memset(used, 0, sizeof(used));
	for (int i = 0; i < pheader->numtris; i++)
	{
		// pick an unused triangle and start the trifan
		if (used[i])
		{
			continue;
		}

		int bestlen = 0;
		int besttype = 0;
		int bestverts[1024];
		int besttris[1024];
		int beststverts[1024];
		for (int type = 0; type < 2; type++)
		{
			for (int startv = 0; startv < 3 ; startv++)
			{
				int len;
				if (type == 1)
				{
					len = StripLength (i, startv);
				}
				else
				{
					len = FanLength (i, startv);
				}
				if (len > bestlen)
				{
					besttype = type;
					bestlen = len;
					for (int j = 0; j < bestlen + 2; j++)
					{
						beststverts[j] = stripstverts[j];
						bestverts[j] = stripverts[j];
					}
					for (int j = 0; j < bestlen; j++)
					{
						besttris[j] = striptris[j];
					}
				}
			}
		}

		// mark the tris on the best strip as used
		for (int j = 0; j < bestlen; j++)
		{
			used[besttris[j]] = 1;
		}

		if (besttype == 1)
		{
			commands[numcommands++] = (bestlen + 2);
		}
		else
		{
			commands[numcommands++] = -(bestlen + 2);
		}

		for (int j = 0; j < bestlen + 2; j++)
		{
			// emit a vertex into the reorder buffer
			int k = bestverts[j];
			vertexorder[numorder++] = k;

			k = beststverts[j];

			// emit s/t coords into the commands stream
			float s = stverts[k].s;
			float t = stverts[k].t;

			if (!triangles[besttris[0]].facesfront && stverts[k].onseam)
			{
				s += pheader->skinwidth / 2;	// on back side
			}
			s = (s + 0.5) / pheader->skinwidth;
			t = (t + 0.5) / pheader->skinheight;

			*(float*)&commands[numcommands++] = s;
			*(float*)&commands[numcommands++] = t;
		}
	}

	commands[numcommands++] = 0;		// end of list marker

	GLog.DWrite("%3i tri %3i vert %3i cmd\n", pheader->numtris, numorder, numcommands);
}

//==========================================================================
//
//	GL_MakeAliasModelDisplayLists
//
//==========================================================================

static void GL_MakeAliasModelDisplayLists(model_t* m, mesh1hdr_t* hdr)
{
	aliasmodel = m;
	paliashdr = hdr;	// (mesh1hdr_t *)Mod_Extradata (m);

	//
	// look for a cached version
	//
	char cache[MAX_QPATH];
	if (GGameType & GAME_Hexen2)
	{
		QStr::Cpy(cache, "glhexen/");
		QStr::StripExtension(m->name + QStr::Length("models/"), cache + QStr::Length("glhexen/"));
	}
	else
	{
		QStr::Cpy(cache, "glquake/");
		QStr::StripExtension(m->name + QStr::Length("progs/"), cache + QStr::Length("glquake/"));
	}
	QStr::Cat(cache, sizeof(cache), ".ms2");

	fileHandle_t f;
	FS_FOpenFileRead(cache, &f, true);
	if (f)
	{
		FS_Read(&numcommands, 4, f);
		FS_Read(&numorder, 4, f);
		FS_Read(&commands, numcommands * sizeof(commands[0]), f);
		FS_Read(&vertexorder, numorder * sizeof(vertexorder[0]), f);
		FS_FCloseFile(f);
	}
	else
	{
		//
		// build it from scratch
		//
		GLog.Write("meshing %s...\n",m->name);

		BuildTris();		// trifans or lists

		//
		// save out the cached version
		//
		f = FS_FOpenFileWrite(cache);
		if (f)
		{
			FS_Write(&numcommands, 4, f);
			FS_Write(&numorder, 4, f);
			FS_Write(&commands, numcommands * sizeof(commands[0]), f);
			FS_Write(&vertexorder, numorder * sizeof(vertexorder[0]), f);
			FS_FCloseFile(f);
		}
	}


	// save the data out

	paliashdr->poseverts = numorder;

	int* cmds = new int[numcommands];
	paliashdr->commands = cmds;
	Com_Memcpy(cmds, commands, numcommands * 4);

	dmdl_trivertx_t* verts = new dmdl_trivertx_t[paliashdr->numposes * paliashdr->poseverts];
	paliashdr->posedata = verts;
	for (int i = 0; i < paliashdr->numposes; i++)
	{
		for (int j = 0; j < numorder; j++)
		{
			*verts++ = poseverts[i][vertexorder[j]];
		}
	}
}

//==========================================================================
//
//	Mod_LoadMdlModel
//
//==========================================================================

void Mod_LoadMdlModel(model_t* mod, const void* buffer)
{
	mdl_t* pinmodel = (mdl_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MESH1_VERSION)
	{
		throw QException(va("%s has wrong version number (%i should be %i)",
			mod->name, version, MESH1_VERSION));
	}

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	int size = sizeof(mesh1hdr_t) + (LittleLong(pinmodel->numframes) - 1) * sizeof(pheader->frames[0]);
	pheader = (mesh1hdr_t*)Mem_Alloc(size);

	mod->q1_flags = LittleLong(pinmodel->flags);

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	pheader->boundingradius = LittleFloat(pinmodel->boundingradius);
	pheader->numskins = LittleLong(pinmodel->numskins);
	pheader->skinwidth = LittleLong(pinmodel->skinwidth);
	pheader->skinheight = LittleLong(pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
	{
		throw QException(va("model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT));
	}

	pheader->numverts = LittleLong(pinmodel->numverts);

	if (pheader->numverts <= 0)
	{
		throw QException(va("model %s has no vertices", mod->name));
	}

	if (pheader->numverts > MAXALIASVERTS)
	{
		throw QException(va("model %s has too many vertices", mod->name));
	}

	pheader->numtris = LittleLong(pinmodel->numtris);

	if (pheader->numtris <= 0)
	{
		throw QException(va("model %s has no triangles", mod->name));
	}

	pheader->numframes = LittleLong (pinmodel->numframes);
	int numframes = pheader->numframes;
	if (numframes < 1)
	{
		throw QException(va("Mod_LoadMdlModel: Invalid # of frames: %d\n", numframes));
	}

	pheader->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->q1_synctype = (synctype_t)LittleLong (pinmodel->synctype);
	mod->q1_numframes = pheader->numframes;

	for (int i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat(pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
	}

	//
	// load the skins
	//
	dmdl_skintype_t* pskintype = (dmdl_skintype_t*)&pinmodel[1];
	pskintype = (dmdl_skintype_t*)Mod_LoadAllSkins(pheader->numskins, pskintype, mod->q1_flags);

	//
	// load base s and t vertices
	//
	dmdl_stvert_t* pinstverts = (dmdl_stvert_t*)pskintype;
	for (int i = 0; i < pheader->numverts; i++)
	{
		stverts[i].onseam = LittleLong(pinstverts[i].onseam);
		stverts[i].s = LittleLong(pinstverts[i].s);
		stverts[i].t = LittleLong(pinstverts[i].t);
	}

	//
	// load triangle lists
	//
	dmdl_triangle_t* pintriangles = (dmdl_triangle_t*)&pinstverts[pheader->numverts];

	for (int i = 0; i < pheader->numtris; i++)
	{
		triangles[i].facesfront = LittleLong(pintriangles[i].facesfront);

		for (int j = 0; j < 3; j++)
		{
			triangles[i].vertindex[j] =	LittleLong(pintriangles[i].vertindex[j]);
			triangles[i].stindex[j]	  = triangles[i].vertindex[j];
		}
	}

	//
	// load the frames
	//
	posenum = 0;
	dmdl_frametype_t* pframetype = (dmdl_frametype_t*)&pintriangles[pheader->numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;

	for (int i = 0; i < numframes; i++)
	{
		mdl_frametype_t frametype = (mdl_frametype_t)LittleLong(pframetype->type);
		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (dmdl_frametype_t*)Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]);
		}
		else
		{
			pframetype = (dmdl_frametype_t*)Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]);
		}
	}

	pheader->numposes = posenum;

	mod->type = MOD_MESH1;

	// FIXME: do this right
	if (GGameType & GAME_Hexen2)
	{
		mod->q1_mins[0] = mins[0] - 10;
		mod->q1_mins[1] = mins[1] - 10;
		mod->q1_mins[2] = mins[2] - 10;
		mod->q1_maxs[0] = maxs[0] + 10;
		mod->q1_maxs[1] = maxs[1] + 10;
		mod->q1_maxs[2] = maxs[2] + 10;
	}
	else
	{
		mod->q1_mins[0] = mod->q1_mins[1] = mod->q1_mins[2] = -16;
		mod->q1_maxs[0] = mod->q1_maxs[1] = mod->q1_maxs[2] = 16;
	}

	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists(mod, pheader);

	mod->q1_cache = pheader;
}

//==========================================================================
//
//	Mod_LoadMdlModelNew
//
//	Reads extra field for num ST verts, and extra index list of them
//
//==========================================================================

void Mod_LoadMdlModelNew(model_t* mod, const void* buffer)
{
	newmdl_t* pinmodel = (newmdl_t*)buffer;

	int version = LittleLong(pinmodel->version);
	if (version != MESH1_NEWVERSION)
	{
		throw QException(va("%s has wrong version number (%i should be %i)",
			mod->name, version, MESH1_NEWVERSION));
	}

	//
	// allocate space for a working header, plus all the data except the frames,
	// skin and group info
	//
	int size = sizeof(mesh1hdr_t) + (LittleLong(pinmodel->numframes) - 1) * sizeof(pheader->frames[0]);
	pheader = (mesh1hdr_t*)Mem_Alloc(size);

	mod->q1_flags = LittleLong (pinmodel->flags);

	//
	// endian-adjust and copy the data, starting with the alias model header
	//
	pheader->boundingradius = LittleFloat(pinmodel->boundingradius);
	pheader->numskins = LittleLong(pinmodel->numskins);
	pheader->skinwidth = LittleLong(pinmodel->skinwidth);
	pheader->skinheight = LittleLong(pinmodel->skinheight);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
	{
		throw QException(va("model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT));
	}

	pheader->numverts = LittleLong(pinmodel->numverts);
	pheader->version = LittleLong(pinmodel->num_st_verts);	//hide num_st in version
	
	if (pheader->numverts <= 0)
	{
		throw QException(va("model %s has no vertices", mod->name));
	}

	if (pheader->numverts > MAXALIASVERTS)
	{
		throw QException(va("model %s has too many vertices", mod->name));
	}

	pheader->numtris = LittleLong(pinmodel->numtris);

	if (pheader->numtris <= 0)
	{
		throw QException(va("model %s has no triangles", mod->name));
	}

	pheader->numframes = LittleLong(pinmodel->numframes);
	int numframes = pheader->numframes;
	if (numframes < 1)
	{
		throw QException(va("Mod_LoadMdlModel: Invalid # of frames: %d\n", numframes));
	}

	pheader->size = LittleFloat(pinmodel->size) * ALIAS_BASE_SIZE_RATIO;
	mod->q1_synctype = (synctype_t)LittleLong(pinmodel->synctype);
	mod->q1_numframes = pheader->numframes;

	for (int i = 0; i < 3; i++)
	{
		pheader->scale[i] = LittleFloat(pinmodel->scale[i]);
		pheader->scale_origin[i] = LittleFloat(pinmodel->scale_origin[i]);
		pheader->eyeposition[i] = LittleFloat(pinmodel->eyeposition[i]);
	}

	//
	// load the skins
	//
	dmdl_skintype_t* pskintype = (dmdl_skintype_t*)&pinmodel[1];
	pskintype = (dmdl_skintype_t*)Mod_LoadAllSkins(pheader->numskins, pskintype, mod->q1_flags);

	//
	// load base s and t vertices
	//
	dmdl_stvert_t* pinstverts = (dmdl_stvert_t*)pskintype;

	for (int i = 0; i < pheader->version; i++)	//version holds num_st_verts
	{
		stverts[i].onseam = LittleLong(pinstverts[i].onseam);
		stverts[i].s = LittleLong(pinstverts[i].s);
		stverts[i].t = LittleLong(pinstverts[i].t);
	}

	//
	// load triangle lists
	//
	dmdl_newtriangle_t* pintriangles = (dmdl_newtriangle_t*)&pinstverts[pheader->version];

	for (int i = 0; i < pheader->numtris; i++)
	{
		triangles[i].facesfront = LittleLong(pintriangles[i].facesfront);

		for (int j = 0; j < 3; j++)
		{
			triangles[i].vertindex[j] = LittleShort(pintriangles[i].vertindex[j]);
			triangles[i].stindex[j] = LittleShort(pintriangles[i].stindex[j]);
		}
	}

	//
	// load the frames
	//
	posenum = 0;
	dmdl_frametype_t* pframetype = (dmdl_frametype_t*)&pintriangles[pheader->numtris];

	mins[0] = mins[1] = mins[2] = 32768;
	maxs[0] = maxs[1] = maxs[2] = -32768;

	for (int i = 0; i < numframes; i++)
	{
		mdl_frametype_t frametype;

		frametype = (mdl_frametype_t)LittleLong(pframetype->type);

		if (frametype == ALIAS_SINGLE)
		{
			pframetype = (dmdl_frametype_t*)Mod_LoadAliasFrame(pframetype + 1, &pheader->frames[i]);
		}
		else
		{
			pframetype = (dmdl_frametype_t*)Mod_LoadAliasGroup(pframetype + 1, &pheader->frames[i]);
		}
	}

	pheader->numposes = posenum;

	mod->type = MOD_MESH1;

	mod->q1_mins[0] = mins[0] - 10;
	mod->q1_mins[1] = mins[1] - 10;
	mod->q1_mins[2] = mins[2] - 10;
	mod->q1_maxs[0] = maxs[0] + 10;
	mod->q1_maxs[1] = maxs[1] + 10;
	mod->q1_maxs[2] = maxs[2] + 10;


	//
	// build the draw lists
	//
	GL_MakeAliasModelDisplayLists(mod, pheader);

	mod->q1_cache = pheader;
}

//==========================================================================
//
//	Mod_FreeMdlModel
//
//==========================================================================

void Mod_FreeMdlModel(model_t* mod)
{
	mesh1hdr_t* pheader = (mesh1hdr_t*)mod->q1_cache;
	delete[] pheader->commands;
	delete[] pheader->posedata;
	Mem_Free(pheader);
}