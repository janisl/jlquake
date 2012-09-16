/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// cl_cgame.c  -- client system interaction with client game

#include "client.h"

// RF, this is only used when running a local server
extern void SVWS_SendMoveSpeedsToGame(int entnum, char* text);
extern int SVWS_GetModelInfo(int clientNum, char* modelName, animModelInfo_t** modelInfo);


/*
====================
CL_GetGameState
====================
*/
void CL_GetGameState(wsgameState_t* gs)
{
	*gs = cl.ws_gameState;
}

/*
====================
CL_GetGlconfig
====================
*/
void CL_GetGlconfig(wsglconfig_t* glconfig)
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
	glconfig->textureFilterAnisotropicAvailable = cls.glconfig.textureFilterAnisotropicAvailable;
}


/*
====================
CL_GetUserCmd
====================
*/
qboolean CL_GetUserCmd(int cmdNumber, wsusercmd_t* ucmd)
{
	// cmds[cmdNumber] is the last properly generated command

	// can't return anything that we haven't created yet
	if (cmdNumber > cl.q3_cmdNumber)
	{
		common->Error("CL_GetUserCmd: %i >= %i", cmdNumber, cl.q3_cmdNumber);
	}

	// the usercmd has been overwritten in the wrapping
	// buffer because it is too far out of date
	if (cmdNumber <= cl.q3_cmdNumber - CMD_BACKUP_Q3)
	{
		return false;
	}

	*ucmd = cl.ws_cmds[cmdNumber & CMD_MASK_Q3];

	return true;
}

int CL_GetCurrentCmdNumber(void)
{
	return cl.q3_cmdNumber;
}


/*
====================
CL_GetParseEntityState
====================
*/
qboolean    CL_GetParseEntityState(int parseEntityNumber, wsentityState_t* state)
{
	// can't return anything that hasn't been parsed yet
	if (parseEntityNumber >= cl.parseEntitiesNum)
	{
		common->Error("CL_GetParseEntityState: %i >= %i",
			parseEntityNumber, cl.parseEntitiesNum);
	}

	// can't return anything that has been overwritten in the circular buffer
	if (parseEntityNumber <= cl.parseEntitiesNum - MAX_PARSE_ENTITIES_Q3)
	{
		return false;
	}

	*state = cl.ws_parseEntities[parseEntityNumber & (MAX_PARSE_ENTITIES_Q3 - 1)];
	return true;
}

/*
====================
CL_GetCurrentSnapshotNumber
====================
*/
void    CL_GetCurrentSnapshotNumber(int* snapshotNumber, int* serverTime)
{
	*snapshotNumber = cl.ws_snap.messageNum;
	*serverTime = cl.ws_snap.serverTime;
}

/*
====================
CL_GetSnapshot
====================
*/
qboolean    CL_GetSnapshot(int snapshotNumber, wssnapshot_t* snapshot)
{
	wsclSnapshot_t* clSnap;
	int i, count;

	if (snapshotNumber > cl.ws_snap.messageNum)
	{
		common->Error("CL_GetSnapshot: snapshotNumber > cl.snapshot.messageNum");
	}

	// if the frame has fallen out of the circular buffer, we can't return it
	if (cl.ws_snap.messageNum - snapshotNumber >= PACKET_BACKUP_Q3)
	{
		return false;
	}

	// if the frame is not valid, we can't return it
	clSnap = &cl.ws_snapshots[snapshotNumber & PACKET_MASK_Q3];
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
	count = clSnap->numEntities;
	if (count > MAX_ENTITIES_IN_SNAPSHOT_WS)
	{
		common->DPrintf("CL_GetSnapshot: truncated %i entities to %i\n", count, MAX_ENTITIES_IN_SNAPSHOT_WS);
		count = MAX_ENTITIES_IN_SNAPSHOT_WS;
	}
	snapshot->numEntities = count;
	for (i = 0; i < count; i++)
	{
		snapshot->entities[i] =
			cl.ws_parseEntities[(clSnap->parseEntitiesNum + i) & (MAX_PARSE_ENTITIES_Q3 - 1)];
	}

	// FIXME: configstring changes and server commands!!!

	return true;
}

/*
==============
CL_SetUserCmdValue
==============
*/
void CL_SetUserCmdValue(int userCmdValue, int holdableValue, float sensitivityScale, int cld)
{
	cl.q3_cgameUserCmdValue        = userCmdValue;
	cl.wb_cgameUserHoldableValue   = holdableValue;
	cl.q3_cgameSensitivity         = sensitivityScale;
	cl.ws_cgameCld                 = cld;
}

