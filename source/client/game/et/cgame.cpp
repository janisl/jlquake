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
#include "../../client_main.h"
#include "../../translate.h"
#include "../../splines/public.h"
#include "../../ui/ui.h"
#include "../../cinematic/public.h"
#include "cg_public.h"
#include "cg_ui_shared.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/precompiler.h"
#include "../../../common/system.h"

static refEntityType_t clet_gameRefEntTypeToEngine[] =
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_SPLASH,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_CORE_TAPER,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,
};

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

void CLET_GetGlconfig(etglconfig_t* glconfig)
{
	String::NCpyZ(glconfig->renderer_string, cls.glconfig.renderer_string, sizeof(glconfig->renderer_string));
	String::NCpyZ(glconfig->vendor_string, cls.glconfig.vendor_string, sizeof(glconfig->vendor_string));
	String::NCpyZ(glconfig->version_string, cls.glconfig.version_string, sizeof(glconfig->version_string));
	String::NCpyZ(glconfig->extensions_string, cls.glconfig.extensions_string, sizeof(glconfig->extensions_string));
	glconfig->maxTextureSize = cls.glconfig.maxTextureSize;
	glconfig->maxActiveTextures = cls.glconfig.maxActiveTextures;
	glconfig->colorBits = cls.glconfig.colorBits;
	glconfig->depthBits = cls.glconfig.depthBits;
	glconfig->stencilBits = cls.glconfig.stencilBits;
	glconfig->driverType = cls.glconfig.driverType;
	glconfig->hardwareType = cls.glconfig.hardwareType;
	glconfig->deviceSupportsGamma = cls.glconfig.deviceSupportsGamma;
	glconfig->textureCompression = cls.glconfig.textureCompression;
	glconfig->textureEnvAddAvailable = cls.glconfig.textureEnvAddAvailable;
	glconfig->anisotropicAvailable = cls.glconfig.anisotropicAvailable;
	glconfig->maxAnisotropy = cls.glconfig.maxAnisotropy;
	glconfig->NVFogAvailable = cls.glconfig.NVFogAvailable;
	glconfig->NVFogMode = cls.glconfig.NVFogMode;
	glconfig->ATIMaxTruformTess = cls.glconfig.ATIMaxTruformTess;
	glconfig->ATINormalMode = cls.glconfig.ATINormalMode;
	glconfig->ATIPointMode = cls.glconfig.ATIPointMode;
	glconfig->vidWidth = cls.glconfig.vidWidth;
	glconfig->vidHeight = cls.glconfig.vidHeight;
	glconfig->windowAspect = cls.glconfig.windowAspect;
	glconfig->displayFrequency = cls.glconfig.displayFrequency;
	glconfig->isFullscreen = cls.glconfig.isFullscreen;
	glconfig->stereoEnabled = cls.glconfig.stereoEnabled;
	glconfig->smpActive = cls.glconfig.smpActive;
}

