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

bool CLET_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	return VM_Call(cgvm, ETCG_GET_TAG, clientNum, tagname, _or);
}

bool CLET_CGameCheckKeyExec(int key)
{
	if (!cgvm)
	{
		return false;
	}
	return VM_Call(cgvm, ETCG_CHECKEXECKEY, key);
}

bool CLET_WantsBindKeys()
{
	if (!cgvm)
	{
		return false;
	}
	return VM_Call(cgvm, ETCG_WANTSBINDKEYS);
}

void CLET_CGameBinaryMessageReceived(const char* buf, int buflen, int serverTime)
{
	VM_Call(cgvm, ETCG_MESSAGERECEIVED, buf, buflen, serverTime);
}

static void CLET_ClearSounds(int clearType)
{
	if (clearType == 0)
	{
		S_ClearSounds(true, false);
	}
	else if (clearType == 1)
	{
		S_ClearSounds(true, true);
	}
}

static void CLET_StartCamera(int camNum, int time)
{
	if (camNum == 0)		// CAM_PRIMARY
	{
		cl.wa_cameraMode = true;
	}
	startCamera(camNum, time);
}

static void CLET_StopCamera(int camNum)
{
	if (camNum == 0)		// CAM_PRIMARY
	{
		cl.wa_cameraMode = false;
	}
}

//	The cgame module is making a system call
qintptr CLET_CgameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case ETCG_PRINT:
		common->Printf("%s", (char*)VMA(1));
		return 0;
	case ETCG_ERROR:
		common->Error("%s", (char*)VMA(1));
		return 0;
	case ETCG_MILLISECONDS:
		return Sys_Milliseconds();
	case ETCG_CVAR_REGISTER:
		Cvar_Register((vmCvar_t*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
		return 0;
	case ETCG_CVAR_UPDATE:
		Cvar_Update((vmCvar_t*)VMA(1));
		return 0;
	case ETCG_CVAR_SET:
		Cvar_Set((char*)VMA(1), (char*)VMA(2));
		return 0;
	case ETCG_CVAR_VARIABLESTRINGBUFFER:
		Cvar_VariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case ETCG_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		Cvar_LatchedVariableStringBuffer((char*)VMA(1), (char*)VMA(2), args[3]);
		return 0;
	case ETCG_ARGC:
		return Cmd_Argc();
	case ETCG_ARGV:
		Cmd_ArgvBuffer(args[1], (char*)VMA(2), args[3]);
		return 0;
	case ETCG_ARGS:
		Cmd_ArgsBuffer((char*)VMA(1), args[2]);
		return 0;
	case ETCG_FS_FOPENFILE:
		return FS_FOpenFileByMode((char*)VMA(1), (fileHandle_t*)VMA(2), (fsMode_t)args[3]);
	case ETCG_FS_READ:
		FS_Read(VMA(1), args[2], args[3]);
		return 0;
	case ETCG_FS_WRITE:
		return FS_Write(VMA(1), args[2], args[3]);
	case ETCG_FS_FCLOSEFILE:
		FS_FCloseFile(args[1]);
		return 0;
	case ETCG_FS_GETFILELIST:
		return FS_GetFileList((char*)VMA(1), (char*)VMA(2), (char*)VMA(3), args[4]);
	case ETCG_FS_DELETEFILE:
		return FS_Delete((char*)VMA(1));
	case ETCG_SENDCONSOLECOMMAND:
		Cbuf_AddText((char*)VMA(1));
		return 0;
	case ETCG_ADDCOMMAND:
		CLT3_AddCgameCommand((char*)VMA(1));
		return 0;
	case ETCG_REMOVECOMMAND:
		Cmd_RemoveCommand((char*)VMA(1));
		return 0;
//---------
	case ETCG_CM_LOADMAP:
		CLT3_CM_LoadMap((char*)VMA(1));
		return 0;
	case ETCG_CM_NUMINLINEMODELS:
		return CM_NumInlineModels();
	case ETCG_CM_INLINEMODEL:
		return CM_InlineModel(args[1]);
	case ETCG_CM_TEMPBOXMODEL:
		return CM_TempBoxModel((float*)VMA(1), (float*)VMA(2), false);
	case ETCG_CM_TEMPCAPSULEMODEL:
		return CM_TempBoxModel((float*)VMA(1), (float*)VMA(2), true);
	case ETCG_CM_POINTCONTENTS:
		return CM_PointContentsQ3((float*)VMA(1), args[2]);
	case ETCG_CM_TRANSFORMEDPOINTCONTENTS:
		return CM_TransformedPointContentsQ3((float*)VMA(1), args[2], (float*)VMA(3), (float*)VMA(4));
	case ETCG_CM_BOXTRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], false);
		return 0;
	case ETCG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), false);
		return 0;
	case ETCG_CM_CAPSULETRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], true);
		return 0;
	case ETCG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), true);
		return 0;
	case ETCG_CM_MARKFRAGMENTS:
		return R_MarkFragmentsWolf(args[1], (const vec3_t*)VMA(2), (float*)VMA(3), args[4], (float*)VMA(5), args[6], (markFragment_t*)VMA(7));

	case ETCG_R_PROJECTDECAL:
		R_ProjectDecal(args[1], args[2], (vec3_t*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7]);
		return 0;
	case ETCG_R_CLEARDECALS:
		R_ClearDecals();
		return 0;

	case ETCG_S_STARTSOUND:
		S_StartSound((float*)VMA(1), args[2], args[3], args[4], args[5] / 255.0);
		return 0;
	case ETCG_S_STARTSOUNDEX:
		S_StartSoundEx((float*)VMA(1), args[2], args[3], args[4], args[5], args[6]);
		return 0;
	case ETCG_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], args[3]);
		return 0;
	case ETCG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds(true);
		return 0;
	case ETCG_S_CLEARSOUNDS:
		CLET_ClearSounds(args[1]);
		return 0;
	case ETCG_S_ADDLOOPINGSOUND:
		// FIXME MrE: handling of looping sounds changed
		S_AddLoopingSound(-1, (float*)VMA(1), (float*)VMA(2), args[3], args[4], args[5], args[6]);
		return 0;
	case ETCG_S_ADDREALLOOPINGSOUND:
		S_AddRealLoopingSound(-1, (float*)VMA(1), (float*)VMA(2), args[3], args[4], args[5], args[6]);
		return 0;
	case ETCG_S_STOPSTREAMINGSOUND:
		S_StopEntStreamingSound(args[1]);
		return 0;
	case ETCG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition(args[1], (float*)VMA(2));
		return 0;
	case ETCG_S_GETVOICEAMPLITUDE:
		return S_GetVoiceAmplitude(args[1]);
	case ETCG_S_GETSOUNDLENGTH:
		return S_GetSoundLength(args[1]);
	case ETCG_S_GETCURRENTSOUNDTIME:
		return S_GetCurrentSoundTime();
	case ETCG_S_RESPATIALIZE:
		S_Respatialize(args[1], (float*)VMA(2), (vec3_t*)VMA(3), args[4]);
		return 0;
	case ETCG_S_REGISTERSOUND:
		return S_RegisterSound((char*)VMA(1));
	case ETCG_S_STARTBACKGROUNDTRACK:
		S_StartBackgroundTrack((char*)VMA(1), (char*)VMA(2), args[3]);			//----(SA)	added fadeup time
		return 0;
	case ETCG_S_FADESTREAMINGSOUND:
		S_FadeStreamingSound(VMF(1), args[2], args[3]);		//----(SA)	added music/all-streaming options
		return 0;
	case ETCG_S_STARTSTREAMINGSOUND:
		return S_StartStreamingSound((char*)VMA(1), (char*)VMA(2), args[3], args[4], args[5]);
	case ETCG_R_LOADWORLDMAP:
		R_LoadWorld((char*)VMA(1));
		return 0;
	case ETCG_R_REGISTERMODEL:
		return R_RegisterModel((char*)VMA(1));
	case ETCG_R_REGISTERSKIN:
		return R_RegisterSkin((char*)VMA(1));
	case ETCG_R_GETSKINMODEL:
		return R_GetSkinModel(args[1], (char*)VMA(2), (char*)VMA(3));
	case ETCG_R_GETMODELSHADER:
		return R_GetShaderFromModel(args[1], args[2], args[3]);
	case ETCG_R_REGISTERSHADER:
		return R_RegisterShader((char*)VMA(1));
	case ETCG_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
		return 0;
	case ETCG_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));
	case ETCG_R_CLEARSCENE:
		R_ClearScene();
		return 0;