/*
==============
CL_AddCgameCommand
==============
*/
void CL_AddCgameCommand(const char* cmdName)
{
	Cmd_AddCommand(cmdName, NULL);
}

/*
==============
CL_CgameError
==============
*/
void CL_CgameError(const char* string)
{
	common->Error("%s", string);
}


/*
=====================
CL_ConfigstringModified
=====================
*/
void CL_ConfigstringModified(void)
{
	char* old, * s;
	int i, index;
	char* dup;
	wsgameState_t oldGs;
	int len;

	index = String::Atoi(Cmd_Argv(1));
	if (index < 0 || index >= MAX_CONFIGSTRINGS_WS)
	{
		common->Error("configstring > MAX_CONFIGSTRINGS_WS");
	}
//	s = Cmd_Argv(2);
	// get everything after "cs <num>"
	s = Cmd_ArgsFrom(2);

	old = cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[index];
	if (!String::Cmp(old, s))
	{
		return;		// unchanged
	}

	// build the new wsgameState_t
	oldGs = cl.ws_gameState;

	memset(&cl.ws_gameState, 0, sizeof(cl.ws_gameState));

	// leave the first 0 for uninitialized strings
	cl.ws_gameState.dataCount = 1;

	for (i = 0; i < MAX_CONFIGSTRINGS_WS; i++)
	{
		if (i == index)
		{
			dup = s;
		}
		else
		{
			dup = oldGs.stringData + oldGs.stringOffsets[i];
		}
		if (!dup[0])
		{
			continue;		// leave with the default empty string
		}

		len = String::Length(dup);

		if (len + 1 + cl.ws_gameState.dataCount > MAX_GAMESTATE_CHARS_Q3)
		{
			common->Error("MAX_GAMESTATE_CHARS_Q3 exceeded");
		}

		// append it to the gameState string buffer
		cl.ws_gameState.stringOffsets[i] = cl.ws_gameState.dataCount;
		memcpy(cl.ws_gameState.stringData + cl.ws_gameState.dataCount, dup, len + 1);
		cl.ws_gameState.dataCount += len + 1;
	}

	if (index == Q3CS_SYSTEMINFO)
	{
		// parse serverId and other cvars
		CL_SystemInfoChanged();
	}

}


