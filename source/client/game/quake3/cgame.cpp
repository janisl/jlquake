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
#include "cg_public.h"

qintptr CLQ3_CgameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case Q3CG_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;
	case Q3CG_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;
	case Q3CG_MILLISECONDS:
		return Sys_Milliseconds();
	case Q3CG_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;
	case Q3CG_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;
	case Q3CG_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;
	case Q3CG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case Q3CG_ARGC:
		return Cmd_Argc();
	case Q3CG_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;
	case Q3CG_ARGS:
		Cmd_ArgsBuffer((char*)VMA(1), args[2]);
		return 0;
	case Q3CG_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);
	case Q3CG_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case Q3CG_FS_WRITE:
		FS_Write(VMA(1), args[2], args[3]);
		return 0;
	case Q3CG_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;
	case Q3CG_FS_SEEK:
		return FS_Seek(args[1], args[2], args[3]);
	case Q3CG_SENDCONSOLECOMMAND:
		Cbuf_AddText((char*)VMA(1));
		return 0;
	case Q3CG_ADDCOMMAND:
		CLT3_AddCgameCommand((char*)VMA(1));
		return 0;
	case Q3CG_REMOVECOMMAND:
		Cmd_RemoveCommand((char*)VMA(1));
		return 0;
//---------
	case Q3CG_CM_LOADMAP:
		CLT3_CM_LoadMap((char*)VMA(1));
		return 0;
	case Q3CG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case Q3CG_CM_INLINEMODEL:
		return CM_InlineModel(args[1]);
	case Q3CG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel((float*)VMA(1), (float*)VMA(2), /*int capsule*/ false);
	case Q3CG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel((float*)VMA(1), (float*)VMA(2), /*int capsule*/ true);
	case Q3CG_CM_POINTCONTENTS:
		return CM_PointContentsQ3((float*)VMA(1), args[2]);
	case Q3CG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContentsQ3((float*)VMA(1), args[2], (float*)VMA(3), (float*)VMA(4));
	case Q3CG_CM_BOXTRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], false);
		return 0;
	case Q3CG_CM_CAPSULETRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], true);
		return 0;
	case Q3CG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), false);
		return 0;
	case Q3CG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), true);
		return 0;
	case Q3CG_CM_MARKFRAGMENTS:
		return R_MarkFragments(args[1], (vec3_t*)VMA(2), (float*)VMA(3), args[4], (float*)VMA(5), args[6], (markFragment_t*)VMA(7));
	case Q3CG_S_STARTSOUND:
		S_StartSound((float*)VMA(1), args[2], args[3], args[4]);
		return 0;
	case Q3CG_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;
	case Q3CG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds(args[1]);
		return 0;
	case Q3CG_S_ADDLOOPINGSOUND:
		S_AddLoopingSound(args[1], (float*)VMA(2), (float*)VMA(3), 0, args[4], 0, 0);
		return 0;
	case Q3CG_S_ADDREALLOOPINGSOUND:
		S_AddRealLoopingSound(args[1], (float*)VMA(2), (float*)VMA(3), 0, args[4], 0, 0);
		return 0;
	case Q3CG_S_STOPLOOPINGSOUND:
		S_StopLoopingSound(args[1]);
		return 0;
	case Q3CG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition(args[1], (float*)VMA(2));
		return 0;
	case Q3CG_S_RESPATIALIZE:
		S_Respatialize(args[1], (float*)VMA(2), (vec3_t*)VMA(3), args[4]);
		return 0;
	case Q3CG_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));
	case Q3CG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), 0);
		return 0;
	case Q3CG_R_LOADWORLDMAP:
		R_LoadWorld((char*)VMA(1));
		return 0;
	case Q3CG_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));
	case Q3CG_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));
	case Q3CG_R_REGISTERSHADER:
		return R_RegisterShader((char*)VMA(1));
	case Q3CG_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));
	case Q3CG_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
	case Q3CG_R_CLEARSCENE:
		R_ClearScene();
		return 0;
//---------
	case Q3CG_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;
	case Q3CG_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;
	case Q3CG_R_LIGHTFORPOINT:
		return R_LightForPoint((float*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4));
	case Q3CG_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5));
		return 0;
	case Q3CG_R_ADDADDITIVELIGHTTOSCENE:
		R_AddAdditiveLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5));
		return 0;
//---------
	case Q3CG_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;
	case Q3CG_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;
	case Q3CG_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;
	case Q3CG_R_LERPTAG:
		return R_LerpTag((orientation_t*)VMA(1), args[2], args[3], args[4], VMF(5), (char*)VMA(6));
//---------
	case Q3CG_GETCURRENTCMDNUMBER:
		return CLT3_GetCurrentCmdNumber();
//---------
	case Q3CG_MEMORY_REMAINING:
		return 0x4000000;
	case Q3CG_KEY_ISDOWN:
		return Key_IsDown(args[1]);
	case Q3CG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case Q3CG_KEY_SETCATCHER:
		KeyQ3_SetCatcher(args[1]);
		return 0;
	case Q3CG_KEY_GETKEY:
		return Key_GetKey((char*)VMA(1));

	case Q3CG_MEMSET:
		Com_Memset(VMA(1), args[2], args[3]);
		return 0;
	case Q3CG_MEMCPY:
		Com_Memcpy(VMA(1), VMA(2), args[3]);
		return 0;
	case Q3CG_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return (qintptr)(char*)VMA(1);
	case Q3CG_SIN:
		return FloatAsInt(sin(VMF(1)));
	case Q3CG_COS:
		return FloatAsInt(cos(VMF(1)));
	case Q3CG_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));
	case Q3CG_SQRT:
		return FloatAsInt(sqrt(VMF(1)));
	case Q3CG_FLOOR:
		return FloatAsInt(floor(VMF(1)));
	case Q3CG_CEIL:
		return FloatAsInt(ceil(VMF(1)));
	case Q3CG_ACOS:
		return FloatAsInt(Q_acos(VMF(1)));

	case Q3CG_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case Q3CG_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case Q3CG_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case Q3CG_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case Q3CG_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case Q3CG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

//---------
	case Q3CG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	case Q3CG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);
//---------
	case Q3CG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);
//---------

	case Q3CG_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case Q3CG_GET_ENTITY_TOKEN:
		return R_GetEntityToken((char*)VMA(1), args[2]);
	case Q3CG_R_INPVS:
		return CLT3_InPvs((float*)VMA(1), (float*)VMA(2));

	default:
		qassert(0);
		common->Error("Bad cgame system trap: %i", static_cast<int>(args[0]));
	}
	return 0;
}
