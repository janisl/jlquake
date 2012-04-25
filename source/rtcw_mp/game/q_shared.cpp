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

// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

// os x game bundles have no standard library links, and the defines are not always defined!

#ifdef MACOS_X
int qmax(int x, int y)
{
	return (((x) > (y)) ? (x) : (y));
}

int qmin(int x, int y)
{
	return (((x) < (y)) ? (x) : (y));
}
#endif

float Com_Clamp(float min, float max, float value)
{
	if (value < min)
	{
		return min;
	}
	if (value > max)
	{
		return max;
	}
	return value;
}

//============================================================================
/*
==================
COM_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
qboolean COM_BitCheck(const int array[], int bitNum)
{
	int i;

	i = 0;
	while (bitNum > 31)
	{
		i++;
		bitNum -= 32;
	}

	return ((array[i] & (1 << bitNum)) != 0);		// (SA) heh, whoops. :)
}

/*
==================
COM_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitSet(int array[], int bitNum)
{
	int i;

	i = 0;
	while (bitNum > 31)
	{
		i++;
		bitNum -= 32;
	}

	array[i] |= (1 << bitNum);
}

/*
==================
COM_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitClear(int array[], int bitNum)
{
	int i;

	i = 0;
	while (bitNum > 31)
	{
		i++;
		bitNum -= 32;
	}

	array[i] &= ~(1 << bitNum);
}
//============================================================================

/*
============================================================================

                    LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_PrintStrlen(const char* string)
{
	int len;
	const char* p;

	if (!string)
	{
		return 0;
	}

	len = 0;
	p = string;
	while (*p)
	{
		if (Q_IsColorString(p))
		{
			p += 2;
			continue;
		}
		p++;
		len++;
	}

	return len;
}


char* Q_CleanStr(char* string)
{
	char* d;
	char* s;
	int c;

	s = string;
	d = string;
	while ((c = *s) != 0)
	{
		if (Q_IsColorString(s))
		{
			s++;
		}
		else if (c >= 0x20 && c <= 0x7E)
		{
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday

Ridah, modified this into a circular list, to further prevent stepping on
previous strings
============
*/
char* QDECL va(char* format, ...)
{
	va_list argptr;
	#define MAX_VA_STRING   32000
	static char temp_buffer[MAX_VA_STRING];
	static char string[MAX_VA_STRING];		// in case va is called by nested functions
	static int index = 0;
	char* buf;
	int len;


	va_start(argptr, format);
	vsprintf(temp_buffer, format,argptr);
	va_end(argptr);

	if ((len = String::Length(temp_buffer)) >= MAX_VA_STRING)
	{
		Com_Error(ERR_DROP, "Attempted to overrun string in call to va()\n");
	}

	if (len + index >= MAX_VA_STRING - 1)
	{
		index = 0;
	}

	buf = &string[index];
	memcpy(buf, temp_buffer, len + 1);

	index += len + 1;

	return buf;
}

/*
=============
TempVector

(SA) this is straight out of g_utils.c around line 210

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float* tv(float x, float y, float z)
{
	static int index;
	static vec3_t vecs[8];
	float* v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1) & 7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}