/*
===================
CL_GetServerCommand

Set up argc/argv for the given command
===================
*/
qboolean CL_GetServerCommand(int serverCommandNumber)
{
	char* s;
	char* cmd;
	static char bigConfigString[BIG_INFO_STRING];

	// if we have irretrievably lost a reliable command, drop the connection
	if (serverCommandNumber <= clc.q3_serverCommandSequence - MAX_RELIABLE_COMMANDS_WOLF)
	{
		// when a demo record was started after the client got a whole bunch of
		// reliable commands then the client never got those first reliable commands
		if (clc.demoplaying)
		{
			return false;
		}
		common->Error("CL_GetServerCommand: a reliable command was cycled out");
		return false;
	}

	if (serverCommandNumber > clc.q3_serverCommandSequence)
	{
		common->Error("CL_GetServerCommand: requested a command not received");
		return false;
	}

	s = clc.q3_serverCommands[serverCommandNumber & (MAX_RELIABLE_COMMANDS_WOLF - 1)];
	clc.q3_lastExecutedServerCommand = serverCommandNumber;

	common->DPrintf("serverCommand: %i : %s\n", serverCommandNumber, s);

rescan:
	Cmd_TokenizeString(s);
	cmd = Cmd_Argv(0);

	if (!String::Cmp(cmd, "disconnect"))
	{
		Com_Error(ERR_SERVERDISCONNECT,"Server disconnected\n");
	}

	if (!String::Cmp(cmd, "bcs0"))
	{
		String::Sprintf(bigConfigString, BIG_INFO_STRING, "cs %s \"%s", Cmd_Argv(1), Cmd_Argv(2));
		return false;
	}

	if (!String::Cmp(cmd, "bcs1"))
	{
		s = Cmd_Argv(2);
		if (String::Length(bigConfigString) + String::Length(s) >= BIG_INFO_STRING)
		{
			common->Error("bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		return false;
	}

	if (!String::Cmp(cmd, "bcs2"))
	{
		s = Cmd_Argv(2);
		if (String::Length(bigConfigString) + String::Length(s) + 1 >= BIG_INFO_STRING)
		{
			common->Error("bcs exceeded BIG_INFO_STRING");
		}
		strcat(bigConfigString, s);
		strcat(bigConfigString, "\"");
		s = bigConfigString;
		goto rescan;
	}

	if (!String::Cmp(cmd, "cs"))
	{
		CL_ConfigstringModified();
		// reparse the string, because CL_ConfigstringModified may have done another Cmd_TokenizeString()
		Cmd_TokenizeString(s);
		return true;
	}

	if (!String::Cmp(cmd, "map_restart"))
	{
		// clear notify lines and outgoing commands before passing
		// the restart to the cgame
		Con_ClearNotify();
		memset(cl.ws_cmds, 0, sizeof(cl.ws_cmds));
		return true;
	}

	if (!String::Cmp(cmd, "popup"))			// direct server to client popup request, bypassing cgame
	{	//		trap_UI_Popup(Cmd_Argv(1));
//		if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
//			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_CLIPBOARD);
//			Menus_OpenByName(Cmd_Argv(1));
//		}
		return false;
	}


	// the clientLevelShot command is used during development
	// to generate 128*128 screenshots from the intermission
	// point of levels for the menu system to use
	// we pass it along to the cgame to make apropriate adjustments,
	// but we also clear the console and notify lines here
	if (!String::Cmp(cmd, "clientLevelShot"))
	{
		// don't do it if we aren't running the server locally,
		// otherwise malicious remote servers could overwrite
		// the existing thumbnails
		if (!com_sv_running->integer)
		{
			return false;
		}
		// close the console
		Con_Close();
		// take a special screenshot next frame
		Cbuf_AddText("wait ; wait ; wait ; wait ; screenshot levelshot\n");
		return true;
	}

	// we may want to put a "connect to other server" command here

	// cgame can now act on the command
	return true;
}


/*
====================
CL_CM_LoadMap

Just adds default parameters that cgame doesn't need to know about
====================
*/
void CL_CM_LoadMap(const char* mapname)
{
	int checksum;

	CM_LoadMap(mapname, true, &checksum);
}

static refEntityType_t gameRefEntTypeToEngine[] =
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

static void CL_GameRefEntToEngine(const wsrefEntity_t* gameRefent, refEntity_t* refent)
{
	Com_Memset(refent, 0, sizeof(*refent));
	if (gameRefent->reType < 0 || gameRefent->reType >= 10)
	{
		refent->reType = RT_MAX_REF_ENTITY_TYPE;
	}
	else
	{
		refent->reType = gameRefEntTypeToEngine[gameRefent->reType];
	}
	refent->renderfx = gameRefent->renderfx & (RF_MINLIGHT | RF_THIRD_PERSON |
											   RF_FIRST_PERSON | RF_DEPTHHACK | RF_NOSHADOW | RF_LIGHTING_ORIGIN |
											   RF_SHADOW_PLANE | RF_WRAP_FRAMES);
	if (gameRefent->renderfx & WSRF_BLINK)
	{
		refent->renderfx |= RF_BLINK;
	}
	refent->hModel = gameRefent->hModel;
	VectorCopy(gameRefent->lightingOrigin, refent->lightingOrigin);
	refent->shadowPlane = gameRefent->shadowPlane;
	AxisCopy(gameRefent->axis, refent->axis);
	AxisCopy(gameRefent->torsoAxis, refent->torsoAxis);
	refent->nonNormalizedAxes = gameRefent->nonNormalizedAxes;
	VectorCopy(gameRefent->origin, refent->origin);
	refent->frame = gameRefent->frame;
	refent->torsoFrame = gameRefent->torsoFrame;
	VectorCopy(gameRefent->scale, refent->scale);
	VectorCopy(gameRefent->oldorigin, refent->oldorigin);
	refent->oldframe = gameRefent->oldframe;
	refent->oldTorsoFrame = gameRefent->oldTorsoFrame;
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
	refent->reFlags = gameRefent->reFlags & (REFLAG_ONLYHAND  | REFLAG_FORCE_LOD |
											 REFLAG_ORIENT_LOD | REFLAG_DEAD_LOD | REFLAG_SCALEDSPHERECULL | REFLAG_FULL_LOD);
	refent->entityNum = gameRefent->entityNum;
}

void CL_AddRefEntityToScene(const wsrefEntity_t* ent)
{
	refEntity_t refent;
	CL_GameRefEntToEngine(ent, &refent);
	R_AddRefEntityToScene(&refent);
}

void CL_RenderScene(const wsrefdef_t* gameRefdef)
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
	if (gameRefdef->rdflags & WSRDF_DRAWSKYBOX)
	{
		rd.rdflags |= RDF_DRAWSKYBOX;
	}
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
	rd.glfog.dirty = gameRefdef->glfog.dirty;
	R_RenderScene(&rd);
}

int CL_LerpTag(orientation_t* tag,  const wsrefEntity_t* gameRefent, const char* tagName, int startIndex)
{
	refEntity_t refent;
	CL_GameRefEntToEngine(gameRefent, &refent);
	return R_LerpTag(tag, &refent, tagName, startIndex);
}

/*
====================
CL_ShutdonwCGame

====================
*/
void CL_ShutdownCGame(void)
{
	in_keyCatchers &= ~KEYCATCH_CGAME;
	cls.q3_cgameStarted = false;
	if (!cgvm)
	{
		return;
	}
	VM_Call(cgvm, CG_SHUTDOWN);
	VM_Free(cgvm);
	cgvm = NULL;
}

/*
====================
CL_CgameSystemCalls

The cgame module is making a system call
====================
*/
qintptr CL_CgameSystemCalls(qintptr* args)
{
	switch (args[0])
	{
	case WSCG_PRINT:
		common->Printf("%s", VMA(1));
		return 0;
	case WSCG_ERROR:
		common->Error("%s", VMA(1));
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
		CL_AddCgameCommand((char*)VMA(1));
		return 0;
	case WSCG_REMOVECOMMAND:
		Cmd_RemoveCommand((char*)VMA(1));
		return 0;
	case WSCG_SENDCLIENTCOMMAND:
		CL_AddReliableCommand((char*)VMA(1));
		return 0;
	case WSCG_UPDATESCREEN:
		// this is used during lengthy level loading, so pump message loop
//		Com_EventLoop();	// FIXME: if a server restarts here, BAD THINGS HAPPEN!
// We can't call Com_EventLoop here, a restart will crash and this _does_ happen
// if there is a map change while we are downloading at pk3.
// ZOID
		SCR_UpdateScreen();
		return 0;
	case WSCG_CM_LOADMAP:
		CL_CM_LoadMap((char*)VMA(1));
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
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7],	/*int capsule*/ false);
		return 0;
	case WSCG_CM_TRANSFORMEDBOXTRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), /*int capsule*/ false);
		return 0;
	case WSCG_CM_CAPSULETRACE:
		CM_BoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7],	/*int capsule*/ true);
		return 0;
	case WSCG_CM_TRANSFORMEDCAPSULETRACE:
		CM_TransformedBoxTraceQ3((q3trace_t*)VMA(1), (float*)VMA(2), (float*)VMA(3), (float*)VMA(4), (float*)VMA(5), args[6], args[7], (float*)VMA(8), (float*)VMA(9), /*int capsule*/ true);
		return 0;
	case WSCG_CM_MARKFRAGMENTS:
		return R_MarkFragmentsWolf(args[1], (const vec3_t*)VMA(2), (float*)VMA(3), args[4], (float*)VMA(5), args[6], (markFragment_t*)VMA(7));
	case WSCG_S_STARTSOUND:
		S_StartSound((float*)VMA(1), args[2], args[3], args[4], 0.5);
		return 0;