//---------
	case ETCG_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;
	case ETCG_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;
	case ETCG_R_ADDPOLYBUFFERTOSCENE:
		R_AddPolyBufferToScene((polyBuffer_t*)VMA(1));
		break;
	case ETCG_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
		return 0;
	case ETCG_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;
	case ETCG_R_SETFOG:
		R_SetFog(args[1], args[2], args[3], VMF(4), VMF(5), VMF(6), VMF(7));
		return 0;
	case ETCG_R_SETGLOBALFOG:
		R_SetGlobalFog(args[1], args[2], VMF(3), VMF(4), VMF(5), VMF(6));
		return 0;
//---------
	case ETCG_R_SAVEVIEWPARMS:
		R_SaveViewParms();
		return 0;
	case ETCG_R_RESTOREVIEWPARMS:
		R_RestoreViewParms();
		return 0;
	case ETCG_R_SETCOLOR:
		R_SetColor((float*)VMA(1));
		return 0;
	case ETCG_R_DRAWSTRETCHPIC:
		R_StretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
		return 0;
	case ETCG_R_DRAWROTATEDPIC:
		R_RotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
		return 0;
	case ETCG_R_DRAWSTRETCHPIC_GRADIENT:
		R_StretchPicGradient(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], (float*)VMA(10), args[11]);
		return 0;
	case ETCG_R_DRAW2DPOLYS:
		R_2DPolyies((polyVert_t*)VMA(1), args[2], args[3]);
		return 0;
	case ETCG_R_MODELBOUNDS:
		R_ModelBounds(args[1], (float*)VMA(2), (float*)VMA(3));
		return 0;
