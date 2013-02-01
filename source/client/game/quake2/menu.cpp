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
#include "qmenu.h"
#include "local.h"
#include "../../client_main.h"
#include "../../input/keycodes.h"
#include "../../renderer/cvars.h"
#include "../../ui/ui.h"
#include "../../ui/console.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/system.h"

#define NUM_CURSOR_FRAMES 15

static const char* menu_in_sound = "misc/menu1.wav";
static const char* menu_move_sound = "misc/menu2.wav";
static const char* menu_out_sound = "misc/menu3.wav";

static bool mq2_entersound;			// play after drawing a frame, so caching
// won't disrupt the sound

static void ( * mq2_drawfunc )();
static const char*( *mq2_keyfunc )( int key );
static void ( * mq2_charfunc )( int key );

static void MQ2_Menu_Game_f();
static void MQ2_Menu_LoadGame_f();
static void MQ2_Menu_SaveGame_f();
static void MQ2_Menu_Credits_f();
static void MQ2_Menu_Multiplayer_f();
static void MQ2_Menu_JoinServer_f();
static void MQ2_Menu_AddressBook_f();
static void MQ2_Menu_StartServer_f();
static void MQ2_Menu_DMOptions_f();
static void MQ2_Menu_PlayerConfig_f();
static void MQ2_Menu_DownloadOptions_f();
static void MQ2_Menu_Options_f();
static void MQ2_Menu_Keys_f();
static void MQ2_Menu_Video_f();
static void MQ2_Menu_Quit_f();

//=============================================================================
/* Support Routines */

#define MAX_MENU_DEPTH  8

struct menulayer_t {
	void ( * draw )( void );
	const char*( *key )( int k );
	void ( * charfunc )( int key );
};

static menulayer_t mq2_layers[ MAX_MENU_DEPTH ];
static int mq2_menudepth;

static void MQ2_Banner( const char* name ) {
	int w, h;
	R_GetPicSize( &w, &h, name );
	UI_DrawNamedPic( viddef.width / 2 - w / 2, viddef.height / 2 - 110, name );
}

static void MQ2_PushMenu( void ( * draw )( void ), const char*( *key )( int k ), void ( * charfunc )( int key ) ) {
	if ( Cvar_VariableValue( "maxclients" ) == 1 && ComQ2_ServerState() ) {
		Cvar_SetLatched( "paused", "1" );
	}

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	int i;
	for ( i = 0; i < mq2_menudepth; i++ ) {
		if ( mq2_layers[ i ].draw == draw &&
			 mq2_layers[ i ].key == key ) {
			mq2_menudepth = i;
		}
	}

	if ( i == mq2_menudepth ) {
		if ( mq2_menudepth >= MAX_MENU_DEPTH ) {
			common->FatalError( "MQ2_PushMenu: MAX_MENU_DEPTH" );
		}
		mq2_layers[ mq2_menudepth ].draw = mq2_drawfunc;
		mq2_layers[ mq2_menudepth ].key = mq2_keyfunc;
		mq2_layers[ mq2_menudepth ].charfunc = mq2_charfunc;
		mq2_menudepth++;
	}

	mq2_drawfunc = draw;
	mq2_keyfunc = key;
	mq2_charfunc = charfunc;

	mq2_entersound = true;

	in_keyCatchers |= KEYCATCH_UI;
}

void MQ2_ForceMenuOff() {
	mq2_drawfunc = 0;
	mq2_keyfunc = 0;
	mq2_charfunc = NULL;
	in_keyCatchers &= ~KEYCATCH_UI;
	mq2_menudepth = 0;
	Key_ClearStates();
	Cvar_SetLatched( "paused", "0" );
}

static void MQ2_PopMenu() {
	S_StartLocalSound( menu_out_sound );
	if ( mq2_menudepth < 1 ) {
		common->FatalError( "MQ2_PopMenu: depth < 1" );
	}
	mq2_menudepth--;

	mq2_drawfunc = mq2_layers[ mq2_menudepth ].draw;
	mq2_keyfunc = mq2_layers[ mq2_menudepth ].key;
	mq2_charfunc = mq2_layers[ mq2_menudepth ].charfunc;

	if ( !mq2_menudepth ) {
		MQ2_ForceMenuOff();
	}
}

