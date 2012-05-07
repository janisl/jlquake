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
#include "quakedef.h"

extern Cvar* r_gamma;

menu_state_t m_state, m_return_state;
static menu_state_t m_quit_prevstate;

void M_Menu_Main_f(void);
void M_Menu_SinglePlayer_f(void);
void M_Menu_Load_f(void);
void M_Menu_Save_f(void);
void M_Menu_MultiPlayer_f(void);
void M_Menu_Setup_f(void);
void M_Menu_Net_f(void);
void M_Menu_Options_f(void);
void M_Menu_Keys_f(void);
void M_Menu_Video_f(void);
void M_Menu_Help_f(void);
void M_Menu_Quit_f(void);
void M_Menu_LanConfig_f(void);
void M_Menu_GameOptions_f(void);
void M_Menu_Search_f(void);
void M_Menu_ServerList_f(void);

void M_Main_Draw(void);
void M_SinglePlayer_Draw(void);
void M_Load_Draw(void);
void M_Save_Draw(void);
void M_MultiPlayer_Draw(void);
void M_Setup_Draw(void);
void M_Net_Draw(void);
void M_Options_Draw(void);
void M_Keys_Draw(void);
void M_Video_Draw(void);
void M_Help_Draw(void);
void M_Quit_Draw(void);
void M_SerialConfig_Draw(void);
void M_ModemConfig_Draw(void);
void M_LanConfig_Draw(void);
void M_GameOptions_Draw(void);
void M_Search_Draw(void);
void M_ServerList_Draw(void);

void M_Main_Key(int key);
void M_SinglePlayer_Key(int key);
void M_Load_Key(int key);
void M_Save_Key(int key);
void M_MultiPlayer_Key(int key);
void M_Setup_Key(int key);
void M_Net_Key(int key);
void M_Options_Key(int key);
void M_Keys_Key(int key);
void M_Video_Key(int key);
void M_Help_Key(int key);
void M_Quit_Key(int key);
void M_SerialConfig_Key(int key);
void M_ModemConfig_Key(int key);
void M_LanConfig_Key(int key);
void M_GameOptions_Key(int key);
void M_Search_Key(int key);
void M_ServerList_Key(int key);

qboolean m_entersound;			// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean m_recursiveDraw;

qboolean m_return_onerror;
char m_return_reason [32];

#define StartingGame    (m_multiplayer_cursor == 1)
#define JoiningGame     (m_multiplayer_cursor == 0)

void M_ConfigureNetSubsystem(void);

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter(int cx, int line, int num)
{
	UI_DrawChar(cx + ((viddef.width - 320) >> 1), line, num);
}

void M_Print(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy, str, 128);
}

void M_PrintWhite(int cx, int cy, const char* str)
{
	UI_DrawString(cx + ((viddef.width - 320) >> 1), cy, str);
}

void M_DrawTransPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1), y, pic);
}

void M_DrawPic(int x, int y, image_t* pic)
{
	UI_DrawPic(x + ((viddef.width - 320) >> 1), y, pic);
}

byte translationTable[256];

static byte menuplyr_pixels[4096];
image_t* translate_texture;