//---------
	case ETCG_GETCURRENTCMDNUMBER:
		return CLT3_GetCurrentCmdNumber();
//---------
	case ETCG_MEMORY_REMAINING:
		return 0x4000000;
	case ETCG_KEY_ISDOWN:
		return Key_IsDown(args[1]);
//---------
	case ETCG_KEY_GETKEY:
		return Key_GetKey((char*)VMA(1));

	case ETCG_KEY_GETOVERSTRIKEMODE:
		return Key_GetOverstrikeMode();
	case ETCG_KEY_SETOVERSTRIKEMODE:
		Key_SetOverstrikeMode(args[1]);
		return 0;

	case ETCG_MEMSET:
		return (qintptr)memset(VMA(1), args[2], args[3]);
	case ETCG_MEMCPY:
		return (qintptr)memcpy(VMA(1), VMA(2), args[3]);
	case ETCG_STRNCPY:
		String::NCpy((char*)VMA(1), (char*)VMA(2), args[3]);
		return args[1];
	case ETCG_SIN:
		return FloatAsInt(sin(VMF(1)));
	case ETCG_COS:
		return FloatAsInt(cos(VMF(1)));
	case ETCG_ATAN2:
		return FloatAsInt(atan2(VMF(1), VMF(2)));
	case ETCG_SQRT:
		return FloatAsInt(sqrt(VMF(1)));
	case ETCG_FLOOR:
		return FloatAsInt(floor(VMF(1)));
	case ETCG_CEIL:
		return FloatAsInt(ceil(VMF(1)));
	case ETCG_ACOS:
		return FloatAsInt(Q_acos(VMF(1)));

	case ETCG_PC_ADD_GLOBAL_DEFINE:
		return PC_AddGlobalDefine((char*)VMA(1));
	case ETCG_PC_LOAD_SOURCE:
		return PC_LoadSourceHandle((char*)VMA(1));
	case ETCG_PC_FREE_SOURCE:
		return PC_FreeSourceHandle(args[1]);
	case ETCG_PC_READ_TOKEN:
		return PC_ReadTokenHandleET(args[1], (etpc_token_t*)VMA(2));
	case ETCG_PC_SOURCE_FILE_AND_LINE:
		return PC_SourceFileAndLine(args[1], (char*)VMA(2), (int*)VMA(3));
	case ETCG_PC_UNREAD_TOKEN:
		PC_UnreadLastTokenHandle(args[1]);
		return 0;

	case ETCG_S_STOPBACKGROUNDTRACK:
		S_StopBackgroundTrack();
		return 0;

//---------
	case ETCG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	case ETCG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);
//---------
	case ETCG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);
//---------

	case ETCG_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case ETCG_TESTPRINTINT:
		common->Printf("%s%i\n", (char*)VMA(1), (int)args[2]);
		return 0;
	case ETCG_TESTPRINTFLOAT:
		common->Printf("%s%f\n", (char*)VMA(1), VMF(2));
		return 0;

	case ETCG_LOADCAMERA:
		return loadCamera(args[1], (char*)VMA(2));
	case ETCG_STARTCAMERA:
		CLET_StartCamera(args[1], args[2]);
		return 0;
	case ETCG_STOPCAMERA:
		CLET_StopCamera(args[1]);
		return 0;
	case ETCG_GETCAMERAINFO:
		return getCameraInfo(args[1], args[2], (float*)VMA(3), (float*)VMA(4), (float*)VMA(5));

	case ETCG_GET_ENTITY_TOKEN:
		return R_GetEntityToken((char*)VMA(1), args[2]);

//---------
	case ETCG_INGAME_CLOSEPOPUP:
		return 0;

//---------
	case ETCG_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;
//---------
	case ETCG_KEY_BINDINGTOKEYS:
		Key_GetKeysForBinding((char*)VMA(1), (int*)VMA(2), (int*)VMA(3));
		return 0;

	case ETCG_TRANSLATE_STRING:
		CL_TranslateString((char*)VMA(1), (char*)VMA(2));
		return 0;

	case ETCG_S_FADEALLSOUNDS:
		S_FadeAllSounds(VMF(1), args[2], args[3]);
		return 0;

	case ETCG_R_INPVS:
		return CLT3_InPvs((float*)VMA(1), (float*)VMA(2));

//---------

	case ETCG_PUMPEVENTLOOP:
		return 0;

//---------
	case ETCG_R_LOADDYNAMICSHADER:
		return R_LoadDynamicShader((char*)VMA(1), (char*)VMA(2));
	case ETCG_R_RENDERTOTEXTURE:
		R_RenderToTexture(args[1], args[2], args[3], args[4], args[5]);
		return 0;
	case ETCG_R_GETTEXTUREID:
		return R_GetTextureId((char*)VMA(1));
	case ETCG_R_FINISH:
		R_Finish();
		return 0;
	default:
		common->Error("Bad cgame system trap: %i", (int)args[0]);
	}
	return 0;
}