static void CLET_GameRefEntToEngine(const etrefEntity_t* gameRefent, refEntity_t* refent)
{
	Com_Memset(refent, 0, sizeof(*refent));
	if (gameRefent->reType < 0 || gameRefent->reType >= 10)
	{
		refent->reType = RT_MAX_REF_ENTITY_TYPE;
	}
	else
	{
		refent->reType = clet_gameRefEntTypeToEngine[gameRefent->reType];
	}
	refent->renderfx = gameRefent->renderfx & (RF_MINLIGHT | RF_THIRD_PERSON |
											   RF_FIRST_PERSON | RF_DEPTHHACK);
	if (gameRefent->renderfx & ETRF_NOSHADOW)
	{
		refent->renderfx |= RF_NOSHADOW;
	}
	if (gameRefent->renderfx & ETRF_LIGHTING_ORIGIN)
	{
		refent->renderfx |= RF_LIGHTING_ORIGIN;
	}
	if (gameRefent->renderfx & ETRF_SHADOW_PLANE)
	{
		refent->renderfx |= RF_SHADOW_PLANE;
	}
	if (gameRefent->renderfx & ETRF_WRAP_FRAMES)
	{
		refent->renderfx |= RF_WRAP_FRAMES;
	}
	if (gameRefent->renderfx & ETRF_BLINK)
	{
		refent->renderfx |= RF_BLINK;
	}
	if (gameRefent->renderfx & ETRF_FORCENOLOD)
	{
		refent->renderfx |= RF_FORCENOLOD;
	}
	refent->hModel = gameRefent->hModel;
	VectorCopy(gameRefent->lightingOrigin, refent->lightingOrigin);
	refent->shadowPlane = gameRefent->shadowPlane;
	AxisCopy(gameRefent->axis, refent->axis);
	AxisCopy(gameRefent->torsoAxis, refent->torsoAxis);
	refent->nonNormalizedAxes = gameRefent->nonNormalizedAxes;
	VectorCopy(gameRefent->origin, refent->origin);
	refent->frame = gameRefent->frame;
	refent->frameModel = gameRefent->frameModel;
	refent->torsoFrame = gameRefent->torsoFrame;
	refent->torsoFrameModel = gameRefent->torsoFrameModel;
	VectorCopy(gameRefent->oldorigin, refent->oldorigin);
	refent->oldframe = gameRefent->oldframe;
	refent->oldframeModel = gameRefent->oldframeModel;
	refent->oldTorsoFrame = gameRefent->oldTorsoFrame;
	refent->oldTorsoFrameModel = gameRefent->oldTorsoFrameModel;
	refent->backlerp = gameRefent->backlerp;
	refent->torsoBacklerp = gameRefent->torsoBacklerp;
	refent->skinNum = gameRefent->skinNum;
	refent->customSkin = gameRefent->customSkin;
	refent->customShader = gameRefent->customShader;
	refent->shaderRGBA[0] = gameRefent->shaderRGBA[0];
	refent->shaderRGBA[1] = gameRefent->shaderRGBA[1];
	refent->shaderRGBA[2] = gameRefent->shaderRGBA[2];
	refent->shaderRGBA[3] = gameRefent->shaderRGBA[3];
	refent->shaderTexCoord[0] = gameRefent->shaderTexCoord[0];
	refent->shaderTexCoord[1] = gameRefent->shaderTexCoord[1];
	refent->shaderTime = gameRefent->shaderTime;
	refent->radius = gameRefent->radius;
	refent->rotation = gameRefent->rotation;
	VectorCopy(gameRefent->fireRiseDir, refent->fireRiseDir);
	refent->fadeStartTime = gameRefent->fadeStartTime;
	refent->fadeEndTime = gameRefent->fadeEndTime;
	refent->hilightIntensity = gameRefent->hilightIntensity;
	refent->reFlags = gameRefent->reFlags & (REFLAG_ONLYHAND |
											 REFLAG_FORCE_LOD | REFLAG_ORIENT_LOD | REFLAG_DEAD_LOD);
	refent->entityNum = gameRefent->entityNum;
}

void CLET_AddRefEntityToScene(const etrefEntity_t* ent)
{
	refEntity_t refent;
	CLET_GameRefEntToEngine(ent, &refent);
	R_AddRefEntityToScene(&refent);
}

void CLET_RenderScene(const etrefdef_t* gameRefdef)
{
	refdef_t rd;
	Com_Memset(&rd, 0, sizeof(rd));
	rd.x = gameRefdef->x;
	rd.y = gameRefdef->y;
	rd.width = gameRefdef->width;
	rd.height = gameRefdef->height;
	rd.fov_x = gameRefdef->fov_x;
	rd.fov_y = gameRefdef->fov_y;
	VectorCopy(gameRefdef->vieworg, rd.vieworg);
	AxisCopy(gameRefdef->viewaxis, rd.viewaxis);
	rd.time = gameRefdef->time;
	rd.rdflags = gameRefdef->rdflags & (RDF_NOWORLDMODEL | RDF_HYPERSPACE |
										RDF_SKYBOXPORTAL | RDF_UNDERWATER | RDF_DRAWINGSKY | RDF_SNOOPERVIEW);
	Com_Memcpy(rd.areamask, gameRefdef->areamask, sizeof(rd.areamask));
	Com_Memcpy(rd.text, gameRefdef->text, sizeof(rd.text));
	rd.glfog.mode = gameRefdef->glfog.mode;
	rd.glfog.hint = gameRefdef->glfog.hint;
	rd.glfog.startTime = gameRefdef->glfog.startTime;
	rd.glfog.finishTime = gameRefdef->glfog.finishTime;
	Vector4Copy(gameRefdef->glfog.color, rd.glfog.color);
	rd.glfog.start = gameRefdef->glfog.start;
	rd.glfog.end = gameRefdef->glfog.end;
	rd.glfog.useEndForClip = gameRefdef->glfog.useEndForClip;
	rd.glfog.density = gameRefdef->glfog.density;
	rd.glfog.registered = gameRefdef->glfog.registered;
	rd.glfog.drawsky = gameRefdef->glfog.drawsky;
	rd.glfog.clearscreen = gameRefdef->glfog.clearscreen;
	R_RenderScene(&rd);
}

