/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "client.h"
#include "../../client/sound/local.h"

sfx_t *S_RegisterSexedSound(int entnum, char *base)
{
	int				n;
	char			*p;
	sfx_t*			sfx;
	fileHandle_t	f;
	char			model[MAX_QPATH];
	char			sexedFilename[MAX_QPATH];
	char			maleFilename[MAX_QPATH];

	// determine what model the client is using
	model[0] = 0;
	q2entity_state_t *ent = &clq2_entities[entnum].current;
	n = CS_PLAYERSKINS + ent->number - 1;
	if (cl.configstrings[n][0])
	{
		p = strchr(cl.configstrings[n], '\\');
		if (p)
		{
			p += 1;
			String::Cpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}
	// if we can't figure it out, they're male
	if (!model[0])
		String::Cpy(model, "male");

	// see if we already know of the model specific sound
	String::Sprintf (sexedFilename, sizeof(sexedFilename), "#players/%s/%s", model, base+1);
	sfx = S_FindName (sexedFilename, false);

	if (!sfx)
	{
		// no, so see if it exists
		FS_FOpenFileRead(&sexedFilename[1], &f, false);
		if (f)
		{
			// yes, close the file and register it
			FS_FCloseFile (f);
			sfx = s_knownSfx + S_RegisterSound (sexedFilename);
		}
		else
		{
			// no, revert to the male sound in the pak0.pak
			String::Sprintf (maleFilename, sizeof(maleFilename), "player/%s/%s", "male", base+1);
			sfx = S_AliasName (sexedFilename, maleFilename);
		}
	}

	return sfx;
}

int S_GetClFrameServertime()
{
	return cl.q2_frame.servertime;
}

bool S_GetDisableScreen()
{
	return !!cls.disable_screen;
}