void M_DrawTextBox(int x, int y, int width, int lines)
{
	image_t* p;
	int cx, cy;
	int n;

	// draw left side
	cx = x;
	cy = y;
	p = R_CachePic("gfx/box_tl.lmp");
	M_DrawTransPic(cx, cy, p);
	p = R_CachePic("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic(cx, cy, p);
	}
	p = R_CachePic("gfx/box_bl.lmp");
	M_DrawTransPic(cx, cy + 8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = R_CachePic("gfx/box_tm.lmp");
		M_DrawTransPic(cx, cy, p);
		p = R_CachePic("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
			{
				p = R_CachePic("gfx/box_mm2.lmp");
			}
			M_DrawTransPic(cx, cy, p);
		}
		p = R_CachePic("gfx/box_bm.lmp");
		M_DrawTransPic(cx, cy + 8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = R_CachePic("gfx/box_tr.lmp");
	M_DrawTransPic(cx, cy, p);
	p = R_CachePic("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic(cx, cy, p);
	}
	p = R_CachePic("gfx/box_br.lmp");
	M_DrawTransPic(cx, cy + 8, p);
}

void M_DrawField(int x, int y, field_t* edit, bool showCursor)
{
	M_DrawTextBox(x - 8, y - 8, edit->widthInChars, 1);
	Field_Draw(edit, x + ((viddef.width - 320) >> 1), y, showCursor);
}

//=============================================================================

int m_save_demonum;

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu_f(void)
{
	m_entersound = true;

	if (in_keyCatchers & KEYCATCH_UI)
	{
		if (m_state != m_main)
		{
			M_Menu_Main_f();
			return;
		}
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		return;
	}
	if (in_keyCatchers & KEYCATCH_CONSOLE)
	{
		Con_ToggleConsole_f();
	}
	else
	{
		M_Menu_Main_f();
	}
}


//=============================================================================
/* MAIN MENU */

int m_main_cursor;
#define MAIN_ITEMS  5


void M_Menu_Main_f(void)
{
	if (!(in_keyCatchers & KEYCATCH_UI))
	{
		m_save_demonum = cls.qh_demonum;
		cls.qh_demonum = -1;
	}
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_main;
	m_entersound = true;
}


void M_Main_Draw(void)
{
	int f;
	image_t* p;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/ttl_main.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
	M_DrawTransPic(72, 32, R_CachePic("gfx/mainmenu.lmp"));

	f = (int)(host_time * 10) % 6;

	M_DrawTransPic(54, 32 + m_main_cursor * 20,R_CachePic(va("gfx/menudot%i.lmp", f + 1)));
}


void M_Main_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		cls.qh_demonum = m_save_demonum;
		if (cls.qh_demonum != -1 && !clc.demoplaying && cls.state != CA_ACTIVE)
		{
			CL_NextDemo();
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
		{
			m_main_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (--m_main_cursor < 0)
		{
			m_main_cursor = MAIN_ITEMS - 1;
		}
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_SinglePlayer_f();
			break;

		case 1:
			M_Menu_MultiPlayer_f();
			break;

		case 2:
			M_Menu_Options_f();
			break;

		case 3:
			M_Menu_Help_f();
			break;

		case 4:
			M_Menu_Quit_f();
			break;
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

int m_singleplayer_cursor;
#define SINGLEPLAYER_ITEMS  3


void M_Menu_SinglePlayer_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_singleplayer;
	m_entersound = true;
}


void M_SinglePlayer_Draw(void)
{
	int f;
	image_t* p;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/ttl_sgl.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
	M_DrawTransPic(72, 32, R_CachePic("gfx/sp_menu.lmp"));

	f = (int)(host_time * 10) % 6;

	M_DrawTransPic(54, 32 + m_singleplayer_cursor * 20,R_CachePic(va("gfx/menudot%i.lmp", f + 1)));
}


void M_SinglePlayer_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
		{
			m_singleplayer_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
		{
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		}
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (sv.active)
			{
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n"))
				{
					break;
				}
			}
			in_keyCatchers &= ~KEYCATCH_UI;
			if (sv.active)
			{
				Cbuf_AddText("disconnect\n");
			}
			Cbuf_AddText("maxplayers 1\n");
			Cbuf_AddText("map start\n");
			break;

		case 1:
			M_Menu_Load_f();
			break;

		case 2:
			M_Menu_Save_f();
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int load_cursor;			// 0 < load_cursor < MAX_SAVEGAMES

#define MAX_SAVEGAMES       12
char m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH + 1];
int loadable[MAX_SAVEGAMES];

void M_ScanSaves(void)
{
	int i, j;
	char name[MAX_OSPATH];
	fileHandle_t f;

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		String::Cpy(m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		sprintf(name, "s%i.sav", i);
		//	This will make sure that only savegames in current game directory
		// in home directory are listed
		if (!FS_FileExists(name))
		{
			continue;
		}
		FS_FOpenFileRead(name, &f, true);
		if (!f)
		{
			continue;
		}
		FS_Read(name, 80, f);
		name[80] = 0;
		//	Skip version
		char* Ptr = name;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		if (*Ptr == '\n')
		{
			*Ptr = 0;
			Ptr++;
		}
		char* SaveName = Ptr;
		while (*Ptr && *Ptr != '\n')
			Ptr++;
		*Ptr = 0;
		String::NCpy(m_filenames[i], SaveName, sizeof(m_filenames[i]) - 1);

		// change _ back to space
		for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
			if (m_filenames[i][j] == '_')
			{
				m_filenames[i][j] = ' ';
			}
		loadable[i] = true;
		FS_FCloseFile(f);
	}
}

void M_Menu_Load_f(void)
{
	m_entersound = true;
	m_state = m_load;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanSaves();
}


void M_Menu_Save_f(void)
{
	if (!sv.active)
	{
		return;
	}
	if (cl.qh_intermission)
	{
		return;
	}
	if (svs.maxclients != 1)
	{
		return;
	}
	m_entersound = true;
	m_state = m_save;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanSaves();
}


void M_Load_Draw(void)
{
	int i;
	image_t* p;

	p = R_CachePic("gfx/p_load.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print(16, 32 + 8 * i, m_filenames[i]);

// line cursor
	M_DrawCharacter(8, 32 + load_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Save_Draw(void)
{
	int i;
	image_t* p;

	p = R_CachePic("gfx/p_save.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print(16, 32 + 8 * i, m_filenames[i]);

// line cursor
	M_DrawCharacter(8, 32 + load_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Load_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f();
		break;

	case K_ENTER:
		S_StartLocalSound("misc/menu2.wav");
		if (!loadable[load_cursor])
		{
			return;
		}
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
		SCR_BeginLoadingPlaque();

		// issue the load command
		Cbuf_AddText(va("load s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
		{
			load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
		{
			load_cursor = 0;
		}
		break;
	}
}


void M_Save_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_SinglePlayer_f();
		break;

	case K_ENTER:
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;
		Cbuf_AddText(va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
		{
			load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
		{
			load_cursor = 0;
		}
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int m_multiplayer_cursor;
#define MULTIPLAYER_ITEMS   3


void M_Menu_MultiPlayer_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_multiplayer;
	m_entersound = true;
}


void M_MultiPlayer_Draw(void)
{
	int f;
	image_t* p;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_multi.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
	M_DrawTransPic(72, 32, R_CachePic("gfx/mp_menu.lmp"));

	f = (int)(host_time * 10) % 6;

	M_DrawTransPic(54, 32 + m_multiplayer_cursor * 20,R_CachePic(va("gfx/menudot%i.lmp", f + 1)));

	if (tcpipAvailable)
	{
		return;
	}
	M_PrintWhite((320 / 2) - ((27 * 8) / 2), 148, "No Communications Available");
}


void M_MultiPlayer_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
		{
			m_multiplayer_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
		{
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		}
		break;

	case K_ENTER:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (tcpipAvailable)
			{
				M_Menu_Net_f();
			}
			break;

		case 1:
			if (tcpipAvailable)
			{
				M_Menu_Net_f();
			}
			break;

		case 2:
			M_Menu_Setup_f();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

int setup_cursor = 4;
int setup_cursor_table[] = {40, 56, 80, 104, 140};

field_t setup_hostname;
field_t setup_myname;
int setup_oldtop;
int setup_oldbottom;
int setup_top;
int setup_bottom;

#define NUM_SETUP_CMDS  5

void M_Menu_Setup_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_setup;
	m_entersound = true;
	String::Cpy(setup_myname.buffer, clqh_name->string);
	setup_myname.cursor = String::Length(setup_myname.buffer);
	setup_myname.maxLength = 15;
	setup_myname.widthInChars = 16;
	String::Cpy(setup_hostname.buffer, hostname->string);
	setup_hostname.cursor = String::Length(setup_hostname.buffer);
	setup_hostname.maxLength = 15;
	setup_hostname.widthInChars = 16;
	setup_top = setup_oldtop = ((int)clqh_color->value) >> 4;
	setup_bottom = setup_oldbottom = ((int)clqh_color->value) & 15;
}


void M_Setup_Draw(void)
{
	image_t* p;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_multi.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	M_Print(64, 40, "Hostname");
	M_DrawField(168, 40, &setup_hostname, setup_cursor == 0);

	M_Print(64, 56, "Your name");
	M_DrawField(168, 56, &setup_myname, setup_cursor == 1);

	M_Print(64, 80, "Shirt color");
	M_Print(64, 104, "Pants color");

	M_DrawTextBox(64, 140 - 8, 14, 1);
	M_Print(72, 140, "Accept Changes");

	p = R_CachePic("gfx/bigbox.lmp");
	M_DrawTransPic(160, 64, p);
	p = R_CachePicWithTransPixels("gfx/menuplyr.lmp", menuplyr_pixels);
	CL_CalcQuakeSkinTranslation(setup_top, setup_bottom, translationTable);
	R_CreateOrUpdateTranslatedImage(translate_texture, "*translate_pic", menuplyr_pixels, translationTable, R_GetImageWidth(p), R_GetImageHeight(p));
	M_DrawPic(172, 72, translate_texture);

	M_DrawCharacter(56, setup_cursor_table [setup_cursor], 12 + ((int)(realtime * 4) & 1));
}


void M_Setup_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
		{
			setup_cursor = NUM_SETUP_CMDS - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
		{
			setup_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		if (setup_cursor < 2)
		{
			break;
		}
		S_StartLocalSound("misc/menu3.wav");
		if (setup_cursor == 2)
		{
			setup_top = setup_top - 1;
		}
		if (setup_cursor == 3)
		{
			setup_bottom = setup_bottom - 1;
		}
		break;
	case K_RIGHTARROW:
		if (setup_cursor < 2)
		{
			break;
		}
forward:
		S_StartLocalSound("misc/menu3.wav");
		if (setup_cursor == 2)
		{
			setup_top = setup_top + 1;
		}
		if (setup_cursor == 3)
		{
			setup_bottom = setup_bottom + 1;
		}
		break;

	case K_ENTER:
		if (setup_cursor == 0 || setup_cursor == 1)
		{
			return;
		}

		if (setup_cursor == 2 || setup_cursor == 3)
		{
			goto forward;
		}

		// setup_cursor == 4 (OK)
		if (String::Cmp(clqh_name->string, setup_myname.buffer) != 0)
		{
			Cbuf_AddText(va("name \"%s\"\n", setup_myname.buffer));
		}
		if (String::Cmp(hostname->string, setup_hostname.buffer) != 0)
		{
			Cvar_Set("hostname", setup_hostname.buffer);
		}
		if (setup_top != setup_oldtop || setup_bottom != setup_oldbottom)
		{
			Cbuf_AddText(va("color %i %i\n", setup_top, setup_bottom));
		}
		m_entersound = true;
		M_Menu_MultiPlayer_f();
		break;
	}
	if (setup_cursor == 0)
	{
		Field_KeyDownEvent(&setup_hostname, k);
	}
	if (setup_cursor == 1)
	{
		Field_KeyDownEvent(&setup_myname, k);
	}

	if (setup_top > 13)
	{
		setup_top = 0;
	}
	if (setup_top < 0)
	{
		setup_top = 13;
	}
	if (setup_bottom > 13)
	{
		setup_bottom = 0;
	}
	if (setup_bottom < 0)
	{
		setup_bottom = 13;
	}
}

void M_Setup_Char(int k)
{
	if (setup_cursor == 0)
	{
		Field_CharEvent(&setup_hostname, k);
	}
	if (setup_cursor == 1)
	{
		Field_CharEvent(&setup_myname, k);
	}
}

//=============================================================================
/* NET MENU */

int m_net_cursor;
int m_net_items;
int m_net_saveHeight;

const char* net_helpMessage [] =
{
/* .........1.........2.... */
	"                        ",
	" Two computers connected",
	"   through two modems.  ",
	"                        ",

	"                        ",
	" Two computers connected",
	" by a null-modem cable. ",
	"                        ",

	" Novell network LANs    ",
	" or Windows 95 DOS-box. ",
	"                        ",
	"(LAN=Local Area Network)",

	" Commonly used to play  ",
	" over the Internet, but ",
	" also used on a Local   ",
	" Area Network.          "
};

void M_Menu_Net_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 4;

	if (m_net_cursor >= m_net_items)
	{
		m_net_cursor = 0;
	}
	m_net_cursor--;
	M_Net_Key(K_DOWNARROW);
}


void M_Net_Draw(void)
{
	int f;
	image_t* p;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_multi.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	f = 89;
	if (tcpipAvailable)
	{
		p = R_CachePic("gfx/netmen4.lmp");
	}
	else
	{
		p = R_CachePic("gfx/dim_tcp.lmp");
	}
	M_DrawTransPic(72, f, p);

	if (m_net_items == 5)	// JDC, could just be removed
	{
		f += 19;
		p = R_CachePic("gfx/netmen5.lmp");
		M_DrawTransPic(72, f, p);
	}

	f = (320 - 26 * 8) / 2;
	M_DrawTextBox(f, 134, 24, 4);
	f += 8;
	M_Print(f, 142, net_helpMessage[m_net_cursor * 4 + 0]);
	M_Print(f, 150, net_helpMessage[m_net_cursor * 4 + 1]);
	M_Print(f, 158, net_helpMessage[m_net_cursor * 4 + 2]);
	M_Print(f, 166, net_helpMessage[m_net_cursor * 4 + 3]);

	f = (int)(host_time * 10) % 6;
	M_DrawTransPic(54, 32 + m_net_cursor * 20,R_CachePic(va("gfx/menudot%i.lmp", f + 1)));
}


void M_Net_Key(int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_MultiPlayer_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
		{
			m_net_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		if (--m_net_cursor < 0)
		{
			m_net_cursor = m_net_items - 1;
		}
		break;

	case K_ENTER:
		m_entersound = true;

		switch (m_net_cursor)
		{
		case 2:
			M_Menu_LanConfig_f();
			break;

		case 3:
			M_Menu_LanConfig_f();
			break;

		case 4:
// multiprotocol
			break;
		}
	}

	if (m_net_cursor == 0)
	{
		goto again;
	}
	if (m_net_cursor == 1)
	{
		goto again;
	}
	if (m_net_cursor == 2)
	{
		goto again;
	}
	if (m_net_cursor == 3 && !tcpipAvailable)
	{
		goto again;
	}
}

//=============================================================================
/* OPTIONS MENU */

#define OPTIONS_ITEMS   13

#define SLIDER_RANGE    10

int options_cursor;

void M_Menu_Options_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_options;
	m_entersound = true;
}


void M_AdjustSliders(int dir)
{
	S_StartLocalSound("misc/menu3.wav");

	switch (options_cursor)
	{
	case 3:	// screen size
		scr_viewsize->value += dir * 10;
		if (scr_viewsize->value < 30)
		{
			scr_viewsize->value = 30;
		}
		if (scr_viewsize->value > 120)
		{
			scr_viewsize->value = 120;
		}
		Cvar_SetValue("viewsize", scr_viewsize->value);
		break;
	case 4:	// gamma
		r_gamma->value += dir * 0.1;
		if (r_gamma->value < 1)
		{
			r_gamma->value = 1;
		}
		if (r_gamma->value > 2)
		{
			r_gamma->value = 2;
		}
		Cvar_SetValue("r_gamma", r_gamma->value);
		break;
	case 5:	// mouse speed
		cl_sensitivity->value += dir * 0.5;
		if (cl_sensitivity->value < 1)
		{
			cl_sensitivity->value = 1;
		}
		if (cl_sensitivity->value > 11)
		{
			cl_sensitivity->value = 11;
		}
		Cvar_SetValue("sensitivity", cl_sensitivity->value);
		break;
	case 6:	// music volume
#ifdef _WIN32
		bgmvolume->value += dir * 1.0;
#else
		bgmvolume->value += dir * 0.1;
#endif
		if (bgmvolume->value < 0)
		{
			bgmvolume->value = 0;
		}
		if (bgmvolume->value > 1)
		{
			bgmvolume->value = 1;
		}
		Cvar_SetValue("bgmvolume", bgmvolume->value);
		break;
	case 7:	// sfx volume
		s_volume->value += dir * 0.1;
		if (s_volume->value < 0)
		{
			s_volume->value = 0;
		}
		if (s_volume->value > 1)
		{
			s_volume->value = 1;
		}
		Cvar_SetValue("s_volume", s_volume->value);
		break;

	case 8:	// allways run
		if (cl_forwardspeed->value > 200)
		{
			Cvar_SetValue("cl_forwardspeed", 200);
			Cvar_SetValue("cl_backspeed", 200);
		}
		else
		{
			Cvar_SetValue("cl_forwardspeed", 400);
			Cvar_SetValue("cl_backspeed", 400);
		}
		break;

	case 9:	// invert mouse
		Cvar_SetValue("m_pitch", -m_pitch->value);
		break;

	case 10:	// lookspring
		Cvar_SetValue("lookspring", !lookspring->value);
		break;
	}
}


void M_DrawSlider(int x, int y, float range)
{
	int i;

	if (range < 0)
	{
		range = 0;
	}
	if (range > 1)
	{
		range = 1;
	}
	M_DrawCharacter(x - 8, y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		M_DrawCharacter(x + i * 8, y, 129);
	M_DrawCharacter(x + i * 8, y, 130);
	M_DrawCharacter(x + (SLIDER_RANGE - 1) * 8 * range, y, 131);
}

void M_DrawCheckbox(int x, int y, int on)
{
#if 0
	if (on)
	{
		M_DrawCharacter(x, y, 131);
	}
	else
	{
		M_DrawCharacter(x, y, 129);
	}
#endif
	if (on)
	{
		M_Print(x, y, "on");
	}
	else
	{
		M_Print(x, y, "off");
	}
}

void M_Options_Draw(void)
{
	float r;
	image_t* p;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_option.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	M_Print(16, 32, "    Customize controls");
	M_Print(16, 40, "         Go to console");
	M_Print(16, 48, "     Reset to defaults");

	M_Print(16, 56, "           Screen size");
	r = (scr_viewsize->value - 30) / (120 - 30);
	M_DrawSlider(220, 56, r);

	M_Print(16, 64, "            Brightness");
	r = (r_gamma->value - 1);
	M_DrawSlider(220, 64, r);

	M_Print(16, 72, "           Mouse Speed");
	r = (cl_sensitivity->value - 1) / 10;
	M_DrawSlider(220, 72, r);

	M_Print(16, 80, "       CD Music Volume");
	r = bgmvolume->value;
	M_DrawSlider(220, 80, r);

	M_Print(16, 88, "          Sound Volume");
	r = s_volume->value;
	M_DrawSlider(220, 88, r);

	M_Print(16, 96,  "            Always Run");
	M_DrawCheckbox(220, 96, cl_forwardspeed->value > 200);

	M_Print(16, 104, "          Invert Mouse");
	M_DrawCheckbox(220, 104, m_pitch->value < 0);

	M_Print(16, 112, "            Lookspring");
	M_DrawCheckbox(220, 112, lookspring->value);

	M_Print(16, 120, "         Video Options");

// cursor
	M_DrawCharacter(200, 32 + options_cursor * 8, 12 + ((int)(realtime * 4) & 1));
}


void M_Options_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_ENTER:
		m_entersound = true;
		switch (options_cursor)
		{
		case 0:
			M_Menu_Keys_f();
			break;
		case 1:
			m_state = m_none;
			Con_ToggleConsole_f();
			break;
		case 2:
			Cbuf_AddText("exec default.cfg\n");
			break;
		case 11:
			M_Menu_Video_f();
			break;
		default:
			M_AdjustSliders(1);
			break;
		}
		return;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
		{
			options_cursor = OPTIONS_ITEMS - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
		{
			options_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		M_AdjustSliders(-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders(1);
		break;
	}
}

//=============================================================================
/* KEYS MENU */

const char* bindnames[][2] =
{
	{"+attack",         "attack"},
	{"impulse 10",      "change weapon"},
	{"+jump",           "jump / swim up"},
	{"+forward",        "walk forward"},
	{"+back",           "backpedal"},
	{"+left",           "turn left"},
	{"+right",          "turn right"},
	{"+speed",          "run"},
	{"+moveleft",       "step left"},
	{"+moveright",      "step right"},
	{"+strafe",         "sidestep"},
	{"+lookup",         "look up"},
	{"+lookdown",       "look down"},
	{"centerview",      "center view"},
	{"+mlook",          "mouse look"},
	{"+moveup",         "swim up"},
	{"+movedown",       "swim down"}
};

#define NUMCOMMANDS (sizeof(bindnames) / sizeof(bindnames[0]))

int keys_cursor;
int bind_grab;

void M_Menu_Keys_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_keys;
	m_entersound = true;
}


void M_FindKeysForCommand(const char* command, int* twokeys)
{
	int count;
	int j;
	int l;
	char* b;

	twokeys[0] = twokeys[1] = -1;
	l = String::Length(command);
	count = 0;

	for (j = 0; j < 256; j++)
	{
		b = keys[j].binding;
		if (!b)
		{
			continue;
		}
		if (!String::NCmp(b, command, l))
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
			{
				break;
			}
		}
	}
}

void M_UnbindCommand(const char* command)
{
	int j;
	int l;
	char* b;

	l = String::Length(command);

	for (j = 0; j < 256; j++)
	{
		b = keys[j].binding;
		if (!b)
		{
			continue;
		}
		if (!String::NCmp(b, command, l))
		{
			Key_SetBinding(j, "");
		}
	}
}


void M_Keys_Draw(void)
{
	int i, l;
	int keys[2];
	const char* name;
	int x, y;
	image_t* p;

	p = R_CachePic("gfx/ttl_cstm.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	if (bind_grab)
	{
		M_Print(12, 32, "Press a key or button for this action");
	}
	else
	{
		M_Print(18, 32, "Enter to change, backspace to clear");
	}

// search for known bindings
	for (i = 0; i < (int)NUMCOMMANDS; i++)
	{
		y = 48 + 8 * i;

		M_Print(16, y, bindnames[i][1]);

		l = String::Length(bindnames[i][0]);

		M_FindKeysForCommand(bindnames[i][0], keys);

		if (keys[0] == -1)
		{
			M_Print(140, y, "???");
		}
		else
		{
			name = Key_KeynumToString(keys[0]);
			M_Print(140, y, name);
			x = String::Length(name) * 8;
			if (keys[1] != -1)
			{
				M_Print(140 + x + 8, y, "or");
				M_Print(140 + x + 32, y, Key_KeynumToString(keys[1]));
			}
		}
	}

	if (bind_grab)
	{
		M_DrawCharacter(130, 48 + keys_cursor * 8, '=');
	}
	else
	{
		M_DrawCharacter(130, 48 + keys_cursor * 8, 12 + ((int)(realtime * 4) & 1));
	}
}


void M_Keys_Key(int k)
{
	char cmd[80];
	int keys[2];

	if (bind_grab)
	{	// defining a key
		S_StartLocalSound("misc/menu1.wav");
		if (k == K_ESCAPE)
		{
			bind_grab = false;
		}
		else if (k != '`')
		{
			sprintf(cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString(k), bindnames[keys_cursor][0]);
			Cbuf_InsertText(cmd);
		}

		bind_grab = false;
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
		M_Menu_Options_f();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		keys_cursor--;
		if (keys_cursor < 0)
		{
			keys_cursor = NUMCOMMANDS - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("misc/menu1.wav");
		keys_cursor++;
		if (keys_cursor >= (int)NUMCOMMANDS)
		{
			keys_cursor = 0;
		}
		break;

	case K_ENTER:		// go into bind mode
		M_FindKeysForCommand(bindnames[keys_cursor][0], keys);
		S_StartLocalSound("misc/menu2.wav");
		if (keys[1] != -1)
		{
			M_UnbindCommand(bindnames[keys_cursor][0]);
		}
		bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_StartLocalSound("misc/menu2.wav");
		M_UnbindCommand(bindnames[keys_cursor][0]);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

#define MAX_COLUMN_SIZE     9
#define MODE_AREA_HEIGHT    (MAX_COLUMN_SIZE + 2)

void M_Menu_Video_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_video;
	m_entersound = true;
}


void M_Video_Draw(void)
{
	image_t* p = R_CachePic("gfx/vidmodes.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	M_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 2,
		"Video modes must be set from the");
	M_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 3,
		"console with set r_mode <number>");
	M_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 4,
		"and set r_colorbits <bits-per-pixel>");
	M_Print(3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 6,
		"Select windowed mode with set r_fullscreen 0");
}


void M_Video_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		S_StartLocalSound("misc/menu1.wav");
		M_Menu_Options_f();
		break;

	default:
		break;
	}
}

//=============================================================================
/* HELP MENU */

int help_page;
#define NUM_HELP_PAGES  6


void M_Menu_Help_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
}



void M_Help_Draw(void)
{
	M_DrawPic(0, 0, R_CachePic(va("gfx/help%i.lmp", help_page)));
}


void M_Help_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Main_f();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
		{
			help_page = 0;
		}
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
		{
			help_page = NUM_HELP_PAGES - 1;
		}
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int msgNumber;
qboolean wasInMenus;

#ifndef _WIN32
const char* quitMessage [] =
{
/* .........1.........2.... */
	"  Are you gonna quit    ",
	"  this game just like   ",
	"   everything else?     ",
	"                        ",

	" Milord, methinks that  ",
	"   thou art a lowly     ",
	" quitter. Is this true? ",
	"                        ",

	" Do I need to bust your ",
	"  face open for trying  ",
	"        to quit?        ",
	"                        ",

	" Man, I oughta smack you",
	"   for trying to quit!  ",
	"     Press Y to get     ",
	"      smacked out.      ",

	" Press Y to quit like a ",
	"   big loser in life.   ",
	"  Press N to stay proud ",
	"    and successful!     ",

	"   If you press Y to    ",
	"  quit, I will summon   ",
	"  Satan all over your   ",
	"      hard drive!       ",

	"  Um, Asmodeus dislikes ",
	" his children trying to ",
	" quit. Press Y to return",
	"   to your Tinkertoys.  ",

	"  If you quit now, I'll ",
	"  throw a blanket-party ",
	"   for you next time!   ",
	"                        "
};
#endif

void M_Menu_Quit_f(void)
{
	if (m_state == m_quit)
	{
		return;
	}
	wasInMenus = !!(in_keyCatchers & KEYCATCH_UI);
	in_keyCatchers |= KEYCATCH_UI;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand() & 7;
}


void M_Quit_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		in_keyCatchers |= KEYCATCH_CONSOLE;
		Host_Quit_f();
		break;

	default:
		break;
	}

}


void M_Quit_Draw(void)
{
	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw();
		m_state = m_quit;
	}

#ifdef _WIN32
	M_DrawTextBox(0, 0, 38, 23);
	M_PrintWhite(16, 12,  "  Quake version 1.09 by id Software\n\n");
	M_PrintWhite(16, 28,  "Programming        Art \n");
	M_Print(16, 36,  " John Carmack       Adrian Carmack\n");
	M_Print(16, 44,  " Michael Abrash     Kevin Cloud\n");
	M_Print(16, 52,  " John Cash          Paul Steed\n");
	M_Print(16, 60,  " Dave 'Zoid' Kirsch\n");
	M_PrintWhite(16, 68,  "Design             Biz\n");
	M_Print(16, 76,  " John Romero        Jay Wilbur\n");
	M_Print(16, 84,  " Sandy Petersen     Mike Wilson\n");
	M_Print(16, 92,  " American McGee     Donna Jackson\n");
	M_Print(16, 100,  " Tim Willits        Todd Hollenshead\n");
	M_PrintWhite(16, 108, "Support            Projects\n");
	M_Print(16, 116, " Barrett Alexander  Shawn Green\n");
	M_PrintWhite(16, 124, "Sound Effects\n");
	M_Print(16, 132, " Trent Reznor and Nine Inch Nails\n\n");
	M_PrintWhite(16, 140, "Quake is a trademark of Id Software,\n");
	M_PrintWhite(16, 148, "inc., (c)1996 Id Software, inc. All\n");
	M_PrintWhite(16, 156, "rights reserved. NIN logo is a\n");
	M_PrintWhite(16, 164, "registered trademark licensed to\n");
	M_PrintWhite(16, 172, "Nothing Interactive, Inc. All rights\n");
	M_PrintWhite(16, 180, "reserved. Press y to exit\n");
#else
	M_DrawTextBox(56, 76, 24, 4);
	M_Print(64, 84,  quitMessage[msgNumber * 4 + 0]);
	M_Print(64, 92,  quitMessage[msgNumber * 4 + 1]);
	M_Print(64, 100, quitMessage[msgNumber * 4 + 2]);
	M_Print(64, 108, quitMessage[msgNumber * 4 + 3]);
#endif
}

//=============================================================================
/* LAN CONFIG MENU */

int lanConfig_cursor = -1;
int lanConfig_cursor_table [] = {52, 72, 104};
#define NUM_LANCONFIG_CMDS  3

int lanConfig_port;
field_t lanConfig_portname;
field_t lanConfig_joinname;

void M_Menu_LanConfig_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame)
		{
			lanConfig_cursor = 2;
		}
		else
		{
			lanConfig_cursor = 1;
		}
	}
	if (StartingGame && lanConfig_cursor == 2)
	{
		lanConfig_cursor = 1;
	}
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname.buffer, "%u", lanConfig_port);
	lanConfig_portname.cursor = String::Length(lanConfig_portname.buffer);
	lanConfig_portname.maxLength = 5;
	lanConfig_portname.widthInChars = 6;
	Field_Clear(&lanConfig_joinname);
	lanConfig_joinname.maxLength = 29;
	lanConfig_joinname.widthInChars = 22;

	m_return_onerror = false;
	m_return_reason[0] = 0;
}


