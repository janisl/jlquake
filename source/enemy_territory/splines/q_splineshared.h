/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

#include "../../common/qcommon.h"

/*
=====================================================================================

SCRIPT PARSING

=====================================================================================
*/

// this just controls the comment printing, it doesn't actually load a file
void Com_BeginParseSession(const char* filename);
void Com_EndParseSession(void);

// Will never return NULL, just empty strings.
// An empty string will only be returned at end of file.
// ParseOnLine will return empty if there isn't another token on this line

// this funny typedef just means a moving pointer into a const char * buffer
const char* Com_Parse(const char*(*data_p));
const char* Com_ParseOnLine(const char*(*data_p));

void Com_UngetToken(void);

void Com_MatchToken(const char*(*buf_p), const char* match, qboolean warning = false);

void Com_ScriptError(const char* msg, ...);
void Com_ScriptWarning(const char* msg, ...);

float Com_ParseFloat(const char*(*buf_p));

void Com_Parse1DMatrix(const char*(*buf_p), int x, float* m);

#endif	// __Q_SHARED_H
