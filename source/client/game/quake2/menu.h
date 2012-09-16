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

#define NUM_CURSOR_FRAMES 15

extern const char* menu_in_sound;
extern const char* menu_move_sound;
extern const char* menu_out_sound;
extern bool mq2_entersound;			// play after drawing a frame, so caching
								// won't disrupt the sound
extern void (* mq2_drawfunc)(void);
extern const char*(*mq2_keyfunc)(int key);
extern void (*mq2_charfunc)(int key);

#define MAX_MENU_DEPTH  8

struct menulayer_t
{
	void (* draw)(void);
	const char*(*key)(int k);
	void (*charfunc)(int key);
};

extern menulayer_t mq2_layers[MAX_MENU_DEPTH];
extern int mq2_menudepth;

void MQ2_ForceMenuOff();
void MQ2_Init();

struct menuframework_s;

void MQ2_Banner(const char* name);
void MQ2_PushMenu(void (* draw)(void), const char*(*key)(int k), void (*charfunc)(int key));
void MQ2_PopMenu();
const char* Default_MenuKey(menuframework_s* m, int key);
void Default_MenuChar(menuframework_s* m, int key);
void MQ2_DrawCharacter(int cx, int cy, int num);
void MQ2_Print(int cx, int cy, const char* str);
void MQ2_DrawCursor(int x, int y, int f);
void MQ2_DrawTextBox(int x, int y, int width, int lines);
void MQ2_Menu_Game_f();