void M_LanConfig_Draw(void)
{
	image_t* p;
	int basex;
	const char* startJoin;
	const char* protocol;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_multi.lmp");
	basex = (320 - R_GetImageWidth(p)) / 2;
	M_DrawPic(basex, 4, p);

	if (StartingGame)
	{
		startJoin = "New Game";
	}
	else
	{
		startJoin = "Join Game";
	}
	protocol = "TCP/IP";
	M_Print(basex, 32, va("%s - %s", startJoin, protocol));
	basex += 8;

	M_Print(basex, lanConfig_cursor_table[0], "Port");
	M_DrawField(basex + 9 * 8, lanConfig_cursor_table[0], &lanConfig_portname, lanConfig_cursor == 0);

	if (JoiningGame)
	{
		M_Print(basex, lanConfig_cursor_table[1], "Search for local games...");
		M_Print(basex, 88, "Join game at:");
		M_DrawField(basex + 16, lanConfig_cursor_table[2], &lanConfig_joinname, lanConfig_cursor == 2);
	}
	else
	{
		M_DrawTextBox(basex, lanConfig_cursor_table[1] - 8, 2, 1);
		M_Print(basex + 8, lanConfig_cursor_table[1], "OK");
	}

	M_DrawCharacter(basex - 8, lanConfig_cursor_table [lanConfig_cursor], 12 + ((int)(realtime * 4) & 1));

	if (*m_return_reason)
	{
		M_PrintWhite(basex, 128, m_return_reason);
	}
}


