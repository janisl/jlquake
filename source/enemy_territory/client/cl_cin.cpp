/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

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

extern glconfig_t glConfig;
extern int s_soundtime;

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

static cinematics_t cin;
static int currentHandle = -1;
static int CL_handle = -1;

void CIN_CloseAllVideos( void ) {
	int i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( cinTable[i] ) {
			CIN_StopCinematic( i );
		}
	}
}


static int CIN_HandleForVideo( void ) {
	int i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( !cinTable[i]) {
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
		/*if( com_logosPlaying->integer && !String::ICmp( s, "cinematic avlogo.roq" ) ) {
			Cvar_Set( "nextmap", "cinematic sdlogo.roq" );	// FIXME: sd logo
			Cvar_Set( "com_logosPlaying", "0" );
		} else {*/
		Cvar_Set( "nextmap", "" );
		//}
	}
	CL_handle = -1;
}

static void RoQShutdown( void ) {
	if ( cinTable[currentHandle]->Cin->OutputFrame ) {
		if ( cinTable[currentHandle]->Status != FMV_IDLE ) {
			Com_DPrintf( "finished cinematic\n" );
			cinTable[currentHandle]->Status = FMV_IDLE;

			if ( cinTable[currentHandle]->AlterGameState ) {
				CIN_FinishCinematic();
			}
			delete cinTable[currentHandle];
			cinTable[currentHandle] = NULL;
			currentHandle = -1;
		}
	}
}

/*
==================
SCR_StopCinematic
==================
*/
e_status CIN_StopCinematic( int handle ) {

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle]->Status == FMV_EOF ) {
		return FMV_EOF;
	}
	currentHandle = handle;

	Com_DPrintf( "trFMV::stop(), closing %s\n", cinTable[currentHandle]->Cin->Name );

	if ( !cinTable[currentHandle]->Cin->OutputFrame ) {
		return FMV_EOF;
	}

	if ( cinTable[currentHandle]->AlterGameState ) {
		if (!CIN_IsInCinematicState() ) {
			return cinTable[currentHandle]->Status;
		}
	}
	cinTable[currentHandle]->Status = FMV_EOF;
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
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle]->Status == FMV_EOF ) {
		return FMV_EOF;
	}

	if ( cin.currentHandle != handle ) {
		currentHandle = handle;
		cin.currentHandle = currentHandle;
		cinTable[currentHandle]->Status = FMV_EOF;
		cinTable[currentHandle]->Reset();
	}

	currentHandle = handle;

	e_status ret = cinTable[currentHandle]->Run();
	if ( cinTable[currentHandle]->Status == FMV_EOF ) {
		ret = FMV_IDLE;

		delete cinTable[currentHandle];
		cinTable[currentHandle] = NULL;
		currentHandle = -1;
	}

	return ret;
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
	s_rawend[0] = s_soundtime;
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
			if ( cinTable[i]->Cin && !String::Cmp( cinTable[i]->Cin->Name, name ) ) {
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

	cinTable[currentHandle] = new QCinematicPlayer(Cin, x, y, w, h, systemBits);

	if ( cinTable[currentHandle]->AlterGameState ) {
		CIN_StartedPlayback();
	}

	Com_DPrintf( "trFMV::play(), playing %s\n", arg );

	return currentHandle;
}

void CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || !cinTable[handle] || cinTable[handle]->Status == FMV_EOF ) {
		return;
	}
	cinTable[handle]->SetExtents(x, y, w, h);
}

/*
==================
SCR_DrawCinematic

==================
*/
void CIN_DrawCinematic( int handle ) {
	float x, y, w, h;
	byte    *buf;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle]->Status == FMV_EOF ) {
		return;
	}

	if ( !cinTable[handle]->Cin->OutputFrame ) {
		return;
	}

	x = cinTable[handle]->XPos;
	y = cinTable[handle]->YPos;
	w = cinTable[handle]->Width;
	h = cinTable[handle]->Height;
	buf = cinTable[handle]->Cin->OutputFrame;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	re.DrawStretchRaw( x, y, w, h, cinTable[handle]->Cin->Width, cinTable[handle]->Cin->Height, buf, handle, cinTable[handle]->Cin->Dirty );
	cinTable[handle]->Cin->Dirty = qfalse;
}

bool CIN_IsInCinematicState()
{
	return cls.state == CA_CINEMATIC;
}

void CL_PlayCinematic_f( void ) {
	char    *arg, *s;
	qboolean holdatend;
	int bits = CIN_system;

	// Arnout: don't allow this while on server
	if ( cls.state > CA_DISCONNECTED && cls.state <= CA_ACTIVE ) {
		return;
	}

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
		} while ( cinTable[currentHandle]->Cin->OutputFrame == NULL && cinTable[currentHandle]->Status == FMV_PLAY );        // wait for first frame (load codebook and sound)
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
	if ( handle >= 0 && handle < MAX_VIDEO_HANDLES && cinTable[handle] ) {
		cinTable[handle]->Upload(handle);
	}
}