int CLET_LerpTag(orientation_t* tag,  const etrefEntity_t* gameRefent, const char* tagName, int startIndex)
{
	refEntity_t refent;
	CLET_GameRefEntToEngine(gameRefent, &refent);
	return R_LerpTag(tag, &refent, tagName, startIndex);
}

void CLET_GetHunkInfo(int* hunkused, int* hunkexpected)
{
	*hunkused = 0;
	*hunkexpected = -1;
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

static void CLET_GetGameState(etgameState_t* gs)
{
	*gs = cl.et_gameState;
}

static void CLET_GetCurrentSnapshotNumber(int* snapshotNumber, int* serverTime)
{
	*snapshotNumber = cl.et_snap.messageNum;
	*serverTime = cl.et_snap.serverTime;
}

static bool CLET_GetSnapshot(int snapshotNumber, etsnapshot_t* snapshot)
{
	if (snapshotNumber > cl.et_snap.messageNum)
	{
		common->Error("CLET_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if (cl.et_snap.messageNum - snapshotNumber >= PACKET_BACKUP_Q3)
	{
		return false;
	}

	// if the frame is not valid, we can't return it
	etclSnapshot_t* clSnap = &cl.et_snapshots[snapshotNumber & PACKET_MASK_Q3];
	if (!clSnap->valid)
	{
		return false;
	}

	// if the entities in the frame have fallen out of their
	// circular buffer, we can't return it
	if (cl.parseEntitiesNum - clSnap->parseEntitiesNum >= MAX_PARSE_ENTITIES_Q3)
	{
		return false;
	}

	// write the snapshot
	snapshot->snapFlags = clSnap->snapFlags;
	snapshot->serverCommandSequence = clSnap->serverCommandNum;
	snapshot->ping = clSnap->ping;
	snapshot->serverTime = clSnap->serverTime;
	memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
	snapshot->ps = clSnap->ps;
	int count = clSnap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT_ET)
	{
		common->DPrintf("CLET_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT_ET);
		count = MAX_ENTITIES_IN_SNAPSHOT_ET;
	}
	snapshot->numEntities = count;
	for (int i = 0; i < count; i++)
	{
		snapshot->entities[i] =
			cl.et_parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES_Q3 - 1)];
	}

	// FIXME: configstring changes and server commands!!!

	return true;
}

static bool CLET_GetUserCmd(int cmdNumber, etusercmd_t* ucmd)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if (cmdNumber > cl.q3_cmdNumber)
	{
		common->Error("CLET_GetUserCmd: %i >= %i", cmdNumber, cl.q3_cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if (cmdNumber <= cl.q3_cmdNumber - CMD_BACKUP_Q3)
	{
		return false;
	}

	*ucmd = cl.et_cmds[cmdNumber & CMD_MASK_Q3];

	return true;
}

static void CLET_SetUserCmdValue(int userCmdValue, int flags, float sensitivityScale, int mpIdentClient)
{
	cl.q3_cgameUserCmdValue = userCmdValue;
	cl.et_cgameFlags = flags;
	cl.q3_cgameSensitivity = sensitivityScale;
	cl.wm_cgameMpIdentClient = mpIdentClient;			// NERVE - SMF
}

static void CLET_SetClientLerpOrigin(float x, float y, float z)
{
	cl.wm_cgameClientLerpOrigin[0] = x;
	cl.wm_cgameClientLerpOrigin[1] = y;
	cl.wm_cgameClientLerpOrigin[2] = z;
}

static void CLET_SendBinaryMessage(const char* buf, int buflen)
{
	if (buflen < 0 || buflen > MAX_BINARY_MESSAGE_ET)
	{
		common->Error("CLET_SendBinaryMessage: bad length %i", buflen);
		clc.et_binaryMessageLength = 0;
		return;
	}

	clc.et_binaryMessageLength = buflen;
	Com_Memcpy(clc.et_binaryMessage, buf, buflen);
}

static int CLET_BinaryMessageStatus()
{
	if (clc.et_binaryMessageLength == 0)
	{
		return ETMESSAGE_EMPTY;
	}

	if (clc.et_binaryMessageOverflowed)
	{
		return ETMESSAGE_WAITING_OVERFLOW;
	}

	return ETMESSAGE_WAITING;
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
	case ETCG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand((char*)VMA(1));
		return 0;
	case ETCG_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
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
	case ETCG_R_ADDREFENTITYTOSCENE:
		CLET_AddRefEntityToScene((etrefEntity_t*)VMA(1));
		return 0;
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
	case ETCG_R_RENDERSCENE:
		CLET_RenderScene((etrefdef_t*)VMA(1));
		return 0;
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
	case ETCG_R_LERPTAG:
		return CLET_LerpTag((orientation_t*)VMA(1), (etrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);
	case ETCG_GETGLCONFIG:
		CLET_GetGlconfig((etglconfig_t*)VMA(1));
		return 0;
	case ETCG_GETGAMESTATE:
		CLET_GetGameState((etgameState_t*)VMA(1));
		return 0;
	case ETCG_GETCURRENTSNAPSHOTNUMBER:
		CLET_GetCurrentSnapshotNumber((int*)VMA(1), (int*)VMA(2));
		return 0;
	case ETCG_GETSNAPSHOT:
		return CLET_GetSnapshot(args[1], (etsnapshot_t*)VMA(2));
	case ETCG_GETSERVERCOMMAND:
		return CLT3_GetServerCommand(args[1]);
	case ETCG_GETCURRENTCMDNUMBER:
		return CLT3_GetCurrentCmdNumber();
	case ETCG_GETUSERCMD:
		return CLET_GetUserCmd(args[1], (etusercmd_t*)VMA(2));
	case ETCG_SETUSERCMDVALUE:
		CLET_SetUserCmdValue(args[1], args[2], VMF(3), args[4]);
		return 0;
	case ETCG_SETCLIENTLERPORIGIN:
		CLET_SetClientLerpOrigin(VMF(1), VMF(2), VMF(3));
		return 0;
	case ETCG_MEMORY_REMAINING:
		return 0x4000000;
	case ETCG_KEY_ISDOWN:
		return Key_IsDown(args[1]);
	case ETCG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case ETCG_KEY_SETCATCHER:
		KeyWM_SetCatcher(args[1]);
		return 0;
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

	case ETCG_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case ETCG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	case ETCG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematicStretched((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);
	case ETCG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);
	case ETCG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);
	case ETCG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;
	case ETCG_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

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

	case ETCG_INGAME_POPUP:
		CLET_InGamePopup(args[1]);
		return 0;
	case ETCG_INGAME_CLOSEPOPUP:
		return 0;

	case ETCG_KEY_GETBINDINGBUF:
		Key_GetBindingBuf(args[1], (char*)VMA(2), args[3]);
		return 0;
	case ETCG_KEY_SETBINDING:
		Key_SetBinding(args[1], (char*)VMA(2));
		return 0;
	case ETCG_KEY_KEYNUMTOSTRINGBUF:
		Key_KeynumToStringBuf(args[1], (char*)VMA(2), args[3]);
		return 0;
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

	case ETCG_GETHUNKDATA:
		CLET_GetHunkInfo((int*)VMA(1), (int*)VMA(2));
		return 0;

	case ETCG_PUMPEVENTLOOP:
		return 0;

	case ETCG_SENDMESSAGE:
		CLET_SendBinaryMessage((char*)VMA(1), args[2]);
		return 0;
	case ETCG_MESSAGESTATUS:
		return CLET_BinaryMessageStatus();
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
