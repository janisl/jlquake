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
#include "../../../server/public.h"

bool CLWS_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	return VM_Call(cgvm, WSCG_GET_TAG, clientNum, tagname, _or);
}

static void CLWS_ClearLoopingSounds(int killAll)
{
	// (SA) modified so no_pvs sounds can function
	S_ClearLoopingSounds(true);
	// RF, if killall, then stop all sounds
	if (killAll == 1)
	{
		S_ClearSounds(true, false);
	}
	else if (killAll == 2)
	{
		S_ClearSounds(true, true);
	}
}

static void CLWS_StartCamera(int camNum, int time)
{
	if (camNum == 0)		// CAM_PRIMARY
	{
		cl.wa_cameraMode = true;	//----(SA)	added
	}
	startCamera(camNum, time);
}

static void CLWS_StopCamera(int camNum)
{
	if (camNum == 0)		// CAM_PRIMARY
	{
		cl.wa_cameraMode = false;
	}
}

//	The cgame module is making a system call
qintptr CLWS_CgameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case WSCG_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;
	case WSCG_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;
	case WSCG_MILLISECONDS:
		return Sys_Milliseconds();
	case WSCG_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;
	case WSCG_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;
	case WSCG_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;
	case WSCG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case WSCG_ARGC:
		return Cmd_Argc();
	case WSCG_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;
	case WSCG_ARGS:
		Cmd_ArgsBuffer((char*)VMA(1), args[2]);
		return 0;
	case WSCG_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);
	case WSCG_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case WSCG_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case WSCG_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;
	case WSCG_SENDCONSOLECOMMAND:
		Cbuf_AddText((char*)VMA(1));
		return 0;
	case WSCG_ADDCOMMAND:
		CLT3_AddCgameCommand((char*)VMA(1));
		return 0;
	case WSCG_REMOVECOMMAND:
		Cmd_RemoveCommand((char*)VMA(1));
		return 0;
//---------
	case WSCG_CM_LOADMAP:
		CLT3_CM_LoadMap((char*)VMA(1));
		return 0;
	case WSCG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case WSCG_CM_INLINEMODEL:
		return CM_InlineModel(args[1]);
	case WSCG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel((float*)VMA(1), (float*)VMA(2), false);
	case WSCG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel((float*)VMA(1), (float*)VMA(2), true);
	case WSCG_CM_POINTCONTENTS:
		return CM_PointContentsQ3((float*)VMA(1), args[2]);
	case WSCG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContentsQ3((float*)VMA(1), args[2], (float*)VMA(3), (float*)VMA(4));
	case WSCG_CM_BOXTRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], false);
		return 0;
	case WSCG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), false);
		return 0;
	case WSCG_CM_CAPSULETRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], true);
		return 0;
	case WSCG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), true);
		return 0;
	case WSCG_CM_MARKFRAGMENTS:
		return R_MarkFragmentsWolf(args[1], (const vec3_t*)VMA(2), (float*)VMA(3), args[4], (float*)VMA(5), args[6], (markFragment_t*)VMA(7));
	case WSCG_S_STARTSOUND:
		S_StartSound((float*)VMA(1), args[2], args[3], args[4], 0.5);
		return 0;
	case WSCG_S_STARTSOUNDEX:
		S_StartSoundEx((float*)VMA(1), args[2], args[3], args[4], args[5], 127);
		return 0;
	case WSCG_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;
	case WSCG_S_CLEARLOOPINGSOUNDS:
		CLWS_ClearLoopingSounds(args[1]);
		return 0;
	case WSCG_S_ADDLOOPINGSOUND:
		// FIXME MrE: handling of looping sounds changed
		S_AddLoopingSound(args[1], (float*)VMA(2), (float*)VMA(3), args[4], args[5], args[6], 0);
		return 0;
	case WSCG_S_STOPSTREAMINGSOUND:
		S_StopEntStreamingSound(args[1]);
		return 0;
	case WSCG_S_STOPLOOPINGSOUND:
		// RF, not functional anymore, since we reverted to old looping code
		return 0;
	case WSCG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition(args[1], (float*)VMA(2));
		return 0;
	case WSCG_S_GETVOICEAMPLITUDE:
		return S_GetVoiceAmplitude(args[1]);
	case WSCG_S_RESPATIALIZE:
		S_Respatialize(args[1], (float*)VMA(2), (vec3_t*)VMA(3), args[4]);
		return 0;
	case WSCG_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));
	case WSCG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), args[3]);			//----(SA)	added fadeup time
		return 0;
	case WSCG_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);		//----(SA)	added music/all-streaming options
		return 0;
	case WSCG_S_STARTSTREAMINGSOUND:
		S_StartStreamingSound((char*)VMA(1), (char*)VMA(2), args[3], args[4], args[5]);
		return 0;
	case WSCG_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], false);		//----(SA)	added
		return 0;
	case WSCG_R_LOADWORLDMAP:
		R_LoadWorld((char*)VMA(1));
		return 0;
	case WSCG_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));
	case WSCG_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));
	case WSCG_R_GETSKINMODEL:
		return R_GetSkinModel(args[1], (char*)VMA(2), (char*)VMA(3));
	case WSCG_R_GETMODELSHADER:
		return R_GetShaderFromModel(args[1], args[2], args[3]);
	case WSCG_R_REGISTERSHADER:
		return R_RegisterShader((char*)VMA(1));
	case WSCG_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
	case WSCG_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));
	case WSCG_R_CLEARSCENE:
		R_ClearScene();
		return 0;
