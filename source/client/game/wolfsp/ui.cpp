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

#include "../../client.h"
#include "local.h"
#include "ui_public.h"
#include "cg_ui_shared.h"

int UIWS_GetActiveMenu()
{
	return VM_Call(uivm, WSUI_GET_ACTIVE_MENU);
}

bool UIWS_ConsoleCommand(int realTime)
{
	return VM_Call(uivm, WSUI_CONSOLE_COMMAND, realTime);
}

void UIWS_DrawConnectScreen(bool overlay)
{
	VM_Call(uivm, WSUI_DRAW_CONNECT_SCREEN, overlay);
}

bool UIWS_HasUniqueCDKey()
{
	return VM_Call(uivm, WSUI_HASUNIQUECDKEY);
}

void CLWS_InGamePopup(char* menu)
{
	if (menu && !String::ICmp(menu, "briefing"))				//----(SA) added
	{
		UIT3_SetActiveMenu(WSUIMENU_BRIEFING);
		return;
	}

	if (cls.state == CA_ACTIVE && !clc.demoplaying)
	{
		// NERVE - SMF
		if (menu && !String::ICmp(menu, "UIMENU_WM_PICKTEAM"))
		{
			UIT3_SetActiveMenu(WSUIMENU_WM_PICKTEAM);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_PICKPLAYER"))
		{
			UIT3_SetActiveMenu(WSUIMENU_WM_PICKPLAYER);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_QUICKMESSAGE"))
		{
			UIT3_SetActiveMenu(WSUIMENU_WM_QUICKMESSAGE);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_LIMBO"))
		{
			UIT3_SetActiveMenu(WSUIMENU_WM_LIMBO);
		}
		// -NERVE - SMF
		else if (menu && !String::ICmp(menu, "hbook1"))				//----(SA)
		{
			UIT3_SetActiveMenu(WSUIMENU_BOOK1);
		}
		else if (menu && !String::ICmp(menu, "hbook2"))				//----(SA)
		{
			UIT3_SetActiveMenu(WSUIMENU_BOOK2);
		}
		else if (menu && !String::ICmp(menu, "hbook3"))				//----(SA)
		{
			UIT3_SetActiveMenu(WSUIMENU_BOOK3);
		}
		else if (menu && !String::ICmp(menu, "pregame"))					//----(SA) added
		{
			UIT3_SetActiveMenu(WSUIMENU_PREGAME);
		}
		else
		{
			UIT3_SetActiveMenu(WSUIMENU_CLIPBOARD);
		}
	}
}

void UIWS_SetEndGameMenu()
{
	UIT3_SetActiveMenu(WSUIMENU_ENDGAME);
}

void UIWS_KeyDownEvent(int key)
{
	if (UIWS_GetActiveMenu() == WSUIMENU_CLIPBOARD)
	{
		// any key gets out of clipboard
		key = K_ESCAPE;
	}
	else if (UIWS_GetActiveMenu() == WSUIMENU_PREGAME)
	{
		if (key != K_MOUSE1)
		{
			return;	// eat all keys except mouse click
		}
	}
	else
	{
		// when in the notebook, check for the key bound to "notebook" and allow that as an escape key
		const char* kb = keys[key].binding;
		if (kb)
		{
			if (!String::ICmp("notebook", kb))
			{
				if (UIWS_GetActiveMenu() == WSUIMENU_NOTEBOOK)
				{
					key = K_ESCAPE;
				}
			}
		}
	}

	UIT3_KeyEvent(key, true);
}

static void UIWS_Notebook()
{
	if (cls.state == CA_ACTIVE && !clc.demoplaying)
	{
		Cvar_Set("cg_youGotMail", "0");		// clear icon	//----(SA)	added
		UIT3_SetActiveMenu(WSUIMENU_NOTEBOOK);		// startup notebook
	}
}

void UIWS_Init()
{
	Cmd_AddCommand("notebook", UIWS_Notebook);
}

static int CLWS_GetConfigString(int index, char* buf, int size)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS_WS)
	{
		return false;
	}

	int offset = cl.ws_gameState.stringOffsets[index];
	if (!offset)
	{
		if (size)
		{
			buf[0] = 0;
		}
		return false;
	}

	String::NCpyZ(buf, cl.ws_gameState.stringData + offset, size);

	return true;
}

//	The ui module is making a system call
qintptr CLWS_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case WSUI_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;

	case WSUI_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;

	case WSUI_MILLISECONDS:
		return Sys_Milliseconds();

	case WSUI_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;

	case WSUI_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;

	case WSUI_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;

	case WSUI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue((char*)VMA(1)));

	case WSUI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case WSUI_CVAR_SETVALUE:
		Cvar_SetValue((char*)VMA(1), VMF(2));
		return 0;

	case WSUI_CVAR_RESET:
		Cvar_Reset((char*)VMA(1));
		return 0;

	case WSUI_CVAR_CREATE:
		Cvar_Get((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case WSUI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], MAX_INFO_STRING_Q3, (char*)VMA(2), args[3]);
		return 0;

	case WSUI_ARGC:
		return Cmd_Argc();

	case WSUI_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WSUI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case WSUI_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);

	case WSUI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

	case WSUI_FS_SEEK:
		FS_Seek(args[1], args[2], args[3]);
		return 0;

	case WSUI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case WSUI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;

	case WSUI_FS_DELETEFILE:
		return FS_Delete((char*)VMA(1));

	case WSUI_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case WSUI_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));

	case WSUI_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));

	case WSUI_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));

	case WSUI_R_CLEARSCENE:
		R_ClearScene();
		return 0;

	case WSUI_R_ADDREFENTITYTOSCENE:
		CLWS_AddRefEntityToScene((wsrefEntity_t*)VMA(1));
		return 0;

	case WSUI_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;

	case WSUI_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;

	case WSUI_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6]);
		return 0;

	case WSUI_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;

	case WSUI_R_RENDERSCENE:
		CLWS_RenderScene((wsrefdef_t*)VMA(1));
		return 0;

	case WSUI_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;

	case WSUI_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case WSUI_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;

