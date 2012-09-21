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

#ifndef _ET_UI_PUBLIC_H
#define _ET_UI_PUBLIC_H

#include "../tech3/ui_shared.h"

enum
{
	ETUIMENU_BAD_CD_KEY = 4,
	ETUIMENU_TEAM,
	ETUIMENU_POSTGAME,
	ETUIMENU_HELP,
};

#define ETUI_API_VERSION  4

enum
{
	ETUI_ERROR,
	ETUI_PRINT,
	ETUI_MILLISECONDS,
	ETUI_CVAR_SET,
	ETUI_CVAR_VARIABLEVALUE,
	ETUI_CVAR_VARIABLESTRINGBUFFER,
	ETUI_CVAR_LATCHEDVARIABLESTRINGBUFFER,
	ETUI_CVAR_SETVALUE,
	ETUI_CVAR_RESET,
	ETUI_CVAR_CREATE,
	ETUI_CVAR_INFOSTRINGBUFFER,
	ETUI_ARGC,
	ETUI_ARGV,
	ETUI_CMD_EXECUTETEXT,
	ETUI_ADDCOMMAND,
	ETUI_FS_FOPENFILE,
	ETUI_FS_READ,
	ETUI_FS_WRITE,
	ETUI_FS_FCLOSEFILE,
	ETUI_FS_GETFILELIST,
	ETUI_FS_DELETEFILE,
	ETUI_FS_COPYFILE,
	ETUI_R_REGISTERMODEL,
	ETUI_R_REGISTERSKIN,
	ETUI_R_REGISTERSHADERNOMIP,
	ETUI_R_CLEARSCENE,
	ETUI_R_ADDREFENTITYTOSCENE,
	ETUI_R_ADDPOLYTOSCENE,
	ETUI_R_ADDPOLYSTOSCENE,
	// JOSEPH 12-6-99
	ETUI_R_ADDLIGHTTOSCENE,
	// END JOSEPH
	//----(SA)
	ETUI_R_ADDCORONATOSCENE,
	//----(SA)
	ETUI_R_RENDERSCENE,
	ETUI_R_SETCOLOR,
	ETUI_R_DRAW2DPOLYS,
	ETUI_R_DRAWSTRETCHPIC,
	ETUI_R_DRAWROTATEDPIC,
	ETUI_UPDATESCREEN,		// 30
	ETUI_CM_LERPTAG,
	ETUI_CM_LOADMODEL,
	ETUI_S_REGISTERSOUND,
	ETUI_S_STARTLOCALSOUND,
	ETUI_S_FADESTREAMINGSOUND,	//----(SA)	added
	ETUI_S_FADEALLSOUNDS,			//----(SA)	added
	ETUI_KEY_KEYNUMTOSTRINGBUF,
	ETUI_KEY_GETBINDINGBUF,
	ETUI_KEY_SETBINDING,
	ETUI_KEY_BINDINGTOKEYS,
	ETUI_KEY_ISDOWN,
	ETUI_KEY_GETOVERSTRIKEMODE,
	ETUI_KEY_SETOVERSTRIKEMODE,
	ETUI_KEY_CLEARSTATES,
	ETUI_KEY_GETCATCHER,
	ETUI_KEY_SETCATCHER,
	ETUI_GETCLIPBOARDDATA,
	ETUI_GETGLCONFIG,
	ETUI_GETCLIENTSTATE,
	ETUI_GETCONFIGSTRING,
	ETUI_LAN_GETLOCALSERVERCOUNT,
	ETUI_LAN_GETLOCALSERVERADDRESSSTRING,
	ETUI_LAN_GETGLOBALSERVERCOUNT,		// 50
	ETUI_LAN_GETGLOBALSERVERADDRESSSTRING,
	ETUI_LAN_GETPINGQUEUECOUNT,
	ETUI_LAN_CLEARPING,
	ETUI_LAN_GETPING,
	ETUI_LAN_GETPINGINFO,
	ETUI_CVAR_REGISTER,
	ETUI_CVAR_UPDATE,
	ETUI_MEMORY_REMAINING,

	ETUI_GET_CDKEY,
	ETUI_SET_CDKEY,
	ETUI_R_REGISTERFONT,
	ETUI_R_MODELBOUNDS,
	ETUI_PC_ADD_GLOBAL_DEFINE,
	ETUI_PC_REMOVE_ALL_GLOBAL_DEFINES,
	ETUI_PC_LOAD_SOURCE,
	ETUI_PC_FREE_SOURCE,
	ETUI_PC_READ_TOKEN,
	ETUI_PC_SOURCE_FILE_AND_LINE,
	ETUI_PC_UNREAD_TOKEN,
	ETUI_S_STOPBACKGROUNDTRACK,
	ETUI_S_STARTBACKGROUNDTRACK,
	ETUI_REAL_TIME,
	ETUI_LAN_GETSERVERCOUNT,
	ETUI_LAN_GETSERVERADDRESSSTRING,
	ETUI_LAN_GETSERVERINFO,
	ETUI_LAN_MARKSERVERVISIBLE,
	ETUI_LAN_UPDATEVISIBLEPINGS,
	ETUI_LAN_RESETPINGS,
	ETUI_LAN_LOADCACHEDSERVERS,
	ETUI_LAN_SAVECACHEDSERVERS,
	ETUI_LAN_ADDSERVER,
	ETUI_LAN_REMOVESERVER,
	ETUI_CIN_PLAYCINEMATIC,
	ETUI_CIN_STOPCINEMATIC,
	ETUI_CIN_RUNCINEMATIC,
	ETUI_CIN_DRAWCINEMATIC,
	ETUI_CIN_SETEXTENTS,
	ETUI_R_REMAP_SHADER,
	ETUI_VERIFY_CDKEY,
	ETUI_LAN_SERVERSTATUS,
	ETUI_LAN_GETSERVERPING,
	ETUI_LAN_SERVERISVISIBLE,
	ETUI_LAN_COMPARESERVERS,
	ETUI_LAN_SERVERISINFAVORITELIST,
	ETUI_CL_GETLIMBOSTRING,			// NERVE - SMF
	ETUI_SET_PBCLSTATUS,				// DHM - Nerve
	ETUI_CHECKAUTOUPDATE,				// DHM - Nerve
	ETUI_GET_AUTOUPDATE,				// DHM - Nerve
	ETUI_CL_TRANSLATE_STRING,
	ETUI_OPENURL,
	ETUI_SET_PBSVSTATUS,				// TTimo

	ETUI_MEMSET = 200,
	ETUI_MEMCPY,
	ETUI_STRNCPY,
	ETUI_SIN,
	ETUI_COS,
	ETUI_ATAN2,
	ETUI_SQRT,
	ETUI_FLOOR,
	ETUI_CEIL,
	ETUI_GETHUNKDATA

};

enum
{

	ETUI_GET_ACTIVE_MENU = 8,
//	void	ETUI_GetActiveMenu( void );

	ETUI_CONSOLE_COMMAND,
//	qboolean UI_ConsoleCommand( void );

	ETUI_DRAW_CONNECT_SCREEN,
//	void	ETUI_DrawConnectScreen( qboolean overlay );
	ETUI_HASUNIQUECDKEY,
// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings
	ETUI_CHECKEXECKEY,		// NERVE - SMF

	ETUI_WANTSBINDKEYS,
};

#endif