//---------
	case WSCG_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;
	case WSCG_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;
	case WSCG_RB_ZOMBIEFXADDNEWHIT:
		return 0;
	case WSCG_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6]);
		return 0;
	case WSCG_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;
	case WSCG_R_SETFOG:
		R_SetFog(args[1], args[2], args[3], VMF(4), VMF(5), VMF(6), VMF(7));
		return 0;
//---------
	case WSCG_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;
	case WSCG_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;
	case WSCG_R_DRAWSTRETCHPIC_GRADIENT:
		R_StretchPicGradient(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], (float*)VMA(10), args[11]);
		return 0;
	case WSCG_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;
//---------
	case WSCG_GETCURRENTCMDNUMBER:
		return CLT3_GetCurrentCmdNumber();
//---------
	case WSCG_MEMORY_REMAINING:
		return 0x4000000;
	case WSCG_KEY_ISDOWN:
		return Key_IsDown(args[1]);
//---------
	case WSCG_KEY_GETKEY:
		return Key_GetKey((char*)VMA(1));

	case WSCG_MEMSET:
		return (qintptr)memset(VMA(1), args[2], args[3]);
	case WSCG_MEMCPY:
		return (qintptr)memcpy(VMA(1), VMA(2), args[3]);
	case WSCG_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];
	case WSCG_SIN:
		return FloatAsInt(sin(VMF(1)));
	case WSCG_COS:
		return FloatAsInt(cos(VMF(1)));
	case WSCG_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));
	case WSCG_SQRT:
		return FloatAsInt(sqrt(VMF(1)));
	case WSCG_FLOOR:
		return FloatAsInt(floor(VMF(1)));
	case WSCG_CEIL:
		return FloatAsInt(ceil(VMF(1)));
	case WSCG_ACOS:
		return FloatAsInt(Q_acos(VMF(1)));

	case WSCG_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case WSCG_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case WSCG_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case WSCG_PC_READ_TOKEN:
		return PC_ReadTokenHandleQ3(args[1], (q3pc_token_t*)VMA(2));
	case WSCG_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));

	case WSCG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

	case WSCG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	case WSCG_SENDMOVESPEEDSTOGAME:
		SVWS_SendMoveSpeedsToGame(args[1], (char*)VMA(2));
		return 0;

	case WSCG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);
//---------
	case WSCG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);
//---------

	case WSCG_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case WSCG_TESTPRINTINT:
		common->Printf("%s%i\n", (char*)VMA(1), static_cast<int>(args[2]));
		return 0;
	case WSCG_TESTPRINTFLOAT:
		common->Printf("%s%f\n", (char*)VMA(1), VMF(2));
		return 0;

	case WSCG_LOADCAMERA:
		return loadCamera(args[1], (char*)VMA(2));
	case WSCG_STARTCAMERA:
		CLWS_StartCamera(args[1], args[2]);
		return 0;
	case WSCG_STOPCAMERA:
		CLWS_StopCamera(args[1]);
		return 0;
	case WSCG_GETCAMERAINFO:
		return getCameraInfo(args[1], args[2], (float*)VMA(3), (float*)VMA(4), (float*)VMA(5));

	case WSCG_GET_ENTITY_TOKEN:
		return R_GetEntityToken((char*)VMA(1), args[2]);

//---------

	case WSCG_INGAME_CLOSEPOPUP:
		UIT3_KeyEvent(K_ESCAPE, true);
		return 0;

//---------

	case WSCG_GETMODELINFO:
		return SVWS_GetModelInfo(args[1], (char*)VMA(2), (animModelInfo_t**)VMA(3));

	default:
		common->Error("Bad cgame system trap: %i", static_cast<int>(args[0]));
	}
	return 0;
}
