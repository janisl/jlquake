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


#include "client.h"
#include "../../client/game/wolfsp/cg_ui_shared.h"
#include "../../client/game/wolfsp/ui_public.h"

void CL_GetGlconfig(wsglconfig_t* config);
void CL_AddRefEntityToScene(const wsrefEntity_t* ent);
void CL_RenderScene(const wsrefdef_t* refdef);
int CL_LerpTag(orientation_t* tag,  const wsrefEntity_t* refent, const char* tagName, int startIndex);

/*
====================
GetConfigString
====================
*/
static int GetConfigString(int index, char* buf, int size)
{
	int offset;

	if (index < 0 || index >= MAX_CONFIGSTRINGS_WS)
	{
		return false;
	}

	offset = cl.ws_gameState.stringOffsets[index];
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

/*
====================
CL_UISystemCalls

The ui module is making a system call
====================
*/
qintptr CL_UISystemCalls(qintptr* args)
{
	switch (args[0])
	{
//-------
	case WSUI_R_ADDREFENTITYTOSCENE:
		CL_AddRefEntityToScene((wsrefEntity_t*)VMA(1));
		return 0;
//-------
	case WSUI_R_RENDERSCENE:
		CL_RenderScene((wsrefdef_t*)VMA(1));
		return 0;
//-------
	case WSUI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;

	case WSUI_CM_LERPTAG:
		return CL_LerpTag((orientation_t*)VMA(1), (wsrefEntity_t*)VMA(2), (char*)VMA(3), args[4]);
//-------
	case WSUI_GETGLCONFIG:
		CL_GetGlconfig((wsglconfig_t*)VMA(1));
		return 0;

	case WSUI_GETCONFIGSTRING:
		return GetConfigString(args[1], (char*)VMA(2), args[3]);
//-------
	case WSUI_LAN_GETPINGQUEUECOUNT:
		return CL_GetPingQueueCount();

	case WSUI_LAN_CLEARPING:
		CL_ClearPing(args[1]);
		return 0;

	case WSUI_LAN_GETPING:
		CL_GetPing(args[1], (char*)VMA(2), args[3], (int*)VMA(4));
		return 0;

	case WSUI_LAN_GETPINGINFO:
		CL_GetPingInfo(args[1], (char*)VMA(2), args[3]);
		return 0;
//-------
	case WSUI_LAN_UPDATEVISIBLEPINGS:
		return CL_UpdateVisiblePings_f(args[1]);
//-------
	case WSUI_LAN_SERVERSTATUS:
		return CL_ServerStatus((char*)VMA(1), (char*)VMA(2), args[3]);
//-------
	case WSUI_REAL_TIME:
		return Com_RealTime((qtime_t*)VMA(1));
//-------
	case WSUI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);
//-------
	case WSUI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case WSUI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;
//-------
	case WSUI_VERIFY_CDKEY:
		return CL_CDKeyValidate((char*)VMA(1), (char*)VMA(2));

	case WSUI_CL_GETLIMBOSTRING:
		return CL_GetLimboString(args[1], (char*)VMA(2));
//-------
	}
	return 0;
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI(void)
{
	int v;
//----(SA)	always dll

	uivm = VM_Create("ui", CL_UISystemCalls, VMI_NATIVE);

	if (!uivm)
	{
		common->FatalError("VM_Create on UI failed");
	}

	// sanity check
	v = VM_Call(uivm, UI_GETAPIVERSION);
	if (v != WSUI_API_VERSION)
	{
		common->FatalError("User Interface is version %d, expected %d", v, WSUI_API_VERSION);
		cls.q3_uiStarted = false;
	}

	// init for this gamestate
//	VM_Call( uivm, UI_INIT );
	VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
}
