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
// gl_ngraph.c

#include "quakedef.h"

#define NET_GRAPHHEIGHT 32

static void R_LineGraph(int x, int y, int h)
{
	int r, g, b;
	if (h == 10000)
	{
		// yellow
		r = 1;
		g = 1;
		b = 0;
	}
	else if (h == 9999)
	{
		// red
		r = 1;
		g = 0;
		b = 0;
	}
	else if (h == 9998)
	{
		// blue
		r = 0;
		g = 0;
		b = 1;
	}
	else
	{
		// white
		r = 1;
		g = 1;
		b = 1;
	}

	if (h > NET_GRAPHHEIGHT)
	{
		h = NET_GRAPHHEIGHT;
	}

	UI_Fill(8 + x, y + NET_GRAPHHEIGHT - h, 1, h, r, g, b, 1);
}

/*
==============
R_NetGraph
==============
*/
void R_NetGraph (void)
{
	int		x, y;
	char st[80];

	x = 0;
	int lost = CL_CalcNet();

	x =	-((viddef.width - 320)>>1);
	y = viddef.height - sb_lines - 24 - NET_GRAPHHEIGHT - 1;

	M_DrawTextBox (x, y, NET_TIMINGS/8, NET_GRAPHHEIGHT/8 + 1);
	y += 8;

	sprintf(st, "%3i%% packet loss", lost);
	Draw_String(8, y, st);
	y += 8;

	for (int a = 0; a < NET_TIMINGS; a++)
	{
		int i = (cls.netchan.outgoing_sequence - a) & NET_TIMINGSMASK;
		R_LineGraph(NET_TIMINGS - 1 - a, y, packet_latency[i]);
	}
}

