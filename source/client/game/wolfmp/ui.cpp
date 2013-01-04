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

#include "local.h"
#include "../../input/keycodes.h"
#include "../../cinematic/public.h"
#include "../../ui/ui.h"
#include "../../translate.h"
#include "ui_public.h"
#include "cg_ui_shared.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/precompiler.h"
#include "../../../common/system.h"

int UIWM_GetActiveMenu()
{
	return VM_Call(uivm, WMUI_GET_ACTIVE_MENU);
}

bool UIWM_ConsoleCommand(int realTime)
{
	return VM_Call(uivm, WMUI_CONSOLE_COMMAND, realTime);
}

void UIWM_DrawConnectScreen(bool overlay)
{
	VM_Call(uivm, WMUI_DRAW_CONNECT_SCREEN, overlay);
}

bool UIWM_HasUniqueCDKey()
{
	return VM_Call(uivm, WMUI_HASUNIQUECDKEY);
}

bool UIWM_CheckExecKey(int key)
{
	return VM_Call(uivm, WMUI_CHECKEXECKEY, key);
}

void CLWM_InGamePopup(char* menu)
{
	if (cls.state == CA_ACTIVE && !clc.demoplaying)
	{
		// NERVE - SMF
		if (menu && !String::ICmp(menu, "UIMENU_WM_PICKTEAM"))
		{
			UIT3_SetActiveMenu(WMUIMENU_WM_PICKTEAM);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_PICKPLAYER"))
		{
			UIT3_SetActiveMenu(WMUIMENU_WM_PICKPLAYER);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_QUICKMESSAGE"))
		{
			UIT3_SetActiveMenu(WMUIMENU_WM_QUICKMESSAGE);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_QUICKMESSAGEALT"))
		{
			UIT3_SetActiveMenu(WMUIMENU_WM_QUICKMESSAGEALT);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_LIMBO"))
		{
			UIT3_SetActiveMenu(WMUIMENU_WM_LIMBO);
		}
		else if (menu && !String::ICmp(menu, "UIMENU_WM_AUTOUPDATE"))
		{
			UIT3_SetActiveMenu(WMUIMENU_WM_AUTOUPDATE);
		}
		// -NERVE - SMF
		else if (menu && !String::ICmp(menu, "hbook1"))				//----(SA)
		{
			UIT3_SetActiveMenu(WMUIMENU_BOOK1);
		}
		else if (menu && !String::ICmp(menu, "hbook2"))				//----(SA)
		{
			UIT3_SetActiveMenu(WMUIMENU_BOOK2);
		}
		else if (menu && !String::ICmp(menu, "hbook3"))				//----(SA)
		{
			UIT3_SetActiveMenu(WMUIMENU_BOOK3);
		}
		else
		{
			UIT3_SetActiveMenu(WMUIMENU_CLIPBOARD);
		}
	}
}

void CLWM_InGameClosePopup(char* menu)
{
	// if popup menu is up, then close it
	if (menu && !String::ICmp(menu, "UIMENU_WM_LIMBO"))
	{
		if (UIWM_GetActiveMenu() == WMUIMENU_WM_LIMBO)
		{
			UIT3_KeyEvent(K_ESCAPE, true);
			UIT3_KeyEvent(K_ESCAPE, true);
		}
	}
}

void UIWM_KeyDownEvent(int key)
{
	if (UIWM_GetActiveMenu() == WMUIMENU_CLIPBOARD)
	{
		// any key gets out of clipboard
		key = K_ESCAPE;
	}
	else
	{
		// when in the notebook, check for the key bound to "notebook" and allow that as an escape key
		const char* kb = keys[key].binding;
		if (kb)
		{
			if (!String::ICmp("notebook", kb))
			{
				if (UIWM_GetActiveMenu() == WMUIMENU_NOTEBOOK)
				{
					key = K_ESCAPE;
				}
			}

			if (!String::ICmp("help", kb))
			{
				if (UIWM_GetActiveMenu() == WMUIMENU_HELP)
				{
					key = K_ESCAPE;
				}
			}
		}
	}

	UIT3_KeyEvent(key, true);
}

static void UIWM_Help()
{
	if (cls.state == CA_ACTIVE && !clc.demoplaying)
	{
		UIT3_SetActiveMenu(WMUIMENU_HELP);			// startup help system
	}
}

void UIWM_Init()
{
	Cmd_AddCommand("help", UIWM_Help);
}

static int CLWM_GetConfigString(int index, char* buf, int size)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS_WM)
	{
		return false;
	}

	int offset = cl.wm_gameState.stringOffsets[index];
	if (!offset)
	{
		if (size)
		{
			buf[0] = 0;
		}
		return false;
	}

	String::NCpyZ(buf, cl.wm_gameState.stringData + offset, size);

	return true;
}

//	The ui module is making a system call
qintptr CLWM_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case WMUI_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;

	case WMUI_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;

	case WMUI_MILLISECONDS:
		return Sys_Milliseconds();

	case WMUI_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;

	case WMUI_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;

	case WMUI_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;

	case WMUI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue((char*)VMA(1)));

	case WMUI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case WMUI_CVAR_SETVALUE:
		Cvar_SetValue((char*)VMA(1), VMF(2));
		return 0;

	case WMUI_CVAR_RESET:
		Cvar_Reset((char*)VMA(1));
		return 0;

	case WMUI_CVAR_CREATE:
		Cvar_Get((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case WMUI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], MAX_INFO_STRING_Q3, (char*)VMA(2), args[3]);
		return 0;

	case WMUI_ARGC:
		return Cmd_Argc();

	case WMUI_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WMUI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case WMUI_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);

	case WMUI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

	case WMUI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case WMUI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;

	case WMUI_FS_DELETEFILE:
		return FS_Delete((char*)VMA(1));

	case WMUI_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case WMUI_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));

	case WMUI_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));

	case WMUI_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));

	case WMUI_R_CLEARSCENE:
		R_ClearScene();
		return 0;

	case WMUI_R_ADDREFENTITYTOSCENE:
		CLWM_AddRefEntityToScene((wmrefEntity_t*)VMA(1));
		return 0;

	case WMUI_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;

	case WMUI_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;

	case WMUI_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6]);
		return 0;

	case WMUI_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;

	case WMUI_R_RENDERSCENE:
		CLWM_RenderScene((wmrefdef_t*)VMA(1));
		return 0;

	case WMUI_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;

	case WMUI_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case WMUI_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;

	case WMUI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case WMUI_CM_LERPTAG:
		return CLWM_LerpTag((orientation_t*)VMA(1), (wmrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);

	case WMUI_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));

	case WMUI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;

	case WMUI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WMUI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WMUI_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;

	case WMUI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case WMUI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case WMUI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case WMUI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case WMUI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case WMUI_KEY_SETCATCHER:
		KeyWM_SetCatcher(args[1]);
		return 0;

	case WMUI_GETCLIPBOARDDATA:
		CLT3_GetClipboardData((char*)VMA(1), args[2]);
		return 0;

	case WMUI_GETCLIENTSTATE:
		UIT3_GetClientState((uiClientState_t*)VMA(1));
		return 0;

	case WMUI_GETGLCONFIG:
		CLWM_GetGlconfig((wmglconfig_t*)VMA(1));
		return 0;

	case WMUI_GETCONFIGSTRING:
		return CLWM_GetConfigString(args[1], (char*)VMA(2), args[3]);

	case WMUI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case WMUI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case WMUI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (char*)VMA(2), (char*)VMA(3));

	case WMUI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (char*)VMA(2));
		return 0;

	case WMUI_LAN_GETPINGQUEUECOUNT:
		return CLT3_GetPingQueueCount();

	case WMUI_LAN_CLEARPING:
		CLT3_ClearPing(args[1]);
		return 0;

	case WMUI_LAN_GETPING:
		CLT3_GetPing(args[1], (char*)VMA(2), args[3], (int*)VMA(4));
		return 0;

	case WMUI_LAN_GETPINGINFO:
		CLT3_GetPingInfo(args[1], (char*)VMA(2), args[3]);
		return 0;

	case WMUI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case WMUI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case WMUI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case WMUI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);

	case WMUI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], args[3]);
		return 0;

	case WMUI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);

	case WMUI_LAN_UPDATEVISIBLEPINGS:
		return CLT3_UpdateVisiblePings(args[1]);

	case WMUI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;

	case WMUI_LAN_SERVERSTATUS:
		return CLT3_ServerStatus((char*)VMA(1), (char*)VMA(2), args[3]);

	case WMUI_SET_PBCLSTATUS:
		return 0;

	case WMUI_SET_PBSVSTATUS:
		return 0;

	case WMUI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);

	case WMUI_MEMORY_REMAINING:
		return 0x4000000;

	case WMUI_GET_CDKEY:
		CLT3UI_GetCDKey((char*)VMA(1), args[2]);
		return 0;

	case WMUI_SET_CDKEY:
		CLT3UI_SetCDKey((char*)VMA(1));
		return 0;

	case WMUI_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;

	case WMUI_MEMSET:
		return (qintptr)memset(VMA(1), args[2], args[3]);

	case WMUI_MEMCPY:
		return (qintptr)memcpy(VMA(1), VMA(2), args[3]);

	case WMUI_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];

	case WMUI_SIN:
		return FloatAsInt(sin(VMF(1)));

	case WMUI_COS:
		return FloatAsInt(cos(VMF(1)));

	case WMUI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case WMUI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case WMUI_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case WMUI_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case WMUI_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case WMUI_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case WMUI_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case WMUI_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case WMUI_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case WMUI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case WMUI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), 0);
		return 0;

	case WMUI_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));

	case WMUI_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematicStretched((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case WMUI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case WMUI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case WMUI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case WMUI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case WMUI_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case WMUI_VERIFY_CDKEY:
		return CLT3_CDKeyValidate((char*)VMA(1), (char*)VMA(2));

	case WMUI_CL_GETLIMBOSTRING:
		return CLT3_GetLimboString(args[1], (char*)VMA(2));

	case WMUI_CL_TRANSLATE_STRING:
		CL_TranslateString((char*)VMA(1), (char*)VMA(2));
		return 0;

	case WMUI_CHECKAUTOUPDATE:
		return 0;

	case WMUI_GET_AUTOUPDATE:
		return 0;

	case WMUI_OPENURL:
		CLT3_OpenURL((const char*)VMA(1));
		return 0;

	default:
		common->Error("Bad UI system trap: %i", static_cast<int>(args[0]));
	}
	return 0;
}
