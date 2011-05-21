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

#define ROQ_QUAD			0x1000
#define ROQ_QUAD_INFO		0x1001
#define ROQ_CODEBOOK		0x1002
#define ROQ_QUAD_VQ			0x1011
#define ROQ_QUAD_JPEG		0x1012
#define ROQ_QUAD_HANG		0x1013
#define ROQ_PACKET			0x1030
#define ZA_SOUND_MONO		0x1020
#define ZA_SOUND_STEREO		0x1021

#define MAX_VIDEO_HANDLES	16

extern	int		s_rawend;


static void RoQ_init( void );

/******************************************************************************
*
* Class:		trFMV
*
* Description:	RoQ/RnR manipulation routines
*				not entirely complete for first run
*
******************************************************************************/

typedef struct {
	int					currentHandle;
} cinematics_t;

typedef struct {
	QCinematicRoq*		Cin;

	char				fileName[MAX_OSPATH];
	int					xpos, ypos, width, height;
	qboolean			looping, holdAtEnd, dirty, alterGameState, silent, shader;
	fileHandle_t		iFile;
	e_status			status;
	unsigned int		startTime;
	unsigned int		lastTime;
	long				tfps;
	long				RoQPlayed;
	long				ROQSize;
	unsigned int		RoQFrameSize;
	long				numQuads;
	unsigned int		roq_id;

	qboolean			inMemory;
	long				roq_flags;
	long				roqF0;
	long				roqF1;
	long				roqFPS;
	int					playonwalls;
} cin_cache;

static cinematics_t		cin;
static cin_cache		cinTable[MAX_VIDEO_HANDLES];
static int				currentHandle = -1;
static int				CL_handle = -1;

void CIN_CloseAllVideos(void) {
	int		i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if (cinTable[i].fileName[0] != 0 ) {
			CIN_StopCinematic(i);
		}
	}
}


static int CIN_HandleForVideo(void) {
	int		i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( cinTable[i].fileName[0] == 0 ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "CIN_HandleForVideo: none free" );
	return -1;
}


extern int CL_ScaledMilliseconds(void);

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/
static void RoQReset() {
	
	if (currentHandle < 0) return;

	FS_FCloseFile( cinTable[currentHandle].iFile );
	FS_FOpenFileRead (cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, qtrue);
	FS_Read(cinTable[currentHandle].Cin->file, 16, cinTable[currentHandle].iFile);
	RoQ_init();
	cinTable[currentHandle].status = FMV_LOOPED;
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQInterrupt(void)
{
	byte				*framedata;
        short		sbuf[32768];
        int		ssize;
        
	if (currentHandle < 0) return;

	FS_Read(cinTable[currentHandle].Cin->file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);
	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) { 
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return; 
	}

	framedata = cinTable[currentHandle].Cin->file;
//
// new frame is ready
//
redump:
	switch(cinTable[currentHandle].roq_id) 
	{
		case	ROQ_QUAD_VQ:
			if ((cinTable[currentHandle].numQuads&1)) {
				cinTable[currentHandle].Cin->normalBuffer0 = cinTable[currentHandle].Cin->t[1];
				cinTable[currentHandle].Cin->RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].Cin->blitVQQuad32fs(cinTable[currentHandle].Cin->qStatus[1], framedata);
				cinTable[currentHandle].Cin->buf = 	cinTable[currentHandle].Cin->linbuf + cinTable[currentHandle].Cin->screenDelta;
			} else {
				cinTable[currentHandle].Cin->normalBuffer0 = cinTable[currentHandle].Cin->t[0];
				cinTable[currentHandle].Cin->RoQPrepMcomp( cinTable[currentHandle].roqF0, cinTable[currentHandle].roqF1 );
				cinTable[currentHandle].Cin->blitVQQuad32fs(cinTable[currentHandle].Cin->qStatus[0], framedata );
				cinTable[currentHandle].Cin->buf = cinTable[currentHandle].Cin->linbuf;
			}
			if (cinTable[currentHandle].numQuads == 0) {		// first frame
				Com_Memcpy(cinTable[currentHandle].Cin->linbuf+cinTable[currentHandle].Cin->screenDelta, cinTable[currentHandle].Cin->linbuf, cinTable[currentHandle].Cin->samplesPerLine*cinTable[currentHandle].Cin->ysize);
			}
			cinTable[currentHandle].numQuads++;
			cinTable[currentHandle].dirty = qtrue;
			break;
		case	ROQ_CODEBOOK:
			decodeCodeBook( framedata, (unsigned short)cinTable[currentHandle].roq_flags );
			break;
		case	ZA_SOUND_MONO:
			if (!cinTable[currentHandle].silent) {
				ssize = RllDecodeMonoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].roq_flags);
                                S_RawSamples( ssize, 22050, 2, 1, (byte *)sbuf, 1.0f );
			}
			break;
		case	ZA_SOUND_STEREO:
			if (!cinTable[currentHandle].silent) {
				if (cinTable[currentHandle].numQuads == -1) {
					S_Update();
					s_rawend = s_soundtime;
				}
				ssize = RllDecodeStereoToStereo( framedata, sbuf, cinTable[currentHandle].RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].roq_flags);
                                S_RawSamples( ssize, 22050, 2, 2, (byte *)sbuf, 1.0f );
			}
			break;
		case	ROQ_QUAD_INFO:
			if (cinTable[currentHandle].numQuads == -1) {
				cinTable[currentHandle].Cin->readQuadInfo( framedata );
				cinTable[currentHandle].Cin->setupQuad();
			}
			if (cinTable[currentHandle].numQuads != 1) cinTable[currentHandle].numQuads = 0;
			break;
		case	ROQ_PACKET:
			cinTable[currentHandle].inMemory = cinTable[currentHandle].roq_flags;
			cinTable[currentHandle].RoQFrameSize = 0;           // for header
			break;
		case	ROQ_QUAD_HANG:
			cinTable[currentHandle].RoQFrameSize = 0;
			break;
		case	ROQ_QUAD_JPEG:
			break;
		default:
			cinTable[currentHandle].status = FMV_EOF;
			break;
	}	
