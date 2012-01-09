/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "client.h"
#include "../../client/cinematic/local.h"

static int				CL_handle = 0;

//=============================================================

/*
==================
SCR_StopCinematic
==================
*/
void SCR_StopCinematic (void)
{
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES && cinTable[CL_handle])
	{
		delete cinTable[CL_handle];
		cinTable[CL_handle] = NULL;
		CL_handle = -1;
	}
}

/*
====================
CIN_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void CIN_FinishCinematic()
{
	// tell the server to advance to the next map / cinematic
	clc.netchan.message.WriteByte(q2clc_stringcmd);
	clc.netchan.message.WriteString2(va("nextserver %i\n", cl.servercount));
}

//==========================================================================


/*
==================
SCR_RunCinematic

==================
*/
void SCR_RunCinematic (void)
{
	if (CL_handle < 0 || CL_handle >= MAX_VIDEO_HANDLES || !cinTable[CL_handle])
	{
		return;
	}

	if (in_keyCatchers != 0)
	{
		// pause if menu or console is up
		cinTable[CL_handle]->StartTime = cls.realtime - cinTable[CL_handle]->Cin->GetCinematicTime();
		return;
	}

	if (CIN_RunCinematic(CL_handle) == FMV_EOF)
	{
		SCR_BeginLoadingPlaque(true);
		CL_handle = -1;
	}
}

/*
==================
SCR_DrawCinematic

Returns true if a cinematic is active, meaning the view rendering
should be skipped
==================
*/
qboolean SCR_DrawCinematic (void)
{
	if (CL_handle < 0 || CL_handle >= MAX_VIDEO_HANDLES || !cinTable[CL_handle])
	{
		return false;
	}

	if (in_keyCatchers & KEYCATCH_UI)
	{
		// pause if menu is up
		return true;
	}

	if (!cinTable[CL_handle]->Cin->OutputFrame)
		return true;

	R_StretchRaw(0, 0, viddef.width, viddef.height,
		cinTable[CL_handle]->Cin->Width, cinTable[CL_handle]->Cin->Height, cinTable[CL_handle]->Cin->OutputFrame, 0, true);

	return true;
}

void CIN_StartedPlayback()
{
	SCR_EndLoadingPlaque();

	cls.state = ca_active;
}

/*
==================
SCR_PlayCinematic

==================
*/
void SCR_PlayCinematic (char *arg)
{
	// make sure CD isn't playing music
	CDAudio_Stop();

	CL_handle = CIN_PlayCinematic(arg, 0, 0, 640, 480, CIN_system);
}

void CIN_SkipCinematic()
{
	if (CL_handle < 0 || CL_handle >= MAX_VIDEO_HANDLES || !cinTable[CL_handle])
	{
		return;
	}
	if (!cl.attractloop && cls.realtime - cinTable[CL_handle]->StartTime > 1000)
	{
		// skip the rest of the cinematic
		SCR_StopCinematic();
		CIN_FinishCinematic();
		SCR_BeginLoadingPlaque(true);
	}
}

bool CIN_IsInCinematicState()
{
	return true;
}
