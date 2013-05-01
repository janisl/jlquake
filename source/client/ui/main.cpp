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
#include "../game/quake_hexen2/menu.h"
#include "../game/quake/local.h"
#include "../game/hexen2/local.h"
#include "../game/quake2/menu.h"
#include "../game/tech3/local.h"
#include "../input/keycodes.h"
#include "../../common/common_defs.h"

void UI_Init() {
	if ( GGameType & GAME_QuakeHexen ) {
		MQH_Init();
	} else if ( GGameType & GAME_Quake2 ) {
		MQ2_Init();
	} else {
		UIT3_Init();
	}
}

void UI_KeyEvent( int key, bool down ) {
	if ( GGameType & GAME_Tech3 ) {
		UIT3_KeyEvent( key, down );
	}
}

void UI_KeyDownEvent( int key ) {
	if ( GGameType & GAME_QuakeHexen ) {
		MQH_Keydown( key );
	} else if ( GGameType & GAME_Quake2 ) {
		MQ2_Keydown( key );
	} else {
		UIT3_KeyDownEvent( key );
	}
}

void UI_CharEvent( int key ) {
	if ( GGameType & GAME_QuakeHexen ) {
		MQH_CharEvent( key );
	} else if ( GGameType & GAME_Quake2 ) {
		MQ2_CharEvent( key );
	} else {
		UIT3_KeyEvent( key | K_CHAR_FLAG, true );
	}
}

void UI_MouseEvent( int dx, int dy ) {
	if ( GGameType & GAME_Tech3 ) {
		UIT3_MouseEvent( dx, dy );
	}
}

void UI_DrawMenu() {
	if ( GGameType & GAME_QuakeHexen ) {
		MQH_Draw();
	} else if ( GGameType & GAME_Quake2 ) {
		MQ2_Draw();
	} else {
		UIT3_Refresh( cls.realtime );
	}
}

bool UI_IsFullscreen() {
	if ( !( GGameType & GAME_Tech3 ) ) {
		return false;
	}
	return UIT3_IsFullscreen();
}

void UI_ForceMenuOff() {
	if ( GGameType & GAME_Quake2 ) {
		MQ2_ForceMenuOff();
	} else if ( GGameType & GAME_Tech3 ) {
		UIT3_ForceMenuOff();
	}
}

void UI_SetMainMenu() {
	if ( GGameType & GAME_QuakeHexen ) {
		MQH_Menu_Main_f();
	} else if ( GGameType & GAME_Quake2 ) {
		MQ2_Menu_Main_f();
	} else {
		UIT3_SetMainMenu();
	}
}

void UI_SetInGameMenu() {
	if ( GGameType & GAME_QuakeHexen ) {
		MQH_Menu_Main_f();
	} else if ( GGameType & GAME_Quake2 ) {
		MQ2_Menu_Main_f();
	} else {
		UIT3_SetInGameMenu();
	}
}

void CL_InitRenderer() {
	// this sets up the renderer and calls R_Init
	R_BeginRegistration( &cls.glconfig );

	if ( GGameType & GAME_QuakeHexen ) {
		Cvar* conwidth = Cvar_Get( "conwidth", "0", 0 );
		Cvar* conheight = Cvar_Get( "conheight", "0", 0 );
		if ( conwidth->integer ) {
			viddef.width = conwidth->integer;
		} else {
			viddef.width = 640;
		}

		viddef.width &= 0xfff8;	// make it a multiple of eight

		if ( viddef.width < 320 ) {
			viddef.width = 320;
		}

		// pick a conheight that matches with correct aspect
		viddef.height = viddef.width / cls.glconfig.windowAspect;

		if ( conheight->integer ) {
			viddef.height = conheight->integer;
		}
		if ( viddef.height < 200 ) {
			viddef.height = 200;
		}

		if ( viddef.height > cls.glconfig.vidHeight ) {
			viddef.height = cls.glconfig.vidHeight;
		}
		if ( viddef.width > cls.glconfig.vidWidth ) {
			viddef.width = cls.glconfig.vidWidth;
		}

		SCRQH_InitImages();
		if ( GGameType & GAME_Hexen2 ) {
			CLH2_InitPlayerTranslation();
			CLH2_ClearEntityTextureArrays();
			SbarH2_InitImages();
		} else {
			Com_Memset( clq1_playertextures, 0, sizeof ( clq1_playertextures ) );
			SbarQ1_InitImages();
		}
	} else if ( GGameType & GAME_Quake2 ) {
		viddef.width = cls.glconfig.vidWidth;
		viddef.height = cls.glconfig.vidHeight;

		char_texture = R_LoadQuake2FontImage( "pics/conchars.pcx" );
	} else {
		// all drawing is done to a 480 pixels high virtual screen size
		// and will be automatically scaled to the real resolution
		viddef.width = 480 * cls.glconfig.windowAspect;
		viddef.height = 480;

		// load character sets
		cls.whiteShader = R_RegisterShader( "white" );
		if ( GGameType & GAME_WolfSP ) {
			cls.charSetShader = R_RegisterShader( "gfx/2d/bigchars" );
			cls.consoleShader = R_RegisterShader( "console" );
			cls.consoleShader2 = R_RegisterShader( "console2" );
		} else if ( GGameType & GAME_WolfMP ) {
			cls.charSetShader = R_RegisterShader( "gfx/2d/hudchars" );
			cls.consoleShader = R_RegisterShader( "console-16bit" );	// JPW NERVE shader works with 16bit
			cls.consoleShader2 = R_RegisterShader( "console2-16bit" );		// JPW NERVE same
		} else if ( GGameType & GAME_ET ) {
			cls.charSetShader = R_RegisterShader( "gfx/2d/consolechars" );
			cls.consoleShader = R_RegisterShader( "console-16bit" );	// JPW NERVE shader works with 16bit
			cls.consoleShader2 = R_RegisterShader( "console2-16bit" );		// JPW NERVE same
		} else {
			cls.charSetShader = R_RegisterShader( "gfx/2d/bigchars" );
			cls.consoleShader = R_RegisterShader( "console" );
		}
	}
}