//----(SA)	added
	case WSCG_S_STARTSOUNDEX:
		S_StartSoundEx((float*)VMA(1), args[2], args[3], args[4], args[5], 127);
		return 0;
//----(SA)	end
	case WSCG_S_STARTLOCALSOUND:
		S_StartLocalSound(args[1], args[2], 127);
		return 0;
	case WSCG_S_CLEARLOOPINGSOUNDS:
		S_ClearLoopingSounds(true);	// (SA) modified so no_pvs sounds can function
		// RF, if killall, then stop all sounds
		if (args[1] == 1)
		{
			S_ClearSounds(true, false);
		}
		else if (args[1] == 2)
		{
			S_ClearSounds(true, true);
		}
		return 0;
	case WSCG_S_ADDLOOPINGSOUND:
		// FIXME MrE: handling of looping sounds changed
		S_AddLoopingSound(args[1], (float*)VMA(2), (float*)VMA(3), args[4], args[5], args[6], 0);
		return 0;
// not in use
//	case WSCG_S_ADDREALLOOPINGSOUND:
//		S_AddLoopingSound( args[1], VMA(2), VMA(3), args[4], args[5], args[6] );
//		//S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4], args[5] );
//		return 0;

//----(SA)	added
	case WSCG_S_STOPSTREAMINGSOUND:
		S_StopEntStreamingSound(args[1]);
		return 0;
