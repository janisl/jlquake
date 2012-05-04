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
#include "client.h"
#include "qmenu.h"

static void  Action_DoEnter(menuaction_s* a);
static void  Action_Draw(menuaction_s* a);
static void  Menu_DrawStatusBar(const char* string);
static void  Menulist_DoEnter(menulist_s* l);
static void  MenuList_Draw(menulist_s* l);
static void  Separator_Draw(menuseparator_s* s);
static void  Slider_DoSlide(menuslider_s* s, int dir);
static void  Slider_Draw(menuslider_s* s);
static void  SpinControl_DoEnter(menulist_s* s);
static void  SpinControl_Draw(menulist_s* s);
static void  SpinControl_DoSlide(menulist_s* s, int dir);

#define RCOLUMN_OFFSET  16
#define LCOLUMN_OFFSET -16

#define VID_WIDTH viddef.width
#define VID_HEIGHT viddef.height

void Action_DoEnter(menuaction_s* a)
{
	if (a->generic.callback)
	{
		a->generic.callback(a);
	}
}

void Action_Draw(menuaction_s* a)
{
	if (a->generic.flags & QMF_LEFT_JUSTIFY)
	{
		if (a->generic.flags & QMF_GRAYED)
		{
			Menu_DrawStringDark(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
		}
		else
		{
			UI_DrawString(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
		}
	}
	else
	{
		if (a->generic.flags & QMF_GRAYED)
		{
			Menu_DrawStringR2LDark(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
		}
		else
		{
			Menu_DrawStringR2L(a->generic.x + a->generic.parent->x + LCOLUMN_OFFSET, a->generic.y + a->generic.parent->y, a->generic.name);
		}
	}
	if (a->generic.ownerdraw)
	{
		a->generic.ownerdraw(a);
	}
}

qboolean Field_DoEnter(menufield_s* f)
{
	if (f->generic.callback)
	{
		f->generic.callback(f);
		return true;
	}
	return false;
}

void Field_Draw(menufield_s* f)
{
	int i;
	char tempbuffer[128] = "";

	if (f->generic.name)
	{
		Menu_DrawStringR2LDark(f->generic.x + f->generic.parent->x + LCOLUMN_OFFSET, f->generic.y + f->generic.parent->y, f->generic.name);
	}

	String::NCpy(tempbuffer, f->field.buffer + f->field.scroll, f->field.widthInChars);

	UI_DrawChar(f->generic.x + f->generic.parent->x + 16, f->generic.y + f->generic.parent->y - 4, 18);
	UI_DrawChar(f->generic.x + f->generic.parent->x + 16, f->generic.y + f->generic.parent->y + 4, 24);

	UI_DrawChar(f->generic.x + f->generic.parent->x + 24 + f->field.widthInChars * 8, f->generic.y + f->generic.parent->y - 4, 20);
	UI_DrawChar(f->generic.x + f->generic.parent->x + 24 + f->field.widthInChars * 8, f->generic.y + f->generic.parent->y + 4, 26);

	for (i = 0; i < f->field.widthInChars; i++)
	{
		UI_DrawChar(f->generic.x + f->generic.parent->x + 24 + i * 8, f->generic.y + f->generic.parent->y - 4, 19);
		UI_DrawChar(f->generic.x + f->generic.parent->x + 24 + i * 8, f->generic.y + f->generic.parent->y + 4, 25);
	}

	UI_DrawString(f->generic.x + f->generic.parent->x + 24, f->generic.y + f->generic.parent->y, tempbuffer);

	if (Menu_ItemAtCursor(f->generic.parent) == f)
	{
		int offset;

		if (f->field.scroll)
		{
			offset = f->field.widthInChars;
		}
		else
		{
			offset = f->field.cursor;
		}

		if (((int)(Sys_Milliseconds_() / 250)) & 1)
		{
			UI_DrawChar(f->generic.x + f->generic.parent->x + (offset + 2) * 8 + 8,
				f->generic.y + f->generic.parent->y,
				11);
		}
		else
		{
			UI_DrawChar(f->generic.x + f->generic.parent->x + (offset + 2) * 8 + 8,
				f->generic.y + f->generic.parent->y,
				' ');
		}
	}
}

qboolean Field_Key(menufield_s* f, int key)
{
	return Field_KeyDownEvent(&f->field, key);
}

void Field_Char(menufield_s* f, int key)
{
	if ((f->generic.flags & QMF_NUMBERSONLY) && key >= 32 && !String::IsDigit(key))
	{
		return;
	}
	Field_CharEvent(&f->field, key);
}

void Menu_AddItem(menuframework_s* menu, void* item)
{
	if (menu->nitems == 0)
	{
		menu->nslots = 0;
	}

	if (menu->nitems < MAXMENUITEMS)
	{
		menu->items[menu->nitems] = item;
		((menucommon_s*)menu->items[menu->nitems])->parent = menu;
		menu->nitems++;
	}

	menu->nslots = Menu_TallySlots(menu);
}

/*
** Menu_AdjustCursor
**
** This function takes the given menu, the direction, and attempts
** to adjust the menu's cursor so that it's at the next available
** slot.
*/
void Menu_AdjustCursor(menuframework_s* m, int dir)
{
	menucommon_s* citem;

	/*
	** see if it's in a valid spot
	*/
	if (m->cursor >= 0 && m->cursor < m->nitems)
	{
		if ((citem = (menucommon_s*)Menu_ItemAtCursor(m)) != 0)
		{
			if (citem->type != MTYPE_SEPARATOR)
			{
				return;
			}
		}
	}

	/*
	** it's not in a valid spot, so crawl in the direction indicated until we
	** find a valid spot
	*/
	if (dir == 1)
	{
		while (1)
		{
			citem = (menucommon_s*)Menu_ItemAtCursor(m);
			if (citem)
			{
				if (citem->type != MTYPE_SEPARATOR)
				{
					break;
				}
			}
			m->cursor += dir;
			if (m->cursor >= m->nitems)
			{
				m->cursor = 0;
			}
		}
	}
	else
	{
		while (1)
		{
			citem = (menucommon_s*)Menu_ItemAtCursor(m);
			if (citem)
			{
				if (citem->type != MTYPE_SEPARATOR)
				{
					break;
				}
			}
			m->cursor += dir;
			if (m->cursor < 0)
			{
				m->cursor = m->nitems - 1;
			}
		}
	}
}

void Menu_Center(menuframework_s* menu)
{
	int height;

	height = ((menucommon_s*)menu->items[menu->nitems - 1])->y;
	height += 10;

	menu->y = (VID_HEIGHT - height) / 2;
}

void Menu_Draw(menuframework_s* menu)
{
	int i;
	menucommon_s* item;

	/*
	** draw contents
	*/
	for (i = 0; i < menu->nitems; i++)
	{
		switch (((menucommon_s*)menu->items[i])->type)
		{
		case MTYPE_FIELD:
			Field_Draw((menufield_s*)menu->items[i]);
			break;
		case MTYPE_SLIDER:
			Slider_Draw((menuslider_s*)menu->items[i]);
			break;
		case MTYPE_LIST:
			MenuList_Draw((menulist_s*)menu->items[i]);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_Draw((menulist_s*)menu->items[i]);
			break;
		case MTYPE_ACTION:
			Action_Draw((menuaction_s*)menu->items[i]);
			break;
		case MTYPE_SEPARATOR:
			Separator_Draw((menuseparator_s*)menu->items[i]);
			break;
		}
	}

	item = (menucommon_s*)Menu_ItemAtCursor(menu);

	if (item && item->cursordraw)
	{
		item->cursordraw(item);
	}
	else if (menu->cursordraw)
	{
		menu->cursordraw(menu);
	}
	else if (item && item->type != MTYPE_FIELD)
	{
		if (item->flags & QMF_LEFT_JUSTIFY)
		{
			UI_DrawChar(menu->x + item->x - 24 + item->cursor_offset, menu->y + item->y, 12 + ((int)(Sys_Milliseconds_() / 250) & 1));
		}
		else
		{
			UI_DrawChar(menu->x + item->cursor_offset, menu->y + item->y, 12 + ((int)(Sys_Milliseconds_() / 250) & 1));
		}
	}

	if (item)
	{
		if (item->statusbarfunc)
		{
			item->statusbarfunc((void*)item);
		}
		else if (item->statusbar)
		{
			Menu_DrawStatusBar(item->statusbar);
		}
		else
		{
			Menu_DrawStatusBar(menu->statusbar);
		}

	}
	else
	{
		Menu_DrawStatusBar(menu->statusbar);
	}
}

void Menu_DrawStatusBar(const char* string)
{
	if (string)
	{
		int l = String::Length(string);
		int maxcol = VID_WIDTH / 8;
		int col = maxcol / 2 - l / 2;

		UI_FillPal(0, VID_HEIGHT - 8, VID_WIDTH, 8, 4);
		UI_DrawString(col * 8, VID_HEIGHT - 8, string);
	}
	else
	{
		UI_FillPal(0, VID_HEIGHT - 8, VID_WIDTH, 8, 0);
	}
}

void Menu_DrawStringDark(int x, int y, const char* string)
{
	UI_DrawString(x, y, string, 128);
}

void Menu_DrawStringR2L(int x, int y, const char* string)
{
	UI_DrawString((x - (String::Length(string) - 1) * 8), y, string);
}

void Menu_DrawStringR2LDark(int x, int y, const char* string)
{
	UI_DrawString((x - (String::Length(string) - 1) * 8), y, string, 128);
}

void* Menu_ItemAtCursor(menuframework_s* m)
{
	if (m->cursor < 0 || m->cursor >= m->nitems)
	{
		return 0;
	}

	return m->items[m->cursor];
}

qboolean Menu_SelectItem(menuframework_s* s)
{
	menucommon_s* item = (menucommon_s*)Menu_ItemAtCursor(s);

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_FIELD:
			return Field_DoEnter((menufield_s*)item);
		case MTYPE_ACTION:
			Action_DoEnter((menuaction_s*)item);
			return true;
		case MTYPE_LIST:
//			Menulist_DoEnter( ( menulist_s * ) item );
			return false;
		case MTYPE_SPINCONTROL:
//			SpinControl_DoEnter( ( menulist_s * ) item );
			return false;
		}
	}
	return false;
}

void Menu_SetStatusBar(menuframework_s* m, const char* string)
{
	m->statusbar = string;
}

void Menu_SlideItem(menuframework_s* s, int dir)
{
	menucommon_s* item = (menucommon_s*)Menu_ItemAtCursor(s);

	if (item)
	{
		switch (item->type)
		{
		case MTYPE_SLIDER:
			Slider_DoSlide((menuslider_s*)item, dir);
			break;
		case MTYPE_SPINCONTROL:
			SpinControl_DoSlide((menulist_s*)item, dir);
			break;
		}
	}
}

int Menu_TallySlots(menuframework_s* menu)
{
	int i;
	int total = 0;

	for (i = 0; i < menu->nitems; i++)
	{
		if (((menucommon_s*)menu->items[i])->type == MTYPE_LIST)
		{
			int nitems = 0;
			const char** n = ((menulist_s*)menu->items[i])->itemnames;

			while (*n)
				nitems++, n++;

			total += nitems;
		}
		else
		{
			total++;
		}
	}

	return total;
}

void Menulist_DoEnter(menulist_s* l)
{
	int start;

	start = l->generic.y / 10 + 1;

	l->curvalue = l->generic.parent->cursor - start;

	if (l->generic.callback)
	{
		l->generic.callback(l);
	}
}

void MenuList_Draw(menulist_s* l)
{
	const char** n;
	int y = 0;

	Menu_DrawStringR2LDark(l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y, l->generic.name);

	n = l->itemnames;

	UI_FillPal(l->generic.x - 112 + l->generic.parent->x, l->generic.parent->y + l->generic.y + l->curvalue * 10 + 10, 128, 10, 16);
	while (*n)
	{
		Menu_DrawStringR2LDark(l->generic.x + l->generic.parent->x + LCOLUMN_OFFSET, l->generic.y + l->generic.parent->y + y + 10, *n);

		n++;
		y += 10;
	}
}

void Separator_Draw(menuseparator_s* s)
{
	if (s->generic.name)
	{
		Menu_DrawStringR2LDark(s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->generic.name);
	}
}

void Slider_DoSlide(menuslider_s* s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue > s->maxvalue)
	{
		s->curvalue = s->maxvalue;
	}
	else if (s->curvalue < s->minvalue)
	{
		s->curvalue = s->minvalue;
	}

	if (s->generic.callback)
	{
		s->generic.callback(s);
	}
}

