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

#include "../../client_main.h"
#include "../../ui/ui.h"
#include "../../cinematic/public.h"
#include "local.h"
#include "cg_public.h"
#include "cg_ui_shared.h"

static refEntityType_t clq3_gameRefEntTypeToEngine[] =
{
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,
};

void CLQ3_GetGlconfig(q3glconfig_t* glconfig)
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
	glconfig->vidWidth = cls.glconfig.vidWidth;
	glconfig->vidHeight = cls.glconfig.vidHeight;
	glconfig->windowAspect = cls.glconfig.windowAspect;
	glconfig->displayFrequency = cls.glconfig.displayFrequency;
	glconfig->isFullscreen = cls.glconfig.isFullscreen;
	glconfig->stereoEnabled = cls.glconfig.stereoEnabled;
	glconfig->smpActive = cls.glconfig.smpActive;
}

static void CLQ3_GameRefEntToEngine(const q3refEntity_t* gameRefent, refEntity_t* refent)
{
	Com_Memset(refent, 0, sizeof(*refent));
	refent->reType = clq3_gameRefEntTypeToEngine[gameRefent->reType];
	refent->renderfx = gameRefent->renderfx & (RF_MINLIGHT | RF_THIRD_PERSON |
											   RF_FIRST_PERSON | RF_DEPTHHACK | RF_NOSHADOW | RF_LIGHTING_ORIGIN |
											   RF_SHADOW_PLANE | RF_WRAP_FRAMES);
	refent->hModel = gameRefent->hModel;
	VectorCopy(gameRefent->lightingOrigin, refent->lightingOrigin);
	refent->shadowPlane = gameRefent->shadowPlane;
	AxisCopy(gameRefent->axis, refent->axis);
	refent->nonNormalizedAxes = gameRefent->nonNormalizedAxes;
	VectorCopy(gameRefent->origin, refent->origin);
	refent->frame = gameRefent->frame;
	VectorCopy(gameRefent->oldorigin, refent->oldorigin);
	refent->oldframe = gameRefent->oldframe;
	refent->backlerp = gameRefent->backlerp;
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
}

void CLQ3_AddRefEntityToScene(const q3refEntity_t* ent)
{
	refEntity_t refent;
	CLQ3_GameRefEntToEngine(ent, &refent);
	R_AddRefEntityToScene(&refent);
}

void CLQ3_RenderScene(const q3refdef_t* gameRefdef)
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
	rd.rdflags = gameRefdef->rdflags & (RDF_NOWORLDMODEL | RDF_HYPERSPACE);
	Com_Memcpy(rd.areamask, gameRefdef->areamask, sizeof(rd.areamask));
	Com_Memcpy(rd.text, gameRefdef->text, sizeof(rd.text));
	R_RenderScene(&rd);
}

static void CLQ3_GetGameState(q3gameState_t* gs)
{
	*gs = cl.q3_gameState;
}

static void CLQ3_GetCurrentSnapshotNumber(int* snapshotNumber, int* serverTime)
{
	*snapshotNumber = cl.q3_snap.messageNum;
	*serverTime = cl.q3_snap.serverTime;
}

static bool CLQ3_GetSnapshot(int snapshotNumber, q3snapshot_t* snapshot)
{
	if (snapshotNumber > cl.q3_snap.messageNum)
	{
		common->Error("CLQ3_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if (cl.q3_snap.messageNum - snapshotNumber >= PACKET_BACKUP_Q3)
	{
		return false;
	}

	// if the frame is not valid, we can't return it
	q3clSnapshot_t* clSnap = &cl.q3_snapshots[snapshotNumber & PACKET_MASK_Q3];
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
	Com_Memcpy(snapshot->areamask, clSnap->areamask, sizeof(snapshot->areamask));
	snapshot->ps = clSnap->ps;
	int count = clSnap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT_Q3)
	{
		common->DPrintf("CLQ3_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT_Q3);
		count = MAX_ENTITIES_IN_SNAPSHOT_Q3;
	}
	snapshot->numEntities = count;
	for (int i = 0; i < count; i++)
	{
		snapshot->entities[i] = cl.q3_parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES_Q3 - 1)];
	}

	// FIXME: configstring changes and server commands!!!

	return true;
}

static bool CLQ3_GetUserCmd(int cmdNumber, q3usercmd_t* ucmd)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if (cmdNumber > cl.q3_cmdNumber)
	{
		common->Error("CLQ3_GetUserCmd: %i >= %i", cmdNumber, cl.q3_cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if (cmdNumber <= cl.q3_cmdNumber - CMD_BACKUP_Q3)
	{
		return false;
	}

	*ucmd = cl.q3_cmds[cmdNumber & CMD_MASK_Q3];

	return true;
}

static void CLQ3_SetUserCmdValue(int userCmdValue, float sensitivityScale)
{
	cl.q3_cgameUserCmdValue = userCmdValue;
	cl.q3_cgameSensitivity = sensitivityScale;
}

//	The cgame module is making a system call
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
	case Q3CG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand((char*)VMA(1));
		return 0;
	case Q3CG_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
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
	case Q3CG_R_ADDREFENTITYTOSCENE:
		CLQ3_AddRefEntityToScene((q3refEntity_t*)VMA(1));
		return 0;
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
	case Q3CG_R_RENDERSCENE:
		CLQ3_RenderScene((q3refdef_t*)VMA(1));
		return 0;
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
	case Q3CG_GETGLCONFIG:
		CLQ3_GetGlconfig((q3glconfig_t*)VMA(1));
		return 0;
	case Q3CG_GETGAMESTATE:
		CLQ3_GetGameState((q3gameState_t*)VMA(1));
		return 0;
	case Q3CG_GETCURRENTSNAPSHOTNUMBER:
		CLQ3_GetCurrentSnapshotNumber((int*)VMA(1), (int*)VMA(2));
		return 0;
	case Q3CG_GETSNAPSHOT:
		return CLQ3_GetSnapshot(args[1], (q3snapshot_t*)VMA(2));
	case Q3CG_GETSERVERCOMMAND:
		return CLT3_GetServerCommand(args[1]);
	case Q3CG_GETCURRENTCMDNUMBER:
		return CLT3_GetCurrentCmdNumber();
	case Q3CG_GETUSERCMD:
		return CLQ3_GetUserCmd(args[1], (q3usercmd_t*)VMA(2));
	case Q3CG_SETUSERCMDVALUE:
		CLQ3_SetUserCmdValue(args[1], VMF(2));
		return 0;
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

	case Q3CG_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case Q3CG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	case Q3CG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematicStretched((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);
	case Q3CG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);
	case Q3CG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);
	case Q3CG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;
	case Q3CG_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

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