//----(SA)	end

	case WSCG_S_STOPLOOPINGSOUND:
		// RF, not functional anymore, since we reverted to old looping code
		//S_StopLoopingSound( args[1] );
		return 0;
	case WSCG_S_UPDATEENTITYPOSITION:
		S_UpdateEntityPosition(args[1], (float*)VMA(2));
		return 0;
// Ridah, talking animations
	case WSCG_S_GETVOICEAMPLITUDE:
		return S_GetVoiceAmplitude(args[1]);
// done.
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

	//----(SA)	added
	case WSCG_R_GETSKINMODEL:
		return R_GetSkinModel(args[1], (char*)VMA(2), (char*)VMA(3));
	case WSCG_R_GETMODELSHADER:
		return R_GetShaderFromModel(args[1], args[2], args[3]);
	//----(SA)	end

	case WSCG_R_REGISTERSHADER:
		return R_RegisterShader((char*)VMA(1));
	case WSCG_R_REGISTERFONT:
		R_RegisterFont((char*)VMA(1), args[2], (fontInfo_t*)VMA(3));
	case WSCG_R_REGISTERSHADERNOMIP:
		return R_RegisterShaderNoMip((char*)VMA(1));
	case WSCG_R_CLEARSCENE:
		R_ClearScene();
		return 0;
	case WSCG_R_ADDREFENTITYTOSCENE:
		CL_AddRefEntityToScene((wsrefEntity_t*)VMA(1));
		return 0;
	case WSCG_R_ADDPOLYTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), 1);
		return 0;
	// Ridah
	case WSCG_R_ADDPOLYSTOSCENE:
		R_AddPolyToScene(args[1], args[2], (polyVert_t*)VMA(3), args[4]);
		return 0;
	case WSCG_RB_ZOMBIEFXADDNEWHIT:
		return 0;
	// done.
	case WSCG_R_ADDLIGHTTOSCENE:
		R_AddLightToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6]);
		return 0;
	case WSCG_R_ADDCORONATOSCENE:
		R_AddCoronaToScene((float*)VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
		return 0;
	case WSCG_R_SETFOG:
		R_SetFog(args[1], args[2], args[3], VMF(4), VMF(5), VMF(6), VMF(7));
		return 0;
	case WSCG_R_RENDERSCENE:
		CL_RenderScene((wsrefdef_t*)VMA(1));
		return 0;
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
	case WSCG_R_LERPTAG:
		return CL_LerpTag((orientation_t*)VMA(1), (wsrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);
	case WSCG_GETGLCONFIG:
		CL_GetGlconfig((wsglconfig_t*)VMA(1));
		return 0;
	case WSCG_GETGAMESTATE:
		CL_GetGameState((wsgameState_t*)VMA(1));
		return 0;
	case WSCG_GETCURRENTSNAPSHOTNUMBER:
		CL_GetCurrentSnapshotNumber((int*)VMA(1), (int*)VMA(2));
		return 0;
	case WSCG_GETSNAPSHOT:
		return CL_GetSnapshot(args[1], (wssnapshot_t*)VMA(2));
	case WSCG_GETSERVERCOMMAND:
		return CL_GetServerCommand(args[1]);
	case WSCG_GETCURRENTCMDNUMBER:
		return CL_GetCurrentCmdNumber();
	case WSCG_GETUSERCMD:
		return CL_GetUserCmd(args[1], (wsusercmd_t*)VMA(2));
	case WSCG_SETUSERCMDVALUE:
		CL_SetUserCmdValue(args[1], args[2], VMF(3), args[4]);			//----(SA)	modified	// NERVE - SMF - added fourth arg [cld]
		return 0;
	case WSCG_MEMORY_REMAINING:
		return 0x4000000;
	case WSCG_KEY_ISDOWN:
		return Key_IsDown(args[1]);
	case WSCG_KEY_GETCATCHER:
		return Key_GetCatcher();
	case WSCG_KEY_SETCATCHER:
		Key_SetCatcher(args[1]);
		return 0;
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

	case WSCG_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
	case WSCG_SNAPVECTOR:
		Sys_SnapVector((float*)VMA(1));
		return 0;

	case WSCG_SENDMOVESPEEDSTOGAME:
		SVWS_SendMoveSpeedsToGame(args[1], (char*)VMA(2));
		return 0;

	case WSCG_CIN_PLAYCINEMATIC:
		return CIN_PlayCinematic((char*)VMA(1), args[2], args[3], args[4], args[5], args[6]);

	case WSCG_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);

	case WSCG_CIN_RUNCINEMATIC:
		return CIN_RunCinematic(args[1]);

	case WSCG_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case WSCG_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;

	case WSCG_R_REMAP_SHADER:
		R_RemapShader((char*)VMA(1), (char*)VMA(2), (char*)VMA(3));
		return 0;

	case WSCG_TESTPRINTINT:
		common->Printf("%s%i\n", VMA(1), args[2]);
		return 0;
	case WSCG_TESTPRINTFLOAT:
		common->Printf("%s%f\n", VMA(1), VMF(2));
		return 0;

	case WSCG_LOADCAMERA:
		return loadCamera(args[1], (char*)VMA(2));

	case WSCG_STARTCAMERA:
		if (args[1] == 0)		// CAM_PRIMARY
		{
			cl.wa_cameraMode = true;	//----(SA)	added
		}
		startCamera(args[1], args[2]);
		return 0;

//----(SA)	added
	case WSCG_STOPCAMERA:
		if (args[1] == 0)		// CAM_PRIMARY
		{
			cl.wa_cameraMode = false;
		}
//		stopCamera(args[1]);
		return 0;
//----(SA)	end

	case WSCG_GETCAMERAINFO:
		return getCameraInfo(args[1], args[2], (float*)VMA(3), (float*)VMA(4), (float*)VMA(5));

	case WSCG_GET_ENTITY_TOKEN:
		return R_GetEntityToken((char*)VMA(1), args[2]);

	case WSCG_INGAME_POPUP:
		if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "briefing"))				//----(SA) added
		{
			VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_BRIEFING);
			return 0;
		}

		if (cls.state == CA_ACTIVE && !clc.demoplaying)
		{
			// NERVE - SMF
			if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "UIMENU_WM_PICKTEAM"))
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_WM_PICKTEAM);
			}
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "UIMENU_WM_PICKPLAYER"))
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_WM_PICKPLAYER);
			}
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "UIMENU_WM_QUICKMESSAGE"))
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_WM_QUICKMESSAGE);
			}
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "UIMENU_WM_LIMBO"))
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_WM_LIMBO);
			}
			// -NERVE - SMF
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "hbook1"))				//----(SA)
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_BOOK1);
			}
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "hbook2"))				//----(SA)
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_BOOK2);
			}
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "hbook3"))				//----(SA)
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_BOOK3);
			}
			else if ((char*)VMA(1) && !String::ICmp((char*)VMA(1), "pregame"))					//----(SA) added
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_PREGAME);
			}
			else
			{
				VM_Call(uivm, UI_SET_ACTIVE_MENU, WSUIMENU_CLIPBOARD);
			}
		}
		return 0;

	// NERVE - SMF
	case WSCG_INGAME_CLOSEPOPUP:
		VM_Call(uivm, UI_KEY_EVENT, K_ESCAPE, true);
		return 0;

	case WSCG_LIMBOCHAT:
		if (VMA(1))
		{
			CL_AddToLimboChat((char*)VMA(1));
		}
		return 0;
	// - NERVE - SMF

	case WSCG_GETMODELINFO:
		return SVWS_GetModelInfo(args[1], (char*)VMA(2), (animModelInfo_t**)VMA(3));

	default:
		common->Error("Bad cgame system trap: %i", args[0]);
	}
	return 0;
}