void M_LanConfig_Key(int key)
{
	int l;

	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f();
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
		{
			lanConfig_cursor = NUM_LANCONFIG_CMDS - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
		{
			lanConfig_cursor = 0;
		}
		break;

	case K_ENTER:
		if (lanConfig_cursor == 0)
		{
			break;
		}

		m_entersound = true;

		M_ConfigureNetSubsystem();

		if (lanConfig_cursor == 1)
		{
			if (StartingGame)
			{
				M_Menu_GameOptions_f();
				break;
			}
			M_Menu_Search_f();
			break;
		}

		if (lanConfig_cursor == 2)
		{
			m_return_state = m_state;
			m_return_onerror = true;
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			Cbuf_AddText(va("connect \"%s\"\n", lanConfig_joinname.buffer));
			break;
		}

		break;
	}
	if (lanConfig_cursor == 0)
	{
		Field_KeyDownEvent(&lanConfig_portname, key);
	}
	if (lanConfig_cursor == 2)
	{
		Field_KeyDownEvent(&lanConfig_joinname, key);
	}

	if (StartingGame && lanConfig_cursor == 2)
	{
		if (key == K_UPARROW)
		{
			lanConfig_cursor = 1;
		}
		else
		{
			lanConfig_cursor = 0;
		}
	}

	l =  String::Atoi(lanConfig_portname.buffer);
	if (l > 65535)
	{
		sprintf(lanConfig_portname.buffer, "%u", lanConfig_port);
		lanConfig_portname.cursor = String::Length(lanConfig_portname.buffer);
	}
	else
	{
		lanConfig_port = l;
	}
}

