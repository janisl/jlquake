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

#ifndef _CLIENT_QUAKE2_QMENU_H
#define _CLIENT_QUAKE2_QMENU_H

#include "../../../common/command_buffer.h"

#define MAXMENUITEMS    64

enum
{
	MTYPE_SLIDER,
	MTYPE_ACTION,
	MTYPE_SPINCONTROL,
	MTYPE_SEPARATOR,
	MTYPE_FIELD,
};

#define QMF_LEFT_JUSTIFY    0x00000001
#define QMF_GRAYED          0x00000002
#define QMF_NUMBERSONLY     0x00000004

struct menuframework_s
{
	int x, y;
	int cursor;

	int nitems;
	int nslots;
	void* items[MAXMENUITEMS];

	const char* statusbar;

	void (* cursordraw)(menuframework_s* m);
};

struct menucommon_s
{
	int type;
	const char* name;
	int x, y;
	menuframework_s* parent;
	int cursor_offset;
	int localdata[4];
	unsigned flags;

	const char* statusbar;

	void (* callback)(void* self);
	void (* statusbarfunc)(void* self);
	void (* ownerdraw)(void* self);
	void (* cursordraw)(void* self);
};

struct menufield_s
{
	menucommon_s generic;

	field_t field;
};

struct menuslider_s
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
};

struct menulist_s
{
	menucommon_s generic;

	int curvalue;

	const char** itemnames;
};

struct menuaction_s
{
	menucommon_s generic;
};

struct menuseparator_s
{
	menucommon_s generic;
};

bool MQ2_Field_Key(menufield_s* field, int key);
void MQ2_Field_Char(menufield_s* f, int key);

void Menu_AddItem(menuframework_s* menu, void* item);
void* Menu_ItemAtCursor(menuframework_s* m);
void Menu_AdjustCursor(menuframework_s* menu, int dir);
void Menu_Center(menuframework_s* menu);
void Menu_Draw(menuframework_s* menu);
bool Menu_SelectItem(menuframework_s* s);
void Menu_SetStatusBar(menuframework_s* s, const char* string);
void Menu_SlideItem(menuframework_s* s, int dir);

#endif

