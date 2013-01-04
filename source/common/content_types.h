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

#ifndef __CONTENT_TYPES_H__
#define __CONTENT_TYPES_H__

//==========================================================================
//
//	Quake content types
//
//==========================================================================

#define BSP29CONTENTS_EMPTY     -1
#define BSP29CONTENTS_SOLID     -2
#define BSP29CONTENTS_WATER     -3
#define BSP29CONTENTS_SLIME     -4
#define BSP29CONTENTS_LAVA      -5
#define BSP29CONTENTS_SKY       -6
#define BSP29CONTENTS_ORIGIN    -7		// removed at csg time
#define BSP29CONTENTS_CLIP      -8		// changed to contents_solid

#define BSP29CONTENTS_CURRENT_0     -9
#define BSP29CONTENTS_CURRENT_90    -10
#define BSP29CONTENTS_CURRENT_180   -11
#define BSP29CONTENTS_CURRENT_270   -12
#define BSP29CONTENTS_CURRENT_UP    -13
#define BSP29CONTENTS_CURRENT_DOWN  -14

//==========================================================================
//
//	Quake 2 content types
//
//==========================================================================

// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely
#define BSP38CONTENTS_SOLID         1		// an eye is never valid in a solid
#define BSP38CONTENTS_WINDOW        2		// translucent, but not watery
#define BSP38CONTENTS_AUX           4
#define BSP38CONTENTS_LAVA          8
#define BSP38CONTENTS_SLIME         16
#define BSP38CONTENTS_WATER         32
#define BSP38CONTENTS_MIST          64
#define BSP38_LAST_VISIBLE_CONTENTS 64

// remaining contents are non-visible, and don't eat brushes

#define BSP38CONTENTS_AREAPORTAL    0x8000

#define BSP38CONTENTS_PLAYERCLIP    0x10000
#define BSP38CONTENTS_MONSTERCLIP   0x20000

// currents can be added to any other contents, and may be mixed
#define BSP38CONTENTS_CURRENT_0     0x40000
#define BSP38CONTENTS_CURRENT_90    0x80000
#define BSP38CONTENTS_CURRENT_180   0x100000
#define BSP38CONTENTS_CURRENT_270   0x200000
#define BSP38CONTENTS_CURRENT_UP    0x400000
#define BSP38CONTENTS_CURRENT_DOWN  0x800000

#define BSP38CONTENTS_ORIGIN        0x1000000	// removed before bsping an entity

#define BSP38CONTENTS_MONSTER       0x2000000	// should never be on a brush, only in game
#define BSP38CONTENTS_DEADMONSTER   0x4000000
#define BSP38CONTENTS_DETAIL        0x8000000	// brushes to be added after vis leafs
#define BSP38CONTENTS_TRANSLUCENT   0x10000000	// auto set if any surface has trans
#define BSP38CONTENTS_LADDER        0x20000000

//==========================================================================
//
//	Quake 3 content types
//
//==========================================================================

// contents flags are seperate bits
// a given brush can contribute multiple content bits

#define BSP46CONTENTS_SOLID         1		// an eye is never valid in a solid
#define BSP46CONTENTS_LAVA          8
#define BSP46CONTENTS_SLIME         16
#define BSP46CONTENTS_WATER         32
#define BSP46CONTENTS_FOG           64

#define BSP46CONTENTS_NOTTEAM1      0x0080
#define BSP46CONTENTS_NOTTEAM2      0x0100
#define BSP46CONTENTS_NOBOTCLIP     0x0200

#define BSP46CONTENTS_AREAPORTAL    0x8000

#define BSP46CONTENTS_PLAYERCLIP    0x10000
#define BSP46CONTENTS_MONSTERCLIP   0x20000
//bot specific contents types
#define BSP46CONTENTS_TELEPORTER    0x40000
#define BSP46CONTENTS_JUMPPAD       0x80000
#define BSP46CONTENTS_CLUSTERPORTAL 0x100000
#define BSP46CONTENTS_DONOTENTER    0x200000
#define BSP46CONTENTS_BOTCLIP       0x400000
#define BSP46CONTENTS_MOVER         0x800000

#define BSP46CONTENTS_ORIGIN        0x1000000	// removed before bsping an entity

#define BSP46CONTENTS_BODY          0x2000000	// should never be on a brush, only in game
#define BSP46CONTENTS_CORPSE        0x4000000
#define BSP46CONTENTS_DETAIL        0x8000000	// brushes not used for the bsp
#define BSP46CONTENTS_STRUCTURAL    0x10000000	// brushes used for the bsp
#define BSP46CONTENTS_TRANSLUCENT   0x20000000	// don't consume surface fragments inside
#define BSP46CONTENTS_TRIGGER       0x40000000
#define BSP46CONTENTS_NODROP        0x80000000	// don't leave bodies or items (death fog, lava)

//==========================================================================
//
//	Wolfenstein content types, in addition to Quake 3 ones
//
//==========================================================================

#define BSP47CONTENTS_LIGHTGRID          0x00000004
#define BSP47CONTENTS_MISSILECLIP        0x00000080
#define BSP47CONTENTS_ITEM               0x00000100
#define BSP47CONTENTS_MOVER              0x00004000
#define BSP47CONTENTS_DONOTENTER_LARGE   0x00400000

//	Only in RTCW single player.
#define BSP47CONTENTS_AI_NOSIGHT     0x1000	// AI cannot see through this
#define BSP47CONTENTS_CLIPSHOT       0x2000	// bullets hit this

#endif