//-------

	case WSUI_CM_LERPTAG:
		return CLWS_LerpTag((orientation_t*)VMA(1), (wsrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);

	case WSUI_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));

	case WSUI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;

	case WSUI_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);
		return 0;

	case WSUI_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], false);
		return 0;

	case WSUI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WSUI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WSUI_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;

	case WSUI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case WSUI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case WSUI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case WSUI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case WSUI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case WSUI_KEY_SETCATCHER:
		KeyQ3_SetCatcher(args[1]);
		return 0;

	case WSUI_GETCLIPBOARDDATA:
		CLT3_GetClipboardData((char*)VMA(1), args[2]);
		return 0;

	case WSUI_GETCLIENTSTATE:
		UIT3_GetClientState((uiClientState_t*)VMA(1));
		return 0;

	case WSUI_GETGLCONFIG:
		CLWS_GetGlconfig((wsglconfig_t*)VMA(1));
		return 0;

	case WSUI_GETCONFIGSTRING:
		return CLWS_GetConfigString(args[1], (char*)VMA(2), args[3]);

	case WSUI_LAN_LOADCACHEDSERVERS:
		//	Not applicable to single player
		return 0;

	case WSUI_LAN_SAVECACHEDSERVERS:
		//	Not applicable to single player
		return 0;

	case WSUI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (char*)VMA(2), (char*)VMA(3));

	case WSUI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (char*)VMA(2));
		return 0;

	case WSUI_LAN_GETPINGQUEUECOUNT:
		return CLT3_GetPingQueueCount();

	case WSUI_LAN_CLEARPING:
		CLT3_ClearPing(args[1]);
		return 0;

	case WSUI_LAN_GETPING:
		CLT3_GetPing(args[1], (char*)VMA(2), args[3], (int*)VMA(4));
		return 0;

	case WSUI_LAN_GETPINGINFO:
		CLT3_GetPingInfo(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WSUI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case WSUI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case WSUI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case WSUI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);

	case WSUI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], args[3]);
		return 0;

	case WSUI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);

	case WSUI_LAN_UPDATEVISIBLEPINGS:
		return CLT3_UpdateVisiblePings(args[1]);

	case WSUI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;

	case WSUI_LAN_SERVERSTATUS:
		return CLT3_ServerStatus((char*)VMA(1), (char*)VMA(2), args[3]);

	case WSUI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);

	case WSUI_MEMORY_REMAINING:
		return 0x4000000;

	case WSUI_GET_CDKEY:
		CLT3UI_GetCDKey((char*)VMA(1), args[2]);
		return 0;

	case WSUI_SET_CDKEY:
		CLT3UI_SetCDKey((char*)VMA(1));
		return 0;

	case WSUI_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;

	case WSUI_MEMSET:
		return (qintptr)memset(VMA(1), args[2], args[3]);

	case WSUI_MEMCPY:
		return (qintptr)memcpy(VMA(1), VMA(2), args[3]);

	case WSUI_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];

	case WSUI_SIN:
		return FloatAsInt(sin(VMF(1)));

	case WSUI_COS:
		return FloatAsInt(cos(VMF(1)));

	case WSUI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case WSUI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case WSUI_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case WSUI_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case WSUI_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case WSUI_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case WSUI_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case WSUI_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case WSUI_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case WSUI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case WSUI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), args[3]);			//----(SA)	added fadeup time
		return 0;

	case WSUI_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));

	case WSUI_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematicStretched((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case WSUI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case WSUI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case WSUI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case WSUI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case WSUI_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

//-------

	default:
		common->Error("Bad UI system trap: %i", static_cast<int>(args[0]));
	}
	return 0;
}
