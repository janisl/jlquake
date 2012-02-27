/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// This file must be identical in the quake and utils directories






// contents flags are seperate bits
// a given brush can contribute multiple content bits

// these definitions also need to be in q_shared.h!

#define CONTENTS_SOLID              0x00000001
#define CONTENTS_LIGHTGRID          0x00000004
#define CONTENTS_LAVA               0x00000008
#define CONTENTS_SLIME              0x00000010
#define CONTENTS_WATER              0x00000020
#define CONTENTS_FOG                0x00000040
#define CONTENTS_MISSILECLIP        0x00000080
#define CONTENTS_ITEM               0x00000100
#define CONTENTS_MOVER              0x00004000
#define CONTENTS_AREAPORTAL         0x00008000
#define CONTENTS_PLAYERCLIP         0x00010000
#define CONTENTS_MONSTERCLIP        0x00020000
#define CONTENTS_TELEPORTER         0x00040000
#define CONTENTS_JUMPPAD            0x00080000
#define CONTENTS_CLUSTERPORTAL      0x00100000
#define CONTENTS_DONOTENTER         0x00200000
#define CONTENTS_DONOTENTER_LARGE   0x00400000
#define CONTENTS_ORIGIN             0x01000000  // removed before bsping an entity
#define CONTENTS_BODY               0x02000000  // should never be on a brush, only in game
#define CONTENTS_CORPSE             0x04000000
#define CONTENTS_DETAIL             0x08000000  // brushes not used for the bsp

#define CONTENTS_STRUCTURAL     0x10000000  // brushes used for the bsp
#define CONTENTS_TRANSLUCENT    0x20000000  // don't consume surface fragments inside
#define CONTENTS_TRIGGER        0x40000000
#define CONTENTS_NODROP         0x80000000  // don't leave bodies or items (death fog, lava)
