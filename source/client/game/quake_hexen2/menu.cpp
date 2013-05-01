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

#include "menu.h"
#include "../../client_main.h"
#include "../../public.h"
#include "../../input/keycodes.h"
#include "../../ui/ui.h"
#include "../../renderer/cvars.h"
#include "../../../server/public.h"
#include "../quake/local.h"
#include "../hexen2/local.h"
#include "network_channel.h"
#include "main.h"
#include "../../ui/console.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"

menu_state_t m_state;
menu_state_t m_return_state;

bool m_return_onerror;
char m_return_reason[ 32 ];

static qhandle_t char_menufonttexture;
static char BigCharWidth[ 27 ][ 27 ];

static float TitlePercent = 0;
static float TitleTargetPercent = 1;
static float LogoPercent = 0;
static float LogoTargetPercent = 1;

static bool mqh_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

static const char* mh2_message;
static const char* mh2_message2;
static int mh2_message_time;

static Cvar* mh2_oldmission;

static int setup_class;

static void MQH_Menu_SinglePlayer_f();
static void MQH_Menu_SinglePlayerConfirm_f();
static void MH2_Menu_Class_f();
static void MH2_Menu_Difficulty_f();
static void MQH_Menu_Load_f();
static void MQH_Menu_Save_f();
static void MQH_Menu_MultiPlayer_f();
static void MQH_Menu_LanConfig_f();
static void MQH_Menu_Search_f();
static void MQH_Menu_ServerList_f();
static void MHW_Menu_Connect_f();
static void MQH_Menu_GameOptions_f();
static void MQH_Menu_Setup_f();
static void MH2_Menu_MLoad_f();
static void MH2_Menu_MSave_f();
static void MQH_Menu_Options_f();
static void MQH_Menu_Keys_f();
static void MQH_Menu_Video_f();
static void MQH_Menu_Help_f();

void MQH_DrawPic( int x, int y, image_t* pic ) {
	UI_DrawPic( x + ( ( viddef.width - 320 ) >> 1 ), y, pic );
}

void MQH_DrawPicShader( int x, int y, qhandle_t shader ) {
	UI_DrawPicShader( x + ( ( viddef.width - 320 ) >> 1 ), y, shader );
}

//	Draws one solid graphics character
static void MQH_DrawCharacter( int cx, int line, int num ) {
	UI_DrawChar( cx + ( ( viddef.width - 320 ) >> 1 ), line, num );
}

void MQH_Print( int cx, int cy, const char* str ) {
	UI_DrawString( cx + ( ( viddef.width - 320 ) >> 1 ), cy, str, GGameType & GAME_Hexen2 ? 256 : 128 );
}

void MQH_PrintWhite( int cx, int cy, const char* str ) {
	UI_DrawString( cx + ( ( viddef.width - 320 ) >> 1 ), cy, str );
}

void MQH_DrawTextBox( int x, int y, int width, int lines ) {
	// draw left side
	int cx = x;
	int cy = y;
	image_t* p = R_CachePic( "gfx/box_tl.lmp" );
	MQH_DrawPic( cx, cy, p );
	p = R_CachePic( "gfx/box_ml.lmp" );
	for ( int n = 0; n < lines; n++ ) {
		cy += 8;
		MQH_DrawPic( cx, cy, p );
	}
	p = R_CachePic( "gfx/box_bl.lmp" );
	MQH_DrawPic( cx, cy + 8, p );

	// draw middle
	cx += 8;
	while ( width > 0 ) {
		cy = y;
		p = R_CachePic( "gfx/box_tm.lmp" );
		MQH_DrawPic( cx, cy, p );
		p = R_CachePic( "gfx/box_mm.lmp" );
		for ( int n = 0; n < lines; n++ ) {
			cy += 8;
			if ( n == 1 ) {
				p = R_CachePic( "gfx/box_mm2.lmp" );
			}
			MQH_DrawPic( cx, cy, p );
		}
		p = R_CachePic( "gfx/box_bm.lmp" );
		MQH_DrawPic( cx, cy + 8, p );
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = R_CachePic( "gfx/box_tr.lmp" );
	MQH_DrawPic( cx, cy, p );
	p = R_CachePic( "gfx/box_mr.lmp" );
	for ( int n = 0; n < lines; n++ ) {
		cy += 8;
		MQH_DrawPic( cx, cy, p );
	}
	p = R_CachePic( "gfx/box_br.lmp" );
	MQH_DrawPic( cx, cy + 8, p );
}

void MQH_DrawTextBox2( int x, int y, int width, int lines ) {
	MQH_DrawTextBox( x,  y + ( ( viddef.height - 200 ) >> 1 ), width, lines );
}

static void MQH_DrawField( int x, int y, field_t* edit, bool showCursor ) {
	MQH_DrawTextBox( x - 8, y - 8, edit->widthInChars, 1 );
	Field_Draw( edit, x + ( ( viddef.width - 320 ) >> 1 ), y, showCursor );
}

static void MH2_ReadBigCharWidth() {
	void* data;
	FS_ReadFile( "gfx/menu/fontsize.lmp", &data );
	Com_Memcpy( BigCharWidth, data, sizeof ( BigCharWidth ) );
	FS_FreeFile( data );
}

static int MH2_DrawBigCharacter( int x, int y, int num, int numNext ) {
	if ( num == ' ' ) {
		return 32;
	}

	if ( num == '/' ) {
		num = 26;
	} else   {
		num -= 65;
	}

	if ( num < 0 || num >= 27 ) {	// only a-z and /
		return 0;
	}

	if ( numNext == '/' ) {
		numNext = 26;
	} else   {
		numNext -= 65;
	}

	UI_DrawCharBase( x, y, num, 20, 20, char_menufonttexture, 8, 4 );

	if ( numNext < 0 || numNext >= 27 ) {
		return 0;
	}

	int add = 0;
	if ( num == ( int )'C' - 65 && numNext == ( int )'P' - 65 ) {
		add = 3;
	}

	return BigCharWidth[ num ][ numNext ] + add;
}

static void MH2_DrawBigString( int x, int y, const char* string ) {
	x += ( ( viddef.width - 320 ) >> 1 );

	int length = String::Length( string );
	for ( int c = 0; c < length; c++ ) {
		x += MH2_DrawBigCharacter( x, y, string[ c ], string[ c + 1 ] );
	}
}

static void MH2_ScrollTitle( const char* name ) {
	static const char* LastName = "";
	static bool CanSwitch = true;

	float delta;
	if ( TitlePercent < TitleTargetPercent ) {
		delta = ( ( TitleTargetPercent - TitlePercent ) / 0.5 ) * cls.frametime * 0.001;
		if ( delta < 0.004 ) {
			delta = 0.004;
		}
		TitlePercent += delta;
		if ( TitlePercent > TitleTargetPercent ) {
			TitlePercent = TitleTargetPercent;
		}
	} else if ( TitlePercent > TitleTargetPercent )     {
		delta = ( ( TitlePercent - TitleTargetPercent ) / 0.15 ) * cls.frametime * 0.001;
		if ( delta < 0.02 ) {
			delta = 0.02;
		}
		TitlePercent -= delta;
		if ( TitlePercent <= TitleTargetPercent ) {
			TitlePercent = TitleTargetPercent;
			CanSwitch = true;
		}
	}

	if ( LogoPercent < LogoTargetPercent ) {
		delta = ( ( LogoTargetPercent - LogoPercent ) / 0.15 ) * cls.frametime * 0.001;
		if ( delta < 0.02 ) {
			delta = 0.02;
		}
		LogoPercent += delta;
		if ( LogoPercent > LogoTargetPercent ) {
			LogoPercent = LogoTargetPercent;
		}
	}

	if ( String::ICmp( LastName,name ) != 0 && TitleTargetPercent != 0 ) {
		TitleTargetPercent = 0;
	}

	if ( CanSwitch ) {
		LastName = name;
		CanSwitch = false;
		TitleTargetPercent = 1;
	}

	image_t* p = R_CachePic( LastName );
	int finaly = ( ( float )R_GetImageHeight( p ) * TitlePercent ) - R_GetImageHeight( p );
	MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, finaly, p );

	if ( m_state != m_keys ) {
		p = R_CachePic( "gfx/menu/hplaque.lmp" );
		finaly = ( ( float )R_GetImageHeight( p ) * LogoPercent ) - R_GetImageHeight( p );
		MQH_DrawPic( 10, finaly, p );
	}
}

//=============================================================================
/* MAIN MENU */

#define MAIN_ITEMS      5
#define MAIN_ITEMS_HW   4

static int mqh_main_cursor;
static int mqh_save_demonum;

void MQH_Menu_Main_f() {
	if ( GGameType & GAME_Hexen2 && m_state == m_none ) {
		LogoTargetPercent = TitleTargetPercent = 1;
		LogoPercent = TitlePercent = 0;
	}
	if ( !( in_keyCatchers & KEYCATCH_UI ) ) {
		mqh_save_demonum = cls.qh_demonum;
		cls.qh_demonum = -1;
	}
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_main;
	mqh_entersound = true;
}

static void MQH_Main_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title0.lmp" );
		if ( GGameType & GAME_HexenWorld ) {
			MH2_DrawBigString( 72, 60 + ( 0 * 20 ), "MULTIPLAYER" );
			MH2_DrawBigString( 72, 60 + ( 1 * 20 ), "OPTIONS" );
			MH2_DrawBigString( 72, 60 + ( 2 * 20 ), "HELP" );
			MH2_DrawBigString( 72, 60 + ( 3 * 20 ), "QUIT" );
		} else   {
			MH2_DrawBigString( 72, 60 + ( 0 * 20 ), "SINGLE PLAYER" );
			MH2_DrawBigString( 72, 60 + ( 1 * 20 ), "MULTIPLAYER" );
			MH2_DrawBigString( 72, 60 + ( 2 * 20 ), "OPTIONS" );
			MH2_DrawBigString( 72, 60 + ( 3 * 20 ), "HELP" );
			MH2_DrawBigString( 72, 60 + ( 4 * 20 ), "QUIT" );
		}

		int f = ( cls.realtime / 100 ) % 8;
		MQH_DrawPic( 43, 54 + mqh_main_cursor * 20,R_CachePic( va( "gfx/menu/menudot%i.lmp", f + 1 ) ) );
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/ttl_main.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		MQH_DrawPic( 72, 32, R_CachePic( "gfx/mainmenu.lmp" ) );

		int f = ( cls.realtime / 100 ) % 6;

		MQH_DrawPic( 54, 32 + mqh_main_cursor * 20,R_CachePic( va( "gfx/menudot%i.lmp", f + 1 ) ) );
	}
}

static void MQH_Main_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		cls.qh_demonum = mqh_save_demonum;
		if ( cls.qh_demonum != -1 && !clc.demoplaying && cls.state == CA_DISCONNECTED ) {
			CL_NextDemo();
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( ++mqh_main_cursor >= ( GGameType & GAME_HexenWorld ? MAIN_ITEMS_HW : MAIN_ITEMS ) ) {
			mqh_main_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( --mqh_main_cursor < 0 ) {
			mqh_main_cursor = ( GGameType & GAME_HexenWorld ? MAIN_ITEMS_HW : MAIN_ITEMS ) - 1;
		}
		break;

	case K_ENTER:
		mqh_entersound = true;

		if ( GGameType & GAME_HexenWorld ) {
			switch ( mqh_main_cursor ) {
			case 4:
				MQH_Menu_SinglePlayer_f();
				break;

			case 0:
				MQH_Menu_MultiPlayer_f();
				break;

			case 1:
				MQH_Menu_Options_f();
				break;

			case 2:
				MQH_Menu_Help_f();
				break;

			case 3:
				MQH_Menu_Quit_f();
				break;
			}
		} else   {
			switch ( mqh_main_cursor ) {
			case 0:
				MQH_Menu_SinglePlayer_f();
				break;

			case 1:
				MQH_Menu_MultiPlayer_f();
				break;

			case 2:
				MQH_Menu_Options_f();
				break;

			case 3:
				MQH_Menu_Help_f();
				break;

			case 4:
				MQH_Menu_Quit_f();
				break;
			}
		}
	}
}

//=============================================================================
/* SINGLE PLAYER MENU */

#define SINGLEPLAYER_ITEMS      3
#define SINGLEPLAYER_ITEMS_H2MP 5

static int mqh_singleplayer_cursor;
static bool mh2_enter_portals;

static void MQH_Menu_SinglePlayer_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_singleplayer;
	if ( !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
		mqh_entersound = true;
		if ( GGameType & GAME_Hexen2 ) {
			Cvar_SetValue( "timelimit", 0 );		//put this here to help play single after dm
		}
	}
}

static void MQH_SinglePlayer_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title1.lmp" );

		if ( GGameType & GAME_HexenWorld ) {
			MQH_DrawTextBox( 60, 10 * 8, 23, 4 );
			MQH_PrintWhite( 92, 12 * 8, "HexenWorld is for" );
			MQH_PrintWhite( 88, 13 * 8, "Internet play only" );
		} else   {
			MH2_DrawBigString( 72,60 + ( 0 * 20 ),"NEW MISSION" );
			MH2_DrawBigString( 72,60 + ( 1 * 20 ),"LOAD" );
			MH2_DrawBigString( 72,60 + ( 2 * 20 ),"SAVE" );
			if ( GGameType & GAME_H2Portals ) {
				if ( mh2_oldmission->value ) {
					MH2_DrawBigString( 72,60 + ( 3 * 20 ),"OLD MISSION" );
				}
				MH2_DrawBigString( 72,60 + ( 4 * 20 ),"VIEW INTRO" );
			}

			int f = ( cls.realtime / 100 ) % 8;
			MQH_DrawPic( 43, 54 + mqh_singleplayer_cursor * 20,R_CachePic( va( "gfx/menu/menudot%i.lmp", f + 1 ) ) );
		}
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/ttl_sgl.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		if ( GGameType & GAME_QuakeWorld ) {
			MQH_DrawTextBox( 60, 10 * 8, 23, 4 );
			MQH_PrintWhite( 92, 12 * 8, "QuakeWorld is for" );
			MQH_PrintWhite( 88, 13 * 8, "Internet play only" );
		} else   {
			MQH_DrawPic( 72, 32, R_CachePic( "gfx/sp_menu.lmp" ) );

			int f = ( cls.realtime / 100 ) % 6;
			MQH_DrawPic( 54, 32 + mqh_singleplayer_cursor * 20,R_CachePic( va( "gfx/menudot%i.lmp", f + 1 ) ) );
		}
	}
}

static void MQH_NewGameConfirmed() {
	in_keyCatchers &= ~KEYCATCH_UI;
	if ( SV_IsServerActive() ) {
		Cbuf_AddText( "disconnect\n" );
	}
	Cbuf_AddText( "maxplayers 1\n" );
	if ( GGameType & GAME_Hexen2 ) {
		SVH2_RemoveGIPFiles( NULL );
		MH2_Menu_Class_f();
	} else   {
		Cbuf_AddText( "map start\n" );
	}
}

static void MQH_SinglePlayer_Key( int key ) {
	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		if ( key == K_ESCAPE || key == K_ENTER ) {
			m_state = m_main;
		}
		return;
	}

	switch ( key ) {
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( ++mqh_singleplayer_cursor >= ( GGameType & GAME_H2Portals ? SINGLEPLAYER_ITEMS_H2MP : SINGLEPLAYER_ITEMS ) ) {
			mqh_singleplayer_cursor = 0;
		}
		if ( GGameType & GAME_H2Portals && !mh2_oldmission->value ) {
			if ( mqh_singleplayer_cursor == 3 ) {
				mqh_singleplayer_cursor = 4;
			}
		}
		break;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( --mqh_singleplayer_cursor < 0 ) {
			mqh_singleplayer_cursor = ( GGameType & GAME_H2Portals ? SINGLEPLAYER_ITEMS_H2MP : SINGLEPLAYER_ITEMS ) - 1;
		}
		if ( GGameType & GAME_H2Portals && !mh2_oldmission->value ) {
			if ( mqh_singleplayer_cursor == 3 ) {
				mqh_singleplayer_cursor = 2;
			}
		}
		break;

	case K_ENTER:
		mqh_entersound = true;

		mh2_enter_portals = false;
		switch ( mqh_singleplayer_cursor ) {
		case 0:
			if ( GGameType & GAME_H2Portals ) {
				mh2_enter_portals = true;
			}

		case 3:
			if ( SV_IsServerActive() ) {
				MQH_Menu_SinglePlayerConfirm_f();
			} else   {
				MQH_NewGameConfirmed();
			}
			break;

		case 1:
			MQH_Menu_Load_f();
			break;

		case 2:
			MQH_Menu_Save_f();
			break;

		case 4:
			in_keyCatchers &= ~KEYCATCH_UI;
			Cbuf_AddText( "playdemo t9\n" );
			break;
		}
	}
}

static void MQH_Menu_SinglePlayerConfirm_f() {
	m_state = m_singleplayer_confirm;
}

static void MQH_SinglePlayerConfirm_Draw() {
	int y = viddef.height * 0.35;
	MQH_DrawTextBox( 32, y - 16, 30, 4 );
	MQH_PrintWhite( 64, y, "Are you sure you want to" );
	MQH_PrintWhite( 92, y + 8, "start a new game?" );
}

static void MQH_SinglePlayerConfirm_Key( int key ) {
	switch ( key ) {
	case 'n':
	case K_ESCAPE:
		MQH_Menu_SinglePlayer_f();
		break;

	case 'y':
		MQH_NewGameConfirmed();
		break;
	}
}

//=============================================================================
/* CLASS CHOICE MENU */

static int mh2_class_flag;
static int mqh_class_cursor;

static const char* h2_ClassNamesU[ NUM_CLASSES_H2MP ] =
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"DEMONESS"
};

static const char* hw_ClassNamesU[ MAX_PLAYER_CLASS ] =
{
	"PALADIN",
	"CRUSADER",
	"NECROMANCER",
	"ASSASSIN",
	"SUCCUBUS",
	"DWARF"
};

static void MH2_Menu_Class_f() {
	mh2_class_flag = 0;
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_class;
}

static void MH2_Menu_Class2_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_class;
	mh2_class_flag = 1;
}

