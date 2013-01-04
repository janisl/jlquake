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

#include "local.h"
#include "../../client_main.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"

#define MAX_CACHED_SKINS        128

Cvar* clqw_baseskin;
Cvar* clqw_noskins;

static char allskins[128];
static qw_skin_t skins[MAX_CACHED_SKINS];
static int numskins;

//  Determines the best skin for the given scoreboard
// slot, and sets scoreboard->skin
void CLQW_SkinFind(q1player_info_t* sc)
{
	char name[128];
	if (allskins[0])
	{
		String::Cpy(name, allskins);
	}
	else
	{
		const char* s = Info_ValueForKey(sc->userinfo, "skin");
		if (s && s[0])
		{
			String::Cpy(name, s);
		}
		else
		{
			String::Cpy(name, clqw_baseskin->string);
		}
	}

	if (strstr(name, "..") || *name == '.')
	{
		String::Cpy(name, "base");
	}

	String::StripExtension(name, name);

	for (int i = 0; i < numskins; i++)
	{
		if (!String::Cmp(name, skins[i].name))
		{
			sc->skin = &skins[i];
			CLQW_SkinCache(sc->skin);
			return;
		}
	}

	if (numskins == MAX_CACHED_SKINS)
	{
		// ran out of spots, so flush everything
		CLQW_SkinSkins_f();
		return;
	}

	qw_skin_t* skin = &skins[numskins];
	sc->skin = skin;
	numskins++;

	Com_Memset(skin, 0, sizeof(*skin));
	String::NCpy(skin->name, name, sizeof(skin->name) - 1);
}

//	Returns a pointer to the skin bitmap, or NULL to use the default
byte* CLQW_SkinCache(qw_skin_t* skin)
{
	if (clc.downloadType == dl_skin)
	{
		return NULL;		// use base until downloaded
	}

	if (clqw_noskins->value == 1)	// JACK: So NOSKINS > 1 will show skins, but
	{
		return NULL;		// not download new ones.
	}

	if (skin->failedload)
	{
		return NULL;
	}

	byte* out = skin->data;
	if (out)
	{
		return out;
	}

	//
	// load the pic from disk
	//
	char name[1024];
	sprintf(name, "skins/%s.pcx", skin->name);
	if (!FS_FOpenFileRead(name, NULL, false))
	{
		common->Printf("Couldn't load skin %s\n", name);
		sprintf(name, "skins/%s.pcx", clqw_baseskin->string);
		if (!FS_FOpenFileRead(name, NULL, false))
		{
			skin->failedload = true;
			return NULL;
		}
	}

	out = R_LoadQuakeWorldSkinData(name);

	if (!out)
	{
		skin->failedload = true;
		common->Printf("Skin %s was malformed.  You should delete it.\n", name);
		return NULL;
	}

	skin->data = out;
	skin->failedload = false;

	return out;
}

void CLQW_SkinNextDownload()
{
	q1player_info_t* sc;
	int i;

	if (clc.downloadNumber == 0)
	{
		common->Printf("Checking skins...\n");
	}
	clc.downloadType = dl_skin;

	for (; clc.downloadNumber != MAX_CLIENTS_QHW; clc.downloadNumber++)
	{
		sc = &cl.q1_players[clc.downloadNumber];
		if (!sc->name[0])
		{
			continue;
		}
		CLQW_SkinFind(sc);
		if (clqw_noskins->value)
		{
			continue;
		}
		if (!CL_CheckOrDownloadFile(va("skins/%s.pcx", sc->skin->name)))
		{
			return;		// started a download
		}
	}

	clc.downloadType = dl_none;

	// now load them in for real
	for (i = 0; i < MAX_CLIENTS_QHW; i++)
	{
		sc = &cl.q1_players[i];
		if (!sc->name[0])
		{
			continue;
		}
		CLQW_SkinCache(sc->skin);
		sc->skin = NULL;
	}

	if (cls.state != CA_ACTIVE)
	{
		// get next signon phase
		CL_AddReliableCommand(va("begin %i", cl.servercount));
	}
}

//	Refind all skins, downloading if needed.
void CLQW_SkinSkins_f()
{
	for (int i = 0; i < numskins; i++)
	{
		if (skins[i].data)
		{
			delete[] skins[i].data;
			skins[i].data = NULL;
		}
	}
	numskins = 0;

	clc.downloadNumber = 0;
	clc.downloadType = dl_skin;
	CLQW_SkinNextDownload();
}

//	Sets all skins to one specific one
void CLQW_SkinAllSkins_f()
{
	String::Cpy(allskins, Cmd_Argv(1));
	CLQW_SkinSkins_f();
}
