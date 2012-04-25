/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// cl_inv.c -- client inventory screen

#include "client.h"

/*
================
CL_ParseInventory
================
*/
void CL_ParseInventory(void)
{
	int i;

	for (i = 0; i < MAX_ITEMS_Q2; i++)
		cl.q2_inventory[i] = net_message.ReadShort();
}


/*
================
Inv_DrawString
================
*/
void Inv_DrawString(int x, int y, const char* string)
{
	while (*string)
	{
		Draw_Char(x, y, *string);
		x += 8;
		string++;
	}
}

void SetStringHighBit(char* s)
{
	while (*s)
		*s++ |= 128;
}

/*
================
CL_DrawInventory
================
*/
#define DISPLAY_ITEMS   17

void CL_DrawInventory(void)
{
	int i, j;
	int num, selected_num, item;
	int index[MAX_ITEMS_Q2];
	char string[1024];
	int x, y;
	char binding[1024];
	const char* bind;
	int selected;
	int top;

	selected = cl.q2_frame.playerstate.stats[Q2STAT_SELECTED_ITEM];

	num = 0;
	selected_num = 0;
	for (i = 0; i < MAX_ITEMS_Q2; i++)
	{
		if (i == selected)
		{
			selected_num = num;
		}
		if (cl.q2_inventory[i])
		{
			index[num] = i;
			num++;
		}
	}

	// determine scroll point
	top = selected_num - DISPLAY_ITEMS / 2;
	if (num - top < DISPLAY_ITEMS)
	{
		top = num - DISPLAY_ITEMS;
	}
	if (top < 0)
	{
		top = 0;
	}

	x = (viddef.width - 256) / 2;
	y = (viddef.height - 240) / 2;

	UI_DrawNamedPic(x, y + 8, "inventory");

	y += 24;
	x += 24;
	Inv_DrawString(x, y, "hotkey ### item");
	Inv_DrawString(x, y + 8, "------ --- ----");
	y += 16;
	for (i = top; i < num && i < top + DISPLAY_ITEMS; i++)
	{
		item = index[i];
		// search for a binding
		String::Sprintf(binding, sizeof(binding), "use %s", cl.q2_configstrings[Q2CS_ITEMS + item]);
		bind = "";
		for (j = 0; j < 256; j++)
			if (keybindings[j] && !String::ICmp(keybindings[j], binding))
			{
				bind = Key_KeynumToString(j);
				break;
			}

		String::Sprintf(string, sizeof(string), "%6s %3i %s", bind, cl.q2_inventory[item],
			cl.q2_configstrings[Q2CS_ITEMS + item]);
		if (item != selected)
		{
			SetStringHighBit(string);
		}
		else	// draw a blinky cursor by the selected item
		{
			if ((int)(cls.realtime * 10) & 1)
			{
				Draw_Char(x - 8, y, 15);
			}
		}
		Inv_DrawString(x, y, string);
		y += 8;
	}


}