static void MH2_Class_Draw() {
	MH2_ScrollTitle( "gfx/menu/title2.lmp" );

	for ( int i = 0; i < ( GGameType & GAME_HexenWorld ? MAX_PLAYER_CLASS : GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ); i++ ) {
		MH2_DrawBigString( 72, 60 + ( i * 20 ), GGameType & GAME_HexenWorld ? hw_ClassNamesU[ i ] : h2_ClassNamesU[ i ] );
	}

	int f = ( cls.realtime / 100 ) % 8;
	MQH_DrawPic( 43, 54 + mqh_class_cursor * 20,R_CachePic( va( "gfx/menu/menudot%i.lmp", f + 1 ) ) );

	MQH_DrawPic( 251,54 + 21, R_CachePic( va( "gfx/cport%d.lmp", mqh_class_cursor + 1 ) ) );
	MQH_DrawPic( 242,54, R_CachePic( "gfx/menu/frame.lmp" ) );
}

static void MH2_Class_Key( int key ) {
	switch ( key ) {
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;

	case K_ESCAPE:
		MQH_Menu_SinglePlayer_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		if ( ++mqh_class_cursor >= ( GGameType & GAME_HexenWorld ? MAX_PLAYER_CLASS : GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) ) {
			mqh_class_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		if ( --mqh_class_cursor < 0 ) {
			mqh_class_cursor = ( GGameType & GAME_HexenWorld ? MAX_PLAYER_CLASS : GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) - 1;
		}
		break;

	case K_ENTER:
		Cbuf_AddText( va( "playerclass %d\n", mqh_class_cursor + 1 ) );
		mqh_entersound = true;
		if ( !mh2_class_flag ) {
			MH2_Menu_Difficulty_f();
		} else   {
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;
	default:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		break;
	}
}

//=============================================================================
/* DIFFICULTY MENU */

#define NUM_DIFFLEVELS  4

static int mh2_diff_cursor;

static const char* DiffNames[ NUM_CLASSES_H2MP ][ NUM_DIFFLEVELS ] =
{
	{	// Paladin
		"APPRENTICE",
		"SQUIRE",
		"ADEPT",
		"LORD"
	},
	{	// Crusader
		"GALLANT",
		"HOLY AVENGER",
		"DIVINE HERO",
		"LEGEND"
	},
	{	// Necromancer
		"SORCERER",
		"DARK SERVANT",
		"WARLOCK",
		"LICH KING"
	},
	{	// Assassin
		"ROGUE",
		"CUTTHROAT",
		"EXECUTIONER",
		"WIDOW MAKER"
	},
	{	// Demoness
		"LARVA",
		"SPAWN",
		"FIEND",
		"SHE BITCH"
	}
};

static void MH2_Menu_Difficulty_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_difficulty;
}

static void MH2_Difficulty_Draw() {
	MH2_ScrollTitle( "gfx/menu/title5.lmp" );

	setup_class = clh2_playerclass->value;

	if ( setup_class < 1 || setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) ) {
		setup_class = GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2;
	}
	setup_class--;

	for ( int i = 0; i < NUM_DIFFLEVELS; ++i ) {
		MH2_DrawBigString( 72, 60 + ( i * 20 ), DiffNames[ setup_class ][ i ] );
	}

	int f = ( int )( cls.realtime / 100 ) % 8;
	MQH_DrawPic( 43, 54 + mh2_diff_cursor * 20, R_CachePic( va( "gfx/menu/menudot%i.lmp", f + 1 ) ) );
}

static void MH2_Difficulty_Key( int key ) {
	switch ( key ) {
	case K_LEFTARROW:
	case K_RIGHTARROW:
		break;
	case K_ESCAPE:
		MH2_Menu_Class_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		if ( ++mh2_diff_cursor >= NUM_DIFFLEVELS ) {
			mh2_diff_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		if ( --mh2_diff_cursor < 0 ) {
			mh2_diff_cursor = NUM_DIFFLEVELS - 1;
		}
		break;
	case K_ENTER:
		Cvar_SetValue( "skill", mh2_diff_cursor );
		mqh_entersound = true;
		if ( mh2_enter_portals ) {
			h2_introTime = 0.0;
			cl.qh_intermission = 12;
			cl.qh_completed_time = cl.qh_serverTimeFloat;
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			cls.qh_demonum = mqh_save_demonum;

			//Cbuf_AddText ("map keep1\n");
		} else   {
			Cbuf_AddText( "map demo1\n" );
		}
		break;
	default:
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		break;
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

#define MAX_SAVEGAMES       12
#define SAVEGAME_COMMENT_LENGTH 39

static int mqh_load_cursor;			// 0 < mqh_load_cursor < MAX_SAVEGAMES

static char mqh_filenames[ MAX_SAVEGAMES ][ SAVEGAME_COMMENT_LENGTH + 1 ];
static bool mqh_loadable[ MAX_SAVEGAMES ];

static void MQH_ScanSaves() {
	int i, j;
	char name[ MAX_OSPATH ];
	fileHandle_t f;

	for ( i = 0; i < MAX_SAVEGAMES; i++ ) {
		String::Cpy( mqh_filenames[ i ], "--- UNUSED SLOT ---" );
		mqh_loadable[ i ] = false;
		if ( GGameType & GAME_Hexen2 ) {
			sprintf( name, "s%i/info.dat", i );
		} else   {
			sprintf( name, "s%i.sav", i );
		}
		//	This will make sure that only savegames in current game directory
		// in home directory are listed
		if ( !FS_FileExists( name ) ) {
			continue;
		}
		FS_FOpenFileRead( name, &f, true );
		if ( !f ) {
			continue;
		}
		FS_Read( name, 80, f );
		name[ 80 ] = 0;
		//	Skip version
		char* Ptr = name;
		while ( *Ptr && *Ptr != '\n' )
			Ptr++;
		if ( *Ptr == '\n' ) {
			*Ptr = 0;
			Ptr++;
		}
		char* SaveName = Ptr;
		while ( *Ptr && *Ptr != '\n' )
			Ptr++;
		*Ptr = 0;
		String::NCpy( mqh_filenames[ i ], SaveName, sizeof ( mqh_filenames[ i ] ) - 1 );

		// change _ back to space
		for ( j = 0; j < SAVEGAME_COMMENT_LENGTH; j++ )
			if ( mqh_filenames[ i ][ j ] == '_' ) {
				mqh_filenames[ i ][ j ] = ' ';
			}
		mqh_loadable[ i ] = true;
		FS_FCloseFile( f );
	}
}

static void MQH_Menu_Load_f() {
	mqh_entersound = true;
	m_state = m_load;
	in_keyCatchers |= KEYCATCH_UI;
	MQH_ScanSaves();
}

static void MQH_Menu_Save_f() {
	if ( !SV_IsServerActive() ) {
		return;
	}
	if ( cl.qh_intermission ) {
		return;
	}
	if ( SVQH_GetMaxClients() != 1 ) {
		return;
	}
	mqh_entersound = true;
	m_state = m_save;
	in_keyCatchers |= KEYCATCH_UI;
	MQH_ScanSaves();
}

static void MQH_Load_Draw() {
	int y;
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/load.lmp" );
		y = 60;
	} else   {
		image_t* p = R_CachePic( "gfx/p_load.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		y = 32;
	}

	for ( int i = 0; i < MAX_SAVEGAMES; i++ ) {
		MQH_Print( 16, y + 8 * i, mqh_filenames[ i ] );
	}

	// line cursor
	MQH_DrawCharacter( 8, y + mqh_load_cursor * 8, 12 + ( ( cls.realtime / 250 ) & 1 ) );
}

static void MQH_Save_Draw() {
	int y;
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/save.lmp" );
		y = 60;
	} else   {
		image_t* p = R_CachePic( "gfx/p_save.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		y = 32;
	}

	for ( int i = 0; i < MAX_SAVEGAMES; i++ ) {
		MQH_Print( 16, y + 8 * i, mqh_filenames[ i ] );
	}

	// line cursor
	MQH_DrawCharacter( 8, y + mqh_load_cursor * 8, 12 + ( ( cls.realtime / 250 ) & 1 ) );
}

static void MQH_Load_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_SinglePlayer_f();
		break;

	case K_ENTER:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu2.wav" : "misc/menu2.wav" );
		if ( !mqh_loadable[ mqh_load_cursor ] ) {
			return;
		}
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
		SCRQH_BeginLoadingPlaque();

		// issue the load command
		Cbuf_AddText( va( "load s%i\n", mqh_load_cursor ) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_load_cursor--;
		if ( mqh_load_cursor < 0 ) {
			mqh_load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_load_cursor++;
		if ( mqh_load_cursor >= MAX_SAVEGAMES ) {
			mqh_load_cursor = 0;
		}
		break;
	}
}

static void MQH_Save_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_SinglePlayer_f();
		break;

	case K_ENTER:
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;
		Cbuf_AddText( va( "save s%i\n", mqh_load_cursor ) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_load_cursor--;
		if ( mqh_load_cursor < 0 ) {
			mqh_load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_load_cursor++;
		if ( mqh_load_cursor >= MAX_SAVEGAMES ) {
			mqh_load_cursor = 0;
		}
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

#define MULTIPLAYER_ITEMS_Q1    3
#define MULTIPLAYER_ITEMS_H2    5
#define MULTIPLAYER_ITEMS_HW    2

static int mqh_multiplayer_cursor;

static void MQH_Menu_MultiPlayer_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_multiplayer;
	if ( !( GGameType & GAME_QuakeWorld ) ) {
		mqh_entersound = true;
	}

	mh2_message = NULL;
}

static void MQH_MultiPlayer_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title4.lmp" );

		MH2_DrawBigString( 72,60 + ( 0 * 20 ),"JOIN A GAME" );
		if ( GGameType & GAME_HexenWorld ) {
			MH2_DrawBigString( 72,60 + ( 1 * 20 ),"SETUP" );
		} else   {
			MH2_DrawBigString( 72,60 + ( 1 * 20 ),"NEW GAME" );
			MH2_DrawBigString( 72,60 + ( 2 * 20 ),"SETUP" );
			MH2_DrawBigString( 72,60 + ( 3 * 20 ),"LOAD" );
			MH2_DrawBigString( 72,60 + ( 4 * 20 ),"SAVE" );
		}

		int f = ( cls.realtime / 100 ) % 8;
		MQH_DrawPic( 43, 54 + mqh_multiplayer_cursor * 20,R_CachePic( va( "gfx/menu/menudot%i.lmp", f + 1 ) ) );

		if ( mh2_message ) {
			MQH_PrintWhite( ( 320 / 2 ) - ( ( 27 * 8 ) / 2 ), 168, mh2_message );
			MQH_PrintWhite( ( 320 / 2 ) - ( ( 27 * 8 ) / 2 ), 176, mh2_message2 );
			if ( cls.realtime - 5000 > mh2_message_time ) {
				mh2_message = NULL;
			}
		}

		if ( GGameType & GAME_HexenWorld || tcpipAvailable ) {
			return;
		}
		MQH_PrintWhite( ( 320 / 2 ) - ( ( 27 * 8 ) / 2 ), 160, "No Communications Available" );
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/p_multi.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		if ( GGameType & GAME_QuakeWorld ) {
			MQH_DrawTextBox( 46, 8 * 8, 27, 9 );
			MQH_PrintWhite( 72, 10 * 8, "If you want to find QW  " );
			MQH_PrintWhite( 72, 11 * 8, "games, head on over to: " );
			MQH_Print( 72, 12 * 8, "   www.quakeworld.net   " );
			MQH_PrintWhite( 72, 13 * 8, "          or            " );
			MQH_Print( 72, 14 * 8, "   www.quakespy.com     " );
			MQH_PrintWhite( 72, 15 * 8, "For pointers on getting " );
			MQH_PrintWhite( 72, 16 * 8, "        started!        " );
		} else   {
			MQH_DrawPic( 72, 32, R_CachePic( "gfx/mp_menu.lmp" ) );

			int f = ( cls.realtime / 100 ) % 6;
			MQH_DrawPic( 54, 32 + mqh_multiplayer_cursor * 20,R_CachePic( va( "gfx/menudot%i.lmp", f + 1 ) ) );

			if ( tcpipAvailable ) {
				return;
			}
			MQH_PrintWhite( ( 320 / 2 ) - ( ( 27 * 8 ) / 2 ), 148, "No Communications Available" );
		}
	}
}

static void MQH_MultiPlayer_Key( int key ) {
	if ( GGameType & GAME_QuakeWorld ) {
		if ( key == K_ESCAPE || key == K_ENTER ) {
			m_state = m_main;
		}
		return;
	}

	switch ( key ) {
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( ++mqh_multiplayer_cursor >= ( GGameType & GAME_HexenWorld ? MULTIPLAYER_ITEMS_HW : GGameType & GAME_Hexen2 ? MULTIPLAYER_ITEMS_H2 : MULTIPLAYER_ITEMS_Q1 ) ) {
			mqh_multiplayer_cursor = 0;
		}
		break;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( --mqh_multiplayer_cursor < 0 ) {
			mqh_multiplayer_cursor = ( GGameType & GAME_HexenWorld ? MULTIPLAYER_ITEMS_HW : GGameType & GAME_Hexen2 ? MULTIPLAYER_ITEMS_H2 : MULTIPLAYER_ITEMS_Q1 ) - 1;
		}
		break;

	case K_ENTER:
		mqh_entersound = true;
		switch ( mqh_multiplayer_cursor ) {
		case 0:
			if ( GGameType & GAME_HexenWorld ) {
				MHW_Menu_Connect_f();
			} else if ( tcpipAvailable )     {
				MQH_Menu_LanConfig_f();
			}
			break;

		case 1:
			if ( GGameType & GAME_HexenWorld ) {
				MQH_Menu_Setup_f();
			} else if ( tcpipAvailable )     {
				MQH_Menu_GameOptions_f();
			}
			break;

		case 2:
			MQH_Menu_Setup_f();
			break;

		case 3:
			MH2_Menu_MLoad_f();
			break;

		case 4:
			MH2_Menu_MSave_f();
			break;
		}
	}
}

//=============================================================================
/* LAN CONFIG MENU */

#define NUM_LANCONFIG_CMDS_Q1   3
#define NUM_LANCONFIG_CMDS_H2   4

static int mqh_lanConfig_cursor = -1;
static int lanConfig_cursor_table_q1[] = { 52, 72, 104 };
static int lanConfig_cursor_table_h2[] = { 80, 100, 120, 152 };

static int lanConfig_port;
static field_t lanConfig_portname;
static field_t lanConfig_joinname;

static void MQH_InitPortName() {
	lanConfig_port = DEFAULTnet_hostport;
	sprintf( lanConfig_portname.buffer, "%u", lanConfig_port );
	lanConfig_portname.cursor = String::Length( lanConfig_portname.buffer );
	lanConfig_portname.maxLength = 5;
	lanConfig_portname.widthInChars = 6;
	Field_Clear( &lanConfig_joinname );
	lanConfig_joinname.maxLength = 29;
	lanConfig_joinname.widthInChars = GGameType & GAME_Hexen2 ? 30 : 22;
}

static void MQH_Menu_LanConfig_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_lanconfig;
	mqh_entersound = true;
	if ( mqh_lanConfig_cursor == -1 ) {
		mqh_lanConfig_cursor = 2;
	}
	MQH_InitPortName();

	m_return_onerror = false;
	m_return_reason[ 0 ] = 0;

	if ( GGameType & GAME_Hexen2 ) {
		setup_class = clh2_playerclass->value;
		if ( setup_class < 1 || setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) ) {
			setup_class = ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 );
		}
		setup_class--;
	}
}

static void MQH_LanConfig_Draw() {
	int basex;
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title4.lmp" );
		basex = 48;
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/p_multi.lmp" );
		basex = ( 320 - R_GetImageWidth( p ) ) / 2;
		MQH_DrawPic( basex, 4, p );
	}

	MQH_Print( basex, GGameType & GAME_Hexen2 ? 60 : 32, "Join Game - TCP/IP" );
	basex += 8;

	MQH_Print( basex, GGameType & GAME_Hexen2 ? lanConfig_cursor_table_h2[ 0 ] : lanConfig_cursor_table_q1[ 0 ], "Port" );
	MQH_DrawField( basex + 9 * 8, GGameType & GAME_Hexen2 ? lanConfig_cursor_table_h2[ 0 ] : lanConfig_cursor_table_q1[ 0 ], &lanConfig_portname, mqh_lanConfig_cursor == 0 );

	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( basex, lanConfig_cursor_table_h2[ 1 ], "Class:" );
		MQH_Print( basex + 8 * 7, lanConfig_cursor_table_h2[ 1 ], h2_ClassNames[ setup_class ] );
	}

	MQH_Print( basex, GGameType & GAME_Hexen2 ? lanConfig_cursor_table_h2[ 2 ] : lanConfig_cursor_table_q1[ 1 ], "Search for local games..." );
	MQH_Print( basex, GGameType & GAME_Hexen2 ? 136 : 88, "Join game at:" );
	MQH_DrawField( basex + 16, GGameType & GAME_Hexen2 ? lanConfig_cursor_table_h2[ 3 ] : lanConfig_cursor_table_q1[ 2 ], &lanConfig_joinname, mqh_lanConfig_cursor == ( GGameType & GAME_Hexen2 ? 3 : 2 ) );

	MQH_DrawCharacter( basex - 8, GGameType & GAME_Hexen2 ? lanConfig_cursor_table_h2[ mqh_lanConfig_cursor ] : lanConfig_cursor_table_q1[ mqh_lanConfig_cursor ], 12 + ( ( cls.realtime / 250 ) & 1 ) );

	if ( *m_return_reason ) {
		MQH_PrintWhite( basex, GGameType & GAME_Hexen2 ? 172 : 128, m_return_reason );
	}
}

static void MQH_ConfigureNetSubsystem() {
	Cbuf_AddText( "stopdemo\n" );

	net_hostport = lanConfig_port;
}

static void MQH_CheckPortValue() {
	int l =  String::Atoi( lanConfig_portname.buffer );
	if ( l > 65535 ) {
		sprintf( lanConfig_portname.buffer, "%u", lanConfig_port );
		lanConfig_portname.cursor = String::Length( lanConfig_portname.buffer );
	} else   {
		lanConfig_port = l;
	}
}

