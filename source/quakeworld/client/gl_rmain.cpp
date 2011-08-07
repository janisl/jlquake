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
// r_main.c

#include "quakedef.h"
#include "glquake.h"

//
// screen size info
//
refdef_t	r_refdef;

QCvar*	gl_nocolors;
QCvar*	gl_reporttjunctions;

extern	QCvar*	scr_fov;

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

void R_HandlePlayerSkin(refEntity_t* Ent, int PlayerNum)
{
	// we can't dynamically colormap textures, so they are cached
	// seperately for the players.  Heads are just uncolored.
	if (!cl.players[PlayerNum].skin)
	{
		Skin_Find(&cl.players[PlayerNum]);
		R_TranslatePlayerSkin(PlayerNum);
	}
	Ent->customSkin = R_GetImageHandle(playertextures[PlayerNum]);
}
