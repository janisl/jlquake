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
 * name:		be_aas.h
 *
 * desc:		Area Awareness System, stuff exported to the AI
 *
 *
 *****************************************************************************/

typedef struct aas_clientmove_s
{
	vec3_t endpos;			//position at the end of movement prediction
	vec3_t velocity;		//velocity at the end of movement prediction
	aas_trace_t trace;		//last trace
	int presencetype;		//presence type at end of movement prediction
	int stopevent;			//event that made the prediction stop
	float endcontents;		//contents at the end of movement prediction
	float time;				//time predicted ahead
	int frames;				//number of frames predicted ahead
} aas_clientmove_t;