static void MQH_LanConfig_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_lanConfig_cursor--;
		if ( mqh_lanConfig_cursor < 0 ) {
			if ( GGameType & GAME_Hexen2 ) {
				mqh_lanConfig_cursor = NUM_LANCONFIG_CMDS_H2 - 1;
			} else   {
				mqh_lanConfig_cursor = NUM_LANCONFIG_CMDS_Q1 - 1;
			}
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_lanConfig_cursor++;
		if ( mqh_lanConfig_cursor >= ( GGameType & GAME_Hexen2 ? NUM_LANCONFIG_CMDS_H2 : NUM_LANCONFIG_CMDS_Q1 ) ) {
			mqh_lanConfig_cursor = 0;
		}
		break;

	case K_ENTER:
		if ( mqh_lanConfig_cursor == 0 || ( GGameType & GAME_Hexen2 && mqh_lanConfig_cursor == 1 ) ) {
			break;
		}

		mqh_entersound = true;
		if ( GGameType & GAME_Hexen2 ) {
			Cbuf_AddText( va( "playerclass %d\n", setup_class + 1 ) );
		}

		MQH_ConfigureNetSubsystem();

		if ( mqh_lanConfig_cursor == ( GGameType & GAME_Hexen2 ? 2 : 1 ) ) {
			MQH_Menu_Search_f();
			break;
		}

		if ( mqh_lanConfig_cursor == ( GGameType & GAME_Hexen2 ? 3 : 2 ) ) {
			m_return_state = m_state;
			m_return_onerror = true;
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			Cbuf_AddText( va( "connect \"%s\"\n", lanConfig_joinname.buffer ) );
			break;
		}
		break;

	case K_LEFTARROW:
		if ( !( GGameType & GAME_Hexen2 ) || mqh_lanConfig_cursor != 1 ) {
			break;
		}

		S_StartLocalSound( "raven/menu3.wav" );
		setup_class--;
		if ( setup_class < 0 ) {
			setup_class = ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) - 1;
		}
		break;

	case K_RIGHTARROW:
		if ( !( GGameType & GAME_Hexen2 ) || mqh_lanConfig_cursor != 1 ) {
			break;
		}

		S_StartLocalSound( "raven/menu3.wav" );
		setup_class++;
		if ( setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) - 1 ) {
			setup_class = 0;
		}
		break;
	}
	if ( mqh_lanConfig_cursor == 0 ) {
		Field_KeyDownEvent( &lanConfig_portname, key );
	}
	if ( mqh_lanConfig_cursor == 2 ) {
		Field_KeyDownEvent( &lanConfig_joinname, key );
	}

	MQH_CheckPortValue();
}

static void MQH_LanConfig_Char( int key ) {
	if ( mqh_lanConfig_cursor == 0 ) {
		if ( key >= 32 && ( key < '0' || key > '9' ) ) {
			return;
		}
		Field_CharEvent( &lanConfig_portname, key );
	}
	if ( mqh_lanConfig_cursor == ( GGameType & GAME_Hexen2 ? 3 : 2 ) ) {
		Field_CharEvent( &lanConfig_joinname, key );
	}
}

//=============================================================================
/* SEARCH MENU */

static bool searchComplete = false;
static int searchCompleteTime;

static void MQH_Menu_Search_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_search;
	mqh_entersound = false;
	slistSilent = true;
	slistLocal = false;
	searchComplete = false;
	NET_Slist_f();

}

static void MQH_Search_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title4.lmp" );
	} else   {
		image_t* p = R_CachePic( "gfx/p_multi.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
	}

	int x = ( 320 / 2 ) - ( ( 12 * 8 ) / 2 ) + 4;
	MQH_DrawTextBox( x - 8, GGameType & GAME_Hexen2 ? 60 : 32, 12, 1 );
	MQH_Print( x, GGameType & GAME_Hexen2 ? 68 : 40, "Searching..." );

	if ( slistInProgress ) {
		NET_Poll();
		return;
	}

	if ( !searchComplete ) {
		searchComplete = true;
		searchCompleteTime = cls.realtime;
	}

	if ( hostCacheCount ) {
		MQH_Menu_ServerList_f();
		return;
	}

	MQH_PrintWhite( ( 320 / 2 ) - ( ( 22 * 8 ) / 2 ), GGameType & GAME_Hexen2 ? 92 : 64, GGameType & GAME_Hexen2 ? "No Hexen II servers found" : "No Quake servers found" );
	if ( cls.realtime - searchCompleteTime < 3000 ) {
		return;
	}

	MQH_Menu_LanConfig_f();
}

static void MQH_Search_Key( int key ) {
}

//=============================================================================
/* SLIST MENU */

static int mqh_slist_cursor;
static bool mqh_slist_sorted;

static void MQH_Menu_ServerList_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_slist;
	mqh_entersound = true;
	mqh_slist_cursor = 0;
	m_return_onerror = false;
	m_return_reason[ 0 ] = 0;
	mqh_slist_sorted = false;
}

static void MQH_ServerList_Draw() {
	if ( !mqh_slist_sorted ) {
		if ( hostCacheCount > 1 ) {
			for ( int i = 0; i < hostCacheCount; i++ ) {
				for ( int j = i + 1; j < hostCacheCount; j++ ) {
					if ( String::Cmp( hostcache[ j ].name, hostcache[ i ].name ) < 0 ) {
						hostcache_t temp;
						Com_Memcpy( &temp, &hostcache[ j ], sizeof ( hostcache_t ) );
						Com_Memcpy( &hostcache[ j ], &hostcache[ i ], sizeof ( hostcache_t ) );
						Com_Memcpy( &hostcache[ i ], &temp, sizeof ( hostcache_t ) );
					}
				}
			}
		}
		mqh_slist_sorted = true;
	}

	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title4.lmp" );
	} else   {
		image_t* p = R_CachePic( "gfx/p_multi.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
	}
	for ( int n = 0; n < hostCacheCount; n++ ) {
		char string [ 64 ];
		if ( hostcache[ n ].maxusers ) {
			sprintf( string, "%-15.15s %-15.15s %2u/%2u\n", hostcache[ n ].name, hostcache[ n ].map, hostcache[ n ].users, hostcache[ n ].maxusers );
		} else   {
			sprintf( string, "%-15.15s %-15.15s\n", hostcache[ n ].name, hostcache[ n ].map );
		}
		MQH_Print( 16, ( GGameType & GAME_Hexen2 ? 60 : 32 ) + 8 * n, string );
	}
	MQH_DrawCharacter( 0, ( GGameType & GAME_Hexen2 ? 60 : 32 ) + mqh_slist_cursor * 8, 12 + ( ( cls.realtime / 250 ) & 1 ) );

	if ( *m_return_reason ) {
		MQH_PrintWhite( 16, GGameType & GAME_Hexen2 ? 176 : 148, m_return_reason );
	}
}

static void MQH_ServerList_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_LanConfig_f();
		break;

	case K_SPACE:
		MQH_Menu_Search_f();
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_slist_cursor--;
		if ( mqh_slist_cursor < 0 ) {
			mqh_slist_cursor = hostCacheCount - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_slist_cursor++;
		if ( mqh_slist_cursor >= hostCacheCount ) {
			mqh_slist_cursor = 0;
		}
		break;

	case K_ENTER:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu2.wav" : "misc/menu2.wav" );
		m_return_state = m_state;
		m_return_onerror = true;
		mqh_slist_sorted = false;
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		Cbuf_AddText( va( "connect \"%s\"\n", hostcache[ mqh_slist_cursor ].cname ) );
		break;
	}
}

//=============================================================================
/* CONNECT MENU */

#define MAX_HOST_NAMES 10
#define MAX_HOST_SIZE 80
#define MAX_CONNECT_CMDS 11

static field_t save_names[ MAX_HOST_NAMES ];

static Cvar* hostname1;
static Cvar* hostname2;
static Cvar* hostname3;
static Cvar* hostname4;
static Cvar* hostname5;
static Cvar* hostname6;
static Cvar* hostname7;
static Cvar* hostname8;
static Cvar* hostname9;
static Cvar* hostname10;

static int connect_cursor = 0;

static int connect_cursor_table[ MAX_CONNECT_CMDS ] =
{
	72 + 0 * 8,
	72 + 1 * 8,
	72 + 2 * 8,
	72 + 3 * 8,
	72 + 4 * 8,
	72 + 5 * 8,
	72 + 6 * 8,
	72 + 7 * 8,
	72 + 8 * 8,
	72 + 9 * 8,

	72 + 11 * 8,
};

static void MHW_Menu_Connect_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_mconnect;
	mqh_entersound = true;

	mh2_message = NULL;

	String::Cpy( save_names[ 0 ].buffer, hostname1->string );
	String::Cpy( save_names[ 1 ].buffer, hostname2->string );
	String::Cpy( save_names[ 2 ].buffer, hostname3->string );
	String::Cpy( save_names[ 3 ].buffer, hostname4->string );
	String::Cpy( save_names[ 4 ].buffer, hostname5->string );
	String::Cpy( save_names[ 5 ].buffer, hostname6->string );
	String::Cpy( save_names[ 6 ].buffer, hostname7->string );
	String::Cpy( save_names[ 7 ].buffer, hostname8->string );
	String::Cpy( save_names[ 8 ].buffer, hostname9->string );
	String::Cpy( save_names[ 9 ].buffer, hostname10->string );
	for ( int i = 0; i < MAX_HOST_NAMES; i++ ) {
		save_names[ i ].cursor = String::Length( save_names[ i ].buffer );
		save_names[ i ].maxLength = 80;
		save_names[ i ].widthInChars = 34;
	}
}

static void MHW_Connect_Draw() {
	MH2_ScrollTitle( "gfx/menu/title4.lmp" );

	if ( connect_cursor < MAX_HOST_NAMES ) {
		MQH_DrawField( 24, 56, &save_names[ connect_cursor ], true );
	}

	int y = 72;
	for ( int i = 0; i < MAX_HOST_NAMES; i++,y += 8 ) {
		char temp[ MAX_HOST_SIZE ];
		sprintf( temp,"%d.",i + 1 );
		if ( i == connect_cursor ) {
			MQH_Print( 24, y, temp );
		} else   {
			MQH_PrintWhite( 24, y, temp );
		}

		String::Cpy( temp,save_names[ i ].buffer );
		temp[ 30 ] = 0;
		if ( i == connect_cursor ) {
			MQH_Print( 56, y, temp );
		} else   {
			MQH_PrintWhite( 56, y, temp );
		}
	}

	MQH_Print( 24, y + 8, "Save Changes" );

	MQH_DrawCharacter( 8, connect_cursor_table[ connect_cursor ], 12 + ( ( cls.realtime / 250 ) & 1 ) );
}

static void MHW_Connect_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		connect_cursor--;
		if ( connect_cursor < 0 ) {
			connect_cursor = MAX_CONNECT_CMDS - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		connect_cursor++;
		if ( connect_cursor >= MAX_CONNECT_CMDS ) {
			connect_cursor = 0;
		}
		break;

	case K_ENTER:
		Cvar_Set( "host1",save_names[ 0 ].buffer );
		Cvar_Set( "host2",save_names[ 1 ].buffer );
		Cvar_Set( "host3",save_names[ 2 ].buffer );
		Cvar_Set( "host4",save_names[ 3 ].buffer );
		Cvar_Set( "host5",save_names[ 4 ].buffer );
		Cvar_Set( "host6",save_names[ 5 ].buffer );
		Cvar_Set( "host7",save_names[ 6 ].buffer );
		Cvar_Set( "host8",save_names[ 7 ].buffer );
		Cvar_Set( "host9",save_names[ 8 ].buffer );
		Cvar_Set( "host10",save_names[ 9 ].buffer );

		if ( connect_cursor < MAX_HOST_NAMES ) {
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
			Cbuf_AddText( va( "connect %s\n", save_names[ connect_cursor ].buffer ) );
		} else   {
			mqh_entersound = true;
			MQH_Menu_MultiPlayer_f();
		}
		break;
	}
	if ( connect_cursor < MAX_HOST_NAMES ) {
		Field_KeyDownEvent( &save_names[ connect_cursor ], k );
	}
}

static void MHW_Connect_Char( int k ) {
	if ( connect_cursor < MAX_HOST_NAMES ) {
		Field_CharEvent( &save_names[ connect_cursor ], k );
	}
}

//=============================================================================
/* GAME OPTIONS MENU */

#define NUM_GAMEOPTIONS_Q1  10
#define NUM_GAMEOPTIONS_H2  12

#define OEM_START 9
#define REG_START 2
#define MP_START 7
#define DM_START 8

struct level_t {
	const char* name;
	const char* description;
};

struct episode_t {
	const char* description;
	int firstLevel;
	int levels;
};

static int mqh_gameoptions_cursor;
static int mqh_maxplayers;
static int mqh_startepisode;
static int mqh_startlevel;

static level_t mq1_levels[] =
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
static level_t mq1_hipnoticlevels[] =
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
static level_t mq1_roguelevels[] =
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

static episode_t mq1_episodes[] =
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
static episode_t mq1_hipnoticepisodes[] =
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
static episode_t mq1_rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

static level_t mh2_levels[] =
{
	{"demo1", "Blackmarsh"},							// 0
	{"demo2", "Barbican"},								// 1

	{"ravdm1", "Deathmatch 1"},							// 2

	{"demo1","Blackmarsh"},								// 3
	{"demo2","Barbican"},								// 4
	{"demo3","The Mill"},								// 5
	{"village1","King's Court"},						// 6
	{"village2","Inner Courtyard"},						// 7
	{"village3","Stables"},								// 8
	{"village4","Palace Entrance"},						// 9
	{"village5","The Forgotten Chapel"},				// 10
	{"rider1a","Famine's Domain"},						// 11

	{"meso2","Plaza of the Sun"},						// 12
	{"meso1","The Palace of Columns"},					// 13
	{"meso3","Square of the Stream"},					// 14
	{"meso4","Tomb of the High Priest"},				// 15
	{"meso5","Obelisk of the Moon"},					// 16
	{"meso6","Court of 1000 Warriors"},					// 17
	{"meso8","Bridge of Stars"},						// 18
	{"meso9","Well of Souls"},							// 19

	{"egypt1","Temple of Horus"},						// 20
	{"egypt2","Ancient Temple of Nefertum"},			// 21
	{"egypt3","Temple of Nefertum"},					// 22
	{"egypt4","Palace of the Pharaoh"},					// 23
	{"egypt5","Pyramid of Anubis"},						// 24
	{"egypt6","Temple of Light"},						// 25
	{"egypt7","Shrine of Naos"},						// 26
	{"rider2c","Pestilence's Lair"},					// 27

	{"romeric1","The Hall of Heroes"},					// 28
	{"romeric2","Gardens of Athena"},					// 29
	{"romeric3","Forum of Zeus"},						// 30
	{"romeric4","Baths of Demetrius"},					// 31
	{"romeric5","Temple of Mars"},						// 32
	{"romeric6","Coliseum of War"},						// 33
	{"romeric7","Reflecting Pool"},						// 34

	{"cath","Cathedral"},								// 35
	{"tower","Tower of the Dark Mage"},					// 36
	{"castle4","The Underhalls"},						// 37
	{"castle5","Eidolon's Ordeal"},						// 38
	{"eidolon","Eidolon's Lair"},						// 39

	{"ravdm1","Atrium of Immolation"},					// 40
	{"ravdm2","Total Carnage"},							// 41
	{"ravdm3","Reckless Abandon"},						// 42
	{"ravdm4","Temple of RA"},							// 43
	{"ravdm5","Tom Foolery"},							// 44

	{"ravdm1", "Deathmatch 1"},							// 45

	//OEM
	{"demo1","Blackmarsh"},								// 46
	{"demo2","Barbican"},								// 47
	{"demo3","The Mill"},								// 48
	{"village1","King's Court"},						// 49
	{"village2","Inner Courtyard"},						// 50
	{"village3","Stables"},								// 51
	{"village4","Palace Entrance"},						// 52
	{"village5","The Forgotten Chapel"},				// 53
	{"rider1a","Famine's Domain"},						// 54

	//Mission Pack
	{"keep1",   "Eidolon's Lair"},						// 55
	{"keep2",   "Village of Turnabel"},					// 56
	{"keep3",   "Duke's Keep"},							// 57
	{"keep4",   "The Catacombs"},						// 58
	{"keep5",   "Hall of the Dead"},					// 59

	{"tibet1",  "Tulku"},								// 60
	{"tibet2",  "Ice Caverns"},							// 61
	{"tibet3",  "The False Temple"},					// 62
	{"tibet4",  "Courtyards of Tsok"},					// 63
	{"tibet5",  "Temple of Kalachakra"},				// 64
	{"tibet6",  "Temple of Bardo"},						// 65
	{"tibet7",  "Temple of Phurbu"},					// 66
	{"tibet8",  "Palace of Emperor Egg Chen"},			// 67
	{"tibet9",  "Palace Inner Chambers"},				// 68
	{"tibet10", "The Inner Sanctum of Praevus"},		// 69
};

static episode_t mh2_episodes[] =
{
	// Demo
	{"Demo", 0, 2},
	{"Demo Deathmatch", 2, 1},

	// Registered
	{"Village", 3, 9},
	{"Meso", 12, 8},
	{"Egypt", 20, 8},
	{"Romeric", 28, 7},
	{"Cathedral", 35, 5},
	{"MISSION PACK", 55, 15},
	{"Deathmatch", 40, 5},

	// OEM
	{"Village", 46, 9},
	{"Deathmatch", 45, 1},
};

static bool mqh_serverInfoMessage = false;
static int mqh_serverInfoMessageTime;

static int mq1_gameoptions_cursor_table[] = {40, 56, 66, 74, 82, 90, 98, 106, 122, 130};
static int mh2_gameoptions_cursor_table[] = {40, 56, 66, 74, 82, 90, 98, 106, 114, 122, 138, 146};

