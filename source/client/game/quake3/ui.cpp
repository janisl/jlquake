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
#include "../../ui/ui.h"
#include "../../cinematic/public.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "ui_public.h"
#include "cg_ui_shared.h"
#include "../../../common/precompiler.h"
#include "../../../common/system.h"

bool UIQ3_ConsoleCommand(int realTime)
{
	return VM_Call(uivm, Q3UI_CONSOLE_COMMAND, realTime);
}

void UIQ3_DrawConnectScreen(bool overlay)
{
	VM_Call(uivm, Q3UI_DRAW_CONNECT_SCREEN, overlay);
}

bool UIQ3_HasUniqueCDKey()
{
	return VM_Call(uivm, Q3UI_HASUNIQUECDKEY);
}

static int CLQ3_GetConfigString(int index, char* buf, int size)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS_Q3)
	{
		return false;
	}

	int offset = cl.q3_gameState.stringOffsets[index];
	if (!offset)
	{
		if (size)
		{
			buf[0] = 0;
		}
		return false;
	}

	String::NCpyZ(buf, cl.q3_gameState.stringData + offset, size);

	return true;
}

//	The ui module is making a system call
qintptr CLQ3_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case Q3UI_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;

	case Q3UI_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;

	case Q3UI_MILLISECONDS:
		return Sys_Milliseconds();

	case Q3UI_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;

	case Q3UI_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;

	case Q3UI_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;

	case Q3UI_CVAR_VARIABLEVALUE:
		return FloatAsInt(Cvar_VariableValue((char*)VMA(1)));

	case Q3UI_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_CVAR_SETVALUE:
		Cvar_SetValue((char*)VMA(1), VMF(2));
		return 0;

	case Q3UI_CVAR_RESET:
		Cvar_Reset((char*)VMA(1));
		return 0;

	case Q3UI_CVAR_CREATE:
		Cvar_Get((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_CVAR_INFOSTRINGBUFFER:
		Cvar_InfoStringBuffer(args[1], MAX_INFO_STRING_Q3, (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_ARGC:
		return Cmd_Argc();

	case Q3UI_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_CMD_EXECUTETEXT:
		Cbuf_ExecuteText(args[1], (char*)VMA(2));
		return 0;

	case Q3UI_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);

	case Q3UI_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;

	case Q3UI_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;

	case Q3UI_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;

	case Q3UI_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);

	case Q3UI_FS_SEEK:
		return FS_Seek(args[1], args[2], args[3]);

	case Q3UI_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));

	case Q3UI_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));

	case Q3UI_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));

	case Q3UI_R_CLEARSCENE:
		R_ClearScene();
		return 0;

	case Q3UI_R_ADDREFENTITYTOSCENE:
		CLQ3_AddRefEntityToScene((q3refEntity_t*)VMA(1));
		return 0;

	case Q3UI_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;

	case Q3UI_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5));
		return 0;

	case Q3UI_R_RENDERSCENE:
		CLQ3_RenderScene((q3refdef_t*)VMA(1));
		return 0;

	case Q3UI_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;

	case Q3UI_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;

	case Q3UI_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;

	case Q3UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case Q3UI_CM_LERPTAG:
		R_LerpTag((orientation_t*)VMA(1), args[2], args[3], args[4], VMF(5), (char*)VMA(6));
		return 0;

	case Q3UI_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));

	case Q3UI_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;

	case Q3UI_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;

	case Q3UI_KEY_ISDOWN:
		return Key_IsDown(args[1]);

	case Q3UI_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();

	case Q3UI_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case Q3UI_KEY_CLEARSTATES:
		Key_ClearStates();
		return 0;

	case Q3UI_KEY_GETCATCHER:
		return Key_GetCatcher();

	case Q3UI_KEY_SETCATCHER:
		KeyQ3_SetCatcher(args[1]);
		return 0;

	case Q3UI_GETCLIPBOARDDATA:
		CLT3_GetClipboardData((char*)VMA(1), args[2]);
		return 0;

	case Q3UI_GETCLIENTSTATE:
		UIT3_GetClientState((uiClientState_t*)VMA(1));
		return 0;

	case Q3UI_GETGLCONFIG:
		CLQ3_GetGlconfig((q3glconfig_t*)VMA(1));
		return 0;

	case Q3UI_GETCONFIGSTRING:
		return CLQ3_GetConfigString(args[1], (char*)VMA(2), args[3]);

	case Q3UI_LAN_LOADCACHEDSERVERS:
		LAN_LoadCachedServers();
		return 0;

	case Q3UI_LAN_SAVECACHEDSERVERS:
		LAN_SaveServersToCache();
		return 0;

	case Q3UI_LAN_ADDSERVER:
		return LAN_AddServer(args[1], (char*)VMA(2), (char*)VMA(3));

	case Q3UI_LAN_REMOVESERVER:
		LAN_RemoveServer(args[1], (char*)VMA(2));
		return 0;

	case Q3UI_LAN_GETPINGQUEUECOUNT:
		return CLT3_GetPingQueueCount();

	case Q3UI_LAN_CLEARPING:
		CLT3_ClearPing(args[1]);
		return 0;

	case Q3UI_LAN_GETPING:
		CLT3_GetPing(args[1], (char*)VMA(2), args[3], (int*)VMA(4));
		return 0;

	case Q3UI_LAN_GETPINGINFO:
		CLT3_GetPingInfo(args[1], (char*)VMA(2), args[3]);
		return 0;

	case Q3UI_LAN_GETSERVERCOUNT:
		return LAN_GetServerCount(args[1]);

	case Q3UI_LAN_GETSERVERADDRESSSTRING:
		LAN_GetServerAddressString(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case Q3UI_LAN_GETSERVERINFO:
		LAN_GetServerInfo(args[1], args[2], (char*)VMA(3), args[4]);
		return 0;

	case Q3UI_LAN_GETSERVERPING:
		return LAN_GetServerPing(args[1], args[2]);

	case Q3UI_LAN_MARKSERVERVISIBLE:
		LAN_MarkServerVisible(args[1], args[2], args[3]);
		return 0;

	case Q3UI_LAN_SERVERISVISIBLE:
		return LAN_ServerIsVisible(args[1], args[2]);

	case Q3UI_LAN_UPDATEVISIBLEPINGS:
		return CLT3_UpdateVisiblePings(args[1]);

	case Q3UI_LAN_RESETPINGS:
		LAN_ResetPings(args[1]);
		return 0;

	case Q3UI_LAN_SERVERSTATUS:
		return CLT3_ServerStatus((char*)VMA(1), (char*)VMA(2), args[3]);

	case Q3UI_LAN_COMPARESERVERS:
		return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);

	case Q3UI_MEMORY_REMAINING:
		return 0x4000000;

	case Q3UI_GET_CDKEY:
		CLT3UI_GetCDKey((char*)VMA(1), args[2]);
		return 0;

	case Q3UI_SET_CDKEY:
		CLT3UI_SetCDKey((char*)VMA(1));
		return 0;

	case Q3UI_SET_PBCLSTATUS:
		return 0;

	case Q3UI_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;

	case Q3UI_MEMSET:
		Com_Memset(VMA(1), args[2], args[3]);
		return 0;

	case Q3UI_MEMCPY:
		Com_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;

	case Q3UI_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return (qintptr)(char*)VMA(1);

	case Q3UI_SIN:
		return FloatAsInt(sin(VMF(1)));

	case Q3UI_COS:
		return FloatAsInt(cos(VMF(1)));

	case Q3UI_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));

	case Q3UI_SQRT:
		return FloatAsInt(sqrt(VMF(1)));

	case Q3UI_FLOOR:
		return FloatAsInt(floor(VMF(1)));

	case Q3UI_CEIL:
		return FloatAsInt(ceil(VMF(1)));

	case Q3UI_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case Q3UI_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case Q3UI_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case Q3UI_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case Q3UI_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case Q3UI_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;
	case Q3UI_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), 0);
		return 0;

	case Q3UI_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));

	case Q3UI_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematicStretched((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case Q3UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case Q3UI_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case Q3UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case Q3UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case Q3UI_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case Q3UI_VERIFY_CDKEY:
		return CLT3_CDKeyValidate((char*)VMA(1), (char*)VMA(2));

	default:
		common->Error("Bad UI system trap: %i", static_cast<int>(args[0]));
	}
	return 0;
}