//
// read in next frame data
//
	if ( cinTable[currentHandle].RoQPlayed >= cinTable[currentHandle].ROQSize ) { 
		if (cinTable[currentHandle].holdAtEnd==qfalse) {
			if (cinTable[currentHandle].looping) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return; 
	}
	
	framedata		 += cinTable[currentHandle].RoQFrameSize;
	cinTable[currentHandle].roq_id		 = framedata[0] + framedata[1]*256;
	cinTable[currentHandle].RoQFrameSize = framedata[2] + framedata[3]*256 + framedata[4]*65536;
	cinTable[currentHandle].roq_flags	 = framedata[6] + framedata[7]*256;
	cinTable[currentHandle].roqF0		 = (char)framedata[7];
	cinTable[currentHandle].roqF1		 = (char)framedata[6];

	if (cinTable[currentHandle].RoQFrameSize>65536||cinTable[currentHandle].roq_id==0x1084) {
		Com_DPrintf("roq_size>65536||roq_id==0x1084\n");
		cinTable[currentHandle].status = FMV_EOF;
		if (cinTable[currentHandle].looping) {
			RoQReset();
		}
		return;
	}
	if (cinTable[currentHandle].inMemory && (cinTable[currentHandle].status != FMV_EOF)) { cinTable[currentHandle].inMemory--; framedata += 8; goto redump; }
//
// one more frame hits the dust
//
//	assert(cinTable[currentHandle].RoQFrameSize <= 65536);
//	r = FS_Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	cinTable[currentHandle].RoQPlayed	+= cinTable[currentHandle].RoQFrameSize+8;
}

/******************************************************************************
*
* Function:		
*
* Description:	
*
******************************************************************************/