static void MQH_Menu_GameOptions_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_gameoptions;
	mqh_entersound = true;
	if ( mqh_maxplayers == 0 ) {
		mqh_maxplayers = SVQH_GetMaxClients();
	}
	if ( mqh_maxplayers < 2 ) {
		mqh_maxplayers = SVQH_GetMaxClientsLimit();
	}

	MQH_InitPortName();
	if ( GGameType & GAME_Hexen2 ) {
		setup_class = clh2_playerclass->value;
		if ( setup_class < 1 || setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) ) {
			setup_class = ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 );
		}
		setup_class--;

		if ( qh_registered->value && ( mqh_startepisode < REG_START || mqh_startepisode >= OEM_START ) ) {
			mqh_startepisode = REG_START;
		}

		if ( svqh_coop->value ) {
			mqh_startlevel = 0;
			if ( mqh_startepisode == 1 ) {
				mqh_startepisode = 0;
			} else if ( mqh_startepisode == DM_START )     {
				mqh_startepisode = REG_START;
			}
			if ( mqh_gameoptions_cursor >= NUM_GAMEOPTIONS_H2 - 1 ) {
				mqh_gameoptions_cursor = 0;
			}
		}
		if ( GGameType & GAME_H2Portals && !mh2_oldmission->value ) {
			mqh_startepisode = MP_START;
		}
	}
}

static void MQH_GameOptions_Draw() {
	int startx;
	int starty;
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title4.lmp" );
		startx = 8;
		starty = 60;
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/p_multi.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		startx = 0;
		starty = 32;
	}

	MQH_DrawTextBox( startx + 152, starty, 10, 1 );
	MQH_Print( startx + 160, starty + 8, "begin game" );

	MQH_Print( startx + 0, starty + 24, "             Port" );
	MQH_DrawField( startx + 160, starty + 24, &lanConfig_portname, mqh_gameoptions_cursor == 1 );
	starty += 10;

	MQH_Print( startx + 0, starty + 24, "      Max players" );
	MQH_Print( startx + 160, starty + 24, va( "%i", mqh_maxplayers ) );

	MQH_Print( startx + 0, starty + 32, "        Game Type" );
	if ( svqh_coop->value ) {
		MQH_Print( startx + 160, starty + 32, "Cooperative" );
	} else   {
		MQH_Print( startx + 160, starty + 32, "Deathmatch" );
	}

	MQH_Print( startx + 0, starty + 40, "        Teamplay" );
	if ( q1_rogue ) {
		const char* msg;
		switch ( ( int )svqh_teamplay->value ) {
		case 1: msg = "No Friendly Fire"; break;
		case 2: msg = "Friendly Fire"; break;
		case 3: msg = "Tag"; break;
		case 4: msg = "Capture the Flag"; break;
		case 5: msg = "One Flag CTF"; break;
		case 6: msg = "Three Team CTF"; break;
		default: msg = "Off"; break;
		}
		MQH_Print( startx + 160, starty + 40, msg );
	} else   {
		const char* msg;
		switch ( ( int )svqh_teamplay->value ) {
		case 1: msg = "No Friendly Fire"; break;
		case 2: msg = "Friendly Fire"; break;
		default: msg = "Off"; break;
		}
		MQH_Print( startx + 160, starty + 40, msg );
	}

	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( startx + 0, starty + 48, "            Class" );
		MQH_Print( startx + 160, starty + 48, h2_ClassNames[ setup_class ] );

		MQH_Print( startx + 0, starty + 56, "       Difficulty" );
		MQH_Print( startx + 160, starty + 56, DiffNames[ setup_class ][ ( int )qh_skill->value ] );
		starty += 8;
	} else   {
		MQH_Print( startx + 0, starty + 48, "            Skill" );
		if ( qh_skill->value == 0 ) {
			MQH_Print( startx + 160, starty + 48, "Easy difficulty" );
		} else if ( qh_skill->value == 1 )     {
			MQH_Print( startx + 160, starty + 48, "Normal difficulty" );
		} else if ( qh_skill->value == 2 )     {
			MQH_Print( startx + 160, starty + 48, "Hard difficulty" );
		} else   {
			MQH_Print( startx + 160, starty + 48, "Nightmare difficulty" );
		}
	}

	MQH_Print( startx + 0, starty + 56, "       Frag Limit" );
	if ( qh_fraglimit->value == 0 ) {
		MQH_Print( startx + 160, starty + 56, "none" );
	} else   {
		MQH_Print( startx + 160, starty + 56, va( "%i frags", ( int )qh_fraglimit->value ) );
	}

	MQH_Print( startx + 0, starty + 64, "       Time Limit" );
	if ( qh_timelimit->value == 0 ) {
		MQH_Print( startx + 160, starty + 64, "none" );
	} else   {
		MQH_Print( startx + 160, starty + 64, va( "%i minutes", ( int )qh_timelimit->value ) );
	}

	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( startx + 0, starty + 72, "     Random Class" );
		if ( h2_randomclass->value ) {
			MQH_Print( startx + 160, starty + 72, "on" );
		} else {
			MQH_Print( startx + 160, starty + 72, "off" );
		}
		starty += 8;
	}

	MQH_Print( startx + 0, starty + 80, "         Episode" );
	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( startx + 160, starty + 80, mh2_episodes[ mqh_startepisode ].description );
	} else if ( q1_hipnotic )     {
		MQH_Print( startx + 160, starty + 80, mq1_hipnoticepisodes[ mqh_startepisode ].description );
	} else if ( q1_rogue )     {
		MQH_Print( startx + 160, starty + 80, mq1_rogueepisodes[ mqh_startepisode ].description );
	} else   {
		MQH_Print( startx + 160, starty + 80, mq1_episodes[ mqh_startepisode ].description );
	}

	MQH_Print( startx + 0, starty + 88, "           Level" );
	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( startx + 160, starty + 88, mh2_levels[ mh2_episodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name );
		MQH_Print( 96, starty + 104, mh2_levels[ mh2_episodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].description );
	} else if ( q1_hipnotic )     {
		MQH_Print( startx + 160, starty + 88, mq1_hipnoticlevels[ mq1_hipnoticepisodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].description );
		MQH_Print( startx + 160, starty + 96, mq1_hipnoticlevels[ mq1_hipnoticepisodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name );
	} else if ( q1_rogue )     {
		MQH_Print( startx + 160, starty + 88, mq1_roguelevels[ mq1_rogueepisodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].description );
		MQH_Print( startx + 160, starty + 96, mq1_roguelevels[ mq1_rogueepisodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name );
	} else   {
		MQH_Print( startx + 160, starty + 88, mq1_levels[ mq1_episodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].description );
		MQH_Print( startx + 160, starty + 96, mq1_levels[ mq1_episodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name );
	}

	// line cursor
	if ( GGameType & GAME_Hexen2 ) {
		MQH_DrawCharacter( 172 - 16, mh2_gameoptions_cursor_table[ mqh_gameoptions_cursor ] + 28, 12 + ( ( cls.realtime / 250 ) & 1 ) );
	} else   {
		MQH_DrawCharacter( 144, mq1_gameoptions_cursor_table[ mqh_gameoptions_cursor ], 12 + ( ( cls.realtime / 250 ) & 1 ) );

		if ( mqh_serverInfoMessage ) {
			if ( ( cls.realtime - mqh_serverInfoMessageTime ) < 5000 ) {
				int x = ( 320 - 26 * 8 ) / 2;
				MQH_DrawTextBox( x, 138, 24, 4 );
				x += 8;
				MQH_Print( x, 146, "  More than 4 players   " );
				MQH_Print( x, 154, " requires using command " );
				MQH_Print( x, 162, "line parameters; please " );
				MQH_Print( x, 170, "   see techinfo.txt.    " );
			} else   {
				mqh_serverInfoMessage = false;
			}
		}
	}
}

static void MQH_ChangeMaxPlayers( int dir ) {
	mqh_maxplayers += dir;
	if ( mqh_maxplayers > SVQH_GetMaxClientsLimit() ) {
		mqh_maxplayers = SVQH_GetMaxClientsLimit();
		mqh_serverInfoMessage = true;
		mqh_serverInfoMessageTime = cls.realtime;
	}
	if ( mqh_maxplayers < 2 ) {
		mqh_maxplayers = 2;
	}
}

static void MQH_ChangeCoop( int dir ) {
	Cvar_SetValue( "coop", svqh_coop->value ? 0 : 1 );
	if ( GGameType & GAME_Hexen2 && svqh_coop->value ) {
		mqh_startlevel = 0;
		if ( mqh_startepisode == 1 ) {
			mqh_startepisode = 0;
		} else if ( mqh_startepisode == DM_START )     {
			mqh_startepisode = REG_START;
		}
		if ( GGameType & GAME_H2Portals && !mh2_oldmission->value ) {
			mqh_startepisode = MP_START;
		}
	}
}

static void MQH_ChangeTeamplay( int dir ) {
	int count;
	if ( q1_rogue ) {
		count = 6;
	} else   {
		count = 2;
	}

	Cvar_SetValue( "teamplay", svqh_teamplay->value + dir );
	if ( svqh_teamplay->value > count ) {
		Cvar_SetValue( "teamplay", 0 );
	} else if ( svqh_teamplay->value < 0 )     {
		Cvar_SetValue( "teamplay", count );
	}
}

static void MQH_ChangeClass( int dir ) {
	setup_class += dir;
	if ( setup_class < 0 ) {
		setup_class = ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) - 1;
	}
	if ( setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) - 1 ) {
		setup_class = 0;
	}
}

static void MQH_ChangeSkill( int dir ) {
	Cvar_SetValue( "skill", qh_skill->value + dir );
	if ( qh_skill->value > 3 ) {
		Cvar_SetValue( "skill", 0 );
	}
	if ( qh_skill->value < 0 ) {
		Cvar_SetValue( "skill", 3 );
	}
}

static void MQH_ChangeFragLimit( int dir ) {
	Cvar_SetValue( "fraglimit", qh_fraglimit->value + dir * 10 );
	if ( qh_fraglimit->value > 100 ) {
		Cvar_SetValue( "fraglimit", 0 );
	}
	if ( qh_fraglimit->value < 0 ) {
		Cvar_SetValue( "fraglimit", 100 );
	}
}

static void MQH_ChangeTimeLimit( int dir ) {
	Cvar_SetValue( "timelimit", qh_timelimit->value + dir * 5 );
	if ( qh_timelimit->value > 60 ) {
		Cvar_SetValue( "timelimit", 0 );
	}
	if ( qh_timelimit->value < 0 ) {
		Cvar_SetValue( "timelimit", 60 );
	}
}

static void MQH_ChangeRandomClass( int dir ) {
	if ( h2_randomclass->value ) {
		Cvar_SetValue( "randomclass", 0 );
	} else   {
		Cvar_SetValue( "randomclass", 1 );
	}
}

static void MQ1_ChangeStartEpisode( int dir ) {
	mqh_startepisode += dir;
	int count;
	//MED 01/06/97 added hipnotic count
	if ( q1_hipnotic ) {
		count = 6;
	}
	//PGM 01/07/97 added rogue count
	//PGM 03/02/97 added 1 for dmatch episode
	else if ( q1_rogue ) {
		count = 4;
	} else if ( qh_registered->value )     {
		count = 7;
	} else   {
		count = 2;
	}

	if ( mqh_startepisode < 0 ) {
		mqh_startepisode = count - 1;
	}

	if ( mqh_startepisode >= count ) {
		mqh_startepisode = 0;
	}

	mqh_startlevel = 0;
}

static void MH2_ChangeStartEpisode( int dir ) {
	mqh_startepisode += dir;

	if ( qh_registered->value ) {
		if ( !( GGameType & GAME_H2Portals ) && mqh_startepisode == MP_START ) {
			mqh_startepisode += dir;
		}
		int count = DM_START;
		if ( !svqh_coop->value ) {
			count++;
		} else   {
			if ( !( GGameType & GAME_H2Portals ) ) {
				count--;
			}
			if ( GGameType & GAME_H2Portals && !mh2_oldmission->value ) {
				mqh_startepisode = MP_START;
			}
		}
		if ( mqh_startepisode < REG_START ) {
			mqh_startepisode = count - 1;
		}

		if ( mqh_startepisode >= count ) {
			mqh_startepisode = REG_START;
		}

		mqh_startlevel = 0;
	} else   {
		int count = 2;

		if ( mqh_startepisode < 0 ) {
			mqh_startepisode = count - 1;
		}

		if ( mqh_startepisode >= count ) {
			mqh_startepisode = 0;
		}

		mqh_startlevel = 0;
	}
}

static void MQH_ChangeStartLevel( int dir ) {
	if ( GGameType & GAME_Hexen2 && svqh_coop->value ) {
		mqh_startlevel = 0;
		return;
	}
	mqh_startlevel += dir;
	int count;
	if ( GGameType & GAME_Hexen2 ) {
		count = mh2_episodes[ mqh_startepisode ].levels;
	} else   {
		//MED 01/06/97 added hipnotic episodes
		if ( q1_hipnotic ) {
			count = mq1_hipnoticepisodes[ mqh_startepisode ].levels;
		}
		//PGM 01/06/97 added hipnotic episodes
		else if ( q1_rogue ) {
			count = mq1_rogueepisodes[ mqh_startepisode ].levels;
		} else   {
			count = mq1_episodes[ mqh_startepisode ].levels;
		}
	}

	if ( mqh_startlevel < 0 ) {
		mqh_startlevel = count - 1;
	}

	if ( mqh_startlevel >= count ) {
		mqh_startlevel = 0;
	}
}

static void MQH_NetStart_Change( int dir ) {
	switch ( mqh_gameoptions_cursor ) {
	case 2:
		MQH_ChangeMaxPlayers( dir );
		break;

	case 3:
		MQH_ChangeCoop( dir );
		break;

	case 4:
		MQH_ChangeTeamplay( dir );
		break;

	case 5:
		if ( GGameType & GAME_Hexen2 ) {
			MQH_ChangeClass( dir );
		} else   {
			MQH_ChangeSkill( dir );
		}
		break;

	case 6:
		if ( GGameType & GAME_Hexen2 ) {
			MQH_ChangeSkill( dir );
		} else   {
			MQH_ChangeFragLimit( dir );
		}
		break;

	case 7:
		if ( GGameType & GAME_Hexen2 ) {
			MQH_ChangeFragLimit( dir );
		} else   {
			MQH_ChangeTimeLimit( dir );
		}
		break;

	case 8:
		if ( GGameType & GAME_Hexen2 ) {
			MQH_ChangeTimeLimit( dir );
		} else   {
			MQ1_ChangeStartEpisode( dir );
		}
		break;

	case 9:
		if ( GGameType & GAME_Hexen2 ) {
			MQH_ChangeRandomClass( dir );
		} else   {
			MQH_ChangeStartLevel( dir );
		}
		break;

	case 10:
		MH2_ChangeStartEpisode( dir );
		break;

	case 11:
		MQH_ChangeStartLevel( dir );
		break;
	}
}

static void MQH_GameOptions_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_gameoptions_cursor--;
		if ( mqh_gameoptions_cursor < 0 ) {
			mqh_gameoptions_cursor = ( GGameType & GAME_Hexen2 ? NUM_GAMEOPTIONS_H2 : NUM_GAMEOPTIONS_Q1 ) - 1;
			if ( GGameType & GAME_Hexen2 && svqh_coop->value ) {
				mqh_gameoptions_cursor--;
			}
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_gameoptions_cursor++;
		if ( GGameType & GAME_Hexen2 && svqh_coop->value ) {
			if ( mqh_gameoptions_cursor >= ( GGameType & GAME_Hexen2 ? NUM_GAMEOPTIONS_H2 : NUM_GAMEOPTIONS_Q1 ) - 1 ) {
				mqh_gameoptions_cursor = 0;
			}
		} else   {
			if ( mqh_gameoptions_cursor >= ( GGameType & GAME_Hexen2 ? NUM_GAMEOPTIONS_H2 : NUM_GAMEOPTIONS_Q1 ) ) {
				mqh_gameoptions_cursor = 0;
			}
		}
		break;

	case K_LEFTARROW:
		if ( mqh_gameoptions_cursor == 0 ) {
			break;
		}
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu3.wav" : "misc/menu3.wav" );
		MQH_NetStart_Change( -1 );
		break;

	case K_RIGHTARROW:
		if ( mqh_gameoptions_cursor == 0 ) {
			break;
		}
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu3.wav" : "misc/menu3.wav" );
		MQH_NetStart_Change( 1 );
		break;

	case K_ENTER:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu2.wav" : "misc/menu2.wav" );
		if ( mqh_gameoptions_cursor == 0 ) {
			if ( SV_IsServerActive() ) {
				Cbuf_AddText( "disconnect\n" );
			}
			if ( GGameType & GAME_Hexen2 ) {
				Cbuf_AddText( va( "playerclass %d\n", setup_class + 1 ) );
			}
			Cbuf_AddText( "listen 0\n" );		// so host_netport will be re-examined
			Cbuf_AddText( va( "maxplayers %u\n", mqh_maxplayers ) );
			SCRQH_BeginLoadingPlaque();

			if ( GGameType & GAME_Hexen2 ) {
				Cbuf_AddText( va( "map %s\n", mh2_levels[ mh2_episodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name ) );
			} else if ( q1_hipnotic )     {
				Cbuf_AddText( va( "map %s\n", mq1_hipnoticlevels[ mq1_hipnoticepisodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name ) );
			} else if ( q1_rogue )     {
				Cbuf_AddText( va( "map %s\n", mq1_roguelevels[ mq1_rogueepisodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name ) );
			} else   {
				Cbuf_AddText( va( "map %s\n", mq1_levels[ mq1_episodes[ mqh_startepisode ].firstLevel + mqh_startlevel ].name ) );
			}
			return;
		}

		MQH_NetStart_Change( 1 );
		break;
	}

	if ( mqh_lanConfig_cursor == 0 ) {
		Field_KeyDownEvent( &lanConfig_portname, key );
	}
	MQH_CheckPortValue();
}

static void MQH_GameOptions_Char( int key ) {
	if ( mqh_gameoptions_cursor == 1 ) {
		if ( key >= 32 && ( key < '0' || key > '9' ) ) {
			return;
		}
		Field_CharEvent( &lanConfig_portname, key );
	}
}

//=============================================================================
/* SETUP MENU */

#define NUM_SETUP_CMDS_Q1  5
#define NUM_SETUP_CMDS_H2  6
#define NUM_SETUP_CMDS_HW  7

#define PLAYER_PIC_WIDTH 68
#define PLAYER_PIC_HEIGHT 114

static int mqh_setup_cursor;
static int setup_cursor_table_q1[] = {40, 56, 80, 104, 140};
static int setup_cursor_table_h2[] = {40, 56, 80, 104, 128, 156};
static int setup_cursor_table_hw[] = {40, 56, 72, 88, 112, 136, 164};
static field_t setup_hostname;
static field_t setup_myname;
static int setup_oldtop;
static int setup_oldbottom;
static int setup_top;
static int setup_bottom;
static int class_limit;
static int which_class;

static byte mqh_translationTable[ 256 ];
static byte mq1_menuplyr_pixels[ 4096 ];
static byte mh2_menuplyr_pixels[ MAX_PLAYER_CLASS ][ PLAYER_PIC_WIDTH * PLAYER_PIC_HEIGHT ];

static image_t* mq1_translate_texture;
static image_t* mh2_translate_texture[ MAX_PLAYER_CLASS ];

static void MQH_Menu_Setup_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_setup;
	mqh_entersound = true;
	String::Cpy( setup_myname.buffer, clqh_name->string );
	setup_myname.cursor = String::Length( setup_myname.buffer );
	setup_myname.maxLength = 15;
	setup_myname.widthInChars = 16;
	if ( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) {
		setup_top = setup_oldtop = ( int )qhw_topcolor->value;
		setup_bottom = setup_oldbottom = ( int )qhw_bottomcolor->value;
	} else   {
		String::Cpy( setup_hostname.buffer, sv_hostname->string );
		setup_hostname.cursor = String::Length( setup_hostname.buffer );
		setup_hostname.maxLength = 15;
		setup_hostname.widthInChars = 16;
		setup_top = setup_oldtop = ( ( int )clqh_color->value ) >> 4;
		setup_bottom = setup_oldbottom = ( ( int )clqh_color->value ) & 15;
	}
	if ( GGameType & GAME_Quake ) {
		mqh_setup_cursor = NUM_SETUP_CMDS_Q1 - 1;
	} else if ( GGameType & GAME_HexenWorld )     {
		if ( !com_portals ) {
			if ( clh2_playerclass->value == CLASS_DEMON ) {
				clh2_playerclass->value = 0;
			}
		}
		if ( String::ICmp( fs_gamedir, "siege" ) ) {
			if ( clh2_playerclass->value == CLASS_DWARF ) {
				clh2_playerclass->value = 0;
			}
		}

		setup_class = clh2_playerclass->value;

		class_limit = MAX_PLAYER_CLASS;

		if ( setup_class < 0 || setup_class > class_limit ) {
			setup_class = 1;
		}
		which_class = setup_class;
		mqh_setup_cursor = NUM_SETUP_CMDS_HW - 1;
	} else   {
		setup_class = clh2_playerclass->value;
		if ( setup_class < 1 || setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) ) {
			setup_class = ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 );
		}
		mqh_setup_cursor = NUM_SETUP_CMDS_H2 - 1;
	}
}

