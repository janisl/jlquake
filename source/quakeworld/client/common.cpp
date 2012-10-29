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
// common.c -- misc functions used in client and server

#ifdef DEDICATED
#include "qwsvdef.h"
#else
#include "quakedef.h"
#endif
#include "../../client/public.h"
#include "../../server/public.h"

/*
================
COM_Init
================
*/
void COM_Init(void)
{
	Com_InitByteOrder();

	COM_InitCommonCvars();

	qh_registered = Cvar_Get("registered", "0", 0);

	FS_InitFilesystem();
	COMQH_CheckRegistered();
}

// char *date = "Oct 24 1996";
static const char* date = __DATE__;
static const char* mon[12] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static char mond[12] =
{ 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

// returns days since Oct 24 1996
int build_number(void)
{
	int m = 0;
	int d = 0;
	int y = 0;
	static int b = 0;

	if (b != 0)
	{
		return b;
	}

	for (m = 0; m < 11; m++)
	{
		if (String::NICmp(&date[0], mon[m], 3) == 0)
		{
			break;
		}
		d += mond[m];
	}

	d += String::Atoi(&date[4]) - 1;

	y = String::Atoi(&date[7]) - 1900;

	b = d + (int)((y - 1) * 365.25);

	if (((y % 4) == 0) && m > 1)
	{
		b += 1;
	}

	b -= 35778;	// Dec 16 1998

	return b;
}
