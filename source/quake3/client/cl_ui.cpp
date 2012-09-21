/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "client.h"
#include "../../client/game/quake3/ui_public.h"

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
//--------
	case Q3UI_UPDATESCREEN:
		SCR_UpdateScreen();
		return 0;
//--------
	case Q3UI_CIN_STOPCINEMATIC:
		return CIN_StopCinematic(args[1]);
//--------
	case Q3UI_CIN_DRAWCINEMATIC:
		CIN_DrawCinematic(args[1]);
		return 0;

	case Q3UI_CIN_SETEXTENTS:
		CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
		return 0;
//--------
	case Q3UI_VERIFY_CDKEY:
		return CL_CDKeyValidate((char*)VMA(1), (char*)VMA(2));
//--------
	}
	return CLQ3_UISystemCalls(args);
}

/*
====================
CL_InitUI
====================
*/
#define UI_OLD_API_VERSION  4

void CL_InitUI(void)
{
	int v;
	vmInterpret_t interpret;

	// load the dll or bytecode
	if (cl_connectedToPureServer != 0)
	{
		// if sv_pure is set we only allow qvms to be loaded
		interpret = VMI_COMPILED;
	}
	else
	{
		interpret = (vmInterpret_t)(int)Cvar_VariableValue("vm_ui");
	}
	uivm = VM_Create("ui", CL_UISystemCalls, interpret);
	if (!uivm)
	{
		common->FatalError("VM_Create on UI failed");
	}

	// sanity check
	v = VM_Call(uivm, UI_GETAPIVERSION);
	if (v == UI_OLD_API_VERSION)
	{
//		common->Printf(S_COLOR_YELLOW "WARNING: loading old Quake III Arena User Interface version %d\n", v );
		// init for this gamestate
		VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
	}
	else if (v != Q3UI_API_VERSION)
	{
		common->Error("User Interface is version %d, expected %d", v, Q3UI_API_VERSION);
		cls.q3_uiStarted = false;
	}
	else
	{
		// init for this gamestate
		VM_Call(uivm, UI_INIT, (cls.state >= CA_AUTHORIZING && cls.state < CA_ACTIVE));
	}
}
