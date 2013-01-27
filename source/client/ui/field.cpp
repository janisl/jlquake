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

#include "ui.h"
#include "../client_main.h"
#include "../system.h"
#include "../input/keycodes.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"

static void Field_Paste( field_t* edit ) {
	char* cbd;
	int pasteLen, i;

	cbd = Sys_GetClipboardData();

	if ( !cbd ) {
		return;
	}

	// send as if typed, so insert / overstrike works properly
	pasteLen = String::Length( cbd );
	for ( i = 0; i < pasteLen; i++ ) {
		Field_CharEvent( edit, cbd[ i ] );
	}

	delete[] cbd;
}

static void Field_Home( field_t* edit ) {
	edit->cursor = 0;
	edit->scroll = 0;
}

static void Field_End( field_t* edit, int len ) {
	edit->cursor = len;
	edit->scroll = edit->cursor - edit->widthInChars;
	if ( edit->scroll < 0 ) {
		edit->scroll = 0;
	}
}

//	Performs the basic line editing functions for the console,
// in-game talk, and menu fields
//	Key events are used for non-printable characters, others are gotten from char events.
bool Field_KeyDownEvent( field_t* edit, int key ) {
	// shift-insert is paste
	if ( ( ( key == K_INS ) || ( key == K_KP_INS ) ) && keys[ K_SHIFT ].down ) {
		Field_Paste( edit );
		return true;
	}

	int len = String::Length( edit->buffer );

	if ( key == K_DEL || key == K_KP_DEL ) {
		if ( edit->cursor < len ) {
			memmove( edit->buffer + edit->cursor,
				edit->buffer + edit->cursor + 1, len - edit->cursor );
		}
		return true;
	}

	if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
		if ( edit->cursor < len ) {
			edit->cursor++;
		}

		if ( edit->cursor >= edit->scroll + edit->widthInChars && edit->cursor <= len ) {
			edit->scroll++;
		}
		return true;
	}

	if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
		if ( edit->cursor > 0 ) {
			edit->cursor--;
		}
		if ( edit->cursor < edit->scroll ) {
			edit->scroll--;
		}
		return true;
	}

	if ( key == K_HOME || key == K_KP_HOME ) {
		Field_Home( edit );
		return true;
	}

	if ( key == K_END || key == K_KP_END ) {
		Field_End( edit, len );
		return true;
	}

	if ( key == K_INS ) {
		key_overstrikeMode = !key_overstrikeMode;
		return true;
	}
	return false;
}

void Field_CharEvent( field_t* edit, int ch ) {
	if ( ch == 'v' - 'a' + 1 ) {		// ctrl-v is paste
		Field_Paste( edit );
		return;
	}

	if ( ch == 'c' - 'a' + 1 ) {		// ctrl-c clears the field
		Field_Clear( edit );
		return;
	}

	int len = String::Length( edit->buffer );

	if ( ch == 'h' - 'a' + 1 ) {		// ctrl-h is backspace
		if ( edit->cursor > 0 ) {
			memmove( edit->buffer + edit->cursor - 1,
				edit->buffer + edit->cursor, len + 1 - edit->cursor );
			edit->cursor--;
			if ( edit->cursor < edit->scroll ) {
				edit->scroll--;
			}
		}
		return;
	}

	if ( ch == 'a' - 'a' + 1 ) {		// ctrl-a is home
		Field_Home( edit );
		return;
	}

	if ( ch == 'e' - 'a' + 1 ) {		// ctrl-e is end
		Field_End( edit, len );
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( ch < 32 ) {
		return;
	}

	int maxLen = edit->maxLength ? edit->maxLength : MAX_EDIT_LINE - 1;
	if ( key_overstrikeMode ) {
		if ( edit->cursor == maxLen ) {
			return;
		}
		edit->buffer[ edit->cursor ] = ch;
		edit->cursor++;
	} else   {		// insert mode
		if ( len == maxLen ) {
			return;	// all full
		}
		memmove( edit->buffer + edit->cursor + 1,
			edit->buffer + edit->cursor, len + 1 - edit->cursor );
		edit->buffer[ edit->cursor ] = ch;
		edit->cursor++;
	}

	if ( edit->cursor >= edit->widthInChars ) {
		edit->scroll++;
	}

	if ( edit->cursor == len + 1 ) {
		edit->buffer[ edit->cursor ] = 0;
	}
}

//	Handles horizontal scrolling and cursor blinking
static void Field_VariableSizeDraw( field_t* edit, int x, int y, int size, bool showCursor ) {
	int drawLen = edit->widthInChars;
	int len = String::Length( edit->buffer ) + 1;

	int prestep;
	// guarantee that cursor will be visible
	if ( len <= drawLen ) {
		prestep = 0;
	} else   {
		if ( edit->scroll + drawLen > len ) {
			edit->scroll = len - drawLen;
			if ( edit->scroll < 0 ) {
				edit->scroll = 0;
			}
		}
		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len ) {
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	if ( drawLen >= MAX_STRING_CHARS ) {
		common->Error( "drawLen >= MAX_STRING_CHARS" );
	}

	char str[ MAX_STRING_CHARS ];
	Com_Memcpy( str, edit->buffer + prestep, drawLen );
	str[ drawLen ] = 0;

	// draw it
	if ( !( GGameType & GAME_Tech3 ) ) {
		UI_DrawString( x, y, str );
	} else if ( size == SMALLCHAR_WIDTH )     {
		SCR_DrawSmallString( x, y, str );
	} else   {
		// draw big string with drop shadow
		SCR_DrawBigString( x, y, str, 1.0 );
	}

	// draw the cursor
	if ( !showCursor ) {
		return;
	}

	if ( ( cls.realtime >> 8 ) & 1 ) {
		return;		// off blink
	}

	//	Fonts in older games have space as character 10
	int cursorChar = ( !( GGameType & GAME_Tech3 ) || key_overstrikeMode ) ? 11 : 10;

	if ( GGameType & GAME_Tech3 ) {
		int i = drawLen - ( String::LengthWithoutColours( str ) + 1 );

		if ( size == SMALLCHAR_WIDTH ) {
			SCR_DrawSmallChar( x + ( edit->cursor - prestep - i ) * size, y, cursorChar );
		} else   {
			str[ 0 ] = cursorChar;
			str[ 1 ] = 0;
			SCR_DrawBigString( x + ( edit->cursor - prestep - i ) * size, y, str, 1.0 );
		}
	} else   {
		UI_DrawChar( x + ( edit->cursor - edit->scroll ) * 8, y, cursorChar );
	}
}

void Field_Draw( field_t* edit, int x, int y, bool showCursor ) {
	Field_VariableSizeDraw( edit, x, y, SMALLCHAR_WIDTH, showCursor );
}

void Field_BigDraw( field_t* edit, int x, int y, bool showCursor ) {
	Field_VariableSizeDraw( edit, x, y, BIGCHAR_WIDTH, showCursor );
}