#define SLIDER_RANGE 10

void Slider_Draw(menuslider_s* s)
{
	int i;

	Menu_DrawStringR2LDark(s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
		s->generic.y + s->generic.parent->y,
		s->generic.name);

	s->range = (s->curvalue - s->minvalue) / (float)(s->maxvalue - s->minvalue);

	if (s->range < 0)
	{
		s->range = 0;
	}
	if (s->range > 1)
	{
		s->range = 1;
	}
	UI_DrawChar(s->generic.x + s->generic.parent->x + RCOLUMN_OFFSET, s->generic.y + s->generic.parent->y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		UI_DrawChar(RCOLUMN_OFFSET + s->generic.x + i * 8 + s->generic.parent->x + 8, s->generic.y + s->generic.parent->y, 129);
	UI_DrawChar(RCOLUMN_OFFSET + s->generic.x + i * 8 + s->generic.parent->x + 8, s->generic.y + s->generic.parent->y, 130);
	UI_DrawChar((int)(8 + RCOLUMN_OFFSET + s->generic.parent->x + s->generic.x + (SLIDER_RANGE - 1) * 8 * s->range), s->generic.y + s->generic.parent->y, 131);
}

void SpinControl_DoEnter(menulist_s* s)
{
	s->curvalue++;
	if (s->itemnames[s->curvalue] == 0)
	{
		s->curvalue = 0;
	}

	if (s->generic.callback)
	{
		s->generic.callback(s);
	}
}

void SpinControl_DoSlide(menulist_s* s, int dir)
{
	s->curvalue += dir;

	if (s->curvalue < 0)
	{
		s->curvalue = 0;
	}
	else if (s->itemnames[s->curvalue] == 0)
	{
		s->curvalue--;
	}

	if (s->generic.callback)
	{
		s->generic.callback(s);
	}
}

void SpinControl_Draw(menulist_s* s)
{
	char buffer[100];

	if (s->generic.name)
	{
		Menu_DrawStringR2LDark(s->generic.x + s->generic.parent->x + LCOLUMN_OFFSET,
			s->generic.y + s->generic.parent->y,
			s->generic.name);
	}
	if (!strchr(s->itemnames[s->curvalue], '\n'))
	{
		UI_DrawString(RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, s->itemnames[s->curvalue]);
	}
	else
	{
		String::Cpy(buffer, s->itemnames[s->curvalue]);
		*strchr(buffer, '\n') = 0;
		UI_DrawString(RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y, buffer);
		String::Cpy(buffer, strchr(s->itemnames[s->curvalue], '\n') + 1);
		UI_DrawString(RCOLUMN_OFFSET + s->generic.x + s->generic.parent->x, s->generic.y + s->generic.parent->y + 10, buffer);
	}
}