static void RoQ_init( void )
{
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds()*com_timescale->value;

	cinTable[currentHandle].RoQPlayed = 24;

/*	get frame rate */	
	cinTable[currentHandle].roqFPS	 = cinTable[currentHandle].Cin->file[ 6] + cinTable[currentHandle].Cin->file[ 7]*256;
	
	if (!cinTable[currentHandle].roqFPS) cinTable[currentHandle].roqFPS = 30;

	cinTable[currentHandle].numQuads = -1;

	cinTable[currentHandle].roq_id		= cinTable[currentHandle].Cin->file[ 8] + cinTable[currentHandle].Cin->file[ 9]*256;
	cinTable[currentHandle].RoQFrameSize	= cinTable[currentHandle].Cin->file[10] + cinTable[currentHandle].Cin->file[11]*256 + cinTable[currentHandle].Cin->file[12]*65536;
	cinTable[currentHandle].roq_flags	= cinTable[currentHandle].Cin->file[14] + cinTable[currentHandle].Cin->file[15]*256;
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

	if (!cinTable[currentHandle].Cin->buf)
	{
		return;
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return;
	}
	Com_DPrintf("finished cinematic\n");
	cinTable[currentHandle].status = FMV_IDLE;

	if (cinTable[currentHandle].iFile) {
		FS_FCloseFile( cinTable[currentHandle].iFile );
		cinTable[currentHandle].iFile = 0;
	}

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
	cinTable[currentHandle].fileName[0] = 0;
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

	Com_DPrintf("trFMV::stop(), closing %s\n", cinTable[currentHandle].fileName);

	if (!cinTable[currentHandle].Cin->buf)
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
	int	start = 0;
	int     thisTime = 0;

	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return FMV_EOF;

	if (cin.currentHandle != handle) {
		currentHandle = handle;
		cin.currentHandle = currentHandle;
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
	thisTime = CL_ScaledMilliseconds()*com_timescale->value;
	if (cinTable[currentHandle].shader && (abs(thisTime - (int)cinTable[currentHandle].lastTime))>100) {
		cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
	}
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].tfps = ((((CL_ScaledMilliseconds()*com_timescale->value) - cinTable[currentHandle].startTime)*3)/100);

	start = cinTable[currentHandle].startTime;
	while(  (cinTable[currentHandle].tfps != cinTable[currentHandle].numQuads)
		&& (cinTable[currentHandle].status == FMV_PLAY) ) 
	{
		RoQInterrupt();
		if (start != cinTable[currentHandle].startTime) {
			// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
		  cinTable[currentHandle].tfps = ((((CL_ScaledMilliseconds()*com_timescale->value)
							  - cinTable[currentHandle].startTime)*3)/100);
			start = cinTable[currentHandle].startTime;
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
	unsigned short RoQID;
	char	name[MAX_OSPATH];
	int		i;

	if (strstr(arg, "/") == NULL && strstr(arg, "\\") == NULL) {
		QStr::Sprintf (name, sizeof(name), "video/%s", arg);
	} else {
		QStr::Sprintf (name, sizeof(name), "%s", arg);
	}

	if (!(systemBits & CIN_system)) {
		for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
			if (!QStr::Cmp(cinTable[i].fileName, name) ) {
				return i;
			}
		}
	}

	Com_DPrintf("SCR_PlayCinematic( %s )\n", arg);

	Com_Memset(&cin, 0, sizeof(cinematics_t) );
	currentHandle = CIN_HandleForVideo();

	cin.currentHandle = currentHandle;

	cinTable[currentHandle].Cin = new QCinematicRoq();
	QStr::Cpy(cinTable[currentHandle].fileName, name);

	cinTable[currentHandle].ROQSize = 0;
	cinTable[currentHandle].ROQSize = FS_FOpenFileRead (cinTable[currentHandle].fileName, &cinTable[currentHandle].iFile, qtrue);

	if (cinTable[currentHandle].ROQSize<=0) {
		Com_DPrintf("play(%s), ROQSize<=0\n", arg);
		cinTable[currentHandle].fileName[0] = 0;
		return -1;
	}

	CIN_SetExtents(currentHandle, x, y, w, h);
	CIN_SetLooping(currentHandle, (systemBits & CIN_loop)!=0);

	cinTable[currentHandle].Cin->CIN_HEIGHT = DEFAULT_CIN_HEIGHT;
	cinTable[currentHandle].Cin->CIN_WIDTH  =  DEFAULT_CIN_WIDTH;
	cinTable[currentHandle].holdAtEnd = (systemBits & CIN_hold) != 0;
	cinTable[currentHandle].alterGameState = (systemBits & CIN_system) != 0;
	cinTable[currentHandle].playonwalls = 1;
	cinTable[currentHandle].silent = (systemBits & CIN_silent) != 0;
	cinTable[currentHandle].shader = (systemBits & CIN_shader) != 0;

	if (cinTable[currentHandle].alterGameState) {
		// close the menu
		if ( uivm ) {
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
		}
	} else {
		cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
	}

	initRoQ();
					
	FS_Read (cinTable[currentHandle].Cin->file, 16, cinTable[currentHandle].iFile);

	RoQID = (unsigned short)(cinTable[currentHandle].Cin->file[0]) + (unsigned short)(cinTable[currentHandle].Cin->file[1])*256;
	if (RoQID == 0x1084)
	{
		RoQ_init();
//		FS_Read (cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);

		cinTable[currentHandle].status = FMV_PLAY;
		Com_DPrintf("trFMV::play(), playing %s\n", arg);

		if (cinTable[currentHandle].alterGameState) {
			cls.state = CA_CINEMATIC;
		}
		
		Con_Close();

		s_rawend = s_soundtime;

		return currentHandle;
	}
	Com_DPrintf("trFMV::play(), invalid RoQ ID\n");

	RoQShutdown();
	return -1;
}

void CIN_SetExtents (int handle, int x, int y, int w, int h) {
	if (handle < 0 || handle>= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF) return;
	cinTable[handle].xpos = x;
	cinTable[handle].ypos = y;
	cinTable[handle].width = w;
	cinTable[handle].height = h;
	cinTable[handle].dirty = qtrue;
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

	if (!cinTable[handle].Cin->buf)
	{
		return;
	}

	x = cinTable[handle].xpos;
	y = cinTable[handle].ypos;
	w = cinTable[handle].width;
	h = cinTable[handle].height;
	buf = cinTable[handle].Cin->buf;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	re.DrawStretchRaw( x, y, w, h, cinTable[handle].Cin->CIN_WIDTH, cinTable[handle].Cin->CIN_HEIGHT, buf, handle, cinTable[handle].dirty);
	cinTable[handle].dirty = qfalse;
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
		} while (cinTable[currentHandle].Cin->buf == NULL && cinTable[currentHandle].status == FMV_PLAY);		// wait for first frame (load codebook and sound)
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
		if (!cinTable[handle].Cin->buf)
		{
			return;
		}
		if (cinTable[handle].playonwalls <= 0 && cinTable[handle].dirty) {
			if (cinTable[handle].playonwalls == 0) {
				cinTable[handle].playonwalls = -1;
			} else {
				if (cinTable[handle].playonwalls == -1) {
					cinTable[handle].playonwalls = -2;
				} else {
					cinTable[handle].dirty = qfalse;
				}
			}
		}
		re.UploadCinematic( 256, 256, 256, 256, cinTable[handle].Cin->buf, handle, cinTable[handle].dirty);
		if (cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1) {
			cinTable[handle].playonwalls--;
		}
	}
}