/*
====================
CL_InitCGame

Should only by called by CL_StartHunkUsers
====================
*/
void CL_InitCGame(void)
{
	const char* info;
	const char* mapname;
	int t1, t2;

	t1 = Sys_Milliseconds();

	// put away the console
	Con_Close();

	// find the current mapname
	info = cl.ws_gameState.stringData + cl.ws_gameState.stringOffsets[Q3CS_SERVERINFO];
	mapname = Info_ValueForKey(info, "mapname");
	String::Sprintf(cl.q3_mapname, sizeof(cl.q3_mapname), "maps/%s.bsp", mapname);

	cgvm = VM_Create("cgame", CL_CgameSystemCalls, VMI_NATIVE);
	if (!cgvm)
	{
		common->Error("VM_Create on cgame failed");
	}
	cls.state = CA_LOADING;

	// init for this gamestate
	// use the lastExecutedServerCommand instead of the serverCommandSequence
	// otherwise server commands sent just before a gamestate are dropped
	VM_Call(cgvm, CG_INIT, clc.q3_serverMessageSequence, clc.q3_lastExecutedServerCommand, clc.q3_clientNum);
//	VM_Call( cgvm, CG_INIT, clc.q3_serverMessageSequence, clc.q3_serverCommandSequence );

	// we will send a usercmd this frame, which
	// will cause the server to send us the first snapshot
	cls.state = CA_PRIMED;

	t2 = Sys_Milliseconds();

	common->Printf("CL_InitCGame: %5.2f seconds\n", (t2 - t1) / 1000.0);

	// have the renderer touch all its images, so they are present
	// on the card even if the driver does deferred loading
	R_EndRegistration();

	// clear anything that got printed
	Con_ClearNotify();
}


