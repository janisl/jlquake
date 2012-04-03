/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

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


/*****************************************************************************
 * name:		cl_cin.c
 *
 * desc:		video and cinematic playback
 *
 *
 * cl_glconfig.hwtype trtypes 3dfx/ragepro need 256x256
 *
 *****************************************************************************/

//#define ADAPTED_TO_STREAMING_SOUND
//	(SA) MISSIONPACK MERGE
//	s_rawend for wolf is [] and for q3 is just a single value
//  I need to ask Ryan if it's as simple as a constant index or
// if some more coding needs to be done.


#include "client.h"
#include "../../client/sound/local.h"
#include "../../wolfclient/cinematic/local.h"
#define MAXSIZE             8
#define MINSIZE             4

#define ROQ_QUAD            0x1000
#define ROQ_QUAD_INFO       0x1001
#define ROQ_CODEBOOK        0x1002
#define ROQ_QUAD_VQ         0x1011
#define ROQ_QUAD_JPEG       0x1012
#define ROQ_QUAD_HANG       0x1013
#define ROQ_PACKET          0x1030
#define ZA_SOUND_MONO       0x1020
#define ZA_SOUND_STEREO     0x1021

#define MAX_VIDEO_HANDLES   16

extern glconfig_t glConfig;

/******************************************************************************
*
* Class:		trFMV
*
* Description:	RoQ/RnR manipulation routines
*				not entirely complete for first run
*
******************************************************************************/

typedef struct {
	int currentHandle;
} cinematics_t;

typedef struct {
	QCinematicPlayer* player;
} cin_cache;

static cinematics_t cin;
static cin_cache cinTable[MAX_VIDEO_HANDLES];
static int currentHandle = -1;
static int CL_handle = -1;

void CIN_CloseAllVideos( void ) {
	int i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( cinTable[i].player ) {
			CIN_StopCinematic( i );
		}
	}
}


static int CIN_HandleForVideo( void ) {
	int i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( !cinTable[i].player ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "CIN_HandleForVideo: none free" );
	return -1;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQReset() {

	if ( currentHandle < 0 ) {
		return;
	}

	cinTable[currentHandle].player->Cin->Reset();
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].player->StartTime = cinTable[currentHandle].player->LastTime = CL_ScaledMilliseconds() * com_timescale->value;

	cinTable[currentHandle].player->Status = FMV_LOOPED;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

void CIN_FinishCinematic()
{
	cls.state = CA_DISCONNECTED;
	// we can't just do a vstr nextmap, because
	// if we are aborting the intro cinematic with
	// a devmap command, nextmap would be valid by
	// the time it was referenced
	const char* s = Cvar_VariableString( "nextmap" );
	if ( s[0] ) {
		Cbuf_ExecuteText( EXEC_APPEND, va( "%s\n", s ) );
		Cvar_Set( "nextmap", "" );
	}
	CL_handle = -1;
}

static void RoQShutdown( void ) {
	if ( !cinTable[currentHandle].player->Cin->OutputFrame ) {
		return;
	}

	if ( cinTable[currentHandle].player->Status == FMV_IDLE ) {
		return;
	}
	Com_DPrintf( "finished cinematic\n" );
	cinTable[currentHandle].player->Status = FMV_IDLE;

	if ( cinTable[currentHandle].player->AlterGameState ) {
		CIN_FinishCinematic();
	}
	delete cinTable[currentHandle].player;
	cinTable[currentHandle].player = NULL;
	currentHandle = -1;
}

/*
==================
SCR_StopCinematic
==================
*/
e_status CIN_StopCinematic( int handle ) {

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].player->Status == FMV_EOF ) {
		return FMV_EOF;
	}
	currentHandle = handle;

	Com_DPrintf( "trFMV::stop(), closing %s\n", cinTable[currentHandle].player->Cin->Name );

	if ( !cinTable[currentHandle].player->Cin->OutputFrame ) {
		return FMV_EOF;
	}

	if ( cinTable[currentHandle].player->AlterGameState ) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].player->Status;
		}
	}
	cinTable[currentHandle].player->Status = FMV_EOF;
	RoQShutdown();

	return FMV_EOF;
}

