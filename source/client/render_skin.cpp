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

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	CommaParse
//
//	This is unfortunate, but the skin files aren't compatable with our normal
// parsing rules.
//
//==========================================================================

static char* CommaParse(char** data_p)
{
	int c = 0, len;
	char *data;
	static	char	com_token[MAX_TOKEN_CHARS_Q3];

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	// make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	while (1)
	{
		// skip whitespace
		while ((c = *data) <= ' ')
		{
			if (!c)
			{
				break;
			}
			data++;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
			{
				data++;
			}
		}
		// skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			while (*data && (*data != '*' || data[1] != '/'))
			{
				data++;
			}
			if (*data)
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if (c == 0)
	{
		return "";
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS_Q3)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS_Q3)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	}
	while (c > 32 && c != ',');

	if (len == MAX_TOKEN_CHARS_Q3)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS_Q3);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

//==========================================================================
//
//	R_RegisterSkin
//
//==========================================================================

qhandle_t R_RegisterSkin(const char* name)
{
	if (!name || !name[0])
	{
		GLog.Write("Empty name passed to R_RegisterSkin\n");
		return 0;
	}

	if (String::Length(name) >= MAX_QPATH)
	{
		GLog.Write("Skin name exceeds MAX_QPATH\n");
		return 0;
	}

	// see if the skin is already loaded
	qhandle_t hSkin;
	for (hSkin = 1; hSkin < tr.numSkins; hSkin++)
	{
		skin_t* skin = tr.skins[hSkin];
		if (!String::ICmp(skin->name, name))
		{
			if (skin->numSurfaces == 0)
			{
				return 0;		// default skin
			}
			return hSkin;
		}
	}

	// allocate a new skin
	if (tr.numSkins == MAX_SKINS)
	{
		GLog.Write(S_COLOR_YELLOW "WARNING: R_RegisterSkin( '%s' ) MAX_SKINS hit\n", name);
		return 0;
	}
	tr.numSkins++;
	skin_t* skin = new skin_t;
	Com_Memset(skin, 0, sizeof(skin_t));
	tr.skins[hSkin] = skin;
	String::NCpyZ(skin->name, name, sizeof(skin->name));
	skin->numSurfaces = 0;

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// If not a .skin file, load as a single shader
	if (String::Cmp(name + String::Length(name) - 5, ".skin"))
	{
		skin->numSurfaces = 1;
		skin->surfaces[0] = new skinSurface_t;
		skin->surfaces[0]->name[0] = 0;
		skin->surfaces[0]->shader = R_FindShader(name, LIGHTMAP_NONE, true);
		return hSkin;
	}

	// load and parse the skin file
	char* text;
    FS_ReadFile(name, (void**)&text);
	if (!text)
	{
		return 0;
	}

	char* text_p = text;
	while (text_p && *text_p)
	{
		// get surface name
		char* token = CommaParse(&text_p);
		char surfName[MAX_QPATH];
		String::NCpyZ(surfName, token, sizeof(surfName));

		if (!token[0])
		{
			break;
		}
		// lowercase the surface name so skin compares are faster
		String::ToLower(surfName);

		if (*text_p == ',')
		{
			text_p++;
		}

		if (strstr(token, "tag_"))
		{
			continue;
		}

		// parse the shader name
		token = CommaParse(&text_p);

		skinSurface_t* surf = new skinSurface_t;
		skin->surfaces[skin->numSurfaces] = surf;
		String::NCpyZ(surf->name, surfName, sizeof(surf->name));
		surf->shader = R_FindShader(token, LIGHTMAP_NONE, true);
		skin->numSurfaces++;
	}

	FS_FreeFile(text);

	// never let a skin have 0 shaders
	if (skin->numSurfaces == 0)
	{
		return 0;		// use default skin
	}

	return hSkin;
}

//==========================================================================
//
//	R_InitSkins
//
//==========================================================================

void R_InitSkins()
{
	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin_t* skin = tr.skins[0] = new skin_t;
	Com_Memset(skin, 0, sizeof(skin_t));
	String::NCpyZ(skin->name, "<default skin>", sizeof(skin->name));
	skin->numSurfaces = 1;
	skin->surfaces[0] = new skinSurface_t;
	skin->surfaces[0]->name[0] = 0;
	skin->surfaces[0]->shader = tr.defaultShader;
}

//==========================================================================
//
//	R_GetSkinByHandle
//
//==========================================================================

skin_t* R_GetSkinByHandle(qhandle_t hSkin)
{
	if (hSkin < 1 || hSkin >= tr.numSkins)
	{
		return tr.skins[0];
	}
	return tr.skins[hSkin];
}

//==========================================================================
//
//	R_SkinList_f
//
//==========================================================================

void R_SkinList_f()
{
	GLog.Write("------------------\n");

	for (int i = 0; i < tr.numSkins; i++)
	{
		skin_t* skin = tr.skins[i];

		GLog.Write("%3i:%s\n", i, skin->name);
		for (int j = 0; j < skin->numSurfaces; j++)
		{
			GLog.Write("       %s = %s\n", skin->surfaces[j]->name, skin->surfaces[j]->shader->name);
		}
	}
	GLog.Write("------------------\n");
}

//==========================================================================
//
//	R_RegisterSkinQ2
//
//==========================================================================

image_t* R_RegisterSkinQ2(const char* name)
{
	return R_FindImageFile(name, true, true, GL_CLAMP, false, IMG8MODE_Skin);
}

//==========================================================================
//
//	R_CreateOrUpdateTranslatedModelSkin
//
//==========================================================================

static void R_CreateOrUpdateTranslatedModelSkin(image_t*& image, const char* name, qhandle_t modelHandle, byte* pixels, byte *translation)
{
	if (!modelHandle)
	{
		return;
	}
	model_t* model = R_GetModelByHandle(modelHandle);
	if (model->type != MOD_MESH1)
	{
		// only translate skins on alias models
		return;
	}

	mesh1hdr_t* mesh1Header = (mesh1hdr_t*)model->q1_cache;
	int width = mesh1Header->skinwidth;
	int height = mesh1Header->skinheight;

	R_CreateOrUpdateTranslatedSkin(image, name, pixels, translation, width, height);
}

//==========================================================================
//
//	R_CreateOrUpdateTranslatedModelSkinQ1
//
//==========================================================================

void R_CreateOrUpdateTranslatedModelSkinQ1(image_t*& image, const char* name, qhandle_t modelHandle, byte *translation)
{
	R_CreateOrUpdateTranslatedModelSkin(image, name, modelHandle, q1_player_8bit_texels, translation);
}

//==========================================================================
//
//	R_CreateOrUpdateTranslatedModelSkinH2
//
//==========================================================================

void R_CreateOrUpdateTranslatedModelSkinH2(image_t*& image, const char* name, qhandle_t modelHandle, byte *translation, int classIndex)
{
	R_CreateOrUpdateTranslatedModelSkin(image, name, modelHandle, h2_player_8bit_texels[classIndex], translation);
}

//==========================================================================
//
//	R_LoadQuakeWorldSkinData
//
//==========================================================================

byte* R_LoadQuakeWorldSkinData(const char* name)
{
	int width;
	int height;
	byte* pixels;
	R_LoadPCX(name, &pixels, NULL, &width, &height);
	if (!pixels)
	{
		return NULL;
	}

	byte* out = new byte[320 * 200];
	Com_Memset(out, 0, 320 * 200);

	byte* outp = out;
	byte* pixp = pixels;
	for (int y = 0; y < height; y++, outp += 320, pixp += width)
	{
		Com_Memcpy(outp, pixp, width);
	}
	delete[] pixels;

	return out;
}