void M_LanConfig_Char(int key)
{
	if (lanConfig_cursor == 0)
	{
		if (key >= 32 && (key < '0' || key > '9'))
		{
			return;
		}
		Field_CharEvent(&lanConfig_portname, key);
	}
	if (lanConfig_cursor == 2)
	{
		Field_CharEvent(&lanConfig_joinname, key);
	}
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	const char* name;
	const char* description;
} level_t;

level_t levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t hipnoticlevels[] =
{
	{"start", "Command HQ"},// 0

	{"hip1m1", "The Pumping Station"},			// 1
	{"hip1m2", "Storage Facility"},
	{"hip1m3", "The Lost Mine"},
	{"hip1m4", "Research Facility"},
	{"hip1m5", "Military Complex"},

	{"hip2m1", "Ancient Realms"},			// 6
	{"hip2m2", "The Black Cathedral"},
	{"hip2m3", "The Catacombs"},
	{"hip2m4", "The Crypt"},
	{"hip2m5", "Mortum's Keep"},
	{"hip2m6", "The Gremlin's Domain"},

	{"hip3m1", "Tur Torment"},		// 12
	{"hip3m2", "Pandemonium"},
	{"hip3m3", "Limbo"},
	{"hip3m4", "The Gauntlet"},

	{"hipend", "Armagon's Lair"},		// 16

	{"hipdm1", "The Edge of Oblivion"}			// 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t roguelevels[] =
{
	{"start",   "Split Decision"},
	{"r1m1",    "Deviant's Domain"},
	{"r1m2",    "Dread Portal"},
	{"r1m3",    "Judgement Call"},
	{"r1m4",    "Cave of Death"},
	{"r1m5",    "Towers of Wrath"},
	{"r1m6",    "Temple of Pain"},
	{"r1m7",    "Tomb of the Overlord"},
	{"r2m1",    "Tempus Fugit"},
	{"r2m2",    "Elemental Fury I"},
	{"r2m3",    "Elemental Fury II"},
	{"r2m4",    "Curse of Osiris"},
	{"r2m5",    "Wizard's Keep"},
	{"r2m6",    "Blood Sacrifice"},
	{"r2m7",    "Last Bastion"},
	{"r2m8",    "Source of Evil"},
	{"ctf1",    "Division of Change"}
};

typedef struct
{
	const char* description;
	int firstLevel;
	int levels;
} episode_t;

episode_t episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t hipnoticepisodes[] =
{
	{"Scourge of Armagon", 0, 1},
	{"Fortress of the Dead", 1, 5},
	{"Dominion of Darkness", 6, 6},
	{"The Rift", 12, 4},
	{"Final Level", 16, 1},
	{"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

int startepisode;
int startlevel;
int maxplayers;
qboolean m_serverInfoMessage = false;
double m_serverInfoMessageTime;

void M_Menu_GameOptions_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_gameoptions;
	m_entersound = true;
	if (maxplayers == 0)
	{
		maxplayers = svs.maxclients;
	}
	if (maxplayers < 2)
	{
		maxplayers = svs.maxclientslimit;
	}
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 112, 120};
#define NUM_GAMEOPTIONS 9
int gameoptions_cursor;

void M_GameOptions_Draw(void)
{
	image_t* p;
	int x;

	M_DrawTransPic(16, 4, R_CachePic("gfx/qplaque.lmp"));
	p = R_CachePic("gfx/p_multi.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);

	M_DrawTextBox(152, 32, 10, 1);
	M_Print(160, 40, "begin game");

	M_Print(0, 56, "      Max players");
	M_Print(160, 56, va("%i", maxplayers));

	M_Print(0, 64, "        Game Type");
	if (coop->value)
	{
		M_Print(160, 64, "Cooperative");
	}
	else
	{
		M_Print(160, 64, "Deathmatch");
	}

	M_Print(0, 72, "        Teamplay");
	if (rogue)
	{
		const char* msg;

		switch ((int)teamplay->value)
		{
		case 1: msg = "No Friendly Fire"; break;
		case 2: msg = "Friendly Fire"; break;
		case 3: msg = "Tag"; break;
		case 4: msg = "Capture the Flag"; break;
		case 5: msg = "One Flag CTF"; break;
		case 6: msg = "Three Team CTF"; break;
		default: msg = "Off"; break;
		}
		M_Print(160, 72, msg);
	}
	else
	{
		const char* msg;

		switch ((int)teamplay->value)
		{
		case 1: msg = "No Friendly Fire"; break;
		case 2: msg = "Friendly Fire"; break;
		default: msg = "Off"; break;
		}
		M_Print(160, 72, msg);
	}

	M_Print(0, 80, "            Skill");
	if (skill->value == 0)
	{
		M_Print(160, 80, "Easy difficulty");
	}
	else if (skill->value == 1)
	{
		M_Print(160, 80, "Normal difficulty");
	}
	else if (skill->value == 2)
	{
		M_Print(160, 80, "Hard difficulty");
	}
	else
	{
		M_Print(160, 80, "Nightmare difficulty");
	}

	M_Print(0, 88, "       Frag Limit");
	if (fraglimit->value == 0)
	{
		M_Print(160, 88, "none");
	}
	else
	{
		M_Print(160, 88, va("%i frags", (int)fraglimit->value));
	}

	M_Print(0, 96, "       Time Limit");
	if (timelimit->value == 0)
	{
		M_Print(160, 96, "none");
	}
	else
	{
		M_Print(160, 96, va("%i minutes", (int)timelimit->value));
	}

	M_Print(0, 112, "         Episode");
	//MED 01/06/97 added hipnotic episodes
	if (hipnotic)
	{
		M_Print(160, 112, hipnoticepisodes[startepisode].description);
	}
	//PGM 01/07/97 added rogue episodes
	else if (rogue)
	{
		M_Print(160, 112, rogueepisodes[startepisode].description);
	}
	else
	{
		M_Print(160, 112, episodes[startepisode].description);
	}

	M_Print(0, 120, "           Level");
	//MED 01/06/97 added hipnotic episodes
	if (hipnotic)
	{
		M_Print(160, 120, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
		M_Print(160, 128, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
	}
	//PGM 01/07/97 added rogue episodes
	else if (rogue)
	{
		M_Print(160, 120, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
		M_Print(160, 128, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
	}
	else
	{
		M_Print(160, 120, levels[episodes[startepisode].firstLevel + startlevel].description);
		M_Print(160, 128, levels[episodes[startepisode].firstLevel + startlevel].name);
	}

// line cursor
	M_DrawCharacter(144, gameoptions_cursor_table[gameoptions_cursor], 12 + ((int)(realtime * 4) & 1));

	if (m_serverInfoMessage)
	{
		if ((realtime - m_serverInfoMessageTime) < 5.0)
		{
			x = (320 - 26 * 8) / 2;
			M_DrawTextBox(x, 138, 24, 4);
			x += 8;
			M_Print(x, 146, "  More than 4 players   ");
			M_Print(x, 154, " requires using command ");
			M_Print(x, 162, "line parameters; please ");
			M_Print(x, 170, "   see techinfo.txt.    ");
		}
		else
		{
			m_serverInfoMessage = false;
		}
	}
}


void M_NetStart_Change(int dir)
{
	int count;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
		{
			maxplayers = svs.maxclientslimit;
			m_serverInfoMessage = true;
			m_serverInfoMessageTime = realtime;
		}
		if (maxplayers < 2)
		{
			maxplayers = 2;
		}
		break;

	case 2:
		Cvar_SetValue("coop", coop->value ? 0 : 1);
		break;

	case 3:
		if (rogue)
		{
			count = 6;
		}
		else
		{
			count = 2;
		}

		Cvar_SetValue("teamplay", teamplay->value + dir);
		if (teamplay->value > count)
		{
			Cvar_SetValue("teamplay", 0);
		}
		else if (teamplay->value < 0)
		{
			Cvar_SetValue("teamplay", count);
		}
		break;

	case 4:
		Cvar_SetValue("skill", skill->value + dir);
		if (skill->value > 3)
		{
			Cvar_SetValue("skill", 0);
		}
		if (skill->value < 0)
		{
			Cvar_SetValue("skill", 3);
		}
		break;

	case 5:
		Cvar_SetValue("fraglimit", fraglimit->value + dir * 10);
		if (fraglimit->value > 100)
		{
			Cvar_SetValue("fraglimit", 0);
		}
		if (fraglimit->value < 0)
		{
			Cvar_SetValue("fraglimit", 100);
		}
		break;

	case 6:
		Cvar_SetValue("timelimit", timelimit->value + dir * 5);
		if (timelimit->value > 60)
		{
			Cvar_SetValue("timelimit", 0);
		}
		if (timelimit->value < 0)
		{
			Cvar_SetValue("timelimit", 60);
		}
		break;

	case 7:
		startepisode += dir;
		//MED 01/06/97 added hipnotic count
		if (hipnotic)
		{
			count = 6;
		}
		//PGM 01/07/97 added rogue count
		//PGM 03/02/97 added 1 for dmatch episode
		else if (rogue)
		{
			count = 4;
		}
		else if (registered->value)
		{
			count = 7;
		}
		else
		{
			count = 2;
		}

		if (startepisode < 0)
		{
			startepisode = count - 1;
		}

		if (startepisode >= count)
		{
			startepisode = 0;
		}

		startlevel = 0;
		break;

	case 8:
		startlevel += dir;
		//MED 01/06/97 added hipnotic episodes
		if (hipnotic)
		{
			count = hipnoticepisodes[startepisode].levels;
		}
		//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
		{
			count = rogueepisodes[startepisode].levels;
		}
		else
		{
			count = episodes[startepisode].levels;
		}

		if (startlevel < 0)
		{
			startlevel = count - 1;
		}

		if (startlevel >= count)
		{
			startlevel = 0;
		}
		break;
	}
}

void M_GameOptions_Key(int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_Menu_Net_f();
		break;

	case K_UPARROW:
		S_StartLocalSound("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
		{
			gameoptions_cursor = NUM_GAMEOPTIONS - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
		{
			gameoptions_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		if (gameoptions_cursor == 0)
		{
			break;
		}
		S_StartLocalSound("misc/menu3.wav");
		M_NetStart_Change(-1);
		break;

	case K_RIGHTARROW:
		if (gameoptions_cursor == 0)
		{
			break;
		}
		S_StartLocalSound("misc/menu3.wav");
		M_NetStart_Change(1);
		break;

	case K_ENTER:
		S_StartLocalSound("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
			{
				Cbuf_AddText("disconnect\n");
			}
			Cbuf_AddText("listen 0\n");		// so host_netport will be re-examined
			Cbuf_AddText(va("maxplayers %u\n", maxplayers));
			SCR_BeginLoadingPlaque();

			if (hipnotic)
			{
				Cbuf_AddText(va("map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name));
			}
			else if (rogue)
			{
				Cbuf_AddText(va("map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name));
			}
			else
			{
				Cbuf_AddText(va("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name));
			}

			return;
		}

		M_NetStart_Change(1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean searchComplete = false;
double searchCompleteTime;

void M_Menu_Search_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_search;
	m_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw(void)
{
	image_t* p;
	int x;

	p = R_CachePic("gfx/p_multi.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
	x = (320 / 2) - ((12 * 8) / 2) + 4;
	M_DrawTextBox(x - 8, 32, 12, 1);
	M_Print(x, 40, "Searching...");

	if (slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (!searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f();
		return;
	}

	M_PrintWhite((320 / 2) - ((22 * 8) / 2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
	{
		return;
	}

	M_Menu_LanConfig_f();
}


void M_Search_Key(int key)
{
}

//=============================================================================
/* SLIST MENU */

int slist_cursor;
qboolean slist_sorted;

void M_Menu_ServerList_f(void)
{
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_slist;
	m_entersound = true;
	slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw(void)
{
	int n;
	char string [64];
	image_t* p;

	if (!slist_sorted)
	{
		if (hostCacheCount > 1)
		{
			int i,j;
			hostcache_t temp;
			for (i = 0; i < hostCacheCount; i++)
				for (j = i + 1; j < hostCacheCount; j++)
					if (String::Cmp(hostcache[j].name, hostcache[i].name) < 0)
					{
						Com_Memcpy(&temp, &hostcache[j], sizeof(hostcache_t));
						Com_Memcpy(&hostcache[j], &hostcache[i], sizeof(hostcache_t));
						Com_Memcpy(&hostcache[i], &temp, sizeof(hostcache_t));
					}
		}
		slist_sorted = true;
	}

	p = R_CachePic("gfx/p_multi.lmp");
	M_DrawPic((320 - R_GetImageWidth(p)) / 2, 4, p);
	for (n = 0; n < hostCacheCount; n++)
	{
		if (hostcache[n].maxusers)
		{
			sprintf(string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[n].name, hostcache[n].map, hostcache[n].users, hostcache[n].maxusers);
		}
		else
		{
			sprintf(string, "%-15.15s %-15.15s\n", hostcache[n].name, hostcache[n].map);
		}
		M_Print(16, 32 + 8 * n, string);
	}
	M_DrawCharacter(0, 32 + slist_cursor * 8, 12 + ((int)(realtime * 4) & 1));

	if (*m_return_reason)
	{
		M_PrintWhite(16, 148, m_return_reason);
	}
}


void M_ServerList_Key(int k)
{
	switch (k)
	{
	case K_ESCAPE:
		M_Menu_LanConfig_f();
		break;

	case K_SPACE:
		M_Menu_Search_f();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor < 0)
		{
			slist_cursor = hostCacheCount - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
		{
			slist_cursor = 0;
		}
		break;

	case K_ENTER:
		S_StartLocalSound("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		Cbuf_AddText(va("connect \"%s\"\n", hostcache[slist_cursor].cname));
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */


void M_Init(void)
{
	Cmd_AddCommand("togglemenu", M_ToggleMenu_f);

	Cmd_AddCommand("menu_main", M_Menu_Main_f);
	Cmd_AddCommand("menu_singleplayer", M_Menu_SinglePlayer_f);
	Cmd_AddCommand("menu_load", M_Menu_Load_f);
	Cmd_AddCommand("menu_save", M_Menu_Save_f);
	Cmd_AddCommand("menu_multiplayer", M_Menu_MultiPlayer_f);
	Cmd_AddCommand("menu_setup", M_Menu_Setup_f);
	Cmd_AddCommand("menu_options", M_Menu_Options_f);
	Cmd_AddCommand("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand("menu_video", M_Menu_Video_f);
	Cmd_AddCommand("help", M_Menu_Help_f);
	Cmd_AddCommand("menu_quit", M_Menu_Quit_f);
}


void M_Draw(void)
{
	if (m_state == m_none || !(in_keyCatchers & KEYCATCH_UI))
	{
		return;
	}

	if (!m_recursiveDraw)
	{
		if (con.displayFrac)
		{
			Con_DrawFullBackground();
			S_ExtraUpdate();
		}
		else
		{
			Draw_FadeScreen();
		}
	}
	else
	{
		m_recursiveDraw = false;
	}

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw();
		break;

	case m_load:
		M_Load_Draw();
		break;

	case m_save:
		M_Save_Draw();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw();
		break;

	case m_setup:
		M_Setup_Draw();
		break;

	case m_net:
		M_Net_Draw();
		break;

	case m_options:
		M_Options_Draw();
		break;

	case m_keys:
		M_Keys_Draw();
		break;

	case m_video:
		M_Video_Draw();
		break;

	case m_help:
		M_Help_Draw();
		break;

	case m_quit:
		M_Quit_Draw();
		break;

	case m_lanconfig:
		M_LanConfig_Draw();
		break;

	case m_gameoptions:
		M_GameOptions_Draw();
		break;

	case m_search:
		M_Search_Draw();
		break;

	case m_slist:
		M_ServerList_Draw();
		break;
	}

	if (m_entersound)
	{
		S_StartLocalSound("misc/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate();
}


void M_Keydown(int key)
{
	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key(key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key(key);
		return;

	case m_load:
		M_Load_Key(key);
		return;

	case m_save:
		M_Save_Key(key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key(key);
		return;

	case m_setup:
		M_Setup_Key(key);
		return;

	case m_net:
		M_Net_Key(key);
		return;

	case m_options:
		M_Options_Key(key);
		return;

	case m_keys:
		M_Keys_Key(key);
		return;

	case m_video:
		M_Video_Key(key);
		return;

	case m_help:
		M_Help_Key(key);
		return;

	case m_quit:
		M_Quit_Key(key);
		return;

	case m_lanconfig:
		M_LanConfig_Key(key);
		return;

	case m_gameoptions:
		M_GameOptions_Key(key);
		return;

	case m_search:
		M_Search_Key(key);
		break;

	case m_slist:
		M_ServerList_Key(key);
		return;
	}
}

void M_CharEvent(int key)
{
	switch (m_state)
	{
	case m_setup:
		M_Setup_Char(key);
		break;
	case m_lanconfig:
		M_LanConfig_Char(key);
		break;
	default:
		break;
	}
}

void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config

	Cbuf_AddText("stopdemo\n");

	net_hostport = lanConfig_port;
}
