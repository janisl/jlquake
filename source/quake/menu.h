/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "../client/game/quake_hexen2/menu.h"

#define MNET_TCP        2

extern int m_activenet;

//
// menus
//
void M_Init(void);
void M_Keydown(int key);
void M_CharEvent(int key);
void M_Draw(void);
void M_ToggleMenu_f(void);

extern image_t* translate_texture;
