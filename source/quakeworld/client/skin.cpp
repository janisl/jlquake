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

char		allskins[128];
#define	MAX_CACHED_SKINS		128
qw_skin_t		skins[MAX_CACHED_SKINS];
int			numskins;

/*
================
Skin_Find

  Determines the best skin for the given scoreboard
  slot, and sets scoreboard->skin

================
*/
void Skin_Find (q1player_info_t *sc)
{
	qw_skin_t		*skin;
	int			i;
	char		name[128];
	const char*	s;

	if (allskins[0])
		String::Cpy(name, allskins);
	else
	{
		s = Info_ValueForKey (sc->userinfo, "skin");
		if (s && s[0])
			String::Cpy(name, s);
		else
			String::Cpy(name, clqw_baseskin->string);
	}

	if (strstr (name, "..") || *name == '.')
		String::Cpy(name, "base");

	String::StripExtension (name, name);

	for (i=0 ; i<numskins ; i++)
	{
		if (!String::Cmp(name, skins[i].name))
		{
			sc->skin = &skins[i];
			CLQW_SkinCache (sc->skin);
			return;
		}
	}

	if (numskins == MAX_CACHED_SKINS)
	{	// ran out of spots, so flush everything
		Skin_Skins_f ();
		return;
	}

	skin = &skins[numskins];
	sc->skin = skin;
	numskins++;

	Com_Memset(skin, 0, sizeof(*skin));
	String::NCpy(skin->name, name, sizeof(skin->name) - 1);
}


/*
=================
Skin_NextDownload
=================
*/
void Skin_NextDownload (void)
{
	q1player_info_t	*sc;
	int			i;

	if (clc.downloadNumber == 0)
		Con_Printf ("Checking skins...\n");
	clc.downloadType = dl_skin;

	for ( 
		; clc.downloadNumber != MAX_CLIENTS_QW
		; clc.downloadNumber++)
	{
		sc = &cl.q1_players[clc.downloadNumber];
		if (!sc->name[0])
			continue;
		Skin_Find (sc);
		if (clqw_noskins->value)
			continue;
		if (!CLQW_CheckOrDownloadFile(va("skins/%s.pcx", sc->skin->name)))
			return;		// started a download
	}

	clc.downloadType = dl_none;

	// now load them in for real
	for (i=0 ; i<MAX_CLIENTS_QW ; i++)
	{
		sc = &cl.q1_players[i];
		if (!sc->name[0])
			continue;
		CLQW_SkinCache (sc->skin);
		sc->skin = NULL;
	}

	if (cls.state != ca_active)
	{	// get next signon phase
		clc.netchan.message.WriteByte(q1clc_stringcmd);
		clc.netchan.message.WriteString2(va("begin %i", cl.servercount));
	}
}


/*
==========
Skin_Skins_f

Refind all skins, downloading if needed.
==========
*/
void	Skin_Skins_f (void)
{
	int		i;

	for (i=0 ; i<numskins ; i++)
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
	Skin_NextDownload ();
}


/*
==========
Skin_AllSkins_f

Sets all skins to one specific one
==========
*/
void	Skin_AllSkins_f (void)
{
	String::Cpy(allskins, Cmd_Argv(1));
	Skin_Skins_f ();
}