/*
====================
CL_GameCommand

See if the current console command is claimed by the cgame
====================
*/
qboolean CL_GameCommand(void)
{
	if (!cgvm)
	{
		return false;
	}

	return VM_Call(cgvm, CG_CONSOLE_COMMAND);
}



/*
=====================
CL_CGameRendering
=====================
*/
void CL_CGameRendering(stereoFrame_t stereo)
{
	VM_Call(cgvm, CG_DRAW_ACTIVE_FRAME, cl.serverTime, stereo, clc.demoplaying);
	VM_Debug(0);
}


/*
=================
CL_AdjustTimeDelta

Adjust the clients view of server time.

We attempt to have cl.serverTime exactly equal the server's view
of time plus the timeNudge, but with variable latencies over
the internet it will often need to drift a bit to match conditions.

Our ideal time would be to have the adjusted time approach, but not pass,
the very latest snapshot.

Adjustments are only made when a new snapshot arrives with a rational
latency, which keeps the adjustment process framerate independent and
prevents massive overadjustment during times of significant packet loss
or bursted delayed packets.
=================
*/

#define RESET_TIME  500

void CL_AdjustTimeDelta(void)
{
	int resetTime;
	int newDelta;
	int deltaDelta;

	cl.q3_newSnapshots = false;

	// the delta never drifts when replaying a demo
	if (clc.demoplaying)
	{
		return;
	}

	// if the current time is WAY off, just correct to the current value
	if (com_sv_running->integer)
	{
		resetTime = 100;
	}
	else
	{
		resetTime = RESET_TIME;
	}

	newDelta = cl.ws_snap.serverTime - cls.realtime;
	deltaDelta = abs(newDelta - cl.q3_serverTimeDelta);

	if (deltaDelta > RESET_TIME)
	{
		cl.q3_serverTimeDelta = newDelta;
		cl.q3_oldServerTime = cl.ws_snap.serverTime;	// FIXME: is this a problem for cgame?
		cl.serverTime = cl.ws_snap.serverTime;
		if (cl_showTimeDelta->integer)
		{
			common->Printf("<RESET> ");
		}
	}
	else if (deltaDelta > 100)
	{
		// fast adjust, cut the difference in half
		if (cl_showTimeDelta->integer)
		{
			common->Printf("<FAST> ");
		}
		cl.q3_serverTimeDelta = (cl.q3_serverTimeDelta + newDelta) >> 1;
	}
	else
	{
		// slow drift adjust, only move 1 or 2 msec

		// if any of the frames between this and the previous snapshot
		// had to be extrapolated, nudge our sense of time back a little
		// the granularity of +1 / -2 is too high for timescale modified frametimes
		if (com_timescale->value == 0 || com_timescale->value == 1)
		{
			if (cl.q3_extrapolatedSnapshot)
			{
				cl.q3_extrapolatedSnapshot = false;
				cl.q3_serverTimeDelta -= 2;
			}
			else
			{
				// otherwise, move our sense of time forward to minimize total latency
				cl.q3_serverTimeDelta++;
			}
		}
	}

	if (cl_showTimeDelta->integer)
	{
		common->Printf("%i ", cl.q3_serverTimeDelta);
	}
}


/*
==================
CL_FirstSnapshot
==================
*/
void CL_FirstSnapshot(void)
{
	// ignore snapshots that don't have entities
	if (cl.ws_snap.snapFlags & Q3SNAPFLAG_NOT_ACTIVE)
	{
		return;
	}
	cls.state = CA_ACTIVE;

	// set the timedelta so we are exactly on this first frame
	cl.q3_serverTimeDelta = cl.ws_snap.serverTime - cls.realtime;
	cl.q3_oldServerTime = cl.ws_snap.serverTime;

	clc.q3_timeDemoBaseTime = cl.ws_snap.serverTime;

	// if this is the first frame of active play,
	// execute the contents of activeAction now
	// this is to allow scripting a timedemo to start right
	// after loading
	if (cl_activeAction->string[0])
	{
		Cbuf_AddText(cl_activeAction->string);
		Cvar_Set("activeAction", "");
	}

	Sys_BeginProfiling();
}