/*
==================
SCR_RunCinematic

Fetch and decompress the pending frame
==================
*/


e_status CIN_RunCinematic( int handle ) {
	// bk001204 - init
	int start = 0;
	int thisTime = 0;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].player->Status == FMV_EOF ) {
		return FMV_EOF;
	}

	if ( cin.currentHandle != handle ) {
		currentHandle = handle;
		cin.currentHandle = currentHandle;
		cinTable[currentHandle].player->Status = FMV_EOF;
		RoQReset();
	}

	if ( cinTable[handle].player->PlayOnWalls < -1 ) {
		return cinTable[handle].player->Status;
	}

	currentHandle = handle;

	if ( cinTable[currentHandle].player->AlterGameState ) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].player->Status;
		}
	}

	if ( cinTable[currentHandle].player->Status == FMV_IDLE ) {
		return cinTable[currentHandle].player->Status;
	}

	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	thisTime = CL_ScaledMilliseconds() * com_timescale->value;
	if ( cinTable[currentHandle].player->Shader && ( abs( thisTime - (int)cinTable[currentHandle].player->LastTime ) ) > 100 ) {
		cinTable[currentHandle].player->StartTime += thisTime - cinTable[currentHandle].player->LastTime;
	}
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	start = cinTable[currentHandle].player->StartTime;
	if (!cinTable[currentHandle].player->Cin->Update(CL_ScaledMilliseconds() - cinTable[currentHandle].player->StartTime))
	{
		if ( cinTable[currentHandle].player->HoldAtEnd == qfalse ) {
			if ( cinTable[currentHandle].player->Looping ) {
				RoQReset();
			} else {
				cinTable[currentHandle].player->Status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].player->Status = FMV_IDLE;
		}
	}

	cinTable[currentHandle].player->LastTime = thisTime;

	if ( cinTable[currentHandle].player->Status == FMV_LOOPED ) {
		cinTable[currentHandle].player->Status = FMV_PLAY;
	}

	if ( cinTable[currentHandle].player->Status == FMV_EOF ) {
		if ( cinTable[currentHandle].player->Looping ) {
			RoQReset();
		} else {
			RoQShutdown();
		}
	}

	return cinTable[currentHandle].player->Status;
}

void CIN_StartedPlayback()
{
	// close the menu
	if ( uivm ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
	}

	cls.state = CA_CINEMATIC;

	Con_Close();

//		s_rawend = s_soundtime;
}

/*
==================
CL_PlayCinematic

==================
*/
int CIN_PlayCinematic( const char *arg, int x, int y, int w, int h, int systemBits ) {
	char name[MAX_OSPATH];
	int i;

	if ( strstr( arg, "/" ) == NULL && strstr( arg, "\\" ) == NULL ) {
		String::Sprintf( name, sizeof( name ), "video/%s", arg );
	} else {
		String::Sprintf( name, sizeof( name ), "%s", arg );
	}

	if ( !( systemBits & CIN_system ) ) {
		for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
			if ( cinTable[i].player && !String::Cmp( cinTable[i].player->Cin->Name, name ) ) {
				return i;
			}
		}
	}

	Com_DPrintf( "SCR_PlayCinematic( %s )\n", arg );

	Com_Memset( &cin, 0, sizeof( cinematics_t ) );
	currentHandle = CIN_HandleForVideo();

	cin.currentHandle = currentHandle;

	QCinematic* Cin = new QCinematicRoq();
	if (!Cin->Open(name))
	{
		delete Cin;
		return -1;
	}

	cinTable[currentHandle].player = new QCinematicPlayer(Cin, x, y, w, h, systemBits);

	if ( cinTable[currentHandle].player->AlterGameState ) {
		CIN_StartedPlayback();
	}

	Com_DPrintf( "trFMV::play(), playing %s\n", arg );

	return currentHandle;
}

void CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || !cinTable[handle].player || cinTable[handle].player->Status == FMV_EOF ) {
		return;
	}
	cinTable[handle].player->SetExtents(x, y, w, h);
}

/*
==================
SCR_DrawCinematic

==================
*/
void CIN_DrawCinematic( int handle ) {
	float x, y, w, h;
	byte    *buf;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].player->Status == FMV_EOF ) {
		return;
	}

	if ( !cinTable[handle].player->Cin->OutputFrame ) {
		return;
	}

	x = cinTable[handle].player->XPos;
	y = cinTable[handle].player->YPos;
	w = cinTable[handle].player->Width;
	h = cinTable[handle].player->Height;
	buf = cinTable[handle].player->Cin->OutputFrame;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	re.DrawStretchRaw( x, y, w, h, cinTable[handle].player->Cin->Width, cinTable[handle].player->Cin->Height, buf, handle, cinTable[handle].player->Cin->Dirty );
	cinTable[handle].player->Cin->Dirty = qfalse;
}

bool CIN_IsInCinematicState()
{
	return cls.state == CA_CINEMATIC;
}

void CL_PlayCinematic_f( void ) {
	char    *arg, *s;
	qboolean holdatend;
	int bits = CIN_system;

	Com_DPrintf( "CL_PlayCinematic_f\n" );
	if (CIN_IsInCinematicState()) {
		SCR_StopCinematic();
	}

	arg = Cmd_Argv( 1 );
	s = Cmd_Argv( 2 );

	holdatend = qfalse;
	if ( ( s && s[0] == '1' ) || String::ICmp( arg,"demoend.roq" ) == 0 || String::ICmp( arg,"end.roq" ) == 0 ) {
		bits |= CIN_hold;
	}
	if ( s && s[0] == '2' ) {
		bits |= CIN_loop;
	}

	S_StopAllSounds();

	CL_handle = CIN_PlayCinematic( arg, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bits );
	if ( CL_handle >= 0 ) {
		do {
			SCR_RunCinematic();
		} while ( cinTable[currentHandle].player->Cin->OutputFrame == NULL && cinTable[currentHandle].player->Status == FMV_PLAY );        // wait for first frame (load codebook and sound)
	}
}


void SCR_DrawCinematic( void ) {
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_DrawCinematic( CL_handle );
	}
}

void SCR_RunCinematic( void ) {
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_RunCinematic( CL_handle );
	}
}

void SCR_StopCinematic( void ) {
	if ( CL_handle >= 0 && CL_handle < MAX_VIDEO_HANDLES ) {
		CIN_StopCinematic( CL_handle );
		S_StopAllSounds();
		CL_handle = -1;
	}
}

void CIN_UploadCinematic( int handle ) {
	if ( handle >= 0 && handle < MAX_VIDEO_HANDLES && cinTable[handle].player ) {
		if ( !cinTable[handle].player->Cin->OutputFrame ) {
			return;
		}
		if ( cinTable[handle].player->PlayOnWalls <= 0 && cinTable[handle].player->Cin->Dirty ) {
			if ( cinTable[handle].player->PlayOnWalls == 0 ) {
				cinTable[handle].player->PlayOnWalls = -1;
			} else {
				if ( cinTable[handle].player->PlayOnWalls == -1 ) {
					cinTable[handle].player->PlayOnWalls = -2;
				} else {
					cinTable[handle].player->Cin->Dirty = qfalse;
				}
			}
		}
		R_UploadCinematic( 256, 256, cinTable[handle].player->Cin->OutputFrame, handle, cinTable[handle].player->Cin->Dirty );
		if ( cl_inGameVideo->integer == 0 && cinTable[handle].player->PlayOnWalls == 1 ) {
			cinTable[handle].player->PlayOnWalls--;
		}
	}
}
