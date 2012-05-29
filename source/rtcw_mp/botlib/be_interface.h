/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


/*****************************************************************************
 * name:		be_interface.h
 *
 * desc:		botlib interface
 *
 *
 *****************************************************************************/

/*
"Do not go where the path leads, rather go where there's no track and leave a trail."

"AAS (Area Awareness System)"

"Part of the Gladiator is BoGuS (Bot Guidance System)"

"ANSI (Advanced Navigational System Interface)"

"to make things work the only thing you really have to do is think things work."

"a madman is just someone living in another reality which isn't shared among many people"
*/

#include "../../server/server.h"
#include "../../server/botlib/local.h"

//#define DEBUG			//debug code

//==========================================================
//
// global variables
//
//==========================================================

extern botlib_import_t botimport;