static void MQH_Setup_Draw() {
	static bool wait;

	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title4.lmp" );
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/p_multi.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
	}

	if ( !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
		MQH_Print( 64, 40, "Hostname" );
		MQH_DrawField( 168, 40, &setup_hostname, mqh_setup_cursor == 0 );
	}

	MQH_Print( 64, 56, "Your name" );
	MQH_DrawField( 168, 56, &setup_myname, mqh_setup_cursor == 1 );

	if ( GGameType & GAME_Hexen2 ) {
		if ( GGameType & GAME_HexenWorld ) {
			MQH_Print( 64, 72, "Spectator: " );
			if ( qhw_spectator->value ) {
				MQH_PrintWhite( 64 + 12 * 8, 72, "YES" );
			} else   {
				MQH_PrintWhite( 64 + 12 * 8, 72, "NO" );
			}

			MQH_Print( 64, 88, "Current Class: " );

			if ( !com_portals ) {
				if ( setup_class == CLASS_DEMON ) {
					setup_class = 0;
				}
			}
			if ( String::ICmp( fs_gamedir, "siege" ) ) {
				if ( setup_class == CLASS_DWARF ) {
					setup_class = 0;
				}
			}
			switch ( setup_class ) {
			case 0:
				MQH_PrintWhite( 88, 96, "Random" );
				break;
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6:
				MQH_PrintWhite( 88, 96, hw_ClassNames[ setup_class - 1 ] );
				break;
			}

			MQH_Print( 64, 112, "First color patch" );
			MQH_Print( 64, 136, "Second color patch" );

			MQH_DrawTextBox( 64, 164 - 8, 14, 1 );
			MQH_Print( 72, 164, "Accept Changes" );

			if ( setup_class == 0 ) {
				int i = ( cls.realtime / 100 ) % 8;

				if ( ( i == 0 && !wait ) || which_class == 0 ) {
					if ( !com_portals ) {	//not succubus
						if ( String::ICmp( fs_gamedir, "siege" ) ) {
							which_class = ( rand() % CLASS_THEIF ) + 1;
						} else   {
							which_class = ( rand() % CLASS_DEMON ) + 1;
							if ( which_class == CLASS_DEMON ) {
								which_class = CLASS_DWARF;
							}
						}
					} else   {
						if ( String::ICmp( fs_gamedir, "siege" ) ) {
							which_class = ( rand() % CLASS_DEMON ) + 1;
						} else   {
							which_class = ( rand() % class_limit ) + 1;
						}
					}
					wait = true;
				} else if ( i )     {
					wait = false;
				}
			} else   {
				which_class = setup_class;
			}
		} else   {
			MQH_Print( 64, 80, "Current Class: " );
			MQH_Print( 88, 88, h2_ClassNames[ setup_class - 1 ] );

			MQH_Print( 64, 104, "First color patch" );
			MQH_Print( 64, 128, "Second color patch" );

			MQH_DrawTextBox( 64, 156 - 8, 14, 1 );
			MQH_Print( 72, 156, "Accept Changes" );

			which_class = setup_class;
		}

		R_CachePicWithTransPixels( va( "gfx/menu/netp%i.lmp", which_class ), mh2_menuplyr_pixels[ which_class - 1 ] );
		CL_CalcHexen2SkinTranslation( setup_top, setup_bottom, which_class, mqh_translationTable );
		R_CreateOrUpdateTranslatedImage( mh2_translate_texture[ which_class - 1 ], va( "*translate_pic%d", which_class ), mh2_menuplyr_pixels[ which_class - 1 ], mqh_translationTable, PLAYER_PIC_WIDTH, PLAYER_PIC_HEIGHT );
		MQH_DrawPic( 220, 72, mh2_translate_texture[ which_class - 1 ] );
	} else   {
		MQH_Print( 64, 80, "Shirt color" );
		MQH_Print( 64, 104, "Pants color" );

		MQH_DrawTextBox( 64, 140 - 8, 14, 1 );
		MQH_Print( 72, 140, "Accept Changes" );

		image_t* p = R_CachePic( "gfx/bigbox.lmp" );
		MQH_DrawPic( 160, 64, p );
		p = R_CachePicWithTransPixels( "gfx/menuplyr.lmp", mq1_menuplyr_pixels );
		CL_CalcQuakeSkinTranslation( setup_top, setup_bottom, mqh_translationTable );
		R_CreateOrUpdateTranslatedImage( mq1_translate_texture, "*translate_pic", mq1_menuplyr_pixels, mqh_translationTable, R_GetImageWidth( p ), R_GetImageHeight( p ) );
		MQH_DrawPic( 172, 72, mq1_translate_texture );
	}

	MQH_DrawCharacter( 56, GGameType & GAME_HexenWorld ? setup_cursor_table_hw[ mqh_setup_cursor ] : GGameType & GAME_Hexen2 ? setup_cursor_table_h2[ mqh_setup_cursor ] :
		setup_cursor_table_q1[ mqh_setup_cursor ], 12 + ( ( cls.realtime / 250 ) & 1 ) );
}

static void MQH_Setup_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_setup_cursor--;
		if ( GGameType & GAME_HexenWorld ) {
			if ( mqh_setup_cursor < 1 ) {
				mqh_setup_cursor = NUM_SETUP_CMDS_HW - 1;
			}
		} else   {
			if ( mqh_setup_cursor < 0 ) {
				mqh_setup_cursor = ( GGameType & GAME_Hexen2 ? NUM_SETUP_CMDS_H2 : NUM_SETUP_CMDS_Q1 ) - 1;
			}
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_setup_cursor++;
		if ( GGameType & GAME_HexenWorld ) {
			if ( mqh_setup_cursor >= NUM_SETUP_CMDS_HW ) {
				mqh_setup_cursor = 1;
			}
		} else   {
			if ( mqh_setup_cursor >= ( GGameType & GAME_Hexen2 ? NUM_SETUP_CMDS_H2 : NUM_SETUP_CMDS_Q1 ) ) {
				mqh_setup_cursor = 0;
			}
		}
		break;

	case K_LEFTARROW:
		if ( mqh_setup_cursor < 2 ) {
			break;
		}
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu3.wav" : "misc/menu3.wav" );
		if ( GGameType & GAME_HexenWorld && mqh_setup_cursor == 2 ) {
			if ( qhw_spectator->value ) {
				Cvar_Set( "spectator","0" );
			} else   {
				Cvar_Set( "spectator","1" );
			}
			cl.qh_spectator = qhw_spectator->value;
		}
		if ( GGameType & GAME_Hexen2 && mqh_setup_cursor == ( GGameType & GAME_HexenWorld ? 3 : 2 ) ) {
			setup_class--;
			if ( GGameType & GAME_HexenWorld ) {
				if ( setup_class < 0 ) {
					setup_class = class_limit;
				}
			} else   {
				if ( setup_class < 1 ) {
					setup_class = ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 );
				}
			}
		}
		if ( mqh_setup_cursor == ( GGameType & GAME_HexenWorld ? 4 : GGameType & GAME_Hexen2 ? 3 : 2 ) ) {
			setup_top = setup_top - 1;
		}
		if ( mqh_setup_cursor == ( GGameType & GAME_HexenWorld ? 5 : GGameType & GAME_Hexen2 ? 4 : 3 ) ) {
			setup_bottom = setup_bottom - 1;
		}
		break;
	case K_RIGHTARROW:
		if ( mqh_setup_cursor < 2 ) {
			break;
		}
forward:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu3.wav" : "misc/menu3.wav" );
		if ( GGameType & GAME_HexenWorld && mqh_setup_cursor == 2 ) {
			if ( qhw_spectator->value ) {
				Cvar_Set( "spectator","0" );
			} else   {
				Cvar_Set( "spectator","1" );
			}
			cl.qh_spectator = qhw_spectator->value;
		}
		if ( GGameType & GAME_Hexen2 && mqh_setup_cursor == ( GGameType & GAME_HexenWorld ? 3 : 2 ) ) {
			setup_class++;
			if ( GGameType & GAME_HexenWorld ) {
				if ( setup_class > class_limit ) {
					setup_class = 0;
				}
			} else   {
				if ( setup_class > ( GGameType & GAME_H2Portals ? NUM_CLASSES_H2MP : NUM_CLASSES_H2 ) ) {
					setup_class = 1;
				}
			}
		}
		if ( mqh_setup_cursor == ( GGameType & GAME_HexenWorld ? 4 : GGameType & GAME_Hexen2 ? 3 : 2 ) ) {
			setup_top = setup_top + 1;
		}
		if ( mqh_setup_cursor == ( GGameType & GAME_HexenWorld ? 5 : GGameType & GAME_Hexen2 ? 4 : 3 ) ) {
			setup_bottom = setup_bottom + 1;
		}
		break;

	case K_ENTER:
		if ( mqh_setup_cursor == 0 || mqh_setup_cursor == 1 ) {
			return;
		}

		if ( mqh_setup_cursor == 2 || mqh_setup_cursor == 3 || ( GGameType & GAME_Hexen2 && mqh_setup_cursor == 4 ) || ( GGameType & GAME_HexenWorld && mqh_setup_cursor == 5 ) ) {
			goto forward;
		}

		if ( String::Cmp( clqh_name->string, setup_myname.buffer ) != 0 ) {
			Cbuf_AddText( va( "name \"%s\"\n", setup_myname.buffer ) );
		}
		if ( !( GGameType & GAME_HexenWorld ) && String::Cmp( sv_hostname->string, setup_hostname.buffer ) != 0 ) {
			Cvar_Set( "hostname", setup_hostname.buffer );
		}
		if ( setup_top != setup_oldtop || setup_bottom != setup_oldbottom ) {
			Cbuf_AddText( va( "color %i %i\n", setup_top, setup_bottom ) );
		}
		if ( GGameType & GAME_Hexen2 ) {
			Cbuf_AddText( va( "playerclass %d\n", setup_class ) );
		}
		mqh_entersound = true;
		MQH_Menu_MultiPlayer_f();
		break;
	}
	if ( !( GGameType & GAME_HexenWorld ) && mqh_setup_cursor == 0 ) {
		Field_KeyDownEvent( &setup_hostname, k );
	}
	if ( mqh_setup_cursor == 1 ) {
		Field_KeyDownEvent( &setup_myname, k );
	}

	int maxColour = GGameType & GAME_Hexen2 ? 10 : 13;
	if ( setup_top > maxColour ) {
		setup_top = 0;
	}
	if ( setup_top < 0 ) {
		setup_top = maxColour;
	}
	if ( setup_bottom > maxColour ) {
		setup_bottom = 0;
	}
	if ( setup_bottom < 0 ) {
		setup_bottom = maxColour;
	}
}

static void MQH_Setup_Char( int k ) {
	if ( mqh_setup_cursor == 0 ) {
		Field_CharEvent( &setup_hostname, k );
	}
	if ( mqh_setup_cursor == 1 ) {
		Field_CharEvent( &setup_myname, k );
	}
}

//=============================================================================
/* MULTIPLAYER LOAD/SAVE MENU */

static void M_ScanMSaves() {
	int i, j;
	char name[ MAX_OSPATH ];
	fileHandle_t f;

	for ( i = 0; i < MAX_SAVEGAMES; i++ ) {
		String::Cpy( mqh_filenames[ i ], "--- UNUSED SLOT ---" );
		mqh_loadable[ i ] = false;
		sprintf( name, "ms%i/info.dat", i );
		if ( !FS_FileExists( name ) ) {
			continue;
		}
		FS_FOpenFileRead( name, &f, true );
		if ( !f ) {
			continue;
		}
		FS_Read( name, 80, f );
		name[ 80 ] = 0;
		//	Skip version
		char* Ptr = name;
		while ( *Ptr && *Ptr != '\n' )
			Ptr++;
		if ( *Ptr == '\n' ) {
			*Ptr = 0;
			Ptr++;
		}
		char* SaveName = Ptr;
		while ( *Ptr && *Ptr != '\n' )
			Ptr++;
		*Ptr = 0;
		String::NCpy( mqh_filenames[ i ], SaveName, sizeof ( mqh_filenames[ i ] ) - 1 );

		// change _ back to space
		for ( j = 0; j < SAVEGAME_COMMENT_LENGTH; j++ )
			if ( mqh_filenames[ i ][ j ] == '_' ) {
				mqh_filenames[ i ][ j ] = ' ';
			}
		mqh_loadable[ i ] = true;
		FS_FCloseFile( f );
	}
}

static void MH2_Menu_MLoad_f() {
	mqh_entersound = true;
	m_state = m_mload;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanMSaves();
}


static void MH2_Menu_MSave_f() {
	if ( !SV_IsServerActive() || cl.qh_intermission || SVQH_GetMaxClients() == 1 ) {
		mh2_message = "Only a network server";
		mh2_message2 = "can save a multiplayer game";
		mh2_message_time = cls.realtime;
		return;
	}
	mqh_entersound = true;
	m_state = m_msave;
	in_keyCatchers |= KEYCATCH_UI;
	M_ScanMSaves();
}

static void MH2_MLoad_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_ENTER:
		S_StartLocalSound( "raven/menu2.wav" );
		if ( !mqh_loadable[ mqh_load_cursor ] ) {
			return;
		}
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;

		if ( SV_IsServerActive() ) {
			Cbuf_AddText( "disconnect\n" );
		}
		Cbuf_AddText( "listen 1\n" );		// so host_netport will be re-examined

		// Host_Loadgame_f can't bring up the loading plaque because too much
		// stack space has been used, so do it now
		SCRQH_BeginLoadingPlaque();

		// issue the load command
		Cbuf_AddText( va( "load ms%i\n", mqh_load_cursor ) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		mqh_load_cursor--;
		if ( mqh_load_cursor < 0 ) {
			mqh_load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		mqh_load_cursor++;
		if ( mqh_load_cursor >= MAX_SAVEGAMES ) {
			mqh_load_cursor = 0;
		}
		break;
	}
}

static void MH2_MSave_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_MultiPlayer_f();
		break;

	case K_ENTER:
		m_state = m_none;
		in_keyCatchers &= ~KEYCATCH_UI;
		Cbuf_AddText( va( "save ms%i\n", mqh_load_cursor ) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		mqh_load_cursor--;
		if ( mqh_load_cursor < 0 ) {
			mqh_load_cursor = MAX_SAVEGAMES - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound( "raven/menu1.wav" );
		mqh_load_cursor++;
		if ( mqh_load_cursor >= MAX_SAVEGAMES ) {
			mqh_load_cursor = 0;
		}
		break;
	}
}

//=============================================================================
/* OPTIONS MENU */

#define OPTIONS_ITEMS_Q1    12
#define OPTIONS_ITEMS_QW    14

enum
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,
	OPT_DEFAULTS,
	OPT_SCRSIZE,	//3
	OPT_GAMMA,		//4
	OPT_MOUSESPEED,	//5
	OPT_MUSICVOL_MUSICTYPE,	//6
	OPT_SNDVOL_MUSICVOL,	//7
	OPT_ALWAYSRUN_SNDVOL,		//8
	OPT_INVMOUSE_ALWAYRUN,	//9
	OPT_LOOKSPRING_INVMOUSE,	//10
	OPT_VIDEO_OLDSBAR_LOOKSPRING,	//11
	OPT_HUDSWAP_CROSSHAIR,	//12
	OPT_VIDEO_ALWAYSMLOOK,	//13
	OPT_VIDEO,		//14
	OPTIONS_ITEMS
};

#define SLIDER_RANGE    10

static int mqh_options_cursor;

static void MQH_Menu_Options_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_options;
	mqh_entersound = true;
}