/*
==================
CL_SetCGameTime
==================
*/
void CL_SetCGameTime(void)
{
	// getting a valid frame message ends the connection process
	if (cls.state != CA_ACTIVE)
	{
		if (cls.state != CA_PRIMED)
		{
			return;
		}
		if (clc.demoplaying)
		{
			// we shouldn't get the first snapshot on the same frame
			// as the gamestate, because it causes a bad time skip
			if (!clc.q3_firstDemoFrameSkipped)
			{
				clc.q3_firstDemoFrameSkipped = true;
				return;
			}
			CL_ReadDemoMessage();
		}
		if (cl.q3_newSnapshots)
		{
			cl.q3_newSnapshots = false;
			CL_FirstSnapshot();
		}
		if (cls.state != CA_ACTIVE)
		{
			return;
		}
	}

	// if we have gotten to this point, cl.ws_snap is guaranteed to be valid
	if (!cl.ws_snap.valid)
	{
		common->Error("CL_SetCGameTime: !cl.ws_snap.valid");
	}

	// allow pause in single player
	if (sv_paused->integer && cl_paused->integer && com_sv_running->integer)
	{
		// paused
		return;
	}

	if (cl.ws_snap.serverTime < cl.q3_oldFrameServerTime)
	{
		// Ridah, if this is a localhost, then we are probably loading a savegame
		if (!String::ICmp(cls.servername, "localhost"))
		{
			// do nothing?
			CL_FirstSnapshot();
		}
		else
		{
			common->Error("cl.ws_snap.serverTime < cl.oldFrameServerTime");
		}
	}
	cl.q3_oldFrameServerTime = cl.ws_snap.serverTime;


	// get our current view of time

	if (clc.demoplaying && cl_freezeDemo->integer)
	{
		// cl_freezeDemo is used to lock a demo in place for single frame advances

	}
	else
	{
		// cl_timeNudge is a user adjustable cvar that allows more
		// or less latency to be added in the interest of better
		// smoothness or better responsiveness.
		int tn;

		tn = cl_timeNudge->integer;
		if (tn < -30)
		{
			tn = -30;
		}
		else if (tn > 30)
		{
			tn = 30;
		}

		cl.serverTime = cls.realtime + cl.q3_serverTimeDelta - tn;

		// guarantee that time will never flow backwards, even if
		// serverTimeDelta made an adjustment or cl_timeNudge was changed
		if (cl.serverTime < cl.q3_oldServerTime)
		{
			cl.serverTime = cl.q3_oldServerTime;
		}
		cl.q3_oldServerTime = cl.serverTime;

		// note if we are almost past the latest frame (without timeNudge),
		// so we will try and adjust back a bit when the next snapshot arrives
		if (cls.realtime + cl.q3_serverTimeDelta >= cl.ws_snap.serverTime - 5)
		{
			cl.q3_extrapolatedSnapshot = true;
		}
	}

	// if we have gotten new snapshots, drift serverTimeDelta
	// don't do this every frame, or a period of packet loss would
	// make a huge adjustment
	if (cl.q3_newSnapshots)
	{
		CL_AdjustTimeDelta();
	}

	if (!clc.demoplaying)
	{
		return;
	}

	// if we are playing a demo back, we can just keep reading
	// messages from the demo file until the cgame definately
	// has valid snapshots to interpolate between

	// a timedemo will always use a deterministic set of time samples
	// no matter what speed machine it is run on,
	// while a normal demo may have different time samples
	// each time it is played back
	if (cl_timedemo->integer)
	{
		if (!clc.q3_timeDemoStart)
		{
			clc.q3_timeDemoStart = Sys_Milliseconds();
		}
		clc.q3_timeDemoFrames++;
		cl.serverTime = clc.q3_timeDemoBaseTime + clc.q3_timeDemoFrames * 50;
	}

	while (cl.serverTime >= cl.ws_snap.serverTime)
	{
		// feed another messag, which should change
		// the contents of cl.ws_snap
		CL_ReadDemoMessage();
		if (cls.state != CA_ACTIVE)
		{
			return;		// end of demo
		}
	}

}

/*
====================
CL_GetTag
====================
*/
bool CL_GetTag(int clientNum, const char* tagname, orientation_t* _or)
{
	if (!cgvm)
	{
		return false;
	}

	return VM_Call(cgvm, WSCG_GET_TAG, clientNum, tagname, _or);
}
