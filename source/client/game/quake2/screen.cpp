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

void SCRQ2_DrawPause()
{
	if (!scr_showpause->value)		// turn off for screenshots
	{
		return;
	}

	if (!cl_paused->value)
	{
		return;
	}

	int w, h;
	R_GetPicSize(&w, &h, "pause");
	UI_DrawNamedPic((viddef.width - w) / 2, viddef.height / 2 + 8, "pause");
}

void SCRQ2_DrawLoading()
{
	if (!scr_draw_loading)
	{
		return;
	}

	scr_draw_loading = false;
	int w, h;
	R_GetPicSize(&w, &h, "loading");
	UI_DrawNamedPic((viddef.width - w) / 2, (viddef.height - h) / 2, "loading");
}