static void MQH_DrawSlider( int x, int y, float range ) {
	if ( range < 0 ) {
		range = 0;
	}
	if ( range > 1 ) {
		range = 1;
	}
	MQH_DrawCharacter( x - 8, y, GGameType & GAME_Hexen2 ? 256 : 128 );
	for ( int i = 0; i < SLIDER_RANGE; i++ ) {
		MQH_DrawCharacter( x + i * 8, y, GGameType & GAME_Hexen2 ? 257 : 129 );
	}
	MQH_DrawCharacter( x + SLIDER_RANGE * 8, y, GGameType & GAME_Hexen2 ? 258 : 130 );
	MQH_DrawCharacter( x + ( SLIDER_RANGE - 1 ) * 8 * range, y, GGameType & GAME_Hexen2 ? 259 : 131 );
}

static void MQH_DrawCheckbox( int x, int y, int on ) {
	if ( on ) {
		MQH_Print( x, y, "on" );
	} else   {
		MQH_Print( x, y, "off" );
	}
}

static void MQH_Options_Draw() {
	int itemsStartY;
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title3.lmp" );
		itemsStartY = 60;
	} else   {
		MQH_DrawPic( 16, 4, R_CachePic( "gfx/qplaque.lmp" ) );
		image_t* p = R_CachePic( "gfx/p_option.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		itemsStartY = 32;
	}

	int y = itemsStartY;
	MQH_Print( 16, y, "    Customize controls" );
	y += 8;
	MQH_Print( 16, y, "         Go to console" );
	y += 8;
	MQH_Print( 16, y, "     Reset to defaults" );
	y += 8;

	MQH_Print( 16, y, "           Screen size" );
	float r = ( scr_viewsize->value - 30 ) / ( 120 - 30 );
	MQH_DrawSlider( 220, y, r );
	y += 8;

	MQH_Print( 16, y, "            Brightness" );
	r = ( r_gamma->value - 1 );
	MQH_DrawSlider( 220, y, r );
	y += 8;

	MQH_Print( 16, y, "           Mouse Speed" );
	r = ( cl_sensitivity->value - 1 ) / 10;
	MQH_DrawSlider( 220, y, r );
	y += 8;

	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( 16, y, "            Music Type" );
		if ( String::ICmp( bgmtype->string,"midi" ) == 0 ) {
			MQH_Print( 220, y, "MIDI" );
		} else if ( String::ICmp( bgmtype->string,"cd" ) == 0 ) {
			MQH_Print( 220, y, "CD" );
		} else {
			MQH_Print( 220, y, "None" );
		}
		y += 8;
	}

	MQH_Print( 16, y, "          Music Volume" );
	r = bgmvolume->value;
	MQH_DrawSlider( 220, y, r );
	y += 8;

	MQH_Print( 16, y, "          Sound Volume" );
	r = s_volume->value;
	MQH_DrawSlider( 220, y, r );
	y += 8;

	MQH_Print( 16, y, "            Always Run" );
	MQH_DrawCheckbox( 220, y, cl_forwardspeed->value > 200 );
	y += 8;

	MQH_Print( 16, y, "          Invert Mouse" );
	MQH_DrawCheckbox( 220, y, m_pitch->value < 0 );
	y += 8;

	MQH_Print( 16, y, "            Lookspring" );
	MQH_DrawCheckbox( 220, y, lookspring->value );
	y += 8;

	if ( GGameType & GAME_QuakeWorld ) {
		MQH_Print( 16, y, "    Use old status bar" );
		MQH_DrawCheckbox( 220, y, clqh_sbar->value );
		y += 8;

		MQH_Print( 16, y, "      HUD on left side" );
		MQH_DrawCheckbox( 220, y, clqw_hudswap->value );
		y += 8;
	}

	if ( GGameType & GAME_Hexen2 ) {
		MQH_Print( 16, y, "        Show Crosshair" );
		MQH_DrawCheckbox( 220, y, crosshair->value );
		y += 8;

		MQH_Print( 16, y, "            Mouse Look" );
		MQH_DrawCheckbox( 220, y, cl_freelook->value );
		y += 8;
	}

	MQH_Print( 16, y, "         Video Options" );

	// cursor
	MQH_DrawCharacter( 200, itemsStartY + mqh_options_cursor * 8, 12 + ( ( cls.realtime / 250 ) & 1 ) );
}

static void MQH_AdjustSliders( int dir ) {
	S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu3.wav" : "misc/menu3.wav" );

	switch ( mqh_options_cursor ) {
	case OPT_SCRSIZE:	// screen size
		scr_viewsize->value += dir * 10;
		if ( scr_viewsize->value < 30 ) {
			scr_viewsize->value = 30;
		}
		if ( scr_viewsize->value > 120 ) {
			scr_viewsize->value = 120;
		}
		Cvar_SetValue( "viewsize", scr_viewsize->value );
		break;
	case OPT_GAMMA:	// gamma
		r_gamma->value += dir * 0.1;
		if ( r_gamma->value < 1 ) {
			r_gamma->value = 1;
		}
		if ( r_gamma->value > 2 ) {
			r_gamma->value = 2;
		}
		Cvar_SetValue( "r_gamma", r_gamma->value );
		break;
	case OPT_MOUSESPEED:	// mouse speed
		cl_sensitivity->value += dir * 0.5;
		if ( cl_sensitivity->value < 1 ) {
			cl_sensitivity->value = 1;
		}
		if ( cl_sensitivity->value > 11 ) {
			cl_sensitivity->value = 11;
		}
		Cvar_SetValue( "sensitivity", cl_sensitivity->value );
		break;
	case OPT_MUSICVOL_MUSICTYPE:	// music volume
		if ( GGameType & GAME_Hexen2 ) {
			if ( String::ICmp( bgmtype->string,"midi" ) == 0 ) {
				if ( dir < 0 ) {
					Cvar_Set( "bgmtype","none" );
				} else   {
					Cvar_Set( "bgmtype","cd" );
				}
			} else if ( String::ICmp( bgmtype->string,"cd" ) == 0 )       {
				if ( dir < 0 ) {
					Cvar_Set( "bgmtype","midi" );
				} else   {
					Cvar_Set( "bgmtype","none" );
				}
			} else   {
				if ( dir < 0 ) {
					Cvar_Set( "bgmtype","cd" );
				} else   {
					Cvar_Set( "bgmtype","midi" );
				}
			}
		} else   {
#ifdef _WIN32
			bgmvolume->value += dir * 1.0;
#else
			bgmvolume->value += dir * 0.1;
#endif
			if ( bgmvolume->value < 0 ) {
				bgmvolume->value = 0;
			}
			if ( bgmvolume->value > 1 ) {
				bgmvolume->value = 1;
			}
			Cvar_SetValue( "bgmvolume", bgmvolume->value );
		}
		break;
	case OPT_SNDVOL_MUSICVOL:	// sfx volume
		if ( GGameType & GAME_Hexen2 ) {
			bgmvolume->value += dir * 0.1;

			if ( bgmvolume->value < 0 ) {
				bgmvolume->value = 0;
			}
			if ( bgmvolume->value > 1 ) {
				bgmvolume->value = 1;
			}
			Cvar_SetValue( "bgmvolume", bgmvolume->value );
		} else   {
			s_volume->value += dir * 0.1;
			if ( s_volume->value < 0 ) {
				s_volume->value = 0;
			}
			if ( s_volume->value > 1 ) {
				s_volume->value = 1;
			}
			Cvar_SetValue( "s_volume", s_volume->value );
		}
		break;
	case OPT_ALWAYSRUN_SNDVOL:	// allways run
		if ( GGameType & GAME_Hexen2 ) {
			s_volume->value += dir * 0.1;
			if ( s_volume->value < 0 ) {
				s_volume->value = 0;
			}
			if ( s_volume->value > 1 ) {
				s_volume->value = 1;
			}
			Cvar_SetValue( "s_volume", s_volume->value );
		} else   {
			if ( cl_forwardspeed->value > 200 ) {
				Cvar_SetValue( "cl_forwardspeed", 200 );
				Cvar_SetValue( "cl_backspeed", 200 );
			} else   {
				Cvar_SetValue( "cl_forwardspeed", 400 );
				Cvar_SetValue( "cl_backspeed", 400 );
			}
		}
		break;
	case OPT_INVMOUSE_ALWAYRUN:	// invert mouse
		if ( GGameType & GAME_Hexen2 ) {
			if ( cl_forwardspeed->value > 200 ) {
				Cvar_SetValue( "cl_forwardspeed", 200 );
			} else   {
				Cvar_SetValue( "cl_forwardspeed", 400 );
			}
		} else   {
			Cvar_SetValue( "m_pitch", -m_pitch->value );
		}
		break;

	case OPT_LOOKSPRING_INVMOUSE:	// lookspring
		if ( GGameType & GAME_Hexen2 ) {
			Cvar_SetValue( "m_pitch", -m_pitch->value );
		} else   {
			Cvar_SetValue( "lookspring", !lookspring->value );
		}
		break;

	case OPT_VIDEO_OLDSBAR_LOOKSPRING:
		if ( GGameType & GAME_Hexen2 ) {
			Cvar_SetValue( "lookspring", !lookspring->value );
		} else   {
			Cvar_SetValue( "cl_sbar", !clqh_sbar->value );
		}
		break;

	case OPT_HUDSWAP_CROSSHAIR:
		if ( GGameType & GAME_Hexen2 ) {
			Cvar_SetValue( "crosshair", !crosshair->value );
		} else   {
			Cvar_SetValue( "cl_hudswap", !clqw_hudswap->value );
		}
		break;

	case OPT_VIDEO_ALWAYSMLOOK:
		Cvar_SetValue( "cl_freelook", !cl_freelook->value );
		break;
	}
}

static void MQH_Options_Key( int k ) {
	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_ENTER:
		mqh_entersound = true;
		switch ( mqh_options_cursor ) {
		case OPT_CUSTOMIZE:
			MQH_Menu_Keys_f();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			in_keyCatchers &= ~KEYCATCH_UI;
			Con_ToggleConsole_f();
			break;
		case OPT_DEFAULTS:
			Cbuf_AddText( "exec default.cfg\n" );
			break;
		case OPT_VIDEO_OLDSBAR_LOOKSPRING:
			if ( GGameType & GAME_Quake && !( GGameType & GAME_QuakeWorld ) ) {
				MQH_Menu_Video_f();
			} else   {
				MQH_AdjustSliders( 1 );
			}
			break;
		case OPT_VIDEO_ALWAYSMLOOK:
			if ( GGameType & GAME_QuakeWorld ) {
				MQH_Menu_Video_f();
			} else   {
				MQH_AdjustSliders( 1 );
			}
			break;
		case OPT_VIDEO:
			if ( GGameType & GAME_Hexen2 ) {
				MQH_Menu_Video_f();
				break;
			}
		default:
			MQH_AdjustSliders( 1 );
			break;
		}
		return;

	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_options_cursor--;
		if ( mqh_options_cursor < 0 ) {
			mqh_options_cursor = ( GGameType & GAME_Hexen2 ? OPTIONS_ITEMS : GGameType & GAME_QuakeWorld ? OPTIONS_ITEMS_QW : OPTIONS_ITEMS_Q1 ) - 1;
		}
		break;

	case K_DOWNARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_options_cursor++;
		if ( mqh_options_cursor >= ( GGameType & GAME_Hexen2 ? OPTIONS_ITEMS : GGameType & GAME_QuakeWorld ? OPTIONS_ITEMS_QW : OPTIONS_ITEMS_Q1 ) ) {
			mqh_options_cursor = 0;
		}
		break;

	case K_LEFTARROW:
		MQH_AdjustSliders( -1 );
		break;

	case K_RIGHTARROW:
		MQH_AdjustSliders( 1 );
		break;
	}
}

//=============================================================================
/* KEYS MENU */

#define KEYS_SIZE 14

static const char* mq1_bindnames[][ 2 ] =
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

static const char* mh2_bindnames[][ 2 ] =
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
	{"+crouch",         "crouch"},
	{"+lookup",         "look up"},
	{"+lookdown",       "look down"},
	{"centerview",      "center view"},
	{"+mlook",          "mouse look"},
	{"+moveup",         "swim up"},
	{"+movedown",       "swim down"},
	{"impulse 13",      "lift object"},
	{"invuse",          "use inv item"},
	{"impulse 44",      "drop inv item"},
	{"+showinfo",       "full inventory"},
	{"+showdm",         "info / frags"},
	{"toggle_dm",       "toggle frags"},
	{"+infoplaque",     "objectives"},
	{"invleft",         "inv move left"},
	{"invright",        "inv move right"},
	{"impulse 100",     "inv:torch"},
	{"impulse 101",     "inv:qrtz flask"},
	{"impulse 102",     "inv:mystic urn"},
	{"impulse 103",     "inv:krater"},
	{"impulse 104",     "inv:chaos devc"},
	{"impulse 105",     "inv:tome power"},
	{"impulse 106",     "inv:summon stn"},
	{"impulse 107",     "inv:invisiblty"},
	{"impulse 108",     "inv:glyph"},
	{"impulse 109",     "inv:boots"},
	{"impulse 110",     "inv:repulsion"},
	{"impulse 111",     "inv:bo peep"},
	{"impulse 112",     "inv:flight"},
	{"impulse 113",     "inv:force cube"},
	{"impulse 114",     "inv:icon defn"}
};

static const char* mhw_bindnames[][ 2 ] =
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
	{"+crouch",         "crouch"},
	{"+lookup",         "look up"},
	{"+lookdown",       "look down"},
	{"centerview",      "center view"},
	{"+mlook",          "mouse look"},
	{"+moveup",         "swim up"},
	{"+movedown",       "swim down"},
	{"impulse 13",      "use object"},
	{"invuse",          "use inv item"},
	{"invdrop",         "drop inv item"},
	{"+showinfo",       "full inventory"},
	{"+showdm",         "info / frags"},
	{"toggle_dm",       "toggle frags"},
	{"+shownames",      "player names"},
	{"invleft",         "inv move left"},
	{"invright",        "inv move right"},
	{"impulse 100",     "inv:torch"},
	{"impulse 101",     "inv:qrtz flask"},
	{"impulse 102",     "inv:mystic urn"},
	{"impulse 103",     "inv:krater"},
	{"impulse 104",     "inv:chaos devc"},
	{"impulse 105",     "inv:tome power"},
	{"impulse 106",     "inv:summon stn"},
	{"impulse 107",     "inv:invisiblty"},
	{"impulse 108",     "inv:glyph"},
	{"impulse 109",     "inv:boots"},
	{"impulse 110",     "inv:repulsion"},
	{"impulse 111",     "inv:bo peep"},
	{"impulse 112",     "inv:flight"},
	{"impulse 113",     "inv:force cube"},
	{"impulse 114",     "inv:icon defn"}
};

static int numBindNames;

static int mqh_keys_cursor;
static bool mqh_bind_grab;
static int mqh_keys_top = 0;

static void MQH_Menu_Keys_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_keys;
	mqh_entersound = true;
	if ( GGameType & GAME_HexenWorld ) {
		numBindNames = sizeof ( mhw_bindnames ) / sizeof ( mhw_bindnames[ 0 ] );
	} else if ( GGameType & GAME_Hexen2 )     {
		numBindNames = sizeof ( mh2_bindnames ) / sizeof ( mh2_bindnames[ 0 ] );
	} else   {
		numBindNames = sizeof ( mq1_bindnames ) / sizeof ( mq1_bindnames[ 0 ] );
	}
}

static void MQH_Keys_Draw() {
	int topy;
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title6.lmp" );
		topy = 64;
	} else   {
		image_t* p = R_CachePic( "gfx/ttl_cstm.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
		topy = 32;
	}

	if ( mqh_bind_grab ) {
		MQH_Print( 12, topy, "Press a key or button for this action" );
	} else   {
		MQH_Print( 18, topy, "Enter to change, backspace to clear" );
	}

	if ( mqh_keys_top ) {
		MQH_DrawCharacter( 6, topy + 16, GGameType & GAME_Hexen2 ? 128 : '-' );
	}
	if ( mqh_keys_top + KEYS_SIZE < numBindNames ) {
		MQH_DrawCharacter( 6, topy + 16 + ( ( KEYS_SIZE - 1 ) * 8 ), GGameType & GAME_Hexen2 ? 129 : '+' );
	}

	// search for known bindings
	for ( int i = 0; i < KEYS_SIZE; i++ ) {
		int y = topy + 16 + 8 * i;

		const char** bindname = GGameType & GAME_HexenWorld ? mhw_bindnames[ i + mqh_keys_top ] :
								GGameType & GAME_Hexen2 ? mh2_bindnames[ i + mqh_keys_top ] : mq1_bindnames[ i + mqh_keys_top ];
		MQH_Print( 16, y, bindname[ 1 ] );

		int key1;
		int key2;
		Key_GetKeysForBinding( bindname[ 0 ], &key1, &key2 );

		if ( key1 == -1 ) {
			MQH_Print( 140, y, "???" );
		} else   {
			const char* name = Key_KeynumToString( key1, true );
			MQH_Print( 140, y, name );
			int x = String::Length( name ) * 8;
			if ( key2 != -1 ) {
				MQH_Print( 140 + x + 8, y, "or" );
				MQH_Print( 140 + x + 32, y, Key_KeynumToString( key2, true ) );
			}
		}
	}

	if ( mqh_bind_grab ) {
		MQH_DrawCharacter( 130, topy + 16 + ( mqh_keys_cursor - mqh_keys_top ) * 8, '=' );
	} else   {
		MQH_DrawCharacter( 130, topy + 16 + ( mqh_keys_cursor - mqh_keys_top ) * 8, 12 + ( ( cls.realtime / 250 ) & 1 ) );
	}
}

