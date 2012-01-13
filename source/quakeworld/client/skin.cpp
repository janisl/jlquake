/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "quakedef.h"

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

void CLQW_SkinNextDownload()
{
	q1player_info_t	*sc;
	int			i;

	if (clc_common->downloadNumber == 0)
	{
		Log::write("Checking skins...\n");
	}
	clc_common->downloadType = dl_skin;

	for (; clc_common->downloadNumber != MAX_CLIENTS_QW; clc_common->downloadNumber++)
	{
		sc = &cl_common->q1_players[clc_common->downloadNumber];
		if (!sc->name[0])
		{
			continue;
		}
		CLQW_SkinFind(sc);
		if (clqw_noskins->value)
		{
			continue;
		}
		if (!CLQW_CheckOrDownloadFile(va("skins/%s.pcx", sc->skin->name)))
		{
			return;		// started a download
		}
	}

	clc_common->downloadType = dl_none;

	// now load them in for real
	for (i = 0; i < MAX_CLIENTS_QW; i++)
	{
		sc = &cl_common->q1_players[i];
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
		clc_common->netchan.message.WriteByte(q1clc_stringcmd);
		clc_common->netchan.message.WriteString2(va("begin %i", cl.servercount));
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

	clc_common->downloadNumber = 0;
	clc_common->downloadType = dl_skin;
	CLQW_SkinNextDownload();
}

//	Sets all skins to one specific one
void CLQW_SkinAllSkins_f()
{
	String::Cpy(allskins, Cmd_Argv(1));
	CLQW_SkinSkins_f ();
}
