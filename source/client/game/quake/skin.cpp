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

Cvar* clqw_baseskin;
Cvar* clqw_noskins;

//	Returns a pointer to the skin bitmap, or NULL to use the default
byte* CLQW_SkinCache(qw_skin_t* skin)
{
	if (clc_common->downloadType == dl_skin)
	{
		return NULL;		// use base until downloaded
	}

	if (clqw_noskins->value == 1) // JACK: So NOSKINS > 1 will show skins, but
	{
		return NULL;	  // not download new ones.
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
		Log::write("Couldn't load skin %s\n", name);
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
		Log::write("Skin %s was malformed.  You should delete it.\n", name);
		return NULL;
	}

	skin->data = out;
	skin->failedload = false;

	return out;
}
