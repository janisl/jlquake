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

#ifndef _BSP47FILE_H
#define _BSP47FILE_H

//	File structure didn't change
#include "bsp46.h"

#define BSP47_VERSION         47

enum
{
	//	New in enemy territory.
	BSP47MST_FOLIAGE = 5
};

#define BSP47SURF_CERAMIC            0x00000040	// out of surf's, so replacing unused 'BSP46SURF_FLESH'
#define BSP47SURF_SPLASH             0x00000040	// out of surf's, so replacing unused 'BSP46SURF_FLESH' - and as BSP47SURF_CERAMIC wasn't used, it's now BSP47SURF_SPLASH
#define BSP47SURF_WOOD               0x00040000
#define BSP47SURF_GRASS              0x00080000
#define BSP47SURF_GRAVEL             0x00100000
#define BSP47SURF_GLASS              0x00200000	// out of surf's, so replacing unused 'SURF_SMGROUP'
#define BSP47SURF_SNOW               0x00400000
#define BSP47SURF_ROOF               0x00800000
#define BSP47SURF_RUBBLE             0x01000000
#define BSP47SURF_CARPET             0x02000000
#define BSP47SURF_MONSTERSLICK       0x04000000	// slick surf that only affects ai's
#define BSP47SURF_MONSLICK_W         0x08000000
#define BSP47SURF_MONSLICK_N         0x10000000
#define BSP47SURF_MONSLICK_E         0x20000000
#define BSP47SURF_MONSLICK_S         0x40000000
#define BSP47SURF_LANDMINE           0x80000000	// ydnar: ok to place landmines on this surface

#endif
