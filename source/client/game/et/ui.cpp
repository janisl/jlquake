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

bool UIET_ConsoleCommand(int realTime)
{
	return VM_Call(uivm, ETUI_CONSOLE_COMMAND, realTime);
}

void UIET_DrawConnectScreen(bool overlay)
{
	VM_Call(uivm, ETUI_DRAW_CONNECT_SCREEN, overlay);
}

bool UIET_HasUniqueCDKey()
{
	return VM_Call(uivm, ETUI_HASUNIQUECDKEY);
}

bool UIET_CheckExecKey(int key)
{
	return VM_Call(uivm, ETUI_CHECKEXECKEY, key);
}

bool UIET_WantsBindKeys()
{
	return VM_Call(uivm, ETUI_WANTSBINDKEYS);
}

void CLET_InGamePopup(int menu)
{
	if (cls.state == CA_ACTIVE && !clc.demoplaying)
	{
		if (uivm)		// Gordon: can be called as the system is shutting down
		{
			UIT3_SetActiveMenu(menu);
		}
	}
}

static bool LAN_ServerIsInFavoriteList(int source, int n)
{
	if (source == WMAS_FAVORITES && n >= 0 && n < MAX_OTHER_SERVERS_Q3)
	{
		return true;
	}

	q3serverInfo_t* server = LAN_GetServerPtr(source, n);
	if (!server)
	{
		return false;
	}

	for (int i = 0; i < cls.q3_numfavoriteservers; i++)
	{
		if (SOCK_CompareAdr(cls.q3_favoriteServers[i].adr, server->adr))
		{
			return true;
		}
	}

	return false;
}

static int CLET_GetConfigString(int index, char* buf, int size)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS_ET)
	{
		return false;
	}

	int offset = cl.et_gameState.stringOffsets[index];
	if (!offset)
	{
		if (size)
		{
			buf[0] = 0;
		}
		return false;
	}

	String::NCpyZ(buf, cl.et_gameState.stringData + offset, size);

	return true;
}

//	The ui module is making a system call
qintptr CLET_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case ETUI_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;

	case ETUI_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;

	case ETUI_MILLISECONDS:
		return Sys_Milliseconds();

	case ETUI_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;

	case ETUI_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;

	case ETUI_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;

	case ETUI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue((char*)VMA(1)));

	case ETUI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case ETUI_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		Cvar_LatchedVariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case ETUI_CVAR_SETVALUE:
		Cvar_SetValue((char*)VMA(1), VMF(2));
		return 0;

	case ETUI_CVAR_RESET:
		Cvar_Reset((char*)VMA(1));
		return 0;

	case ETUI_CVAR_CREATE:
		Cvar_Get((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case ETUI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], MAX_INFO_STRING_Q3, (char*)VMA(2), args[3]);
		return 0;

	case ETUI_ARGC:
		return Cmd_Argc();

	case ETUI_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;

	case ETUI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case ETUI_ADDCOMMAND:
		Cmd_AddCommand((char*)VMA(1), NULL);
		return 0;

	case ETUI_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);

	case ETUI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

	case ETUI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case ETUI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;

	case ETUI_FS_DELETEFILE:
		return FS_Delete((char*)VMA(1));

	case ETUI_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case ETUI_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));

	case ETUI_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));

	case ETUI_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));

	case ETUI_R_CLEARSCENE:
		R_ClearScene();
		return 0;

	case ETUI_R_ADDREFENTITYTOSCENE:
		CLET_AddRefEntityToScene((etrefEntity_t*)VMA(1));
		return 0;

	case ETUI_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;

	case ETUI_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;

	case ETUI_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
		return 0;

	case ETUI_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;

	case ETUI_R_RENDERSCENE:
		CLET_RenderScene((etrefdef_t*)VMA(1));
		return 0;

	case ETUI_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;

	case ETUI_R_DRAW2DPOLYS:
		R_2DPolyies((polyVert_t*)VMA(1), args[2], args[3]);
		return 0;

	case ETUI_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case ETUI_R_DRAWROTATEDPIC:
		R_RotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
		return 0;

	case ETUI_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;