static void MQH_Keys_Key( int k ) {
	int keys[ 2 ];

	if ( mqh_bind_grab ) {	// defining a key
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		if ( k == K_ESCAPE ) {
			mqh_bind_grab = false;
		} else if ( k != '`' )     {
			Key_SetBinding( k, GGameType & GAME_HexenWorld ? mhw_bindnames[ mqh_keys_cursor ][ 0 ] :
				GGameType & GAME_Hexen2 ? mh2_bindnames[ mqh_keys_cursor ][ 0 ] : mq1_bindnames[ mqh_keys_cursor ][ 0 ] );
		}

		mqh_bind_grab = false;
		return;
	}

	switch ( k ) {
	case K_ESCAPE:
		MQH_Menu_Options_f();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_keys_cursor--;
		if ( mqh_keys_cursor < 0 ) {
			mqh_keys_cursor = numBindNames - 1;
		}
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		mqh_keys_cursor++;
		if ( mqh_keys_cursor >= numBindNames ) {
			mqh_keys_cursor = 0;
		}
		break;

	case K_ENTER:		// go into bind mode
		Key_GetKeysForBinding( GGameType & GAME_HexenWorld ? mhw_bindnames[ mqh_keys_cursor ][ 0 ] :
			GGameType & GAME_Hexen2 ? mh2_bindnames[ mqh_keys_cursor ][ 0 ] : mq1_bindnames[ mqh_keys_cursor ][ 0 ], &keys[ 0 ], &keys[ 1 ] );
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu2.wav" : "misc/menu2.wav" );
		if ( keys[ 1 ] != -1 ) {
			Key_UnbindCommand( GGameType & GAME_HexenWorld ? mhw_bindnames[ mqh_keys_cursor ][ 0 ] :
				GGameType & GAME_Hexen2 ? mh2_bindnames[ mqh_keys_cursor ][ 0 ] : mq1_bindnames[ mqh_keys_cursor ][ 0 ] );
		}
		mqh_bind_grab = true;
		break;

	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu2.wav" : "misc/menu2.wav" );
		Key_UnbindCommand( GGameType & GAME_HexenWorld ? mhw_bindnames[ mqh_keys_cursor ][ 0 ] :
			GGameType & GAME_Hexen2 ? mh2_bindnames[ mqh_keys_cursor ][ 0 ] : mq1_bindnames[ mqh_keys_cursor ][ 0 ] );
		break;
	}

	if ( mqh_keys_cursor < mqh_keys_top ) {
		mqh_keys_top = mqh_keys_cursor;
	} else if ( mqh_keys_cursor >= mqh_keys_top + KEYS_SIZE )     {
		mqh_keys_top = mqh_keys_cursor - KEYS_SIZE + 1;
	}
}

//=============================================================================
/* VIDEO MENU */

#define MAX_COLUMN_SIZE     9
#define MODE_AREA_HEIGHT    ( MAX_COLUMN_SIZE + 2 )

static void MQH_Menu_Video_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_video;
	mqh_entersound = true;
}

static void MQH_Video_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		MH2_ScrollTitle( "gfx/menu/title7.lmp" );
	} else   {
		image_t* p = R_CachePic( "gfx/vidmodes.lmp" );
		MQH_DrawPic( ( 320 - R_GetImageWidth( p ) ) / 2, 4, p );
	}

	MQH_Print( 3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 2,
		"Video modes must be set from the" );
	MQH_Print( 3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 3,
		"console with set r_mode <number>" );
	MQH_Print( 3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 4,
		"and set r_colorbits <bits-per-pixel>" );
	MQH_Print( 3 * 8, 36 + MODE_AREA_HEIGHT * 8 + 8 * 6,
		"Select windowed mode with set r_fullscreen 0" );
}

static void MQH_Video_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu1.wav" : "misc/menu1.wav" );
		MQH_Menu_Options_f();
		break;

	default:
		break;
	}
}

//=============================================================================
/* HELP MENU */

#define NUM_HELP_PAGES_Q1   6
#define NUM_HELP_PAGES_H2   5
#define NUM_SG_HELP_PAGES   10	//Siege has more help

static int mqh_help_page;

static void MQH_Menu_Help_f() {
	in_keyCatchers |= KEYCATCH_UI;
	m_state = m_help;
	mqh_entersound = true;
	mqh_help_page = 0;
}

static void MQH_Help_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		if ( GGameType & GAME_HexenWorld && clhw_siege ) {
			MQH_DrawPic( 0, 0, R_CachePic( va( "gfx/menu/sghelp%02i.lmp", mqh_help_page + 1 ) ) );
		} else   {
			MQH_DrawPic( 0, 0, R_CachePic( va( "gfx/menu/help%02i.lmp", mqh_help_page + 1 ) ) );
		}
	} else   {
		MQH_DrawPic( 0, 0, R_CachePic( va( "gfx/help%i.lmp", mqh_help_page ) ) );
	}
}

static void MQH_Help_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
		MQH_Menu_Main_f();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		mqh_entersound = true;
		if ( GGameType & GAME_HexenWorld && clhw_siege ) {
			if ( ++mqh_help_page >= NUM_SG_HELP_PAGES ) {
				mqh_help_page = 0;
			}
		} else if ( ++mqh_help_page >= ( GGameType & GAME_Hexen2 ? NUM_HELP_PAGES_H2 : NUM_HELP_PAGES_Q1 ) )       {
			mqh_help_page = 0;
		}
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		mqh_entersound = true;
		if ( --mqh_help_page < 0 ) {
			if ( GGameType & GAME_HexenWorld && clhw_siege ) {
				mqh_help_page = NUM_SG_HELP_PAGES - 1;
			} else   {
				mqh_help_page = ( GGameType & GAME_Hexen2 ? NUM_HELP_PAGES_H2 : NUM_HELP_PAGES_Q1 ) - 1;
			}
		}
		break;
	}

}

//=============================================================================
/* QUIT MENU */

#define QUIT_SIZE_H2 18

#define VSTR( x ) # x
#define VSTR2( x ) VSTR( x )

static bool wasInMenus;
static menu_state_t m_quit_prevstate;

static float LinePos;
static int LineTimes;
static int MaxLines;
static const char** LineText;
static bool SoundPlayed;

#define MAX_LINES_H2 138
static const char* CreditTextH2[ MAX_LINES_H2 ] =
{
	"Project Director: James Monroe",
	"Creative Director: Brian Raffel",
	"Project Coordinator: Kevin Schilder",
	"",
	"Lead Programmer: James Monroe",
	"",
	"Programming:",
	"   Mike Gummelt",
	"   Josh Weier",
	"",
	"Additional Programming:",
	"   Josh Heitzman",
	"   Nathan Albury",
	"   Rick Johnson",
	"",
	"Assembly Consultant:",
	"   Mr. John Scott",
	"",
	"Lead Design: Jon Zuk",
	"",
	"Design:",
	"   Tom Odell",
	"   Jeremy Statz",
	"   Mike Renner",
	"   Eric Biessman",
	"   Kenn Hoekstra",
	"   Matt Pinkston",
	"   Bobby Duncanson",
	"   Brian Raffel",
	"",
	"Art Director: Les Dorscheid",
	"",
	"Art:",
	"   Kim Lathrop",
	"   Gina Garren",
	"   Joe Koberstein",
	"   Kevin Long",
	"   Jeff Butler",
	"   Scott Rice",
	"   John Payne",
	"   Steve Raffel",
	"",
	"Animation:",
	"   Eric Turman",
	"   Chaos (Mike Werckle)",
	"",
	"Music:",
	"   Kevin Schilder",
	"",
	"Sound:",
	"   Chia Chin Lee",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve Stringer",
	"",
	"Marketing Product Manager:",
	"   Henk Hartong",
	"",
	"Marketing Associate:",
	"   Kevin Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   Doug Jacobs",
	"",
	"Quality Assurance Team:",
	"   Steve Rosenthal, Steve Elwell,",
	"   Chad Bordwell, David Baker,",
	"   Aaron Casillas, Damien Fischer,",
	"   Winnie Lee, Igor Krinitskiy,",
	"   Samantha Lee, John Park",
	"   Ian Stevens, Chris Toft",
	"",
	"Production Testers:",
	"   Steve Rosenthal and",
	"   Chad Bordwell",
	"",
	"Additional QA and Support:",
	"    Tony Villalobos",
	"    Jason Sullivan",
	"",
	"Installer by:",
	"   Steve Stringer, Adam Goldberg,",
	"   Tanya Martino, Eric Schmidt,",
	"   Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey Chico and Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Our Big Toe:",
	"   Mitch Lasky",
	"",
	"",
	"Special Thanks to:",
	"  Id software",
	"  The original Hexen2 crew",
	"   We couldn't have done it",
	"   without you guys!",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"",
	"No yaks were harmed in the",
	"making of this game!"
};

#define MAX_LINES2_H2 150
static const char* Credit2TextH2[ MAX_LINES2_H2 ] =
{
	"PowerTrip: James (emorog) Monroe",
	"Cartoons: Brian Raffel",
	"         (use more puzzles)",
	"Doc Keeper: Kevin Schilder",
	"",
	"Whip cracker: James Monroe",
	"",
	"Whipees:",
	"   Mike (i didn't break it) Gummelt",
	"   Josh (extern) Weier",
	"",
	"We don't deserve whipping:",
	"   Josh (I'm not on this project)",
	"         Heitzman",
	"   Nathan (deer hunter) Albury",
	"   Rick (model crusher) Johnson",
	"",
	"Bit Packer:",
	"   Mr. John (Slaine) Scott",
	"",
	"Lead Slacker: Jon (devil boy) Zuk",
	"",
	"Other Slackers:",
	"   Tom (can i have an office) Odell",
	"   Jeremy (nt crashed again) Statz",
	"   Mike (i should be doing my ",
	"         homework) Renner",
	"   Eric (the nose) Biessman",
	"   Kenn (.plan) Hoekstra",
	"   Matt (big elbow) Pinkston",
	"   Bobby (needs haircut) Duncanson",
	"   Brian (they're in my town) Raffel",
	"",
	"Use the mouse: Les Dorscheid",
	"",
	"What's a mouse?:",
	"   Kim (where's my desk) Lathrop",
	"   Gina (i can do your laundry)",
	"        Garren",
	"   Joe (broken axle) Koberstein",
	"   Kevin (titanic) Long",
	"   Jeff (norbert) Butler",
	"   Scott (what's the DEL key for?)",
	"          Rice",
	"   John (Shpluuurt!) Payne",
	"   Steve (crash) Raffel",
	"",
	"Boners:",
	"   Eric (terminator) Turman",
	"   Chaos Device",
	"",
	"Drum beater:",
	"   Kevin Schilder",
	"",
	"Whistle blower:",
	"   Chia Chin (bruce) Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve 'Ferris' Stringer",
	"",
	"Marketing Product Manager:",
	"   Henk 'GODMODE' Hartong",
	"",
	"Marketing Associate:",
	"   Kevin 'Kraffinator' Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim 'Outlaw' Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   Doug Jacobs",
	"",
	"Shadow Finders:",
	"   Steve Rosenthal, Steve Elwell,",
	"   Chad Bordwell,",
	"   David 'Spice Girl' Baker,",
	"   Error Casillas, Damien Fischer,",
	"   Winnie Lee,"
	"   Ygor Krynytyskyy,",
	"   Samantha (Crusher) Lee, John Park",
	"   Ian Stevens, Chris Toft",
	"",
	"Production Testers:",
	"   Steve 'Damn It's Cold!'",
	"       Rosenthal and",
	"   Chad 'What Hotel Receipt?'",
	"        Bordwell",
	"",
	"Additional QA and Support:",
	"    Tony Villalobos",
	"    Jason Sullivan",
	"",
	"Installer by:",
	"   Steve 'Bahh' Stringer,",
	"   Adam Goldberg, Tanya Martino,",
	"   Eric Schmidt, Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey 'Damien' Chico and",
	"   Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Our Big Toe:",
	"   Mitch Lasky",
	"",
	"",
	"Special Thanks to:",
	"  Id software",
	"  Anyone who ever worked for Raven,",
	"  (except for Alex)",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special Thanks To:",
	"   E.H.S., The Osmonds,",
	"   B.B.V.D., Daisy The Lovin' Lamb,",
	"  'You Killed' Kenny,",
	"   and Baby Biessman.",
	"",
};

#define MAX_LINES_HW 145 + 25
static const char* CreditTextHW[ MAX_LINES_HW ] =
{
	"HexenWorld",
	"",
	"Lead Programmer: Rick Johnson",
	"",
	"Programming:",
	"   Nathan Albury",
	"   Ron Midthun",
	"   Steve Sengele",
	"   Mike Gummelt",
	"   James Monroe",
	"",
	"Deathmatch Levels:",
	"   Kenn Hoekstra",
	"   Mike Renner",
	"   Jeremy Statz",
	"   Jon Zuk",
	"",
	"Special thanks to:",
	"   Dave Kirsch",
	"   William Mull",
	"   Jack Mathews",
	"",
	"",
	"Hexen2",
	"",
	"Project Director: Brian Raffel",
	"",
	"Lead Programmer: Rick Johnson",
	"",
	"Programming:",
	"   Ben Gokey",
	"   Bob Love",
	"   Mike Gummelt",
	"",
	"Additional Programming:",
	"   Josh Weier",
	"",
	"Lead Design: Eric Biessman",
	"",
	"Design:",
	"   Brian Raffel",
	"   Brian Frank",
	"   Tom Odell",
	"",
	"Art Director: Brian Pelletier",
	"",
	"Art:",
	"   Shane Gurno",
	"   Jim Sumwalt",
	"   Mark Morgan",
	"   Kim Lathrop",
	"   Ted Halsted",
	"   Rebecca Rettenmund",
	"   Les Dorscheid",
	"",
	"Animation:",
	"   Chaos (Mike Werckle)",
	"   Brian Shubat",
	"",
	"Cinematics:",
	"   Jeff Dewitt",
	"   Jeffrey P. Lampo",
	"",
	"Music:",
	"   Kevin Schilder",
	"",
	"Sound:",
	"   Kevin Schilder",
	"   Chia Chin Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve Stringer",
	"",
	"Localization Producer:",
	"   Sandi Isaacs",
	"",
	"Marketing Product Manager:",
	"   Henk Hartong",
	"",
	"European Marketing",
	"Product Director:",
	"   Janine Johnson",
	"",
	"Marketing Associate:",
	"   Kevin Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   John Tam",
	"",
	"Quality Assurance Team:",
	"   Steve Rosenthal, Mike Spann,",
	"   Steve Elwell, Kelly Wand,",
	"   Kip Stolberg, Igor Krinitskiy,",
	"   Ian Stevens, Marilena Wahmann,",
	"   David Baker, Winnie Lee",
	"",
	"Documentation:",
	"   Mike Rivera, Sylvia Orzel,",
	"   Belinda Vansickle",
	"",
	"Chronicle of Deeds written by:",
	"   Joe Grant Bell",
	"",
	"Localization:",
	"   Nathalie Dove, Lucy Morgan,",
	"   Alex Wylde, Nicky Kerth",
	"",
	"Installer by:",
	"   Steve Stringer, Adam Goldberg,",
	"   Tanya Martino, Eric Schmidt,",
	"   Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey Chico and Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Deal Guru:",
	"   Mitch Lasky",
	"",
	"",
	"Thanks to Id software:",
	"   John Carmack",
	"   Adrian Carmack",
	"   Kevin Cloud",
	"   Barrett 'Bear'  Alexander",
	"   American McGee",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special thanks to Gary McTaggart",
	"at 3dfx for his help with",
	"the gl version!",
	"",
	"No snails were harmed in the",
	"making of this game!"
};

#define MAX_LINES2_HW 158 + 27
static const char* Credit2TextHW[ MAX_LINES2_HW ] =
{
	"HexenWorld",
	"",
	"Superior Groucher:"
	"   Rick 'Grrr' Johnson",
	"",
	"Bug Creators:",
	"   Nathan 'Glory Code' Albury",
	"   Ron 'Stealth' Midthun",
	"   Steve 'Tie Dye' Sengele",
	"   Mike 'Foos' Gummelt",
	"   James 'Happy' Monroe",
	"",
	"Sloppy Joe Makers:",
	"   Kenn 'I'm a broken man'",
	"      Hoekstra",
	"   Mike 'Outback' Renner",
	"   Jeremy 'Under-rated' Statz",
	"   Jon Zuk",
	"",
	"Avoid the Noid:",
	"   Dave 'Zoid' Kirsch",
	"   William 'Phoeb' Mull",
	"   Jack 'Morbid' Mathews",
	"",
	"",
	"Hexen2",
	"",
	"Map Master: ",
	"   'Caffeine Buzz' Raffel",
	"",
	"Code Warrior:",
	"   Rick 'Superfly' Johnson",
	"",
	"Grunt Boys:",
	"   'Judah' Ben Gokey",
	"   Bob 'Whipped' Love",
	"   Mike '9-Pointer' Gummelt",
	"",
	"Additional Grunting:",
	"   Josh 'Intern' Weier",
	"",
	"Whippin' Boy:",
	"   Eric 'Baby' Biessman",
	"",
	"Crazy Levelers:",
	"   'Big Daddy' Brian Raffel",
	"   Brian 'Red' Frank",
	"   Tom 'Texture Alignment' Odell",
	"",
	"Art Lord:",
	"   Brian 'Mr. Calm' Pelletier",
	"",
	"Pixel Monkies:",
	"   Shane 'Duh' Gurno",
	"   'Slim' Jim Sumwalt",
	"   Mark 'Dad Gummit' Morgan",
	"   Kim 'Toy Master' Lathrop",
	"   'Drop Dead' Ted Halsted",
	"   Rebecca 'Zombie' Rettenmund",
	"   Les 'is not more' Dorscheid",
	"",
	"Salad Shooters:",
	"   Mike 'Chaos' Werckle",
	"   Brian 'Mutton Chops' Shubat",
	"",
	"Last Minute Addition:",
	"   Jeff 'Bud Bundy' Dewitt",
	"   Jeffrey 'Misspalld' Lampo",
	"",
	"Random Notes:",
	"   Kevin 'I Already Paid' Schilder",
	"",
	"Grunts, Groans, and Moans:",
	"   Kevin 'I Already Paid' Schilder",
	"   Chia 'Pet' Chin Lee",
	"",
	"",
	"Activision",
	"",
	"Producer:",
	"   Steve 'Ferris' Stringer",
	"",
	"Localization Producer:",
	"   Sandi 'Boneduster' Isaacs",
	"",
	"Marketing Product Manager:",
	"   Henk 'A-10' Hartong",
	"",
	"European Marketing",
	"Product Director:",
	"   Janine Johnson",
	"",
	"Marketing Associate:",
	"   Kevin 'Savage' Kraff",
	"",
	"Senior Quality",
	"Assurance Lead:",
	"   Tim 'Outlaw' Vanlaw",
	"",
	"Quality Assurance Lead:",
	"   John 'Armadillo' Tam",
	"",
	"Quality Assurance Team:",
	"   Steve 'Rhinochoadicus'",
	"      Rosenthal,",
	"   Mike 'Dragonhawk' Spann,",
	"   Steve 'Zendog' Elwell,",
	"   Kelly 'Li'l Bastard' Wand,",
	"   Kip 'Angus' Stolberg,",
	"   Igor 'Russo' Krinitskiy,",
	"   Ian 'Cracker' Stevens,",
	"   Marilena 'Raveness-X' Wahmann,",
	"   David 'Spicegirl' Baker,",
	"   Winnie 'Mew' Lee",
	"",
	"Documentation:",
	"   Mike Rivera, Sylvia Orzel,",
	"   Belinda Vansickle",
	"",
	"Chronicle of Deeds written by:",
	"   Joe Grant Bell",
	"",
	"Localization:",
	"   Nathalie Dove, Lucy Morgan,",
	"   Alex Wylde, Nicky Kerth",
	"",
	"Installer by:",
	"   Steve 'Bahh' Stringer,",
	"   Adam Goldberg, Tanya Martino,",
	"   Eric Schmidt, Ronnie Lane",
	"",
	"Art Assistance by:",
	"   Carey 'Damien' Chico and",
	"   Franz Boehm",
	"",
	"BizDev Babe:",
	"   Jamie Bafus",
	"",
	"And...",
	"",
	"Deal Guru:",
	"   Mitch 'I'll buy that' Lasky",
	"",
	"",
	"Thanks to Id software:",
	"   John Carmack",
	"   Adrian Carmack",
	"   Kevin Cloud",
	"   Barrett 'Bear'  Alexander",
	"   American McGee",
	"",
	"",
	"Published by Id Software, Inc.",
	"Distributed by Activision, Inc.",
	"",
	"The Id Software Technology used",
	"under license in Hexen II (tm)",
	"(c) 1996, 1997 Id Software, Inc.",
	"All Rights Reserved.",
	"",
	"Hexen(r) is a registered trademark",
	"of Raven Software Corp.",
	"Hexen II (tm) and the Raven logo",
	"are trademarks of Raven Software",
	"Corp.  The Id Software name and",
	"id logo are trademarks of",
	"Id Software, Inc.  Activision(r)",
	"is a registered trademark of",
	"Activision, Inc. All other",
	"trademarks are the property of",
	"their respective owners.",
	"",
	"",
	"",
	"Send bug descriptions to:",
	"   h2bugs@mail.ravensoft.com",
	"",
	"Special thanks to Bob for",
	"remembering 'P' is for Polymorph",
	"",
	"",
	"See the next movie in the long",
	"awaited sequel, starring",
	"Bobby Love in,",
	"   Out of Traction, Back in Action!",
};

