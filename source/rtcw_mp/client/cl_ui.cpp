/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "client.h"
#include "../../client/game/wolfmp/ui_public.h"

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
	case WMUI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
//-------
	case WMUI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);
//-------
	case WMUI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case WMUI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;
//-------
	case WMUI_VERIFY_CDKEY:
		return CL_CDKeyValidate((char*)VMA(1), (char*)VMA(2));

	case WMUI_CL_GETLIMBOSTRING:
		return CL_GetLimboString(args[1], (char*)VMA(2));
//-------
	case WMUI_OPENURL:
		CL_OpenURL((const char*)VMA(1));
		return 0;
//-------
	}
	return CLWM_UISystemCalls(args);
}

/*
====================
CL_InitUI
====================
*/

void CL_InitUI(void)
{
	int v;

	uivm = VM_Create("ui", CL_UISystemCalls, VMI_NATIVE);
	if (!uivm)
	{
		common->FatalError("VM_Create on UI failed");
	}

	// sanity check
	v = VM_Call(uivm, UI_GETAPIVERSION);
	if (v != WMUI_API_VERSION)
	{
		common->FatalError("User Interface is version %d, expected %d", v, WMUI_API_VERSION);
		cls.q3_uiStarted = false;
	}

	// init for this gamestate
	VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
}