//-------

	case ETUI_CM_LERPTAG:
		return CLET_LerpTag((orientation_t*)VMA(1), (etrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);

	case ETUI_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));

	case ETUI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], args[3]);
		return 0;

	case ETUI_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);
		return 0;

	case ETUI_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], args[3]);
		return 0;

	case ETUI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case ETUI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case ETUI_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;

	case ETUI_KEY_BINDINGTOKEYS:
		Key_GetKeysForBinding((char*)VMA(1), (int*)VMA(2), (int*)VMA(3));
		return 0;

	case ETUI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case ETUI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case ETUI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case ETUI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case ETUI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case ETUI_KEY_SETCATCHER:
		KeyWM_SetCatcher(args[1]);
		return 0;

	case ETUI_GETCLIPBOARDDATA:
		CLT3_GetClipboardData((char*)VMA(1), args[2]);
		return 0;

	case ETUI_GETCLIENTSTATE:
		UIT3_GetClientState((uiClientState_t*)VMA(1));
		return 0;

	case ETUI_GETGLCONFIG:
		CLET_GetGlconfig((etglconfig_t*)VMA(1));
		return 0;

	case ETUI_GETCONFIGSTRING:
		return CLET_GetConfigString(args[1], (char*)VMA(2), args[3]);

	case ETUI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case ETUI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case ETUI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (char*)VMA(2), (char*)VMA(3));

	case ETUI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (char*)VMA(2));
		return 0;

//-------

	case ETUI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case ETUI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case ETUI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case ETUI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);

	case ETUI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], args[3]);
		return 0;

	case ETUI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);

//-------

	case ETUI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;

//-------

	case ETUI_LAN_SERVERISINFAVORITELIST:
		return LAN_ServerIsInFavoriteList(args[1], args[2]);

	case ETUI_SET_PBCLSTATUS:
		return 0;

	case ETUI_SET_PBSVSTATUS:
		return 0;

	case ETUI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);

	case ETUI_MEMORY_REMAINING:
		return 0x4000000;

	case ETUI_GET_CDKEY:
		CLT3UI_GetCDKey((char*)VMA(1), args[2]);
		return 0;

	case ETUI_SET_CDKEY:
		CLT3UI_SetCDKey((char*)VMA(1));
		return 0;

	case ETUI_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;

	case ETUI_MEMSET:
		return (qintptr)memset(VMA(1), args[2], args[3]);

	case ETUI_MEMCPY:
		return (qintptr)memcpy(VMA(1), VMA(2), args[3]);

	case ETUI_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];

	case ETUI_SIN:
		return FloatAsInt(sin(VMF(1)));

	case ETUI_COS:
		return FloatAsInt(cos(VMF(1)));

	case ETUI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case ETUI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case ETUI_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case ETUI_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case ETUI_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case ETUI_PC_REMOVE_ALL_GLOBAL_DEFINES:
		PC_RemoveAllGlobalDefines();
		return 0;
	case ETUI_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case ETUI_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case ETUI_PC_READ_TOKEN:
		return PC_ReadTokenHandleET(args[1], (etpc_token_t*)VMA(2));
	case ETUI_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));
	case ETUI_PC_UNREAD_TOKEN:
		PC_UnreadLastTokenHandle(args[1]);
		return 0;

	case ETUI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case ETUI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), args[3]);			//----(SA)	added fadeup time
		return 0;

	case ETUI_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));

	case ETUI_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

//-------

	case ETUI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

//-------

	case ETUI_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

//-------

	case ETUI_CL_TRANSLATE_STRING:
		CL_TranslateString((char*)VMA(1), (char*)VMA(2));
		return 0;

	case ETUI_CHECKAUTOUPDATE:
		return 0;

	case ETUI_GET_AUTOUPDATE:
		return 0;

//-------

	case ETUI_GETHUNKDATA:
		CLET_GetHunkInfo((int*)VMA(1), (int*)VMA(2));
		return 0;

	default:
		common->Error("Bad UI system trap: %i", (int)args[0]);
	}
	return 0;
}