static const char* Default_MenuKey( menuframework_s* m, int key ) {
	if ( m ) {
		menucommon_s* item = ( menucommon_s* )Menu_ItemAtCursor( m );
		if ( item != 0 ) {
			if ( item->type == MTYPE_FIELD ) {
				if ( MQ2_Field_Key( ( menufield_s* )item, key ) ) {
					return NULL;
				}
			}
		}
	}

	const char* sound = NULL;
	switch ( key ) {
	case K_ESCAPE:
		MQ2_PopMenu();
		return menu_out_sound;
	case K_KP_UPARROW:
	case K_UPARROW:
		if ( m ) {
			m->cursor--;
			Menu_AdjustCursor( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if ( m ) {
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if ( m ) {
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		if ( m ) {
			Menu_SlideItem( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		if ( m ) {
			Menu_SlideItem( m, 1 );
			sound = menu_move_sound;
		}
		break;

	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3:
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:

	case K_KP_ENTER:
	case K_ENTER:
		if ( m ) {
			Menu_SelectItem( m );
		}
		sound = menu_move_sound;
		break;
	}

	return sound;
}

static void Default_MenuChar( menuframework_s* m, int key ) {
	if ( m ) {
		menucommon_s* item = ( menucommon_s* )Menu_ItemAtCursor( m );
		if ( item ) {
			if ( item->type == MTYPE_FIELD ) {
				MQ2_Field_Char( ( menufield_s* )item, key );
			}
		}
	}
}

//	Draws one solid graphics character
// cx and cy are in 320*240 coordinates, and will be centered on
// higher res screens.
static void MQ2_DrawCharacter( int cx, int cy, int num ) {
	UI_DrawChar( cx + ( ( viddef.width - 320 ) >> 1 ), cy + ( ( viddef.height - 240 ) >> 1 ), num );
}

static void MQ2_Print( int cx, int cy, const char* str ) {
	UI_DrawString( cx + ( ( viddef.width - 320 ) >> 1 ), cy + ( ( viddef.height - 240 ) >> 1 ), str, 128 );
}

//	Draws an animating cursor with the point at
// x,y.  The pic will extend to the left of x,
// and both above and below y.
static void MQ2_DrawCursor( int x, int y, int f ) {
	static bool cached;
	if ( !cached ) {
		for ( int i = 0; i < NUM_CURSOR_FRAMES; i++ ) {
			char cursorname[ 80 ];
			String::Sprintf( cursorname, sizeof ( cursorname ), "m_cursor%d", i );

			R_RegisterPic( cursorname );
		}
		cached = true;
	}

	char cursorname[ 80 ];
	String::Sprintf( cursorname, sizeof ( cursorname ), "m_cursor%d", f );
	UI_DrawNamedPic( x, y, cursorname );
}

static void MQ2_DrawTextBox( int x, int y, int width, int lines ) {
	// draw left side
	int cx = x;
	int cy = y;
	MQ2_DrawCharacter( cx, cy, 1 );
	for ( int n = 0; n < lines; n++ ) {
		cy += 8;
		MQ2_DrawCharacter( cx, cy, 4 );
	}
	MQ2_DrawCharacter( cx, cy + 8, 7 );

	// draw middle
	cx += 8;
	while ( width > 0 ) {
		cy = y;
		MQ2_DrawCharacter( cx, cy, 2 );
		for ( int n = 0; n < lines; n++ ) {
			cy += 8;
			MQ2_DrawCharacter( cx, cy, 5 );
		}
		MQ2_DrawCharacter( cx, cy + 8, 8 );
		width -= 1;
		cx += 8;
	}

	// draw right side
	cy = y;
	MQ2_DrawCharacter( cx, cy, 3 );
	for ( int n = 0; n < lines; n++ ) {
		cy += 8;
		MQ2_DrawCharacter( cx, cy, 6 );
	}
	MQ2_DrawCharacter( cx, cy + 8, 9 );
}

static float ClampCvar( float min, float max, float value ) {
	if ( value < min ) {
		return min;
	}
	if ( value > max ) {
		return max;
	}
	return value;
}

/*
=======================================================================

MAIN MENU

=======================================================================
*/

#define MAIN_ITEMS  5

static int m_main_cursor;

static void M_Main_Draw() {
	int i;
	int w, h;
	int ystart;
	int xoffset;
	int widest = -1;
	int totalheight = 0;
	char litname[ 80 ];
	const char* names[] =
	{
		"m_main_game",
		"m_main_multiplayer",
		"m_main_options",
		"m_main_video",
		"m_main_quit",
		0
	};

	for ( i = 0; names[ i ] != 0; i++ ) {
		R_GetPicSize( &w, &h, names[ i ] );

		if ( w > widest ) {
			widest = w;
		}
		totalheight += ( h + 12 );
	}

	ystart = ( viddef.height / 2 - 110 );
	xoffset = ( viddef.width - widest + 70 ) / 2;

	for ( i = 0; names[ i ] != 0; i++ ) {
		if ( i != m_main_cursor ) {
			UI_DrawNamedPic( xoffset, ystart + i * 40 + 13, names[ i ] );
		}
	}
	String::Cpy( litname, names[ m_main_cursor ] );
	String::Cat( litname, sizeof ( litname ), "_sel" );
	UI_DrawNamedPic( xoffset, ystart + m_main_cursor * 40 + 13, litname );

	MQ2_DrawCursor( xoffset - 25, ystart + m_main_cursor * 40 + 11, ( int )( cls.realtime / 100 ) % NUM_CURSOR_FRAMES );

	R_GetPicSize( &w, &h, "m_main_plaque" );
	UI_DrawNamedPic( xoffset - 30 - w, ystart, "m_main_plaque" );

	UI_DrawNamedPic( xoffset - 30 - w, ystart + h + 5, "m_main_logo" );
}

static const char* M_Main_Key( int key ) {
	const char* sound = menu_move_sound;

	switch ( key ) {
	case K_ESCAPE:
		MQ2_PopMenu();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if ( ++m_main_cursor >= MAIN_ITEMS ) {
			m_main_cursor = 0;
		}
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if ( --m_main_cursor < 0 ) {
			m_main_cursor = MAIN_ITEMS - 1;
		}
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		mq2_entersound = true;

		switch ( m_main_cursor ) {
		case 0:
			MQ2_Menu_Game_f();
			break;

		case 1:
			MQ2_Menu_Multiplayer_f();
			break;

		case 2:
			MQ2_Menu_Options_f();
			break;

		case 3:
			MQ2_Menu_Video_f();
			break;

		case 4:
			MQ2_Menu_Quit_f();
			break;
		}
	}

	return NULL;
}

void MQ2_Menu_Main_f() {
	MQ2_PushMenu( M_Main_Draw, M_Main_Key, NULL );
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static int m_game_cursor;

static menuframework_s s_game_menu;
static menuaction_s s_easy_game_action;
static menuaction_s s_medium_game_action;
static menuaction_s s_hard_game_action;
static menuaction_s s_load_game_action;
static menuaction_s s_save_game_action;
static menuaction_s s_credits_action;
static menuseparator_s s_blankline;

static void StartGame() {
	// disable updates and start the cinematic going
	cl.servercount = -1;
	MQ2_ForceMenuOff();
	Cvar_SetValueLatched( "deathmatch", 0 );
	Cvar_SetValueLatched( "coop", 0 );

	Cvar_SetValueLatched( "gamerules", 0 );			//PGM

	Cbuf_AddText( "loading ; killserver ; wait ; newgame\n" );
	in_keyCatchers &= ~KEYCATCH_UI;
}

static void EasyGameFunc( void* data ) {
	Cvar_Set( "skill", "0" );
	StartGame();
}

static void MediumGameFunc( void* data ) {
	Cvar_Set( "skill", "1" );
	StartGame();
}

static void HardGameFunc( void* data ) {
	Cvar_Set( "skill", "2" );
	StartGame();
}

static void LoadGameFunc( void* unused ) {
	MQ2_Menu_LoadGame_f();
}

static void SaveGameFunc( void* unused ) {
	MQ2_Menu_SaveGame_f();
}

static void CreditsFunc( void* unused ) {
	MQ2_Menu_Credits_f();
}

static void Game_MenuInit() {
	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	s_easy_game_action.generic.type = MTYPE_ACTION;
	s_easy_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_easy_game_action.generic.x        = 0;
	s_easy_game_action.generic.y        = 0;
	s_easy_game_action.generic.name = "easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type   = MTYPE_ACTION;
	s_medium_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_medium_game_action.generic.x      = 0;
	s_medium_game_action.generic.y      = 10;
	s_medium_game_action.generic.name   = "medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type = MTYPE_ACTION;
	s_hard_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_hard_game_action.generic.x        = 0;
	s_hard_game_action.generic.y        = 20;
	s_hard_game_action.generic.name = "hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_load_game_action.generic.type = MTYPE_ACTION;
	s_load_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_load_game_action.generic.x        = 0;
	s_load_game_action.generic.y        = 40;
	s_load_game_action.generic.name = "load game";
	s_load_game_action.generic.callback = LoadGameFunc;

	s_save_game_action.generic.type = MTYPE_ACTION;
	s_save_game_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_save_game_action.generic.x        = 0;
	s_save_game_action.generic.y        = 50;
	s_save_game_action.generic.name = "save game";
	s_save_game_action.generic.callback = SaveGameFunc;

	s_credits_action.generic.type   = MTYPE_ACTION;
	s_credits_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_credits_action.generic.x      = 0;
	s_credits_action.generic.y      = 60;
	s_credits_action.generic.name   = "credits";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem( &s_game_menu, ( void* )&s_easy_game_action );
	Menu_AddItem( &s_game_menu, ( void* )&s_medium_game_action );
	Menu_AddItem( &s_game_menu, ( void* )&s_hard_game_action );
	Menu_AddItem( &s_game_menu, ( void* )&s_blankline );
	Menu_AddItem( &s_game_menu, ( void* )&s_load_game_action );
	Menu_AddItem( &s_game_menu, ( void* )&s_save_game_action );
	Menu_AddItem( &s_game_menu, ( void* )&s_blankline );
	Menu_AddItem( &s_game_menu, ( void* )&s_credits_action );

	Menu_Center( &s_game_menu );
}

static void Game_MenuDraw() {
	MQ2_Banner( "m_banner_game" );
	Menu_AdjustCursor( &s_game_menu, 1 );
	Menu_Draw( &s_game_menu );
}

static const char* Game_MenuKey( int key ) {
	return Default_MenuKey( &s_game_menu, key );
}

static void MQ2_Menu_Game_f() {
	Game_MenuInit();
	MQ2_PushMenu( Game_MenuDraw, Game_MenuKey, NULL );
	m_game_cursor = 1;
}

/*
=============================================================================

LOADGAME MENU

=============================================================================
*/

#define MAX_SAVEGAMES   15

static menuframework_s s_savegame_menu;

static menuframework_s s_loadgame_menu;
static menuaction_s s_loadgame_actions[ MAX_SAVEGAMES ];

static char mq2_savestrings[ MAX_SAVEGAMES ][ 32 ];
static bool mq2_savevalid[ MAX_SAVEGAMES ];

static void Create_Savestrings() {
	for ( int i = 0; i < MAX_SAVEGAMES; i++ ) {
		char name[ MAX_OSPATH ];
		String::Sprintf( name, sizeof ( name ), "save/save%i/server.ssv", i );
		if ( !FS_FileExists( name ) ) {
			String::Cpy( mq2_savestrings[ i ], "<EMPTY>" );
			mq2_savevalid[ i ] = false;
		} else   {
			fileHandle_t f;
			FS_FOpenFileRead( name, &f, true );
			FS_Read( mq2_savestrings[ i ], sizeof ( mq2_savestrings[ i ] ), f );
			FS_FCloseFile( f );
			mq2_savevalid[ i ] = true;
		}
	}
}

static void LoadGameCallback( void* self ) {
	menuaction_s* a = ( menuaction_s* )self;

	if ( mq2_savevalid[ a->generic.localdata[ 0 ] ] ) {
		Cbuf_AddText( va( "load save%i\n",  a->generic.localdata[ 0 ] ) );
	}
	MQ2_ForceMenuOff();
}

static void LoadGame_MenuInit() {
	s_loadgame_menu.x = viddef.width / 2 - 120;
	s_loadgame_menu.y = viddef.height / 2 - 58;
	s_loadgame_menu.nitems = 0;

	Create_Savestrings();

	for ( int i = 0; i < MAX_SAVEGAMES; i++ ) {
		s_loadgame_actions[ i ].generic.name          = mq2_savestrings[ i ];
		s_loadgame_actions[ i ].generic.flags         = QMF_LEFT_JUSTIFY;
		s_loadgame_actions[ i ].generic.localdata[ 0 ]  = i;
		s_loadgame_actions[ i ].generic.callback      = LoadGameCallback;

		s_loadgame_actions[ i ].generic.x = 0;
		s_loadgame_actions[ i ].generic.y = ( i ) * 10;
		if ( i > 0 ) {		// separate from autosave
			s_loadgame_actions[ i ].generic.y += 10;
		}

		s_loadgame_actions[ i ].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_loadgame_menu, &s_loadgame_actions[ i ] );
	}
}

static void LoadGame_MenuDraw() {
	MQ2_Banner( "m_banner_load_game" );
//	Menu_AdjustCursor( &s_loadgame_menu, 1 );
	Menu_Draw( &s_loadgame_menu );
}

static const char* LoadGame_MenuKey( int key ) {
	if ( key == K_ESCAPE || key == K_ENTER ) {
		s_savegame_menu.cursor = s_loadgame_menu.cursor - 1;
		if ( s_savegame_menu.cursor < 0 ) {
			s_savegame_menu.cursor = 0;
		}
	}
	return Default_MenuKey( &s_loadgame_menu, key );
}

static void MQ2_Menu_LoadGame_f() {
	LoadGame_MenuInit();
	MQ2_PushMenu( LoadGame_MenuDraw, LoadGame_MenuKey, NULL );
}


/*
=============================================================================

SAVEGAME MENU

=============================================================================
*/
static menuaction_s s_savegame_actions[ MAX_SAVEGAMES ];

static void SaveGameCallback( void* self ) {
	menuaction_s* a = ( menuaction_s* )self;

	Cbuf_AddText( va( "save save%i\n", a->generic.localdata[ 0 ] ) );
	MQ2_ForceMenuOff();
}

static void SaveGame_MenuDraw() {
	MQ2_Banner( "m_banner_save_game" );
	Menu_AdjustCursor( &s_savegame_menu, 1 );
	Menu_Draw( &s_savegame_menu );
}

static void SaveGame_MenuInit() {
	s_savegame_menu.x = viddef.width / 2 - 120;
	s_savegame_menu.y = viddef.height / 2 - 58;
	s_savegame_menu.nitems = 0;

	Create_Savestrings();

	// don't include the autosave slot
	for ( int i = 0; i < MAX_SAVEGAMES - 1; i++ ) {
		s_savegame_actions[ i ].generic.name = mq2_savestrings[ i + 1 ];
		s_savegame_actions[ i ].generic.localdata[ 0 ] = i + 1;
		s_savegame_actions[ i ].generic.flags = QMF_LEFT_JUSTIFY;
		s_savegame_actions[ i ].generic.callback = SaveGameCallback;

		s_savegame_actions[ i ].generic.x = 0;
		s_savegame_actions[ i ].generic.y = ( i ) * 10;

		s_savegame_actions[ i ].generic.type = MTYPE_ACTION;

		Menu_AddItem( &s_savegame_menu, &s_savegame_actions[ i ] );
	}
}

static const char* SaveGame_MenuKey( int key ) {
	if ( key == K_ENTER || key == K_ESCAPE ) {
		s_loadgame_menu.cursor = s_savegame_menu.cursor - 1;
		if ( s_loadgame_menu.cursor < 0 ) {
			s_loadgame_menu.cursor = 0;
		}
	}
	return Default_MenuKey( &s_savegame_menu, key );
}

static void MQ2_Menu_SaveGame_f() {
	if ( !ComQ2_ServerState() ) {
		return;		// not playing a game

	}
	SaveGame_MenuInit();
	MQ2_PushMenu( SaveGame_MenuDraw, SaveGame_MenuKey, NULL );
	Create_Savestrings();
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static const char** credits;
static char* creditsIndex[ 256 ];
static char* creditsBuffer;
static const char* idcredits[] =
{
	"+QUAKE II BY ID SOFTWARE",
	"",
	"+PROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	"+ART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	"+LEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	"+BIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	"+SPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	"+ADDITIONAL SUPPORT",
	"",
	"+LINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	"+CINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char* xatcredits[] =
{
	"+QUAKE II MISSION PACK: THE RECKONING",
	"+BY",
	"+XATRIX ENTERTAINMENT, INC.",
	"",
	"+DESIGN AND DIRECTION",
	"Drew Markham",
	"",
	"+PRODUCED BY",
	"Greg Goodrich",
	"",
	"+PROGRAMMING",
	"Rafael Paiz",
	"",
	"+LEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	"+LEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	"+ART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	"+COMPUTER GRAPHICS SUPERVISOR AND",
	"+CHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	"+SENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	"+CHARACTER ANIMATION AND",
	"+MOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	"+ART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	"+INTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	"+3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	"+ADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	"+ADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	"+PRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	"+SOUND DESIGN",
	"Gary Bradfield",
	"",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	"+AND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	"+THANKS TO INTERGRAPH COMPUTER SYTEMS",
	"+IN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char* roguecredits[] =
{
	"+QUAKE II MISSION PACK 2: GROUND ZERO",
	"+BY",
	"+ROGUE ENTERTAINMENT, INC.",
	"",
	"+PRODUCED BY",
	"Jim Molinets",
	"",
	"+PROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	"+LEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	"+ART DIRECTION",
	"Rich Fleider",
	"",
	"+ART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	"+ANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	"+SOUND",
	"James Grunke",
	"",
	"+GROUND ZERO THEME",
	"+AND",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"+VWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	"+AND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static void MQ2_Credits_MenuDraw() {
	//	draw the credits
	int y = viddef.height - ( ( cls.realtime - credits_start_time ) / 40.0F );
	for ( int i = 0; credits[ i ] && y < viddef.height; y += 10, i++ ) {
		if ( y <= -8 ) {
			continue;
		}

		int bold = false;
		int stringoffset = 0;
		if ( credits[ i ][ 0 ] == '+' ) {
			bold = true;
			stringoffset = 1;
		} else   {
			bold = false;
			stringoffset = 0;
		}

		int x = ( viddef.width - String::Length( &credits[ i ][ stringoffset ] ) * 8 ) / 2;
		if ( bold ) {
			UI_DrawString( x, y, &credits[ i ][ stringoffset ], 128 );
		} else   {
			UI_DrawString( x, y, &credits[ i ][ stringoffset ] );
		}
	}

	if ( y < 0 ) {
		credits_start_time = cls.realtime;
	}
}

static const char* MQ2_Credits_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
		if ( creditsBuffer ) {
			FS_FreeFile( creditsBuffer );
		}
		MQ2_PopMenu();
		break;
	}

	return menu_out_sound;

}

static void MQ2_Menu_Credits_f() {
	creditsBuffer = NULL;
	int count = FS_ReadFile( "credits", ( void** )&creditsBuffer );
	if ( count != -1 ) {
		char* p = creditsBuffer;
		int n;
		for ( n = 0; n < 255; n++ ) {
			creditsIndex[ n ] = p;
			while ( *p != '\r' && *p != '\n' ) {
				p++;
				if ( --count == 0 ) {
					break;
				}
			}
			if ( *p == '\r' ) {
				*p++ = 0;
				if ( --count == 0 ) {
					break;
				}
			}
			*p++ = 0;
			if ( --count == 0 ) {
				break;
			}
		}
		creditsIndex[ ++n ] = 0;
		credits = ( const char** )creditsIndex;
	} else   {
		int isdeveloper = FS_GetQuake2GameType();

		if ( isdeveloper == 1 ) {			// xatrix
			credits = xatcredits;
		} else if ( isdeveloper == 2 )     {// ROGUE
			credits = roguecredits;
		} else   {
			credits = idcredits;
		}

	}

	credits_start_time = cls.realtime;
	MQ2_PushMenu( MQ2_Credits_MenuDraw, MQ2_Credits_Key, NULL );
}

/*
=======================================================================

MULTIPLAYER MENU

=======================================================================
*/

static menuframework_s s_multiplayer_menu;
static menuaction_s s_join_network_server_action;
static menuaction_s s_start_network_server_action;
static menuaction_s s_player_setup_action;

static void Multiplayer_MenuDraw() {
	MQ2_Banner( "m_banner_multiplayer" );

	Menu_AdjustCursor( &s_multiplayer_menu, 1 );
	Menu_Draw( &s_multiplayer_menu );
}

static void PlayerSetupFunc( void* unused ) {
	MQ2_Menu_PlayerConfig_f();
}

static void JoinNetworkServerFunc( void* unused ) {
	MQ2_Menu_JoinServer_f();
}

static void StartNetworkServerFunc( void* unused ) {
	MQ2_Menu_StartServer_f();
}

static void Multiplayer_MenuInit() {
	s_multiplayer_menu.x = viddef.width * 0.50 - 64;
	s_multiplayer_menu.nitems = 0;

	s_join_network_server_action.generic.type   = MTYPE_ACTION;
	s_join_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_join_network_server_action.generic.x      = 0;
	s_join_network_server_action.generic.y      = 0;
	s_join_network_server_action.generic.name   = " join network server";
	s_join_network_server_action.generic.callback = JoinNetworkServerFunc;

	s_start_network_server_action.generic.type  = MTYPE_ACTION;
	s_start_network_server_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_start_network_server_action.generic.x     = 0;
	s_start_network_server_action.generic.y     = 10;
	s_start_network_server_action.generic.name  = " start network server";
	s_start_network_server_action.generic.callback = StartNetworkServerFunc;

	s_player_setup_action.generic.type  = MTYPE_ACTION;
	s_player_setup_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_player_setup_action.generic.x     = 0;
	s_player_setup_action.generic.y     = 20;
	s_player_setup_action.generic.name  = " player setup";
	s_player_setup_action.generic.callback = PlayerSetupFunc;

	Menu_AddItem( &s_multiplayer_menu, ( void* )&s_join_network_server_action );
	Menu_AddItem( &s_multiplayer_menu, ( void* )&s_start_network_server_action );
	Menu_AddItem( &s_multiplayer_menu, ( void* )&s_player_setup_action );

	Menu_SetStatusBar( &s_multiplayer_menu, NULL );

	Menu_Center( &s_multiplayer_menu );
}

static const char* Multiplayer_MenuKey( int key ) {
	return Default_MenuKey( &s_multiplayer_menu, key );
}

static void MQ2_Menu_Multiplayer_f() {
	Multiplayer_MenuInit();
	MQ2_PushMenu( Multiplayer_MenuDraw, Multiplayer_MenuKey, NULL );
}

/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 8

static menuframework_s s_joinserver_menu;
static menuseparator_s s_joinserver_server_title;
static menuaction_s s_joinserver_search_action;
static menuaction_s s_joinserver_address_book_action;
static menuaction_s s_joinserver_server_actions[ MAX_LOCAL_SERVERS ];

static int mq2_num_servers;
#define NO_SERVER_STRING    "<no server>"

// user readable information
static char local_server_names[ MAX_LOCAL_SERVERS ][ 80 ];

// network address
static netadr_t local_server_netadr[ MAX_LOCAL_SERVERS ];

void MQ2_AddToServerList( netadr_t adr, const char* info ) {
	if ( mq2_num_servers == MAX_LOCAL_SERVERS ) {
		return;
	}
	while ( *info == ' ' ) {
		info++;
	}

	// ignore if duplicated
	for ( int i = 0; i < mq2_num_servers; i++ ) {
		if ( !String::Cmp( info, local_server_names[ i ] ) ) {
			return;
		}
	}

	local_server_netadr[ mq2_num_servers ] = adr;
	String::NCpy( local_server_names[ mq2_num_servers ], info, sizeof ( local_server_names[ 0 ] ) - 1 );
	mq2_num_servers++;
}

static void JoinServerFunc( void* self ) {
	int index = ( menuaction_s* )self - s_joinserver_server_actions;

	if ( String::ICmp( local_server_names[ index ], NO_SERVER_STRING ) == 0 ) {
		return;
	}

	if ( index >= mq2_num_servers ) {
		return;
	}

	char buffer[ 128 ];
	String::Sprintf( buffer, sizeof ( buffer ), "connect %s\n", SOCK_AdrToString( local_server_netadr[ index ] ) );
	Cbuf_AddText( buffer );
	MQ2_ForceMenuOff();
}

static void AddressBookFunc( void* self ) {
	MQ2_Menu_AddressBook_f();
}

static void SearchLocalGames() {
	R_VerifyNoRenderCommands();
	mq2_num_servers = 0;
	for ( int i = 0; i < MAX_LOCAL_SERVERS; i++ ) {
		String::Cpy( local_server_names[ i ], NO_SERVER_STRING );
	}

	MQ2_DrawTextBox( 8, 120 - 48, 36, 3 );
	MQ2_Print( 16 + 16, 120 - 48 + 8,  "Searching for local servers, this" );
	MQ2_Print( 16 + 16, 120 - 48 + 16, "could take up to a minute, so" );
	MQ2_Print( 16 + 16, 120 - 48 + 24, "please be patient." );

	// the text box won't show up unless we do a buffer swap
	R_EndFrame( NULL, NULL );

	// send out info packets
	CLQ2_PingServers_f();
}

static void SearchLocalGamesFunc( void* self ) {
	SearchLocalGames();
}

static void JoinServer_MenuInit() {
	s_joinserver_menu.x = viddef.width * 0.50 - 120;
	s_joinserver_menu.nitems = 0;

	s_joinserver_address_book_action.generic.type   = MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name   = "address book";
	s_joinserver_address_book_action.generic.flags  = QMF_LEFT_JUSTIFY;
	s_joinserver_address_book_action.generic.x      = 0;
	s_joinserver_address_book_action.generic.y      = 0;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name = "refresh server list";
	s_joinserver_search_action.generic.flags    = QMF_LEFT_JUSTIFY;
	s_joinserver_search_action.generic.x    = 0;
	s_joinserver_search_action.generic.y    = 10;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "search for servers";

	s_joinserver_server_title.generic.type = MTYPE_SEPARATOR;
	s_joinserver_server_title.generic.name = "connect to...";
	s_joinserver_server_title.generic.x    = 80;
	s_joinserver_server_title.generic.y    = 30;

	for ( int i = 0; i < MAX_LOCAL_SERVERS; i++ ) {
		s_joinserver_server_actions[ i ].generic.type = MTYPE_ACTION;
		String::Cpy( local_server_names[ i ], NO_SERVER_STRING );
		s_joinserver_server_actions[ i ].generic.name = local_server_names[ i ];
		s_joinserver_server_actions[ i ].generic.flags    = QMF_LEFT_JUSTIFY;
		s_joinserver_server_actions[ i ].generic.x        = 0;
		s_joinserver_server_actions[ i ].generic.y        = 40 + i * 10;
		s_joinserver_server_actions[ i ].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[ i ].generic.statusbar = "press ENTER to connect";
	}

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_address_book_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_title );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_search_action );

	for ( int i = 0; i < 8; i++ ) {
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_actions[ i ] );
	}

	Menu_Center( &s_joinserver_menu );

	SearchLocalGames();
}

static void JoinServer_MenuDraw() {
	MQ2_Banner( "m_banner_join_server" );
	Menu_Draw( &s_joinserver_menu );
}

static const char* JoinServer_MenuKey( int key ) {
	return Default_MenuKey( &s_joinserver_menu, key );
}

static void MQ2_Menu_JoinServer_f() {
	JoinServer_MenuInit();
	MQ2_PushMenu( JoinServer_MenuDraw, JoinServer_MenuKey, NULL );
}

/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/

#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_s s_addressbook_menu;
static menufield_s s_addressbook_fields[ NUM_ADDRESSBOOK_ENTRIES ];

static void AddressBook_MenuInit() {
	s_addressbook_menu.x = viddef.width / 2 - 142;
	s_addressbook_menu.y = viddef.height / 2 - 58;
	s_addressbook_menu.nitems = 0;

	for ( int i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++ ) {
		char buffer[ 20 ];
		String::Sprintf( buffer, sizeof ( buffer ), "adr%d", i );

		Cvar* adr = Cvar_Get( buffer, "", CVAR_ARCHIVE );

		s_addressbook_fields[ i ].generic.type = MTYPE_FIELD;
		s_addressbook_fields[ i ].generic.name = 0;
		s_addressbook_fields[ i ].generic.callback = 0;
		s_addressbook_fields[ i ].generic.x       = 0;
		s_addressbook_fields[ i ].generic.y       = i * 18 + 0;
		s_addressbook_fields[ i ].generic.localdata[ 0 ] = i;
		s_addressbook_fields[ i ].field.cursor          = 0;
		s_addressbook_fields[ i ].field.maxLength = 60;
		s_addressbook_fields[ i ].field.widthInChars = 30;

		String::Cpy( s_addressbook_fields[ i ].field.buffer, adr->string );

		Menu_AddItem( &s_addressbook_menu, &s_addressbook_fields[ i ] );
	}
}

static const char* AddressBook_MenuKey( int key ) {
	if ( key == K_ESCAPE ) {
		for ( int index = 0; index < NUM_ADDRESSBOOK_ENTRIES; index++ ) {
			char buffer[ 20 ];
			String::Sprintf( buffer, sizeof ( buffer ), "adr%d", index );
			Cvar_SetLatched( buffer, s_addressbook_fields[ index ].field.buffer );
		}
	}
	return Default_MenuKey( &s_addressbook_menu, key );
}

static void AddressBook_MenuDraw() {
	MQ2_Banner( "m_banner_addressbook" );
	Menu_Draw( &s_addressbook_menu );
}

static void AddressBook_MenuChar( int key ) {
	Default_MenuChar( &s_addressbook_menu, key );
}

static void MQ2_Menu_AddressBook_f() {
	AddressBook_MenuInit();
	MQ2_PushMenu( AddressBook_MenuDraw, AddressBook_MenuKey, AddressBook_MenuChar );
}

/*
=============================================================================

START SERVER MENU

=============================================================================
*/

static menuframework_s s_startserver_menu;
static char** mapnames;
static int nummaps;

static menuaction_s s_startserver_start_action;
static menuaction_s s_startserver_dmoptions_action;
static menufield_s s_timelimit_field;
static menufield_s s_fraglimit_field;
static menufield_s s_maxclients_field;
static menufield_s s_hostname_field;
static menulist_s s_startmap_list;
static menulist_s s_rules_box;

static void DMOptionsFunc( void* self ) {
	if ( s_rules_box.curvalue == 1 ) {
		return;
	}
	MQ2_Menu_DMOptions_f();
}

static void RulesChangeFunc( void* self ) {
	// DM
	if ( s_rules_box.curvalue == 0 ) {
		s_maxclients_field.generic.statusbar = NULL;
		s_startserver_dmoptions_action.generic.statusbar = NULL;
	} else if ( s_rules_box.curvalue == 1 )     {	// coop				// PGM
		s_maxclients_field.generic.statusbar = "4 maximum for cooperative";
		if ( String::Atoi( s_maxclients_field.field.buffer ) > 4 ) {
			String::Cpy( s_maxclients_field.field.buffer, "4" );
		}
		s_startserver_dmoptions_action.generic.statusbar = "N/A for cooperative";
	}
	// ROGUE GAMES
	else if ( FS_GetQuake2GameType() == 2 ) {
		if ( s_rules_box.curvalue == 2 ) {			// tag
			s_maxclients_field.generic.statusbar = NULL;
			s_startserver_dmoptions_action.generic.statusbar = NULL;
		}
	}
}

static void StartServerActionFunc( void* self ) {
	char startmap[ 1024 ];
	String::Cpy( startmap, strchr( mapnames[ s_startmap_list.curvalue ], '\n' ) + 1 );

	int maxclients = String::Atoi( s_maxclients_field.field.buffer );
	int timelimit = String::Atoi( s_timelimit_field.field.buffer );
	int fraglimit = String::Atoi( s_fraglimit_field.field.buffer );

	Cvar_SetValueLatched( "maxclients", ClampCvar( 0, maxclients, maxclients ) );
	Cvar_SetValueLatched( "timelimit", ClampCvar( 0, timelimit, timelimit ) );
	Cvar_SetValueLatched( "fraglimit", ClampCvar( 0, fraglimit, fraglimit ) );
	Cvar_SetLatched( "hostname", s_hostname_field.field.buffer );

	if ( ( s_rules_box.curvalue < 2 ) || ( FS_GetQuake2GameType() != 2 ) ) {
		Cvar_SetValueLatched( "deathmatch", !s_rules_box.curvalue );
		Cvar_SetValueLatched( "coop", s_rules_box.curvalue );
		Cvar_SetValueLatched( "gamerules", 0 );
	} else   {
		Cvar_SetValueLatched( "deathmatch", 1 );	// deathmatch is always true for rogue games, right?
		Cvar_SetValueLatched( "coop", 0 );				// FIXME - this might need to depend on which game we're running
		Cvar_SetValueLatched( "gamerules", s_rules_box.curvalue );
	}

	const char* spot = NULL;
	if ( s_rules_box.curvalue == 1 ) {		// PGM
		if ( String::ICmp( startmap, "bunk1" ) == 0 ) {
			spot = "start";
		} else if ( String::ICmp( startmap, "mintro" ) == 0 )       {
			spot = "start";
		} else if ( String::ICmp( startmap, "fact1" ) == 0 )       {
			spot = "start";
		} else if ( String::ICmp( startmap, "power1" ) == 0 )       {
			spot = "pstart";
		} else if ( String::ICmp( startmap, "biggun" ) == 0 )       {
			spot = "bstart";
		} else if ( String::ICmp( startmap, "hangar1" ) == 0 )       {
			spot = "unitstart";
		} else if ( String::ICmp( startmap, "city1" ) == 0 )       {
			spot = "unitstart";
		} else if ( String::ICmp( startmap, "boss1" ) == 0 )       {
			spot = "bosstart";
		}
	}

	if ( spot ) {
		if ( ComQ2_ServerState() ) {
			Cbuf_AddText( "disconnect\n" );
		}
		Cbuf_AddText( va( "gamemap \"*%s$%s\"\n", startmap, spot ) );
	} else   {
		Cbuf_AddText( va( "map %s\n", startmap ) );
	}

	MQ2_ForceMenuOff();
}

static void StartServer_MenuInit() {
	static const char* dm_coop_names[] =
	{
		"deathmatch",
		"cooperative",
		0
	};
	static const char* dm_coop_names_rogue[] =
	{
		"deathmatch",
		"cooperative",
		"tag",
		0
	};

	//	load the list of map names
	char* buffer;
	int length = FS_ReadFile( "maps.lst", ( void** )&buffer );
	if ( length == -1 ) {
		common->Error( "couldn't find maps.lst\n" );
	}

	const char* s = buffer;

	int i = 0;
	while ( i < length ) {
		if ( s[ i ] == '\r' ) {
			nummaps++;
		}
		i++;
	}

	if ( nummaps == 0 ) {
		common->Error( "no maps in maps.lst\n" );
	}

	mapnames = ( char** )Mem_Alloc( sizeof ( char* ) * ( nummaps + 1 ) );
	Com_Memset( mapnames, 0, sizeof ( char* ) * ( nummaps + 1 ) );

	s = buffer;

	for ( i = 0; i < nummaps; i++ ) {
		char shortname[ MAX_TOKEN_CHARS_Q2 ];
		String::Cpy( shortname, String::Parse2( &s ) );
		int l = String::Length( shortname );
		for ( int j = 0; j < l; j++ ) {
			shortname[ j ] = String::ToUpper( shortname[ j ] );
		}
		char longname[ MAX_TOKEN_CHARS_Q2 ];
		String::Cpy( longname, String::Parse2( &s ) );
		char scratch[ 200 ];
		String::Sprintf( scratch, sizeof ( scratch ), "%s\n%s", longname, shortname );

		mapnames[ i ] = ( char* )Mem_Alloc( String::Length( scratch ) + 1 );
		String::Cpy( mapnames[ i ], scratch );
	}
	mapnames[ nummaps ] = 0;

	FS_FreeFile( buffer );

	//	initialize the menu stuff
	s_startserver_menu.x = viddef.width * 0.50;
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x   = 0;
	s_startmap_list.generic.y   = 0;
	s_startmap_list.generic.name    = "initial map";
	s_startmap_list.itemnames = ( const char** )mapnames;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x   = 0;
	s_rules_box.generic.y   = 20;
	s_rules_box.generic.name    = "rules";

	//PGM - rogue games only available with rogue DLL.
	if ( FS_GetQuake2GameType() == 2 ) {
		s_rules_box.itemnames = dm_coop_names_rogue;
	} else   {
		s_rules_box.itemnames = dm_coop_names;
	}

	if ( Cvar_VariableValue( "coop" ) ) {
		s_rules_box.curvalue = 1;
	} else   {
		s_rules_box.curvalue = 0;
	}
	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x = 0;
	s_timelimit_field.generic.y = 36;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.field.maxLength = 3;
	s_timelimit_field.field.widthInChars = 3;
	String::Cpy( s_timelimit_field.field.buffer, Cvar_VariableString( "timelimit" ) );

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x = 0;
	s_fraglimit_field.generic.y = 54;
	s_fraglimit_field.generic.statusbar = "0 = no limit";
	s_fraglimit_field.field.maxLength = 3;
	s_fraglimit_field.field.widthInChars = 3;
	String::Cpy( s_fraglimit_field.field.buffer, Cvar_VariableString( "fraglimit" ) );

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x    = 0;
	s_maxclients_field.generic.y    = 72;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.field.maxLength = 3;
	s_maxclients_field.field.widthInChars = 3;
	if ( Cvar_VariableValue( "maxclients" ) == 1 ) {
		String::Cpy( s_maxclients_field.field.buffer, "8" );
	} else   {
		String::Cpy( s_maxclients_field.field.buffer, Cvar_VariableString( "maxclients" ) );
	}

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "hostname";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x  = 0;
	s_hostname_field.generic.y  = 90;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.field.maxLength = 12;
	s_hostname_field.field.widthInChars = 12;
	String::Cpy( s_hostname_field.field.buffer, Cvar_VariableString( "hostname" ) );

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name = " deathmatch flags";
	s_startserver_dmoptions_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_startserver_dmoptions_action.generic.x    = 24;
	s_startserver_dmoptions_action.generic.y    = 108;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name = " begin";
	s_startserver_start_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x    = 24;
	s_startserver_start_action.generic.y    = 128;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	Menu_AddItem( &s_startserver_menu, &s_startmap_list );
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	Menu_AddItem( &s_startserver_menu, &s_timelimit_field );
	Menu_AddItem( &s_startserver_menu, &s_fraglimit_field );
	Menu_AddItem( &s_startserver_menu, &s_maxclients_field );
	Menu_AddItem( &s_startserver_menu, &s_hostname_field );
	Menu_AddItem( &s_startserver_menu, &s_startserver_dmoptions_action );
	Menu_AddItem( &s_startserver_menu, &s_startserver_start_action );

	Menu_Center( &s_startserver_menu );

	// call this now to set proper inital state
	RulesChangeFunc( NULL );
}

static void StartServer_MenuDraw() {
	Menu_Draw( &s_startserver_menu );
}

static const char* StartServer_MenuKey( int key ) {
	if ( key == K_ESCAPE ) {
		if ( mapnames ) {
			for ( int i = 0; i < nummaps; i++ ) {
				Mem_Free( mapnames[ i ] );
			}
			Mem_Free( mapnames );
		}
		mapnames = 0;
		nummaps = 0;
	}

	return Default_MenuKey( &s_startserver_menu, key );
}

static void StartServer_MenuChar( int key ) {
	Default_MenuChar( &s_startserver_menu, key );
}

static void MQ2_Menu_StartServer_f() {
	StartServer_MenuInit();
	MQ2_PushMenu( StartServer_MenuDraw, StartServer_MenuKey, StartServer_MenuChar );
}

/*
=============================================================================

DMOPTIONS BOOK MENU

=============================================================================
*/
static char dmoptions_statusbar[ 128 ];

static menuframework_s s_dmoptions_menu;

static menulist_s s_friendlyfire_box;
static menulist_s s_falls_box;
static menulist_s s_weapons_stay_box;
static menulist_s s_instant_powerups_box;
static menulist_s s_powerups_box;
static menulist_s s_health_box;
static menulist_s s_spawn_farthest_box;
static menulist_s s_teamplay_box;
static menulist_s s_samelevel_box;
static menulist_s s_force_respawn_box;
static menulist_s s_armor_box;
static menulist_s s_allow_exit_box;
static menulist_s s_infinite_ammo_box;
static menulist_s s_fixed_fov_box;
static menulist_s s_quad_drop_box;

//ROGUE
static menulist_s s_no_mines_box;
static menulist_s s_no_nukes_box;
static menulist_s s_stack_double_box;
static menulist_s s_no_spheres_box;
//ROGUE

static void DMFlagCallback( void* self ) {
	menulist_s* f = ( menulist_s* )self;

	int flags = Cvar_VariableValue( "dmflags" );

	int bit = 0;
	if ( f == &s_friendlyfire_box ) {
		if ( f->curvalue ) {
			flags &= ~Q2DF_NO_FRIENDLY_FIRE;
		} else   {
			flags |= Q2DF_NO_FRIENDLY_FIRE;
		}
		goto setvalue;
	} else if ( f == &s_falls_box )     {
		if ( f->curvalue ) {
			flags &= ~Q2DF_NO_FALLING;
		} else   {
			flags |= Q2DF_NO_FALLING;
		}
		goto setvalue;
	} else if ( f == &s_weapons_stay_box )     {
		bit = Q2DF_WEAPONS_STAY;
	} else if ( f == &s_instant_powerups_box )     {
		bit = Q2DF_INSTANT_ITEMS;
	} else if ( f == &s_allow_exit_box )     {
		bit = Q2DF_ALLOW_EXIT;
	} else if ( f == &s_powerups_box )     {
		if ( f->curvalue ) {
			flags &= ~Q2DF_NO_ITEMS;
		} else   {
			flags |= Q2DF_NO_ITEMS;
		}
		goto setvalue;
	} else if ( f == &s_health_box )     {
		if ( f->curvalue ) {
			flags &= ~Q2DF_NO_HEALTH;
		} else   {
			flags |= Q2DF_NO_HEALTH;
		}
		goto setvalue;
	} else if ( f == &s_spawn_farthest_box )     {
		bit = Q2DF_SPAWN_FARTHEST;
	} else if ( f == &s_teamplay_box )     {
		if ( f->curvalue == 1 ) {
			flags |=  Q2DF_SKINTEAMS;
			flags &= ~Q2DF_MODELTEAMS;
		} else if ( f->curvalue == 2 )     {
			flags |=  Q2DF_MODELTEAMS;
			flags &= ~Q2DF_SKINTEAMS;
		} else   {
			flags &= ~( Q2DF_MODELTEAMS | Q2DF_SKINTEAMS );
		}

		goto setvalue;
	} else if ( f == &s_samelevel_box )     {
		bit = Q2DF_SAME_LEVEL;
	} else if ( f == &s_force_respawn_box )     {
		bit = Q2DF_FORCE_RESPAWN;
	} else if ( f == &s_armor_box )     {
		if ( f->curvalue ) {
			flags &= ~Q2DF_NO_ARMOR;
		} else   {
			flags |= Q2DF_NO_ARMOR;
		}
		goto setvalue;
	} else if ( f == &s_infinite_ammo_box )     {
		bit = Q2DF_INFINITE_AMMO;
	} else if ( f == &s_fixed_fov_box )     {
		bit = Q2DF_FIXED_FOV;
	} else if ( f == &s_quad_drop_box )     {
		bit = Q2DF_QUAD_DROP;
	} else if ( FS_GetQuake2GameType() == 2 )     {
		if ( f == &s_no_mines_box ) {
			bit = Q2DF_NO_MINES;
		} else if ( f == &s_no_nukes_box )     {
			bit = Q2DF_NO_NUKES;
		} else if ( f == &s_stack_double_box )     {
			bit = Q2DF_NO_STACK_DOUBLE;
		} else if ( f == &s_no_spheres_box )     {
			bit = Q2DF_NO_SPHERES;
		}
	}

	if ( f ) {
		if ( f->curvalue == 0 ) {
			flags &= ~bit;
		} else   {
			flags |= bit;
		}
	}

setvalue:
	Cvar_SetValueLatched( "dmflags", flags );

	String::Sprintf( dmoptions_statusbar, sizeof ( dmoptions_statusbar ), "dmflags = %d", flags );

}

static void DMOptions_MenuInit() {
	static const char* yes_no_names[] =
	{
		"no", "yes", 0
	};
	static const char* teamplay_names[] =
	{
		"disabled", "by skin", "by model", 0
	};
	int dmflags = Cvar_VariableValue( "dmflags" );
	int y = 0;

	s_dmoptions_menu.x = viddef.width * 0.50;
	s_dmoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.x   = 0;
	s_falls_box.generic.y   = y;
	s_falls_box.generic.name    = "falling damage";
	s_falls_box.generic.callback = DMFlagCallback;
	s_falls_box.itemnames = yes_no_names;
	s_falls_box.curvalue = ( dmflags & Q2DF_NO_FALLING ) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.x    = 0;
	s_weapons_stay_box.generic.y    = y += 10;
	s_weapons_stay_box.generic.name = "weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = ( dmflags & Q2DF_WEAPONS_STAY ) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x    = 0;
	s_instant_powerups_box.generic.y    = y += 10;
	s_instant_powerups_box.generic.name = "instant powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = ( dmflags & Q2DF_INSTANT_ITEMS ) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x    = 0;
	s_powerups_box.generic.y    = y += 10;
	s_powerups_box.generic.name = "allow powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = ( dmflags & Q2DF_NO_ITEMS ) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x  = 0;
	s_health_box.generic.y  = y += 10;
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name   = "allow health";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = ( dmflags & Q2DF_NO_HEALTH ) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x   = 0;
	s_armor_box.generic.y   = y += 10;
	s_armor_box.generic.name    = "allow armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = ( dmflags & Q2DF_NO_ARMOR ) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x  = 0;
	s_spawn_farthest_box.generic.y  = y += 10;
	s_spawn_farthest_box.generic.name   = "spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = ( dmflags & Q2DF_SPAWN_FARTHEST ) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x   = 0;
	s_samelevel_box.generic.y   = y += 10;
	s_samelevel_box.generic.name    = "same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = ( dmflags & Q2DF_SAME_LEVEL ) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x   = 0;
	s_force_respawn_box.generic.y   = y += 10;
	s_force_respawn_box.generic.name    = "force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = ( dmflags & Q2DF_FORCE_RESPAWN ) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.x    = 0;
	s_teamplay_box.generic.y    = y += 10;
	s_teamplay_box.generic.name = "teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemnames = teamplay_names;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x  = 0;
	s_allow_exit_box.generic.y  = y += 10;
	s_allow_exit_box.generic.name   = "allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = ( dmflags & Q2DF_ALLOW_EXIT ) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x   = 0;
	s_infinite_ammo_box.generic.y   = y += 10;
	s_infinite_ammo_box.generic.name    = "infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = ( dmflags & Q2DF_INFINITE_AMMO ) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.x   = 0;
	s_fixed_fov_box.generic.y   = y += 10;
	s_fixed_fov_box.generic.name    = "fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemnames = yes_no_names;
	s_fixed_fov_box.curvalue = ( dmflags & Q2DF_FIXED_FOV ) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x   = 0;
	s_quad_drop_box.generic.y   = y += 10;
	s_quad_drop_box.generic.name    = "quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = ( dmflags & Q2DF_QUAD_DROP ) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x    = 0;
	s_friendlyfire_box.generic.y    = y += 10;
	s_friendlyfire_box.generic.name = "friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = ( dmflags & Q2DF_NO_FRIENDLY_FIRE ) == 0;

	if ( FS_GetQuake2GameType() == 2 ) {
		s_no_mines_box.generic.type = MTYPE_SPINCONTROL;
		s_no_mines_box.generic.x    = 0;
		s_no_mines_box.generic.y    = y += 10;
		s_no_mines_box.generic.name = "remove mines";
		s_no_mines_box.generic.callback = DMFlagCallback;
		s_no_mines_box.itemnames = yes_no_names;
		s_no_mines_box.curvalue = ( dmflags & Q2DF_NO_MINES ) != 0;

		s_no_nukes_box.generic.type = MTYPE_SPINCONTROL;
		s_no_nukes_box.generic.x    = 0;
		s_no_nukes_box.generic.y    = y += 10;
		s_no_nukes_box.generic.name = "remove nukes";
		s_no_nukes_box.generic.callback = DMFlagCallback;
		s_no_nukes_box.itemnames = yes_no_names;
		s_no_nukes_box.curvalue = ( dmflags & Q2DF_NO_NUKES ) != 0;

		s_stack_double_box.generic.type = MTYPE_SPINCONTROL;
		s_stack_double_box.generic.x    = 0;
		s_stack_double_box.generic.y    = y += 10;
		s_stack_double_box.generic.name = "2x/4x stacking off";
		s_stack_double_box.generic.callback = DMFlagCallback;
		s_stack_double_box.itemnames = yes_no_names;
		s_stack_double_box.curvalue = ( dmflags & Q2DF_NO_STACK_DOUBLE ) != 0;

		s_no_spheres_box.generic.type = MTYPE_SPINCONTROL;
		s_no_spheres_box.generic.x  = 0;
		s_no_spheres_box.generic.y  = y += 10;
		s_no_spheres_box.generic.name   = "remove spheres";
		s_no_spheres_box.generic.callback = DMFlagCallback;
		s_no_spheres_box.itemnames = yes_no_names;
		s_no_spheres_box.curvalue = ( dmflags & Q2DF_NO_SPHERES ) != 0;

	}

	Menu_AddItem( &s_dmoptions_menu, &s_falls_box );
	Menu_AddItem( &s_dmoptions_menu, &s_weapons_stay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_instant_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_health_box );
	Menu_AddItem( &s_dmoptions_menu, &s_armor_box );
	Menu_AddItem( &s_dmoptions_menu, &s_spawn_farthest_box );
	Menu_AddItem( &s_dmoptions_menu, &s_samelevel_box );
	Menu_AddItem( &s_dmoptions_menu, &s_force_respawn_box );
	Menu_AddItem( &s_dmoptions_menu, &s_teamplay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_allow_exit_box );
	Menu_AddItem( &s_dmoptions_menu, &s_infinite_ammo_box );
	Menu_AddItem( &s_dmoptions_menu, &s_fixed_fov_box );
	Menu_AddItem( &s_dmoptions_menu, &s_quad_drop_box );
	Menu_AddItem( &s_dmoptions_menu, &s_friendlyfire_box );

	if ( FS_GetQuake2GameType() == 2 ) {
		Menu_AddItem( &s_dmoptions_menu, &s_no_mines_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_nukes_box );
		Menu_AddItem( &s_dmoptions_menu, &s_stack_double_box );
		Menu_AddItem( &s_dmoptions_menu, &s_no_spheres_box );
	}

	Menu_Center( &s_dmoptions_menu );

	// set the original dmflags statusbar
	DMFlagCallback( 0 );
	Menu_SetStatusBar( &s_dmoptions_menu, dmoptions_statusbar );
}

static void DMOptions_MenuDraw() {
	Menu_Draw( &s_dmoptions_menu );
}

static const char* DMOptions_MenuKey( int key ) {
	return Default_MenuKey( &s_dmoptions_menu, key );
}

static void MQ2_Menu_DMOptions_f() {
	DMOptions_MenuInit();
	MQ2_PushMenu( DMOptions_MenuDraw, DMOptions_MenuKey, NULL );
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/

static menuframework_s s_player_config_menu;
static menufield_s s_player_name_field;
static menulist_s s_player_model_box;
static menulist_s s_player_skin_box;
static menulist_s s_player_handedness_box;
static menulist_s s_player_rate_box;
static menuseparator_s s_player_skin_title;
static menuseparator_s s_player_model_title;
static menuseparator_s s_player_hand_title;
static menuseparator_s s_player_rate_title;
static menuaction_s s_player_download_action;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

struct playermodelinfo_s {
	int nskins;
	char** skindisplaynames;
	char displayname[ MAX_DISPLAYNAME ];
	char directory[ MAX_QPATH ];
};

static playermodelinfo_s s_pmi[ MAX_PLAYERMODELS ];
static char* s_pmnames[ MAX_PLAYERMODELS ];
static int s_numplayermodels;

static int rate_tbl[] = { 2500, 3200, 5000, 10000, 25000, 0 };
static const char* rate_names[] = { "28.8 Modem", "33.6 Modem", "Single ISDN",
									"Dual ISDN/Cable", "T1/LAN", "User defined", 0 };

static void DownloadOptionsFunc( void* self ) {
	MQ2_Menu_DownloadOptions_f();
}

static void HandednessCallback( void* unused ) {
	Cvar_SetValueLatched( "hand", s_player_handedness_box.curvalue );
}

static void RateCallback( void* unused ) {
	if ( s_player_rate_box.curvalue != sizeof ( rate_tbl ) / sizeof ( *rate_tbl ) - 1 ) {
		Cvar_SetValueLatched( "rate", rate_tbl[ s_player_rate_box.curvalue ] );
	}
}

static void ModelCallback( void* unused ) {
	s_player_skin_box.itemnames = ( const char** )s_pmi[ s_player_model_box.curvalue ].skindisplaynames;
	s_player_skin_box.curvalue = 0;
}

static qboolean IconOfSkinExists( char* skin, char** pcxfiles, int npcxfiles ) {
	int i;
	char scratch[ 1024 ];

	String::Cpy( scratch, skin );
	*String::RChr( scratch, '.' ) = 0;
	String::Cat( scratch, sizeof ( scratch ), "_i.pcx" );

	for ( i = 0; i < npcxfiles; i++ ) {
		if ( String::Cmp( pcxfiles[ i ], scratch ) == 0 ) {
			return true;
		}
	}

	return false;
}

static void PlayerConfig_ScanDirectories( void ) {
	s_numplayermodels = 0;

	//	get a list of directories
	int ndirs = 0;
	char** dirnames = FS_ListFiles( "players", "/", &ndirs );
	if ( !dirnames ) {
		return;
	}

	//	go through the subdirectories
	int npms = ndirs;
	if ( npms > MAX_PLAYERMODELS ) {
		npms = MAX_PLAYERMODELS;
	}

	for ( int i = 0; i < npms; i++ ) {
		if ( dirnames[ i ] == 0 ) {
			continue;
		}

		// verify the existence of tris.md2
		char scratch[ 1024 ];
		String::Cpy( scratch, "players/" );
		String::Cat( scratch, sizeof ( scratch ), dirnames[ i ] );
		String::Cat( scratch, sizeof ( scratch ), "/tris.md2" );
		fileHandle_t f;
		FS_FOpenFileRead( scratch, &f, false );
		if ( !f ) {
			continue;
		}
		FS_FCloseFile( f );

		// verify the existence of at least one pcx skin
		String::Cpy( scratch, "players/" );
		String::Cat( scratch, sizeof ( scratch ), dirnames[ i ] );
		int npcxfiles;
		char** pcxnames = FS_ListFiles( scratch, ".pcx", &npcxfiles );

		if ( !pcxnames ) {
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		int nskins = 0;
		for ( int k = 0; k < npcxfiles - 1; k++ ) {
			if ( !strstr( pcxnames[ k ], "_i.pcx" ) ) {
				if ( IconOfSkinExists( pcxnames[ k ], pcxnames, npcxfiles - 1 ) ) {
					nskins++;
				}
			}
		}
		if ( !nskins ) {
			FS_FreeFileList( pcxnames );
			continue;
		}

		char** skinnames = ( char** )Mem_Alloc( sizeof ( char* ) * ( nskins + 1 ) );
		Com_Memset( skinnames, 0, sizeof ( char* ) * ( nskins + 1 ) );

		// copy the valid skins
		for ( int s = 0, k = 0; k < npcxfiles - 1; k++ ) {
			if ( !strstr( pcxnames[ k ], "_i.pcx" ) ) {
				if ( IconOfSkinExists( pcxnames[ k ], pcxnames, npcxfiles - 1 ) ) {
					String::Cpy( scratch, pcxnames[ k ] );

					if ( String::RChr( scratch, '.' ) ) {
						*String::RChr( scratch, '.' ) = 0;
					}

					skinnames[ s ] = __CopyString( scratch );
					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[ s_numplayermodels ].nskins = nskins;
		s_pmi[ s_numplayermodels ].skindisplaynames = skinnames;

		// make short name for the model
		String::NCpy( s_pmi[ s_numplayermodels ].displayname, dirnames[ i ], MAX_DISPLAYNAME - 1 );
		String::Cpy( s_pmi[ s_numplayermodels ].directory, dirnames[ i ] );

		FS_FreeFileList( pcxnames );

		s_numplayermodels++;
	}
	FS_FreeFileList( dirnames );
}

static int pmicmpfnc( const void* _a, const void* _b ) {
	const playermodelinfo_s* a = ( const playermodelinfo_s* )_a;
	const playermodelinfo_s* b = ( const playermodelinfo_s* )_b;

	/*
	** sort by male, female, then alphabetical
	*/
	if ( String::Cmp( a->directory, "male" ) == 0 ) {
		return -1;
	} else if ( String::Cmp( b->directory, "male" ) == 0 )       {
		return 1;
	}

	if ( String::Cmp( a->directory, "female" ) == 0 ) {
		return -1;
	} else if ( String::Cmp( b->directory, "female" ) == 0 )       {
		return 1;
	}

	return String::Cmp( a->directory, b->directory );
}

static bool PlayerConfig_MenuInit() {
	char currentdirectory[ 1024 ];
	char currentskin[ 1024 ];
	int i = 0;

	int currentdirectoryindex = 0;
	int currentskinindex = 0;

	static const char* handedness[] = { "right", "left", "center", 0 };

	PlayerConfig_ScanDirectories();

	if ( s_numplayermodels == 0 ) {
		return false;
	}

	if ( q2_hand->value < 0 || q2_hand->value > 2 ) {
		Cvar_SetValueLatched( "hand", 0 );
	}

	String::Cpy( currentdirectory, clq2_skin->string );

	if ( strchr( currentdirectory, '/' ) ) {
		String::Cpy( currentskin, strchr( currentdirectory, '/' ) + 1 );
		*strchr( currentdirectory, '/' ) = 0;
	} else if ( strchr( currentdirectory, '\\' ) )       {
		String::Cpy( currentskin, strchr( currentdirectory, '\\' ) + 1 );
		*strchr( currentdirectory, '\\' ) = 0;
	} else   {
		String::Cpy( currentdirectory, "male" );
		String::Cpy( currentskin, "grunt" );
	}

	qsort( s_pmi, s_numplayermodels, sizeof ( s_pmi[ 0 ] ), pmicmpfnc );

	Com_Memset( s_pmnames, 0, sizeof ( s_pmnames ) );
	for ( i = 0; i < s_numplayermodels; i++ ) {
		s_pmnames[ i ] = s_pmi[ i ].displayname;
		if ( String::ICmp( s_pmi[ i ].directory, currentdirectory ) == 0 ) {
			int j;

			currentdirectoryindex = i;

			for ( j = 0; j < s_pmi[ i ].nskins; j++ ) {
				if ( String::ICmp( s_pmi[ i ].skindisplaynames[ j ], currentskin ) == 0 ) {
					currentskinindex = j;
					break;
				}
			}
		}
	}

	s_player_config_menu.x = viddef.width / 2 - 95;
	s_player_config_menu.y = viddef.height / 2 - 97;
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x       = 0;
	s_player_name_field.generic.y       = 0;
	s_player_name_field.field.maxLength = 20;
	s_player_name_field.field.widthInChars = 20;
	String::Cpy( s_player_name_field.field.buffer, clq2_name->string );
	s_player_name_field.field.cursor = String::Length( clq2_name->string );

	s_player_model_title.generic.type = MTYPE_SEPARATOR;
	s_player_model_title.generic.name = "model";
	s_player_model_title.generic.x    = -8;
	s_player_model_title.generic.y   = 60;

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.x    = -56;
	s_player_model_box.generic.y    = 70;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -48;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = ( const char** )s_pmnames;

	s_player_skin_title.generic.type = MTYPE_SEPARATOR;
	s_player_skin_title.generic.name = "skin";
	s_player_skin_title.generic.x    = -16;
	s_player_skin_title.generic.y    = 84;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.x = -56;
	s_player_skin_box.generic.y = 94;
	s_player_skin_box.generic.name  = 0;
	s_player_skin_box.generic.callback = 0;
	s_player_skin_box.generic.cursor_offset = -48;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = ( const char** )s_pmi[ currentdirectoryindex ].skindisplaynames;

	s_player_hand_title.generic.type = MTYPE_SEPARATOR;
	s_player_hand_title.generic.name = "handedness";
	s_player_hand_title.generic.x    = 32;
	s_player_hand_title.generic.y    = 108;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.x   = -56;
	s_player_handedness_box.generic.y   = 118;
	s_player_handedness_box.generic.name    = 0;
	s_player_handedness_box.generic.cursor_offset = -48;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableValue( "hand" );
	s_player_handedness_box.itemnames = handedness;

	for ( i = 0; i < ( int )( sizeof ( rate_tbl ) / sizeof ( *rate_tbl ) - 1 ); i++ )
		if ( Cvar_VariableValue( "rate" ) == rate_tbl[ i ] ) {
			break;
		}

	s_player_rate_title.generic.type = MTYPE_SEPARATOR;
	s_player_rate_title.generic.name = "connect speed";
	s_player_rate_title.generic.x    = 56;
	s_player_rate_title.generic.y    = 156;

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x = -56;
	s_player_rate_box.generic.y = 166;
	s_player_rate_box.generic.name  = 0;
	s_player_rate_box.generic.cursor_offset = -48;
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_player_download_action.generic.type = MTYPE_ACTION;
	s_player_download_action.generic.name   = "download options";
	s_player_download_action.generic.flags = QMF_LEFT_JUSTIFY;
	s_player_download_action.generic.x  = -24;
	s_player_download_action.generic.y  = 186;
	s_player_download_action.generic.statusbar = NULL;
	s_player_download_action.generic.callback = DownloadOptionsFunc;

	Menu_AddItem( &s_player_config_menu, &s_player_name_field );
	Menu_AddItem( &s_player_config_menu, &s_player_model_title );
	Menu_AddItem( &s_player_config_menu, &s_player_model_box );
	if ( s_player_skin_box.itemnames ) {
		Menu_AddItem( &s_player_config_menu, &s_player_skin_title );
		Menu_AddItem( &s_player_config_menu, &s_player_skin_box );
	}
	Menu_AddItem( &s_player_config_menu, &s_player_hand_title );
	Menu_AddItem( &s_player_config_menu, &s_player_handedness_box );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_title );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_box );
	Menu_AddItem( &s_player_config_menu, &s_player_download_action );

	return true;
}

static void PlayerConfig_MenuDraw() {
	refdef_t refdef;
	Com_Memset( &refdef, 0, sizeof ( refdef ) );

	refdef.x = viddef.width / 2;
	refdef.y = viddef.height / 2 - 72;
	refdef.width = 144;
	refdef.height = 168;
	refdef.fov_x = 40;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime;
	AxisClear( refdef.viewaxis );

	if ( s_pmi[ s_player_model_box.curvalue ].skindisplaynames ) {
		refEntity_t entity;
		Com_Memset( &entity, 0, sizeof ( entity ) );

		char scratch[ MAX_QPATH ];
		String::Sprintf( scratch, sizeof ( scratch ), "players/%s/tris.md2", s_pmi[ s_player_model_box.curvalue ].directory );
		entity.hModel = R_RegisterModel( scratch );
		String::Sprintf( scratch, sizeof ( scratch ), "players/%s/%s.pcx", s_pmi[ s_player_model_box.curvalue ].directory, s_pmi[ s_player_model_box.curvalue ].skindisplaynames[ s_player_skin_box.curvalue ] );
		entity.customSkin = R_GetImageHandle( R_RegisterSkinQ2( scratch ) );
		entity.renderfx = RF_ABSOLUTE_LIGHT;
		entity.absoluteLight = 1;
		entity.origin[ 0 ] = 80;
		entity.origin[ 1 ] = 0;
		entity.origin[ 2 ] = 0;
		VectorCopy( entity.origin, entity.oldorigin );
		entity.frame = 0;
		entity.oldframe = 0;
		entity.backlerp = 0.0;
		vec3_t angles;
		angles[ 0 ] = 0;
		angles[ 1 ] = ( cls.realtime / 10 ) % 360;
		angles[ 2 ] = 0;
		AnglesToAxis( angles, entity.axis );
		R_ClearScene();
		R_AddRefEntityToScene( &entity );

		Com_Memset( refdef.areamask, 0, sizeof ( refdef.areamask ) );
		refdef.rdflags = RDF_NOWORLDMODEL;

		Menu_Draw( &s_player_config_menu );

		MQ2_DrawTextBox( ( refdef.x ) * ( 320.0F / viddef.width ) - 8, ( viddef.height / 2 ) * ( 240.0F / viddef.height ) - 77, refdef.width / 8, refdef.height / 8 );
		refdef.height += 4;
		UI_Fill( refdef.x, refdef.y, refdef.width, refdef.height, 0.3, 0.3, 0.3, 1 );

		R_RenderScene( &refdef );

		String::Sprintf( scratch, sizeof ( scratch ), "/players/%s/%s_i.pcx",
			s_pmi[ s_player_model_box.curvalue ].directory,
			s_pmi[ s_player_model_box.curvalue ].skindisplaynames[ s_player_skin_box.curvalue ] );
		UI_DrawNamedPic( s_player_config_menu.x - 40, refdef.y, scratch );
	}
}

static const char* PlayerConfig_MenuKey( int key ) {
	if ( key == K_ESCAPE ) {
		Cvar_SetLatched( "name", s_player_name_field.field.buffer );

		char scratch[ 1024 ];
		String::Sprintf( scratch, sizeof ( scratch ), "%s/%s",
			s_pmi[ s_player_model_box.curvalue ].directory,
			s_pmi[ s_player_model_box.curvalue ].skindisplaynames[ s_player_skin_box.curvalue ] );

		Cvar_SetLatched( "skin", scratch );

		for ( int i = 0; i < s_numplayermodels; i++ ) {
			for ( int j = 0; j < s_pmi[ i ].nskins; j++ ) {
				if ( s_pmi[ i ].skindisplaynames[ j ] ) {
					Mem_Free( s_pmi[ i ].skindisplaynames[ j ] );
				}
				s_pmi[ i ].skindisplaynames[ j ] = 0;
			}
			Mem_Free( s_pmi[ i ].skindisplaynames );
			s_pmi[ i ].skindisplaynames = 0;
			s_pmi[ i ].nskins = 0;
		}
	}
	return Default_MenuKey( &s_player_config_menu, key );
}

static void PlayerConfig_MenuChar( int key ) {
	Default_MenuChar( &s_player_config_menu, key );
}

static void MQ2_Menu_PlayerConfig_f() {
	if ( !PlayerConfig_MenuInit() ) {
		Menu_SetStatusBar( &s_multiplayer_menu, "No valid player models found" );
		return;
	}
	Menu_SetStatusBar( &s_multiplayer_menu, NULL );
	MQ2_PushMenu( PlayerConfig_MenuDraw, PlayerConfig_MenuKey, PlayerConfig_MenuChar );
}

/*
=============================================================================

DOWNLOADOPTIONS BOOK MENU

=============================================================================
*/
static menuframework_s s_downloadoptions_menu;

static menuseparator_s s_download_title;
static menulist_s s_allow_download_box;
static menulist_s s_allow_download_maps_box;
static menulist_s s_allow_download_models_box;
static menulist_s s_allow_download_players_box;
static menulist_s s_allow_download_sounds_box;

static void DownloadCallback( void* self ) {
	menulist_s* f = ( menulist_s* )self;

	if ( f == &s_allow_download_box ) {
		Cvar_SetValueLatched( "allow_download", f->curvalue );
	} else if ( f == &s_allow_download_maps_box )     {
		Cvar_SetValueLatched( "allow_download_maps", f->curvalue );
	} else if ( f == &s_allow_download_models_box )     {
		Cvar_SetValueLatched( "allow_download_models", f->curvalue );
	} else if ( f == &s_allow_download_players_box )     {
		Cvar_SetValueLatched( "allow_download_players", f->curvalue );
	} else if ( f == &s_allow_download_sounds_box )     {
		Cvar_SetValueLatched( "allow_download_sounds", f->curvalue );
	}
}

static void DownloadOptions_MenuInit() {
	static const char* yes_no_names[] =
	{
		"no", "yes", 0
	};
	int y = 0;

	s_downloadoptions_menu.x = viddef.width * 0.50;
	s_downloadoptions_menu.nitems = 0;

	s_download_title.generic.type = MTYPE_SEPARATOR;
	s_download_title.generic.name = "Download Options";
	s_download_title.generic.x    = 48;
	s_download_title.generic.y   = y;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x  = 0;
	s_allow_download_box.generic.y  = y += 20;
	s_allow_download_box.generic.name   = "allow downloading";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = ( Cvar_VariableValue( "allow_download" ) != 0 );

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x = 0;
	s_allow_download_maps_box.generic.y = y += 20;
	s_allow_download_maps_box.generic.name  = "maps";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = ( Cvar_VariableValue( "allow_download_maps" ) != 0 );

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x  = 0;
	s_allow_download_players_box.generic.y  = y += 10;
	s_allow_download_players_box.generic.name   = "player models/skins";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = ( Cvar_VariableValue( "allow_download_players" ) != 0 );

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x   = 0;
	s_allow_download_models_box.generic.y   = y += 10;
	s_allow_download_models_box.generic.name    = "models";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = ( Cvar_VariableValue( "allow_download_models" ) != 0 );

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x   = 0;
	s_allow_download_sounds_box.generic.y   = y += 10;
	s_allow_download_sounds_box.generic.name    = "sounds";
	s_allow_download_sounds_box.generic.callback = DownloadCallback;
	s_allow_download_sounds_box.itemnames = yes_no_names;
	s_allow_download_sounds_box.curvalue = ( Cvar_VariableValue( "allow_download_sounds" ) != 0 );

	Menu_AddItem( &s_downloadoptions_menu, &s_download_title );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_maps_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_players_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_models_box );
	Menu_AddItem( &s_downloadoptions_menu, &s_allow_download_sounds_box );

	Menu_Center( &s_downloadoptions_menu );

	// skip over title
	if ( s_downloadoptions_menu.cursor == 0 ) {
		s_downloadoptions_menu.cursor = 1;
	}
}

static void DownloadOptions_MenuDraw() {
	Menu_Draw( &s_downloadoptions_menu );
}

static const char* DownloadOptions_MenuKey( int key ) {
	return Default_MenuKey( &s_downloadoptions_menu, key );
}

static void MQ2_Menu_DownloadOptions_f() {
	DownloadOptions_MenuInit();
	MQ2_PushMenu( DownloadOptions_MenuDraw, DownloadOptions_MenuKey, NULL );
}

/*
=======================================================================

CONTROLS MENU

=======================================================================
*/

extern Cvar* in_joystick;

static menuframework_s s_options_menu;
static menuaction_s s_options_defaults_action;
static menuaction_s s_options_customize_options_action;
static menuslider_s s_options_sensitivity_slider;
static menulist_s s_options_freelook_box;
static menulist_s s_options_alwaysrun_box;
static menulist_s s_options_invertmouse_box;
static menulist_s s_options_lookspring_box;
static menulist_s s_options_crosshair_box;
static menuslider_s s_options_sfxvolume_slider;
static menulist_s s_options_joystick_box;
static menulist_s s_options_cdvolume_box;
static menulist_s s_options_console_action;

static void CrosshairFunc( void* unused ) {
	Cvar_SetValueLatched( "crosshair", s_options_crosshair_box.curvalue );
}

static void JoystickFunc( void* unused ) {
	Cvar_SetValueLatched( "in_joystick", s_options_joystick_box.curvalue );
}

static void CustomizeControlsFunc( void* unused ) {
	MQ2_Menu_Keys_f();
}

static void AlwaysRunFunc( void* unused ) {
	Cvar_SetValueLatched( "cl_run", s_options_alwaysrun_box.curvalue );
}

static void FreeLookFunc( void* unused ) {
	Cvar_SetValueLatched( "cl_freelook", s_options_freelook_box.curvalue );
}

static void MouseSpeedFunc( void* unused ) {
	Cvar_SetValueLatched( "sensitivity", s_options_sensitivity_slider.curvalue / 2.0F );
}

static void ControlsSetMenuItemValues( void ) {
	s_options_sfxvolume_slider.curvalue     = Cvar_VariableValue( "s_volume" ) * 10;
	s_options_cdvolume_box.curvalue         = !Cvar_VariableValue( "cd_nocd" );
	s_options_sensitivity_slider.curvalue   = ( cl_sensitivity->value ) * 2;

	Cvar_SetValueLatched( "cl_run", ClampCvar( 0, 1, cl_run->value ) );
	s_options_alwaysrun_box.curvalue        = cl_run->value;

	s_options_invertmouse_box.curvalue      = m_pitch->value < 0;

	Cvar_SetValueLatched( "lookspring", ClampCvar( 0, 1, lookspring->value ) );
	s_options_lookspring_box.curvalue       = lookspring->value;

	Cvar_SetValueLatched( "cl_freelook", ClampCvar( 0, 1, cl_freelook->value ) );
	s_options_freelook_box.curvalue         = cl_freelook->value;

	Cvar_SetValueLatched( "crosshair", ClampCvar( 0, 3, crosshair->value ) );
	s_options_crosshair_box.curvalue        = crosshair->value;

	Cvar_SetValueLatched( "in_joystick", ClampCvar( 0, 1, in_joystick->value ) );
	s_options_joystick_box.curvalue     = in_joystick->value;
}

static void ControlsResetDefaultsFunc( void* unused ) {
	Cbuf_AddText( "exec default.cfg\n" );
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

static void InvertMouseFunc( void* unused ) {
	if ( s_options_invertmouse_box.curvalue == 0 ) {
		Cvar_SetValueLatched( "m_pitch", fabs( m_pitch->value ) );
	} else   {
		Cvar_SetValueLatched( "m_pitch", -fabs( m_pitch->value ) );
	}
}

static void LookspringFunc( void* unused ) {
	Cvar_SetValueLatched( "lookspring", s_options_lookspring_box.curvalue );
}

static void UpdateVolumeFunc( void* unused ) {
	Cvar_SetValueLatched( "s_volume", s_options_sfxvolume_slider.curvalue / 10 );
}

static void UpdateCDVolumeFunc( void* unused ) {
	Cvar_SetValueLatched( "cd_nocd", !s_options_cdvolume_box.curvalue );
}

static void ConsoleFunc( void* unused ) {
	//	the proper way to do this is probably to have ToggleConsole_f accept a parameter
	if ( cl.q2_attractloop ) {
		Cbuf_AddText( "killserver\n" );
		return;
	}

	Con_ClearTyping();
	Con_ClearNotify();

	MQ2_ForceMenuOff();
	in_keyCatchers |= KEYCATCH_CONSOLE;
}

static void Options_MenuInit() {
	static const char* cd_music_items[] =
	{
		"disabled",
		"enabled",
		0
	};

	static const char* yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char* crosshair_names[] =
	{
		"none",
		"cross",
		"dot",
		"angle",
		0
	};

	s_options_menu.x = viddef.width / 2;
	s_options_menu.y = viddef.height / 2 - 58;
	s_options_menu.nitems = 0;

	s_options_sfxvolume_slider.generic.type = MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x    = 0;
	s_options_sfxvolume_slider.generic.y    = 0;
	s_options_sfxvolume_slider.generic.name = "effects volume";
	s_options_sfxvolume_slider.generic.callback = UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue     = 0;
	s_options_sfxvolume_slider.maxvalue     = 10;
	s_options_sfxvolume_slider.curvalue     = Cvar_VariableValue( "s_volume" ) * 10;

	s_options_cdvolume_box.generic.type = MTYPE_SPINCONTROL;
	s_options_cdvolume_box.generic.x        = 0;
	s_options_cdvolume_box.generic.y        = 10;
	s_options_cdvolume_box.generic.name = "CD music";
	s_options_cdvolume_box.generic.callback = UpdateCDVolumeFunc;
	s_options_cdvolume_box.itemnames        = cd_music_items;
	s_options_cdvolume_box.curvalue         = !Cvar_VariableValue( "cd_nocd" );

	s_options_sensitivity_slider.generic.type   = MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x      = 0;
	s_options_sensitivity_slider.generic.y      = 50;
	s_options_sensitivity_slider.generic.name   = "mouse speed";
	s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	s_options_sensitivity_slider.minvalue       = 2;
	s_options_sensitivity_slider.maxvalue       = 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x   = 0;
	s_options_alwaysrun_box.generic.y   = 60;
	s_options_alwaysrun_box.generic.name    = "always run";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x = 0;
	s_options_invertmouse_box.generic.y = 70;
	s_options_invertmouse_box.generic.name  = "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	s_options_lookspring_box.generic.type = MTYPE_SPINCONTROL;
	s_options_lookspring_box.generic.x  = 0;
	s_options_lookspring_box.generic.y  = 80;
	s_options_lookspring_box.generic.name   = "lookspring";
	s_options_lookspring_box.generic.callback = LookspringFunc;
	s_options_lookspring_box.itemnames = yesno_names;

	s_options_freelook_box.generic.type = MTYPE_SPINCONTROL;
	s_options_freelook_box.generic.x    = 0;
	s_options_freelook_box.generic.y    = 100;
	s_options_freelook_box.generic.name = "free look";
	s_options_freelook_box.generic.callback = FreeLookFunc;
	s_options_freelook_box.itemnames = yesno_names;

	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x   = 0;
	s_options_crosshair_box.generic.y   = 110;
	s_options_crosshair_box.generic.name    = "crosshair";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = crosshair_names;

	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x    = 0;
	s_options_joystick_box.generic.y    = 120;
	s_options_joystick_box.generic.name = "use joystick";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;

	s_options_customize_options_action.generic.type = MTYPE_ACTION;
	s_options_customize_options_action.generic.x        = 0;
	s_options_customize_options_action.generic.y        = 140;
	s_options_customize_options_action.generic.name = "customize controls";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_defaults_action.generic.type  = MTYPE_ACTION;
	s_options_defaults_action.generic.x     = 0;
	s_options_defaults_action.generic.y     = 150;
	s_options_defaults_action.generic.name  = "reset defaults";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type   = MTYPE_ACTION;
	s_options_console_action.generic.x      = 0;
	s_options_console_action.generic.y      = 160;
	s_options_console_action.generic.name   = "go to console";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem( &s_options_menu, ( void* )&s_options_sfxvolume_slider );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_cdvolume_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_alwaysrun_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_invertmouse_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_lookspring_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_freelook_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_crosshair_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_joystick_box );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_customize_options_action );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_defaults_action );
	Menu_AddItem( &s_options_menu, ( void* )&s_options_console_action );
}

static void Options_MenuDraw() {
	MQ2_Banner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_menu, 1 );
	Menu_Draw( &s_options_menu );
}

static const char* Options_MenuKey( int key ) {
	return Default_MenuKey( &s_options_menu, key );
}

static void MQ2_Menu_Options_f() {
	Options_MenuInit();
	MQ2_PushMenu( Options_MenuDraw, Options_MenuKey, NULL );
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/

static const char* bindnames[][ 2 ] =
{
	{"+attack",         "attack"},
	{"weapnext",        "next weapon"},
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
	{"+moveup",         "up / jump"},
	{"+movedown",       "down / crouch"},

	{"inven",           "inventory"},
	{"invuse",          "use item"},
	{"invdrop",         "drop item"},
	{"invprev",         "prev item"},
	{"invnext",         "next item"},

	{"cmd help",        "help computer" },
	{"cmd help",        "help computer" },
	{ 0, 0 }
};

static int bind_grab;

static menuframework_s s_keys_menu;
static menuaction_s s_keys_attack_action;
static menuaction_s s_keys_change_weapon_action;
static menuaction_s s_keys_walk_forward_action;
static menuaction_s s_keys_backpedal_action;
static menuaction_s s_keys_turn_left_action;
static menuaction_s s_keys_turn_right_action;
static menuaction_s s_keys_run_action;
static menuaction_s s_keys_step_left_action;
static menuaction_s s_keys_step_right_action;
static menuaction_s s_keys_sidestep_action;
static menuaction_s s_keys_look_up_action;
static menuaction_s s_keys_look_down_action;
static menuaction_s s_keys_center_view_action;
static menuaction_s s_keys_mouse_look_action;
static menuaction_s s_keys_move_up_action;
static menuaction_s s_keys_move_down_action;
static menuaction_s s_keys_inventory_action;
static menuaction_s s_keys_inv_use_action;
static menuaction_s s_keys_inv_drop_action;
static menuaction_s s_keys_inv_prev_action;
static menuaction_s s_keys_inv_next_action;

static menuaction_s s_keys_help_computer_action;

static void KeyCursorDrawFunc( menuframework_s* menu ) {
	if ( bind_grab ) {
		UI_DrawChar( menu->x, menu->y + menu->cursor * 9, '=' );
	} else   {
		UI_DrawChar( menu->x, menu->y + menu->cursor * 9, 12 + ( ( Sys_Milliseconds() / 250 ) & 1 ) );
	}
}

static void DrawKeyBindingFunc( void* self ) {
	menuaction_s* a = ( menuaction_s* )self;

	int key0;
	int key1;
	Key_GetKeysForBinding( bindnames[ a->generic.localdata[ 0 ] ][ 0 ], &key0, &key1 );

	if ( key0 == -1 ) {
		UI_DrawString( a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, "???" );
	} else   {
		const char* name = Key_KeynumToString( key0, true );

		UI_DrawString( a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, name );

		int x = String::Length( name ) * 8;

		if ( key1 != -1 ) {
			UI_DrawString( a->generic.x + a->generic.parent->x + 24 + x, a->generic.y + a->generic.parent->y, "or" );
			UI_DrawString( a->generic.x + a->generic.parent->x + 48 + x, a->generic.y + a->generic.parent->y, Key_KeynumToString( key1, true ) );
		}
	}
}

static void KeyBindingFunc( void* self ) {
	menuaction_s* a = ( menuaction_s* )self;

	int key0;
	int key1;
	Key_GetKeysForBinding( bindnames[ a->generic.localdata[ 0 ] ][ 0 ], &key0, &key1 );

	if ( key1 != -1 ) {
		Key_UnbindCommand( bindnames[ a->generic.localdata[ 0 ] ][ 0 ] );
	}

	bind_grab = true;

	Menu_SetStatusBar( &s_keys_menu, "press a key or button for this action" );
}

static void Keys_MenuInit() {
	int y = 0;
	int i = 0;

	s_keys_menu.x = viddef.width * 0.50;
	s_keys_menu.nitems = 0;
	s_keys_menu.cursordraw = KeyCursorDrawFunc;

	s_keys_attack_action.generic.type   = MTYPE_ACTION;
	s_keys_attack_action.generic.flags  = QMF_GRAYED;
	s_keys_attack_action.generic.x      = 0;
	s_keys_attack_action.generic.y      = y;
	s_keys_attack_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack_action.generic.localdata[ 0 ] = i;
	s_keys_attack_action.generic.name   = bindnames[ s_keys_attack_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_change_weapon_action.generic.type    = MTYPE_ACTION;
	s_keys_change_weapon_action.generic.flags  = QMF_GRAYED;
	s_keys_change_weapon_action.generic.x       = 0;
	s_keys_change_weapon_action.generic.y       = y += 9;
	s_keys_change_weapon_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_action.generic.localdata[ 0 ] = ++i;
	s_keys_change_weapon_action.generic.name    = bindnames[ s_keys_change_weapon_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_walk_forward_action.generic.type = MTYPE_ACTION;
	s_keys_walk_forward_action.generic.flags  = QMF_GRAYED;
	s_keys_walk_forward_action.generic.x        = 0;
	s_keys_walk_forward_action.generic.y        = y += 9;
	s_keys_walk_forward_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_walk_forward_action.generic.localdata[ 0 ] = ++i;
	s_keys_walk_forward_action.generic.name = bindnames[ s_keys_walk_forward_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_backpedal_action.generic.type    = MTYPE_ACTION;
	s_keys_backpedal_action.generic.flags  = QMF_GRAYED;
	s_keys_backpedal_action.generic.x       = 0;
	s_keys_backpedal_action.generic.y       = y += 9;
	s_keys_backpedal_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_backpedal_action.generic.localdata[ 0 ] = ++i;
	s_keys_backpedal_action.generic.name    = bindnames[ s_keys_backpedal_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_turn_left_action.generic.type    = MTYPE_ACTION;
	s_keys_turn_left_action.generic.flags  = QMF_GRAYED;
	s_keys_turn_left_action.generic.x       = 0;
	s_keys_turn_left_action.generic.y       = y += 9;
	s_keys_turn_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_left_action.generic.localdata[ 0 ] = ++i;
	s_keys_turn_left_action.generic.name    = bindnames[ s_keys_turn_left_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_turn_right_action.generic.type   = MTYPE_ACTION;
	s_keys_turn_right_action.generic.flags  = QMF_GRAYED;
	s_keys_turn_right_action.generic.x      = 0;
	s_keys_turn_right_action.generic.y      = y += 9;
	s_keys_turn_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_right_action.generic.localdata[ 0 ] = ++i;
	s_keys_turn_right_action.generic.name   = bindnames[ s_keys_turn_right_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_run_action.generic.type  = MTYPE_ACTION;
	s_keys_run_action.generic.flags  = QMF_GRAYED;
	s_keys_run_action.generic.x     = 0;
	s_keys_run_action.generic.y     = y += 9;
	s_keys_run_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_run_action.generic.localdata[ 0 ] = ++i;
	s_keys_run_action.generic.name  = bindnames[ s_keys_run_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_step_left_action.generic.type    = MTYPE_ACTION;
	s_keys_step_left_action.generic.flags  = QMF_GRAYED;
	s_keys_step_left_action.generic.x       = 0;
	s_keys_step_left_action.generic.y       = y += 9;
	s_keys_step_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_left_action.generic.localdata[ 0 ] = ++i;
	s_keys_step_left_action.generic.name    = bindnames[ s_keys_step_left_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_step_right_action.generic.type   = MTYPE_ACTION;
	s_keys_step_right_action.generic.flags  = QMF_GRAYED;
	s_keys_step_right_action.generic.x      = 0;
	s_keys_step_right_action.generic.y      = y += 9;
	s_keys_step_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_right_action.generic.localdata[ 0 ] = ++i;
	s_keys_step_right_action.generic.name   = bindnames[ s_keys_step_right_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_sidestep_action.generic.type = MTYPE_ACTION;
	s_keys_sidestep_action.generic.flags  = QMF_GRAYED;
	s_keys_sidestep_action.generic.x        = 0;
	s_keys_sidestep_action.generic.y        = y += 9;
	s_keys_sidestep_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_sidestep_action.generic.localdata[ 0 ] = ++i;
	s_keys_sidestep_action.generic.name = bindnames[ s_keys_sidestep_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_look_up_action.generic.type  = MTYPE_ACTION;
	s_keys_look_up_action.generic.flags  = QMF_GRAYED;
	s_keys_look_up_action.generic.x     = 0;
	s_keys_look_up_action.generic.y     = y += 9;
	s_keys_look_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_up_action.generic.localdata[ 0 ] = ++i;
	s_keys_look_up_action.generic.name  = bindnames[ s_keys_look_up_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_look_down_action.generic.type    = MTYPE_ACTION;
	s_keys_look_down_action.generic.flags  = QMF_GRAYED;
	s_keys_look_down_action.generic.x       = 0;
	s_keys_look_down_action.generic.y       = y += 9;
	s_keys_look_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_down_action.generic.localdata[ 0 ] = ++i;
	s_keys_look_down_action.generic.name    = bindnames[ s_keys_look_down_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_center_view_action.generic.type  = MTYPE_ACTION;
	s_keys_center_view_action.generic.flags  = QMF_GRAYED;
	s_keys_center_view_action.generic.x     = 0;
	s_keys_center_view_action.generic.y     = y += 9;
	s_keys_center_view_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_center_view_action.generic.localdata[ 0 ] = ++i;
	s_keys_center_view_action.generic.name  = bindnames[ s_keys_center_view_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_mouse_look_action.generic.type   = MTYPE_ACTION;
	s_keys_mouse_look_action.generic.flags  = QMF_GRAYED;
	s_keys_mouse_look_action.generic.x      = 0;
	s_keys_mouse_look_action.generic.y      = y += 9;
	s_keys_mouse_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_mouse_look_action.generic.localdata[ 0 ] = ++i;
	s_keys_mouse_look_action.generic.name   = bindnames[ s_keys_mouse_look_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_move_up_action.generic.type  = MTYPE_ACTION;
	s_keys_move_up_action.generic.flags  = QMF_GRAYED;
	s_keys_move_up_action.generic.x     = 0;
	s_keys_move_up_action.generic.y     = y += 9;
	s_keys_move_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_up_action.generic.localdata[ 0 ] = ++i;
	s_keys_move_up_action.generic.name  = bindnames[ s_keys_move_up_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_move_down_action.generic.type    = MTYPE_ACTION;
	s_keys_move_down_action.generic.flags  = QMF_GRAYED;
	s_keys_move_down_action.generic.x       = 0;
	s_keys_move_down_action.generic.y       = y += 9;
	s_keys_move_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_down_action.generic.localdata[ 0 ] = ++i;
	s_keys_move_down_action.generic.name    = bindnames[ s_keys_move_down_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_inventory_action.generic.type    = MTYPE_ACTION;
	s_keys_inventory_action.generic.flags  = QMF_GRAYED;
	s_keys_inventory_action.generic.x       = 0;
	s_keys_inventory_action.generic.y       = y += 9;
	s_keys_inventory_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inventory_action.generic.localdata[ 0 ] = ++i;
	s_keys_inventory_action.generic.name    = bindnames[ s_keys_inventory_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_inv_use_action.generic.type  = MTYPE_ACTION;
	s_keys_inv_use_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_use_action.generic.x     = 0;
	s_keys_inv_use_action.generic.y     = y += 9;
	s_keys_inv_use_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_use_action.generic.localdata[ 0 ] = ++i;
	s_keys_inv_use_action.generic.name  = bindnames[ s_keys_inv_use_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_inv_drop_action.generic.type = MTYPE_ACTION;
	s_keys_inv_drop_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_drop_action.generic.x        = 0;
	s_keys_inv_drop_action.generic.y        = y += 9;
	s_keys_inv_drop_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_drop_action.generic.localdata[ 0 ] = ++i;
	s_keys_inv_drop_action.generic.name = bindnames[ s_keys_inv_drop_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_inv_prev_action.generic.type = MTYPE_ACTION;
	s_keys_inv_prev_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_prev_action.generic.x        = 0;
	s_keys_inv_prev_action.generic.y        = y += 9;
	s_keys_inv_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_prev_action.generic.localdata[ 0 ] = ++i;
	s_keys_inv_prev_action.generic.name = bindnames[ s_keys_inv_prev_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_inv_next_action.generic.type = MTYPE_ACTION;
	s_keys_inv_next_action.generic.flags  = QMF_GRAYED;
	s_keys_inv_next_action.generic.x        = 0;
	s_keys_inv_next_action.generic.y        = y += 9;
	s_keys_inv_next_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_next_action.generic.localdata[ 0 ] = ++i;
	s_keys_inv_next_action.generic.name = bindnames[ s_keys_inv_next_action.generic.localdata[ 0 ] ][ 1 ];

	s_keys_help_computer_action.generic.type    = MTYPE_ACTION;
	s_keys_help_computer_action.generic.flags  = QMF_GRAYED;
	s_keys_help_computer_action.generic.x       = 0;
	s_keys_help_computer_action.generic.y       = y += 9;
	s_keys_help_computer_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_help_computer_action.generic.localdata[ 0 ] = ++i;
	s_keys_help_computer_action.generic.name    = bindnames[ s_keys_help_computer_action.generic.localdata[ 0 ] ][ 1 ];

	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_attack_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_change_weapon_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_walk_forward_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_backpedal_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_turn_left_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_turn_right_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_run_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_step_left_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_step_right_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_sidestep_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_look_up_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_look_down_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_center_view_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_mouse_look_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_move_up_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_move_down_action );

	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_inventory_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_inv_use_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_inv_drop_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_inv_prev_action );
	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_inv_next_action );

	Menu_AddItem( &s_keys_menu, ( void* )&s_keys_help_computer_action );

	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
	Menu_Center( &s_keys_menu );
}

static void Keys_MenuDraw() {
	Menu_AdjustCursor( &s_keys_menu, 1 );
	Menu_Draw( &s_keys_menu );
}

static const char* Keys_MenuKey( int key ) {
	menuaction_s* item = ( menuaction_s* )Menu_ItemAtCursor( &s_keys_menu );

	if ( bind_grab ) {
		if ( key != K_ESCAPE && key != '`' ) {
			char cmd[ 1024 ];

			String::Sprintf( cmd, sizeof ( cmd ), "bind \"%s\" \"%s\"\n", Key_KeynumToString( key, false ), bindnames[ item->generic.localdata[ 0 ] ][ 0 ] );
			Cbuf_InsertText( cmd );
		}

		Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
		bind_grab = false;
		return menu_out_sound;
	}

	switch ( key ) {
	case K_KP_ENTER:
	case K_ENTER:
		KeyBindingFunc( item );
		return menu_in_sound;
	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
	case K_KP_DEL:
		Key_UnbindCommand( bindnames[ item->generic.localdata[ 0 ] ][ 0 ] );
		return menu_out_sound;
	default:
		return Default_MenuKey( &s_keys_menu, key );
	}
}

static void MQ2_Menu_Keys_f() {
	Keys_MenuInit();
	MQ2_PushMenu( Keys_MenuDraw, Keys_MenuKey, NULL );
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

static menuframework_s s_opengl_menu;

static menulist_s s_mode_list;
static menuslider_s s_tq_slider;
static menuslider_s s_screensize_slider;
static menuslider_s s_brightness_slider;
static menulist_s s_fs_box;
static menulist_s s_finish_box;
static menuaction_s s_cancel_action;
static menuaction_s s_defaults_action;

static void VID_MenuInit();

static void ScreenSizeCallback( void* s ) {
	menuslider_s* slider = ( menuslider_s* )s;

	Cvar_SetValueLatched( "viewsize", slider->curvalue * 10 );
}

static void BrightnessCallback( void* s ) {
	menuslider_s* slider = ( menuslider_s* )s;

	float gamma = ( 0.8 - ( slider->curvalue / 10.0 - 0.5 ) ) + 0.5;

	Cvar_SetValue( "r_gamma", gamma );
}

static void ResetDefaults( void* unused ) {
	VID_MenuInit();
}

static void ApplyChanges( void* unused ) {
	//	scale to a range of 0.8 to 1.5
	float gamma = s_brightness_slider.curvalue / 10.0;

	Cvar_SetValueLatched( "r_gamma", gamma );
	Cvar_SetValueLatched( "r_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValueLatched( "r_fullscreen", s_fs_box.curvalue );
	Cvar_SetValueLatched( "r_finish", s_finish_box.curvalue );
	Cvar_SetValueLatched( "r_mode", s_mode_list.curvalue );

	MQ2_ForceMenuOff();
}

static void CancelChanges( void* unused ) {
	MQ2_PopMenu();
}

static void VID_MenuInit() {
	static const char* resolutions[] =
	{
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 1024]",
		"[1600 1200]",
		0
	};
	static const char* yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	if ( !r_picmip ) {
		r_picmip = Cvar_Get( "r_picmip", "0", 0 );
	}
	if ( !r_mode ) {
		r_mode = Cvar_Get( "r_mode", "3", 0 );
	}
	if ( !r_finish ) {
		r_finish = Cvar_Get( "r_finish", "0", CVAR_ARCHIVE );
	}

	s_mode_list.curvalue = r_mode->value;

	if ( !scr_viewsize ) {
		scr_viewsize = Cvar_Get( "viewsize", "100", CVAR_ARCHIVE );
	}

	s_screensize_slider.curvalue = scr_viewsize->value / 10;

	s_opengl_menu.x = viddef.width * 0.50;
	s_opengl_menu.nitems = 0;

	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.generic.x = 0;
	s_mode_list.generic.y = 0;
	s_mode_list.itemnames = resolutions;

	s_screensize_slider.generic.type    = MTYPE_SLIDER;
	s_screensize_slider.generic.x       = 0;
	s_screensize_slider.generic.y       = 10;
	s_screensize_slider.generic.name    = "screen size";
	s_screensize_slider.minvalue = 3;
	s_screensize_slider.maxvalue = 12;
	s_screensize_slider.generic.callback = ScreenSizeCallback;

	s_brightness_slider.generic.type    = MTYPE_SLIDER;
	s_brightness_slider.generic.x   = 0;
	s_brightness_slider.generic.y   = 20;
	s_brightness_slider.generic.name    = "brightness";
	s_brightness_slider.generic.callback = BrightnessCallback;
	s_brightness_slider.minvalue = 8;
	s_brightness_slider.maxvalue = 20;
	s_brightness_slider.curvalue = r_gamma->value * 10;

	s_fs_box.generic.type = MTYPE_SPINCONTROL;
	s_fs_box.generic.x  = 0;
	s_fs_box.generic.y  = 30;
	s_fs_box.generic.name   = "fullscreen";
	s_fs_box.itemnames = yesno_names;
	s_fs_box.curvalue = r_fullscreen->value;

	s_tq_slider.generic.type    = MTYPE_SLIDER;
	s_tq_slider.generic.x       = 0;
	s_tq_slider.generic.y       = 40;
	s_tq_slider.generic.name    = "texture quality";
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;
	s_tq_slider.curvalue = 3 - r_picmip->value;

	s_finish_box.generic.type = MTYPE_SPINCONTROL;
	s_finish_box.generic.x  = 0;
	s_finish_box.generic.y  = 50;
	s_finish_box.generic.name   = "sync every frame";
	s_finish_box.curvalue = r_finish->value;
	s_finish_box.itemnames = yesno_names;

	s_defaults_action.generic.type = MTYPE_ACTION;
	s_defaults_action.generic.name = "reset to defaults";
	s_defaults_action.generic.x    = 0;
	s_defaults_action.generic.y    = 60;
	s_defaults_action.generic.callback = ResetDefaults;

	s_cancel_action.generic.type = MTYPE_ACTION;
	s_cancel_action.generic.name = "cancel";
	s_cancel_action.generic.x    = 0;
	s_cancel_action.generic.y    = 70;
	s_cancel_action.generic.callback = CancelChanges;

	Menu_AddItem( &s_opengl_menu, ( void* )&s_mode_list );
	Menu_AddItem( &s_opengl_menu, ( void* )&s_screensize_slider );
	Menu_AddItem( &s_opengl_menu, ( void* )&s_brightness_slider );
	Menu_AddItem( &s_opengl_menu, ( void* )&s_fs_box );
	Menu_AddItem( &s_opengl_menu, ( void* )&s_tq_slider );
	Menu_AddItem( &s_opengl_menu, ( void* )&s_finish_box );

	Menu_AddItem( &s_opengl_menu, ( void* )&s_defaults_action );
	Menu_AddItem( &s_opengl_menu, ( void* )&s_cancel_action );

	Menu_Center( &s_opengl_menu );
	s_opengl_menu.x -= 8;
}

static void VID_MenuDraw() {
	//	draw the banner
	int w, h;
	R_GetPicSize( &w, &h, "m_banner_video" );
	UI_DrawNamedPic( viddef.width / 2 - w / 2, viddef.height / 2 - 110, "m_banner_video" );

	//	move cursor to a reasonable starting position
	Menu_AdjustCursor( &s_opengl_menu, 1 );

	//	draw the menu
	Menu_Draw( &s_opengl_menu );
}

static const char* VID_MenuKey( int key ) {
	menuframework_s* m = &s_opengl_menu;
	static const char* sound = "misc/menu1.wav";

	switch ( key ) {
	case K_ESCAPE:
		ApplyChanges( 0 );
		return NULL;
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		Menu_AdjustCursor( m, -1 );
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		break;
	case K_KP_ENTER:
	case K_ENTER:
		if ( !Menu_SelectItem( m ) ) {
			ApplyChanges( NULL );
		}
		break;
	}

	return sound;
}

static void MQ2_Menu_Video_f() {
	VID_MenuInit();
	MQ2_PushMenu( VID_MenuDraw, VID_MenuKey, NULL );
}

/*
=======================================================================

QUIT MENU

=======================================================================
*/

static const char* M_Quit_Key( int key ) {
	switch ( key ) {
	case K_ESCAPE:
	case 'n':
	case 'N':
		MQ2_PopMenu();
		break;

	case 'Y':
	case 'y':
		Com_Quit_f();
		break;

	default:
		break;
	}

	return NULL;
}

static void M_Quit_Draw() {
	int w, h;
	R_GetPicSize( &w, &h, "quit" );
	UI_DrawNamedPic( ( viddef.width - w ) / 2, ( viddef.height - h ) / 2, "quit" );
}

static void MQ2_Menu_Quit_f() {
	MQ2_PushMenu( M_Quit_Draw, M_Quit_Key, NULL );
}

//=============================================================================
/* Menu Subsystem */

void MQ2_Init() {
	Cmd_AddCommand( "menu_main", MQ2_Menu_Main_f );
	Cmd_AddCommand( "menu_game", MQ2_Menu_Game_f );
	Cmd_AddCommand( "menu_loadgame", MQ2_Menu_LoadGame_f );
	Cmd_AddCommand( "menu_savegame", MQ2_Menu_SaveGame_f );
	Cmd_AddCommand( "menu_credits", MQ2_Menu_Credits_f );
	Cmd_AddCommand( "menu_multiplayer", MQ2_Menu_Multiplayer_f );
	Cmd_AddCommand( "menu_joinserver", MQ2_Menu_JoinServer_f );
	Cmd_AddCommand( "menu_addressbook", MQ2_Menu_AddressBook_f );
	Cmd_AddCommand( "menu_startserver", MQ2_Menu_StartServer_f );
	Cmd_AddCommand( "menu_dmoptions", MQ2_Menu_DMOptions_f );
	Cmd_AddCommand( "menu_playerconfig", MQ2_Menu_PlayerConfig_f );
	Cmd_AddCommand( "menu_downloadoptions", MQ2_Menu_DownloadOptions_f );
	Cmd_AddCommand( "menu_options", MQ2_Menu_Options_f );
	Cmd_AddCommand( "menu_keys", MQ2_Menu_Keys_f );
	Cmd_AddCommand( "menu_video", MQ2_Menu_Video_f );
	Cmd_AddCommand( "menu_quit", MQ2_Menu_Quit_f );
}

void MQ2_Draw() {
	if ( !( in_keyCatchers & KEYCATCH_UI ) ) {
		return;
	}

	// dim everything behind it down
	UI_Fill( 0, 0, viddef.width, viddef.height, 0, 0, 0, 0.8 );

	mq2_drawfunc();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if ( mq2_entersound ) {
		S_StartLocalSound( menu_in_sound );
		mq2_entersound = false;
	}
}

void MQ2_Keydown( int key ) {
	if ( mq2_keyfunc ) {
		const char* s = mq2_keyfunc( key );
		if ( s != 0 ) {
			S_StartLocalSound( ( char* )s );
		}
	}
}

void MQ2_CharEvent( int key ) {
	if ( mq2_charfunc ) {
		mq2_charfunc( key );
	}
}
