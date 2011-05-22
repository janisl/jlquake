/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
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

/*****************************************************************************
 * name:		cl_cin.c
 *
 * desc:		video and cinematic playback
 *
 * $Archive: /MissionPack/code/client/cl_cin.c $
 *
 *****************************************************************************/

#include "client.h"
#include "../../client/sound_local.h"
#include "../../client/cinematic_local.h"

#define MAX_VIDEO_HANDLES	16

extern	int		s_rawend;


/******************************************************************************
*
* Class:		trFMV
*
* Description:	RoQ/RnR manipulation routines
*				not entirely complete for first run
*
******************************************************************************/

typedef struct {
	QCinematic*			Cin;

	int					xpos, ypos, width, height;
	qboolean			looping, holdAtEnd, alterGameState, shader;
	e_status			status;
	unsigned int		startTime;
	unsigned int		lastTime;

	int					playonwalls;
} cin_cache;

static cin_cache		cinTable[MAX_VIDEO_HANDLES];
static int				currentHandle = -1;
static int				CL_handle = -1;

void CIN_CloseAllVideos()
{
	for (int i = 0; i < MAX_VIDEO_HANDLES; i++)
	{
		if (cinTable[i].Cin)
		{
			CIN_StopCinematic(i);
		}
	}
}


static int CIN_HandleForVideo()
{
	for (int i = 0; i < MAX_VIDEO_HANDLES; i++)
	{
		if (!cinTable[i].Cin)
		{
			return i;
		}
	}
	throw QDropException("CIN_HandleForVideo: none free");
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/
static void RoQReset() {
	
	if (currentHandle < 0) return;

	cinTable[currentHandle].Cin->Reset();
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds();

	cinTable[currentHandle].status = FMV_LOOPED;
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQShutdown( void ) {
	const char *s;

	if (!cinTable[currentHandle].Cin->OutputFrame)
	{
		return;
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return;
	}
	Com_DPrintf("finished cinematic\n");
	cinTable[currentHandle].status = FMV_IDLE;

	if (cinTable[currentHandle].alterGameState) {
		cls.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		s = Cvar_VariableString( "nextmap" );
		if ( s[0] ) {
			Cbuf_ExecuteText( EXEC_APPEND, va("%s\n", s) );
			Cvar_Set( "nextmap", "" );
		}
		CL_handle = -1;
	}
	delete cinTable[currentHandle].Cin;
	cinTable[currentHandle].Cin = NULL;
	currentHandle = -1;
}

/*
==================
SCR_StopCinematic
==================
*/
e_status CIN_StopCinematic(int handle) {
	
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;
	currentHandle = handle;

	Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].Cin->Name);

	if (!cinTable[currentHandle].Cin->OutputFrame)
	{
		return FMV_EOF;
	}

	if (cinTable[currentHandle].alterGameState) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}
	cinTable[currentHandle].status = FMV_EOF;
	RoQShutdown();

	return FMV_EOF;
}

/*
==================
SCR_RunCinematic

Fetch and decompress the pending frame
==================
*/


e_status CIN_RunCinematic (int handle)
{
        // bk001204 - init
	int     thisTime = 0;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;

	if (currentHandle != handle)
	{
		currentHandle = handle;
		cinTable[currentHandle].status = FMV_EOF;
		RoQReset();
	}

	if (cinTable[handle].playonwalls < -1)
	{
		return cinTable[handle].status;
	}

	currentHandle = handle;

	if (cinTable[currentHandle].alterGameState) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}

	if (cinTable[currentHandle].status == FMV_IDLE) {
		return cinTable[currentHandle].status;
	}

	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	thisTime = CL_ScaledMilliseconds();
	if (cinTable[currentHandle].shader && (abs(thisTime - (int)cinTable[currentHandle].lastTime))>100) {
		cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
	}
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	if (!cinTable[currentHandle].Cin->Update(CL_ScaledMilliseconds() - cinTable[currentHandle].startTime))
	{ 
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
	}

	cinTable[currentHandle].lastTime = thisTime;

	if (cinTable[currentHandle].status == FMV_LOOPED) {
		cinTable[currentHandle].status = FMV_PLAY;
	}

	if (cinTable[currentHandle].status == FMV_EOF) {
	  if (cinTable[currentHandle].looping) {
		RoQReset();
	  } else {
		RoQShutdown();
	  }
	}

	return cinTable[currentHandle].status;
}