void MQH_Menu_Quit_f() {
	if ( m_state == m_quit ) {
		return;
	}
	wasInMenus = !!( in_keyCatchers & KEYCATCH_UI );
	in_keyCatchers |= KEYCATCH_UI;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	mqh_entersound = true;
	if ( GGameType & GAME_Hexen2 ) {
		LinePos = 0;
		LineTimes = 0;
		if ( GGameType & GAME_HexenWorld ) {
			LineText = CreditTextHW;
			MaxLines = MAX_LINES_HW;
		} else   {
			LineText = CreditTextH2;
			MaxLines = MAX_LINES_H2;
		}
		SoundPlayed = false;
	}
}

static void MQH_Quit_Draw() {
	if ( GGameType & GAME_Hexen2 ) {
		LinePos += cls.frametime * 0.001 * 1.75;
		if ( LinePos > MaxLines + QUIT_SIZE_H2 + 2 ) {
			LinePos = 0;
			SoundPlayed = false;
			LineTimes++;
			if ( LineTimes >= 2 ) {
				if ( GGameType & GAME_HexenWorld ) {
					MaxLines = MAX_LINES2_HW;
					LineText = Credit2TextHW;
				} else   {
					MaxLines = MAX_LINES2_H2;
					LineText = Credit2TextH2;
					CDAudio_Play( 12, false );
				}
			}
		}

		int y = 12;
		MQH_DrawTextBox( 0, 0, 38, 23 );
		if ( GGameType & GAME_HexenWorld ) {
			MQH_PrintWhite( 16, y,  "      Hexen2World version " VSTR2( VERSION ) "      " );    y += 8;
		} else   {
			MQH_PrintWhite( 16, y,  "        Hexen II version 1.12       " );  y += 8;
		}
		MQH_PrintWhite( 16, y,  "         by Raven Software          " );  y += 16;

		if ( LinePos > 55 && !SoundPlayed && LineText == ( GGameType & GAME_HexenWorld ? Credit2TextHW : Credit2TextH2 ) ) {
			S_StartLocalSound( "rj/steve.wav" );
			SoundPlayed = true;
		}
		int topy = y;
		int place = floor( LinePos );
		y -= floor( ( LinePos - place ) * 8 );
		for ( int i = 0; i < QUIT_SIZE_H2; i++,y += 8 ) {
			if ( i + place - QUIT_SIZE_H2 >= MaxLines ) {
				break;
			}
			if ( i + place < QUIT_SIZE_H2 ) {
				continue;
			}

			if ( LineText[ i + place - QUIT_SIZE_H2 ][ 0 ] == ' ' ) {
				MQH_PrintWhite( 24,y,LineText[ i + place - QUIT_SIZE_H2 ] );
			} else   {
				MQH_Print( 24,y,LineText[ i + place - QUIT_SIZE_H2 ] );
			}
		}

		image_t* p = R_CachePic( "gfx/box_mm2.lmp" );
		int x = 24;
		y = topy - 8;
		for ( int i = 4; i < ( GGameType & GAME_HexenWorld ? 36 : 38 ); i++,x += 8 ) {
			MQH_DrawPic( x, y, p );		//background at top for smooth scroll out
			MQH_DrawPic( x, y + ( QUIT_SIZE_H2 * 8 ), p );	//draw at bottom for smooth scroll in
		}

		y += ( QUIT_SIZE_H2 * 8 ) + 8;
		MQH_PrintWhite( 16, y,  "          Press y to exit           " );
	} else if ( GGameType & GAME_QuakeWorld )     {
		const char* cmsg[] =
		{
			//    0123456789012345678901234567890123456789
			"0            QuakeWorld",
			"1    version " VSTR2( VERSION ) " by id Software",
			"0Programming",
			"1 John Carmack    Michael Abrash",
			"1 John Cash       Christian Antkow",
			"0Additional Programming",
			"1 Dave 'Zoid' Kirsch",
			"1 Jack 'morbid' Mathews",
			"0Id Software is not responsible for",
			"0providing technical support for",
			"0QUAKEWORLD(tm). (c)1996 Id Software,",
			"0Inc.  All Rights Reserved.",
			"0QUAKEWORLD(tm) is a trademark of Id",
			"0Software, Inc.",
			"1NOTICE: THE COPYRIGHT AND TRADEMARK",
			"1NOTICES APPEARING  IN YOUR COPY OF",
			"1QUAKE(r) ARE NOT MODIFIED BY THE USE",
			"1OF QUAKEWORLD(tm) AND REMAIN IN FULL",
			"1FORCE.",
			"0NIN(r) is a registered trademark",
			"0licensed to Nothing Interactive, Inc.",
			"0All rights reserved. Press y to exit",
			NULL
		};

		MQH_DrawTextBox( 0, 0, 38, 23 );
		int y = 12;
		for ( const char** p = cmsg; *p; p++, y += 8 ) {
			if ( **p == '0' ) {
				MQH_PrintWhite( 16, y, *p + 1 );
			} else   {
				MQH_Print( 16, y, *p + 1 );
			}
		}
	} else   {
		MQH_DrawTextBox( 0, 0, 38, 23 );
		MQH_PrintWhite( 16, 12,  "  Quake version 1.09 by id Software\n\n" );
		MQH_PrintWhite( 16, 28,  "Programming        Art \n" );
		MQH_Print( 16, 36,  " John Carmack       Adrian Carmack\n" );
		MQH_Print( 16, 44,  " Michael Abrash     Kevin Cloud\n" );
		MQH_Print( 16, 52,  " John Cash          Paul Steed\n" );
		MQH_Print( 16, 60,  " Dave 'Zoid' Kirsch\n" );
		MQH_PrintWhite( 16, 68,  "Design             Biz\n" );
		MQH_Print( 16, 76,  " John Romero        Jay Wilbur\n" );
		MQH_Print( 16, 84,  " Sandy Petersen     Mike Wilson\n" );
		MQH_Print( 16, 92,  " American McGee     Donna Jackson\n" );
		MQH_Print( 16, 100,  " Tim Willits        Todd Hollenshead\n" );
		MQH_PrintWhite( 16, 108, "Support            Projects\n" );
		MQH_Print( 16, 116, " Barrett Alexander  Shawn Green\n" );
		MQH_PrintWhite( 16, 124, "Sound Effects\n" );
		MQH_Print( 16, 132, " Trent Reznor and Nine Inch Nails\n\n" );
		MQH_PrintWhite( 16, 140, "Quake is a trademark of Id Software,\n" );
		MQH_PrintWhite( 16, 148, "inc., (c)1996 Id Software, inc. All\n" );
		MQH_PrintWhite( 16, 156, "rights reserved. NIN logo is a\n" );
		MQH_PrintWhite( 16, 164, "registered trademark licensed to\n" );
		MQH_PrintWhite( 16, 172, "Nothing Interactive, Inc. All rights\n" );
		MQH_PrintWhite( 16, 180, "reserved. Press y to exit\n" );
	}
}

static void MQH_Quit_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
	case 'n':
	case 'N':
		if ( wasInMenus ) {
			m_state = m_quit_prevstate;
			mqh_entersound = true;
		} else   {
			in_keyCatchers &= ~KEYCATCH_UI;
			m_state = m_none;
		}
		break;

	case 'Y':
	case 'y':
		in_keyCatchers |= KEYCATCH_CONSOLE;
		Com_Quit_f();
		break;

	default:
		break;
	}

}

//=============================================================================
/* Menu Subsystem */

void MQH_ToggleMenu_f() {
	mqh_entersound = true;

	if ( in_keyCatchers & KEYCATCH_UI ) {
		if ( m_state != m_main ) {
			m_state = m_none;
			MQH_Menu_Main_f();
			return;
		}
		in_keyCatchers &= ~KEYCATCH_UI;
		m_state = m_none;
		return;
	}
	if ( in_keyCatchers & KEYCATCH_CONSOLE && ( !( GGameType & GAME_HexenWorld ) || cls.state == CA_ACTIVE ) ) {
		Con_ToggleConsole_f();
	} else   {
		MQH_Menu_Main_f();
	}
}

void MQH_Init() {
	Cmd_AddCommand( "togglemenu", MQH_ToggleMenu_f );
	Cmd_AddCommand( "menu_main", MQH_Menu_Main_f );
	Cmd_AddCommand( "menu_options", MQH_Menu_Options_f );
	Cmd_AddCommand( "menu_keys", MQH_Menu_Keys_f );
	Cmd_AddCommand( "menu_video", MQH_Menu_Video_f );
	Cmd_AddCommand( "help", MQH_Menu_Help_f );
	Cmd_AddCommand( "menu_quit", MQH_Menu_Quit_f );
	if ( !( GGameType & ( GAME_QuakeWorld | GAME_HexenWorld ) ) ) {
		Cmd_AddCommand( "menu_singleplayer", MQH_Menu_SinglePlayer_f );
		Cmd_AddCommand( "menu_load", MQH_Menu_Load_f );
		Cmd_AddCommand( "menu_save", MQH_Menu_Save_f );
		Cmd_AddCommand( "menu_multiplayer", MQH_Menu_MultiPlayer_f );
		Cmd_AddCommand( "menu_setup", MQH_Menu_Setup_f );
	}
	if ( GGameType & GAME_Hexen2 ) {
		Cmd_AddCommand( "menu_class", MH2_Menu_Class2_f );

		MH2_ReadBigCharWidth();

		if ( !( GGameType & GAME_HexenWorld ) ) {
			mh2_oldmission = Cvar_Get( "m_oldmission", "0", CVAR_ARCHIVE );
		}
	}
	if ( GGameType & GAME_HexenWorld ) {
		Cmd_AddCommand( "menu_connect", MHW_Menu_Connect_f );

		hostname1 = Cvar_Get( "host1","equalizer.ravensoft.com", CVAR_ARCHIVE );
		hostname2 = Cvar_Get( "host2","", CVAR_ARCHIVE );
		hostname3 = Cvar_Get( "host3","", CVAR_ARCHIVE );
		hostname4 = Cvar_Get( "host4","", CVAR_ARCHIVE );
		hostname5 = Cvar_Get( "host5","", CVAR_ARCHIVE );
		hostname6 = Cvar_Get( "host6","", CVAR_ARCHIVE );
		hostname7 = Cvar_Get( "host7","", CVAR_ARCHIVE );
		hostname8 = Cvar_Get( "host8","", CVAR_ARCHIVE );
		hostname9 = Cvar_Get( "host9","", CVAR_ARCHIVE );
		hostname10 = Cvar_Get( "host10","", CVAR_ARCHIVE );
	}
}

void MQH_InitImages() {
	mq1_translate_texture = NULL;
	Com_Memset( mh2_translate_texture, 0, sizeof ( mh2_translate_texture ) );
	if ( GGameType & GAME_Hexen2 ) {
		char_menufonttexture = R_LoadBigFontImage( "gfx/menu/bigfont2.lmp" );
	}
}

void MQH_FadeScreen() {
	if ( GGameType & GAME_Hexen2 ) {
		UI_Fill( 0, 0, viddef.width, viddef.height, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.2 );
		for ( int c = 0; c < 40; c++ ) {
			int x = rand() % viddef.width - 20;
			int y = rand() % viddef.height - 20;
			int w = ( rand() % 40 ) + 20;
			int h = ( rand() % 40 ) + 20;
			UI_Fill( x, y, w, h, 208.0 / 255.0, 180.0 / 255.0, 80.0 / 255.0, 0.035 );
		}
	} else   {
		UI_Fill( 0, 0, viddef.width, viddef.height, 0, 0, 0, 0.8 );
	}
}

void MQH_Draw() {
	if ( m_state == m_none || !( in_keyCatchers & KEYCATCH_UI ) ) {
		return;
	}

	if ( con.displayFrac ) {
		Con_DrawFullBackground();
		S_ExtraUpdate();
	} else   {
		MQH_FadeScreen();
	}

	switch ( m_state ) {
	case m_none:
		break;

	case m_main:
		MQH_Main_Draw();
		break;

	case m_singleplayer:
		MQH_SinglePlayer_Draw();
		break;

	case m_singleplayer_confirm:
		MQH_SinglePlayerConfirm_Draw();
		break;

	case m_class:
		MH2_Class_Draw();
		break;

	case m_difficulty:
		MH2_Difficulty_Draw();
		break;

	case m_load:
	case m_mload:
		MQH_Load_Draw();
		break;

	case m_save:
	case m_msave:
		MQH_Save_Draw();
		break;

	case m_multiplayer:
		MQH_MultiPlayer_Draw();
		break;

	case m_lanconfig:
		MQH_LanConfig_Draw();
		break;

	case m_search:
		MQH_Search_Draw();
		break;

	case m_slist:
		MQH_ServerList_Draw();
		break;

	case m_mconnect:
		MHW_Connect_Draw();
		break;

	case m_gameoptions:
		MQH_GameOptions_Draw();
		break;

	case m_setup:
		MQH_Setup_Draw();
		break;

	case m_options:
		MQH_Options_Draw();
		break;

	case m_keys:
		MQH_Keys_Draw();
		break;

	case m_video:
		MQH_Video_Draw();
		break;

	case m_help:
		MQH_Help_Draw();
		break;

	case m_quit:
		MQH_Quit_Draw();
		break;
	}

	if ( mqh_entersound ) {
		S_StartLocalSound( GGameType & GAME_Hexen2 ? "raven/menu2.wav" : "misc/menu2.wav" );
		mqh_entersound = false;
	}

	S_ExtraUpdate();
}

void MQH_Keydown( int key ) {
	switch ( m_state ) {
	case m_none:
		return;

	case m_main:
		MQH_Main_Key( key );
		return;

	case m_singleplayer:
		MQH_SinglePlayer_Key( key );
		return;

	case m_singleplayer_confirm:
		MQH_SinglePlayerConfirm_Key( key );
		return;

	case m_class:
		MH2_Class_Key( key );
		return;

	case m_difficulty:
		MH2_Difficulty_Key( key );
		return;

	case m_load:
		MQH_Load_Key( key );
		return;

	case m_save:
		MQH_Save_Key( key );
		return;

	case m_multiplayer:
		MQH_MultiPlayer_Key( key );
		return;

	case m_lanconfig:
		MQH_LanConfig_Key( key );
		return;

	case m_search:
		MQH_Search_Key( key );
		return;

	case m_slist:
		MQH_ServerList_Key( key );
		return;

	case m_mconnect:
		MHW_Connect_Key( key );
		break;

	case m_gameoptions:
		MQH_GameOptions_Key( key );
		return;

	case m_setup:
		MQH_Setup_Key( key );
		return;

	case m_mload:
		MH2_MLoad_Key( key );
		return;

	case m_msave:
		MH2_MSave_Key( key );
		return;

	case m_options:
		MQH_Options_Key( key );
		return;

	case m_keys:
		MQH_Keys_Key( key );
		return;

	case m_video:
		MQH_Video_Key( key );
		return;

	case m_help:
		MQH_Help_Key( key );
		return;

	case m_quit:
		MQH_Quit_Key( key );
		return;
	}
}

void MQH_CharEvent( int key ) {
	switch ( m_state ) {
	case m_lanconfig:
		MQH_LanConfig_Char( key );
		break;

	case m_mconnect:
		MHW_Connect_Char( key );
		break;

	case m_setup:
		MQH_Setup_Char( key );
		break;

	case m_gameoptions:
		MQH_GameOptions_Char( key );
		break;

	default:
		break;
	}
}