/*
==================
CL_PlayCinematic

==================
*/
int CIN_PlayCinematic( const char *arg, int x, int y, int w, int h, int systemBits ) {
	char	name[MAX_OSPATH];
	int		i;

	CIN_MakeFullName(arg, name);

	if (!(systemBits & CIN_system))
	{
		for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ )
		{
			if (cinTable[i].Cin && !QStr::Cmp(cinTable[i].Cin->Name, name))
			{
				return i;
			}
		}
	}

	Com_DPrintf("SCR_PlayCinematic( %s )\n", arg);

	currentHandle = CIN_HandleForVideo();

	cinTable[currentHandle].Cin = CIN_Open(name);

	if (!cinTable[currentHandle].Cin)
	{
		//RoQShutdown();
		return -1;
	}

	CIN_SetExtents(currentHandle, x, y, w, h);
	CIN_SetLooping(currentHandle, (systemBits & CIN_loop)!=0);

	cinTable[currentHandle].holdAtEnd = (systemBits & CIN_hold) != 0;
	cinTable[currentHandle].alterGameState = (systemBits & CIN_system) != 0;
	cinTable[currentHandle].playonwalls = 1;
	cinTable[currentHandle].Cin->Silent = (systemBits & CIN_silent) != 0;
	cinTable[currentHandle].shader = (systemBits & CIN_shader) != 0;

	if (cinTable[currentHandle].alterGameState) {
		// close the menu
		if ( uivm ) {
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
		}
	} else {
		cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
	}


	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds();

	cinTable[currentHandle].status = FMV_PLAY;
	Com_DPrintf("trFMV::play(), playing %s\n", arg);

	if (cinTable[currentHandle].alterGameState) {
		cls.state = CA_CINEMATIC;
	}
	
	Con_Close();

	s_rawend = s_soundtime;

	return currentHandle;
}

void CIN_SetExtents (int handle, int x, int y, int w, int h) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].xpos = x;
	cinTable[handle].ypos = y;
	cinTable[handle].width = w;
	cinTable[handle].height = h;
	cinTable[handle].Cin->Dirty = qtrue;
}

void CIN_SetLooping(int handle, qboolean loop) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].looping = loop;
}

/*
==================
SCR_DrawCinematic

==================
*/
void CIN_DrawCinematic (int handle) {
	float	x, y, w, h;
	byte	*buf;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;

	if (!cinTable[handle].Cin->OutputFrame)
	{
		return;
	}

	x = cinTable[handle].xpos;
	y = cinTable[handle].ypos;
	w = cinTable[handle].width;
	h = cinTable[handle].height;
	buf = cinTable[handle].Cin->OutputFrame;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	re.DrawStretchRaw( x, y, w, h, cinTable[handle].Cin->Width, cinTable[handle].Cin->Height, buf, handle, cinTable[handle].Cin->Dirty);
	cinTable[handle].Cin->Dirty = qfalse;
}

void CL_PlayCinematic_f(void) {
	char	*arg, *s;
	qboolean	holdatend;
	int bits = CIN_system;

	Com_DPrintf("CL_PlayCinematic_f\n");
	if (cls.state == CA_CINEMATIC) {
		SCR_StopCinematic();
	}

	arg = Cmd_Argv( 1 );
	s = Cmd_Argv(2);

	holdatend = qfalse;
	if ((s && s[0] == '1') || QStr::ICmp(arg,"demoend.roq")==0 || QStr::ICmp(arg,"end.roq")==0) {
		bits |= CIN_hold;
	}
	if (s && s[0] == '2') {
		bits |= CIN_loop;
	}

	S_StopAllSounds ();

	CL_handle = CIN_PlayCinematic( arg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits );
	if (CL_handle >= 0) {
		do {
			SCR_RunCinematic();
		} while (cinTable[currentHandle].Cin->OutputFrame == NULL && cinTable[currentHandle].status == FMV_PLAY);		// wait for first frame (load codebook and sound)
	}
}


void SCR_DrawCinematic (void) {
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_DrawCinematic(CL_handle);
	}
}

void SCR_RunCinematic (void)
{
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_RunCinematic(CL_handle);
	}
}

void SCR_StopCinematic(void) {
	if (CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES) {
		CIN_StopCinematic(CL_handle);
		S_StopAllSounds ();
		CL_handle = -1;
	}
}

void CIN_UploadCinematic(int handle) {
	if (handle >= 0 && handle < MAX_VIDEO_HANDLES) {
		if (!cinTable[handle].Cin->OutputFrame)
		{
			return;
		}
		if (cinTable[handle].playonwalls <= 0 && cinTable[handle].Cin->Dirty)
		{
			if (cinTable[handle].playonwalls == 0) {
				cinTable[handle].playonwalls = -1;
			} else {
				if (cinTable[handle].playonwalls == -1) {
					cinTable[handle].playonwalls = -2;
				} else {
					cinTable[handle].Cin->Dirty = qfalse;
				}
			}
		}
		R_UploadCinematic(256, 256, cinTable[handle].Cin->OutputFrame, handle, cinTable[handle].Cin->Dirty);
		if (cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1) {
			cinTable[handle].playonwalls--;
		}
	}
}

