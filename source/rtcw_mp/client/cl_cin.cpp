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


static void RoQ_init( void );

/******************************************************************************
*
* Class:		trFMV
*
* Description:	RoQ/RnR manipulation routines
*				not entirely complete for first run
*
******************************************************************************/

extern long ROQ_YY_tab[256];
extern long ROQ_UB_tab[256];
extern long ROQ_UG_tab[256];
extern long ROQ_VG_tab[256];
extern long ROQ_VR_tab[256];

extern short sqrTable[256];

typedef struct {
	long oldXOff, oldYOff, oldysize, oldxsize;

	int currentHandle;
} cinematics_t;

typedef struct {
	QCinematicRoq* Cin;
	int xpos, ypos, width, height;
	qboolean looping, holdAtEnd, alterGameState, shader;
	e_status status;
	unsigned int startTime;
	unsigned int lastTime;
	long tfps;

	void ( *VQ0 )( byte *status, void *qdata );
	void ( *VQ1 )( byte *status, void *qdata );
	void ( *VQNormal )( byte *status, void *qdata );
	void ( *VQBuffer )( byte *status, void *qdata );

	unsigned int xsize, ysize;

	int playonwalls;
	long drawX, drawY;
} cin_cache;

static cinematics_t cin;
static cin_cache cinTable[MAX_VIDEO_HANDLES];
static int currentHandle = -1;
static int CL_handle = -1;

void CIN_CloseAllVideos( void ) {
	int i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( cinTable[i].Cin ) {
			CIN_StopCinematic( i );
		}
	}
}


static int CIN_HandleForVideo( void ) {
	int i;

	for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
		if ( !cinTable[i].Cin ) {
			return i;
		}
	}
	Com_Error( ERR_DROP, "CIN_HandleForVideo: none free" );
	return -1;
}

long RllDecodeMonoToStereo( unsigned char *from,short *to,unsigned int size,char signedOutput,unsigned short flag );
long RllDecodeStereoToStereo( unsigned char *from,short *to,unsigned int size,char signedOutput, unsigned short flag );
void move8_32( byte *src, byte *dst, int spl );
void move4_32( byte *src, byte *dst, int spl  );
void blit8_32( byte *src, byte *dst, int spl  );
void blit4_32( byte *src, byte *dst, int spl  );
void blit2_32( byte *src, byte *dst, int spl  );

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void blitVQQuad32fs( byte **status, unsigned char *data ) {
	unsigned short newd, celdata, code;
	unsigned int index, i;
	int spl;

	newd    = 0;
	celdata = 0;
	index   = 0;

	spl = cinTable[currentHandle].Cin->samplesPerLine;

	do {
		if ( !newd ) {
			newd = 7;
			celdata = data[0] + data[1] * 256;
			data += 2;
		} else {
			newd--;
		}

		code = ( unsigned short )( celdata & 0xc000 );
		celdata <<= 2;

		switch ( code ) {
		case    0x8000:                                                     // vq code
			blit8_32( (byte *)&cinTable[currentHandle].Cin->vq8[( *data ) * 128], status[index], spl );
			data++;
			index += 5;
			break;
		case    0xc000:                                                     // drop
			index++;                                                        // skip 8x8
			for ( i = 0; i < 4; i++ ) {
				if ( !newd ) {
					newd = 7;
					celdata = data[0] + data[1] * 256;
					data += 2;
				} else {
					newd--;
				}

				code = ( unsigned short )( celdata & 0xc000 ); celdata <<= 2;

				switch ( code ) {                                           // code in top two bits of code
				case    0x8000:                                             // 4x4 vq code
					blit4_32( (byte *)&cinTable[currentHandle].Cin->vq4[( *data ) * 32], status[index], spl );
					data++;
					break;
				case    0xc000:                                             // 2x2 vq code
					blit2_32( (byte *)&cinTable[currentHandle].Cin->vq2[( *data ) * 8], status[index], spl );
					data++;
					blit2_32( (byte *)&cinTable[currentHandle].Cin->vq2[( *data ) * 8], status[index] + 8, spl );
					data++;
					blit2_32( (byte *)&cinTable[currentHandle].Cin->vq2[( *data ) * 8], status[index] + spl * 2, spl );
					data++;
					blit2_32( (byte *)&cinTable[currentHandle].Cin->vq2[( *data ) * 8], status[index] + spl * 2 + 8, spl );
					data++;
					break;
				case    0x4000:                                             // motion compensation
					move4_32( status[index] + cinTable[currentHandle].Cin->mcomp[( *data )], status[index], spl );
					data++;
					break;
				}
				index++;
			}
			break;
		case    0x4000:                                                     // motion compensation
			move8_32( status[index] + cinTable[currentHandle].Cin->mcomp[( *data )], status[index], spl );
			data++;
			index += 5;
			break;
		case    0x0000:
			index += 5;
			break;
		}
	} while ( status[index] != NULL );
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

#define VQ2TO4( a,b,c,d ) {	\
		*c++ = a[0];	\
		*d++ = a[0];	\
		*d++ = a[0];	\
		*c++ = a[1];	\
		*d++ = a[1];	\
		*d++ = a[1];	\
		*c++ = b[0];	\
		*d++ = b[0];	\
		*d++ = b[0];	\
		*c++ = b[1];	\
		*d++ = b[1];	\
		*d++ = b[1];	\
		*d++ = a[0];	\
		*d++ = a[0];	\
		*d++ = a[1];	\
		*d++ = a[1];	\
		*d++ = b[0];	\
		*d++ = b[0];	\
		*d++ = b[1];	\
		*d++ = b[1];	\
		a += 2; b += 2; }

#define VQ2TO2( a,b,c,d ) {	\
		*c++ = *a;	\
		*d++ = *a;	\
		*d++ = *a;	\
		*c++ = *b;	\
		*d++ = *b;	\
		*d++ = *b;	\
		*d++ = *a;	\
		*d++ = *a;	\
		*d++ = *b;	\
		*d++ = *b;	\
		a++; b++; }

void yuv_to_rgb24(long y, long u, long v, byte* out);

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void decodeCodeBook( byte *input, unsigned short roq_flags ) {
	long i, j, two, four;
	unsigned short  *bptr;
	long y0,y1,y2,y3,cr,cb;
	unsigned int *iaptr, *ibptr, *icptr, *idptr;

	if ( !roq_flags ) {
		two = four = 256;
	} else {
		two  = roq_flags >> 8;
		if ( !two ) {
			two = 256;
		}
		four = roq_flags & 0xff;
	}

	four *= 2;

	bptr = (unsigned short *)cinTable[currentHandle].Cin->vq2;

	byte* rgb_ptr = (byte*)bptr;
	for ( i = 0; i < two; i++ ) {
		y0 = (long)*input++;
		y1 = (long)*input++;
		y2 = (long)*input++;
		y3 = (long)*input++;
		cr = (long)*input++;
		cb = (long)*input++;
		yuv_to_rgb24( y0, cr, cb, rgb_ptr );
		yuv_to_rgb24( y1, cr, cb, rgb_ptr + 4 );
		yuv_to_rgb24( y2, cr, cb, rgb_ptr + 8 );
		yuv_to_rgb24( y3, cr, cb, rgb_ptr + 12 );
		rgb_ptr += 16;
	}

	icptr = (unsigned int *)cinTable[currentHandle].Cin->vq4;
	idptr = (unsigned int *)cinTable[currentHandle].Cin->vq8;

	for ( i = 0; i < four; i++ ) {
		iaptr = (unsigned int *)cinTable[currentHandle].Cin->vq2 + ( *input++ ) * 4;
		ibptr = (unsigned int *)cinTable[currentHandle].Cin->vq2 + ( *input++ ) * 4;
		for ( j = 0; j < 2; j++ )
			VQ2TO4( iaptr, ibptr, icptr, idptr );
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void recurseQuad( long startX, long startY, long quadSize, long xOff, long yOff ) {
	byte *scroff;
	long bigx, bigy, lowx, lowy, useY;
	long offset;

	offset = cinTable[currentHandle].Cin->screenDelta;

	lowx = lowy = 0;
	bigx = cinTable[currentHandle].xsize;
	bigy = cinTable[currentHandle].ysize;

	if ( bigx > cinTable[currentHandle].Cin->Width) {
		bigx = cinTable[currentHandle].Cin->Width;
	}
	if ( bigy > cinTable[currentHandle].Cin->Height ) {
		bigy = cinTable[currentHandle].Cin->Height;
	}

	if ( ( startX >= lowx ) && ( startX + quadSize ) <= ( bigx ) && ( startY + quadSize ) <= ( bigy ) && ( startY >= lowy ) && quadSize <= MAXSIZE ) {
		useY = startY;
		scroff = cinTable[currentHandle].Cin->linbuf + ( useY + ( ( cinTable[currentHandle].Cin->Height - bigy ) >> 1 ) + yOff ) * ( cinTable[currentHandle].Cin->samplesPerLine ) + ( ( ( startX + xOff ) ) * 4 );

		cinTable[currentHandle].Cin->qStatus[0][cinTable[currentHandle].Cin->onQuad  ] = scroff;
		cinTable[currentHandle].Cin->qStatus[1][cinTable[currentHandle].Cin->onQuad++] = scroff + offset;
	}

	if ( quadSize != MINSIZE ) {
		quadSize >>= 1;
		recurseQuad( startX,          startY, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY, quadSize, xOff, yOff );
		recurseQuad( startX,          startY + quadSize, quadSize, xOff, yOff );
		recurseQuad( startX + quadSize, startY + quadSize, quadSize, xOff, yOff );
	}
}


/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void setupQuad( long xOff, long yOff ) {
	long numQuadCels, i,x,y;
	byte *temp;

	if ( xOff == cin.oldXOff && yOff == cin.oldYOff && cinTable[currentHandle].ysize == cin.oldysize && cinTable[currentHandle].xsize == cin.oldxsize ) {
		return;
	}

	cin.oldXOff = xOff;
	cin.oldYOff = yOff;
	cin.oldysize = cinTable[currentHandle].ysize;
	cin.oldxsize = cinTable[currentHandle].xsize;

	numQuadCels  = ( cinTable[currentHandle].Cin->Width * cinTable[currentHandle].Cin->Height ) / ( 16 );
	numQuadCels += numQuadCels / 4 + numQuadCels / 16;
	numQuadCels += 64;                            // for overflow

	numQuadCels  = ( cinTable[currentHandle].xsize * cinTable[currentHandle].ysize ) / ( 16 );
	numQuadCels += numQuadCels / 4;
	numQuadCels += 64;                            // for overflow

	cinTable[currentHandle].Cin->onQuad = 0;

	for ( y = 0; y < (long)cinTable[currentHandle].ysize; y += 16 )
		for ( x = 0; x < (long)cinTable[currentHandle].xsize; x += 16 )
			recurseQuad( x, y, 16, xOff, yOff );

	temp = NULL;

	for ( i = ( numQuadCels - 64 ); i < numQuadCels; i++ ) {
		cinTable[currentHandle].Cin->qStatus[0][i] = temp;             // eoq
		cinTable[currentHandle].Cin->qStatus[1][i] = temp;             // eoq
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void readQuadInfo( byte *qData ) {
	if ( currentHandle < 0 ) {
		return;
	}

	cinTable[currentHandle].xsize    = qData[0] + qData[1] * 256;
	cinTable[currentHandle].ysize    = qData[2] + qData[3] * 256;
	cinTable[currentHandle].Cin->maxsize  = qData[4] + qData[5] * 256;
	cinTable[currentHandle].Cin->minsize  = qData[6] + qData[7] * 256;

	cinTable[currentHandle].Cin->Height = cinTable[currentHandle].ysize;
	cinTable[currentHandle].Cin->Width = cinTable[currentHandle].xsize;

	cinTable[currentHandle].Cin->samplesPerLine = cinTable[currentHandle].Cin->Width * 4;
	cinTable[currentHandle].Cin->screenDelta = cinTable[currentHandle].Cin->Height * cinTable[currentHandle].Cin->samplesPerLine;

	cinTable[currentHandle].VQ0 = cinTable[currentHandle].VQNormal;
	cinTable[currentHandle].VQ1 = cinTable[currentHandle].VQBuffer;

	cinTable[currentHandle].Cin->t[0] = cinTable[currentHandle].Cin->screenDelta;
	cinTable[currentHandle].Cin->t[1] = -cinTable[currentHandle].Cin->screenDelta;

	cinTable[currentHandle].drawX = cinTable[currentHandle].Cin->Width;
	cinTable[currentHandle].drawY = cinTable[currentHandle].Cin->Height;

	// voodoo can't do it at all
	if ( glConfig.maxTextureSize <= 256 ) {
		if ( cinTable[currentHandle].drawX > 256 ) {
			cinTable[currentHandle].drawX = 256;
		}
		if ( cinTable[currentHandle].drawY > 256 ) {
			cinTable[currentHandle].drawY = 256;
		}
		if ( cinTable[currentHandle].Cin->Width != 256 || cinTable[currentHandle].Cin->Height != 256 ) {
			Com_Printf( "HACK: approxmimating cinematic for Rage Pro or Voodoo\n" );
		}
	}
//#ifdef __MACOS__
//	cinTable[currentHandle].drawX = 256;
//	cinTable[currentHandle].drawX = 256;
//#endif
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQPrepMcomp( long xoff, long yoff ) {
	long i, j, x, y, temp, temp2;

	i = cinTable[currentHandle].Cin->samplesPerLine; j = 4;
	if ( cinTable[currentHandle].xsize == ( cinTable[currentHandle].ysize * 4 ) ) {
		j = j + j; i = i + i;
	}

	for ( y = 0; y < 16; y++ ) {
		temp2 = ( y + yoff - 8 ) * i;
		for ( x = 0; x < 16; x++ ) {
			temp = ( x + xoff - 8 ) * j;
			cinTable[currentHandle].Cin->mcomp[( x * 16 ) + y] = cinTable[currentHandle].Cin->normalBuffer0 - ( temp2 + temp );
		}
	}
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

void initRoQ();

static void initRoQ_() {
	if ( currentHandle < 0 ) {
		return;
	}

	cinTable[currentHandle].VQNormal = ( void( * ) ( byte *, void * ) )blitVQQuad32fs;
	cinTable[currentHandle].VQBuffer = ( void( * ) ( byte *, void * ) )blitVQQuad32fs;
	initRoQ();
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/
/*
static byte* RoQFetchInterlaced( byte *source ) {
	int x, *src, *dst;

	if (currentHandle < 0) return NULL;

	src = (int *)source;
	dst = (int *)cinTable[currentHandle].buf2;

	for(x=0;x<256*256;x++) {
		*dst = *src;
		dst++; src += 2;
	}
	return cinTable[currentHandle].buf2;
}
*/
static void RoQReset() {

	if ( currentHandle < 0 ) {
		return;
	}

	// DHM - Properly close file so we don't run out of handles
	FS_FCloseFile( cinTable[currentHandle].Cin->iFile );
	cinTable[currentHandle].Cin->iFile = 0;
	// dhm - end

	FS_FOpenFileRead( cinTable[currentHandle].Cin->Name, &cinTable[currentHandle].Cin->iFile, qtrue );
	FS_Read( cinTable[currentHandle].Cin->file, 16, cinTable[currentHandle].Cin->iFile );
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

static void RoQInterrupt( void ) {
	byte                *framedata;
	short sbuf[32768];
	int ssize;

	if ( currentHandle < 0 ) {
		return;
	}

	FS_Read( cinTable[currentHandle].Cin->file, cinTable[currentHandle].Cin->RoQFrameSize + 8, cinTable[currentHandle].Cin->iFile );
	if ( cinTable[currentHandle].Cin->RoQPlayed >= cinTable[currentHandle].Cin->ROQSize ) {
		if ( cinTable[currentHandle].holdAtEnd == qfalse ) {
			if ( cinTable[currentHandle].looping ) {
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
	switch ( cinTable[currentHandle].Cin->roq_id )
	{
	case    ROQ_QUAD_VQ:
		if ( ( cinTable[currentHandle].Cin->numQuads & 1 ) ) {
			cinTable[currentHandle].Cin->normalBuffer0 = cinTable[currentHandle].Cin->t[1];
			RoQPrepMcomp( cinTable[currentHandle].Cin->roqF0, cinTable[currentHandle].Cin->roqF1 );
			cinTable[currentHandle].VQ1( (byte *)cinTable[currentHandle].Cin->qStatus[1], framedata );
			cinTable[currentHandle].Cin->OutputFrame =   cinTable[currentHandle].Cin->linbuf + cinTable[currentHandle].Cin->screenDelta;
		} else {
			cinTable[currentHandle].Cin->normalBuffer0 = cinTable[currentHandle].Cin->t[0];
			RoQPrepMcomp( cinTable[currentHandle].Cin->roqF0, cinTable[currentHandle].Cin->roqF1 );
			cinTable[currentHandle].VQ0( (byte *)cinTable[currentHandle].Cin->qStatus[0], framedata );
			cinTable[currentHandle].Cin->OutputFrame =   cinTable[currentHandle].Cin->linbuf;
		}
		if ( cinTable[currentHandle].Cin->numQuads == 0 ) {          // first frame
			Com_Memcpy( cinTable[currentHandle].Cin->linbuf + cinTable[currentHandle].Cin->screenDelta, cinTable[currentHandle].Cin->linbuf, cinTable[currentHandle].Cin->samplesPerLine * cinTable[currentHandle].ysize );
		}
		cinTable[currentHandle].Cin->numQuads++;
		cinTable[currentHandle].Cin->Dirty = qtrue;
		break;
	case    ROQ_CODEBOOK:
		decodeCodeBook( framedata, (unsigned short)cinTable[currentHandle].Cin->roq_flags );
		break;
	case    ZA_SOUND_MONO:
		if ( !cinTable[currentHandle].Cin->Silent ) {
			ssize = RllDecodeMonoToStereo( framedata, sbuf, cinTable[currentHandle].Cin->RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].Cin->roq_flags );
			S_RawSamples( ssize, 22050, 2, 1, (byte *)sbuf, 1.0f, 1.0f, 0 );
		}
		break;
	case    ZA_SOUND_STEREO:
		if ( !cinTable[currentHandle].Cin->Silent ) {
			ssize = RllDecodeStereoToStereo( framedata, sbuf, cinTable[currentHandle].Cin->RoQFrameSize, 0, (unsigned short)cinTable[currentHandle].Cin->roq_flags );
			S_RawSamples( ssize, 22050, 2, 2, (byte *)sbuf, 1.0f, 1.0f, 0 );
		}
		break;
	case    ROQ_QUAD_INFO:
		if ( cinTable[currentHandle].Cin->numQuads == -1 ) {
			readQuadInfo( framedata );
			setupQuad( 0, 0 );
			// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
			cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds() * com_timescale->value;
		}
		if ( cinTable[currentHandle].Cin->numQuads != 1 ) {
			cinTable[currentHandle].Cin->numQuads = 0;
		}
		break;
	case    ROQ_PACKET:
		cinTable[currentHandle].Cin->inMemory = cinTable[currentHandle].Cin->roq_flags;
		cinTable[currentHandle].Cin->RoQFrameSize = 0;               // for header
		break;
	case    ROQ_QUAD_HANG:
		cinTable[currentHandle].Cin->RoQFrameSize = 0;
		break;
	case    ROQ_QUAD_JPEG:
		break;
	default:
		cinTable[currentHandle].status = FMV_EOF;
		break;
	}
//
// read in next frame data
//
	if ( cinTable[currentHandle].Cin->RoQPlayed >= cinTable[currentHandle].Cin->ROQSize ) {
		if ( cinTable[currentHandle].holdAtEnd == qfalse ) {
			if ( cinTable[currentHandle].looping ) {
				RoQReset();
			} else {
				cinTable[currentHandle].status = FMV_EOF;
			}
		} else {
			cinTable[currentHandle].status = FMV_IDLE;
		}
		return;
	}

	framedata        += cinTable[currentHandle].Cin->RoQFrameSize;
	cinTable[currentHandle].Cin->roq_id       = framedata[0] + framedata[1] * 256;
	cinTable[currentHandle].Cin->RoQFrameSize = framedata[2] + framedata[3] * 256 + framedata[4] * 65536;
	cinTable[currentHandle].Cin->roq_flags    = framedata[6] + framedata[7] * 256;
	cinTable[currentHandle].Cin->roqF0        = (char)framedata[7];
	cinTable[currentHandle].Cin->roqF1        = (char)framedata[6];

	if ( cinTable[currentHandle].Cin->RoQFrameSize > 65536 || cinTable[currentHandle].Cin->roq_id == 0x1084 ) {
		Com_DPrintf( "roq_size>65536||roq_id==0x1084\n" );
		cinTable[currentHandle].status = FMV_EOF;
		if ( cinTable[currentHandle].looping ) {
			RoQReset();
		}
		return;
	}
	if ( cinTable[currentHandle].Cin->inMemory && ( cinTable[currentHandle].status != FMV_EOF ) ) {
		cinTable[currentHandle].Cin->inMemory--; framedata += 8; goto redump;
	}
//
// one more frame hits the dust
//
//	assert(cinTable[currentHandle].RoQFrameSize <= 65536);
//	r = FS_Read( cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile );
	cinTable[currentHandle].Cin->RoQPlayed   += cinTable[currentHandle].Cin->RoQFrameSize + 8;
}

/******************************************************************************
*
* Function:
*
* Description:
*
******************************************************************************/

static void RoQ_init( void ) {
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].startTime = cinTable[currentHandle].lastTime = CL_ScaledMilliseconds() * com_timescale->value;

	cinTable[currentHandle].Cin->RoQPlayed = 24;

/*	get frame rate */
	cinTable[currentHandle].Cin->roqFPS   = cinTable[currentHandle].Cin->file[ 6] + cinTable[currentHandle].Cin->file[ 7] * 256;

	if ( !cinTable[currentHandle].Cin->roqFPS ) {
		cinTable[currentHandle].Cin->roqFPS = 30;
	}

	cinTable[currentHandle].Cin->numQuads = -1;

	cinTable[currentHandle].Cin->roq_id      = cinTable[currentHandle].Cin->file[ 8] + cinTable[currentHandle].Cin->file[ 9] * 256;
	cinTable[currentHandle].Cin->RoQFrameSize    = cinTable[currentHandle].Cin->file[10] + cinTable[currentHandle].Cin->file[11] * 256 + cinTable[currentHandle].Cin->file[12] * 65536;
	cinTable[currentHandle].Cin->roq_flags   = cinTable[currentHandle].Cin->file[14] + cinTable[currentHandle].Cin->file[15] * 256;

	if ( cinTable[currentHandle].Cin->RoQFrameSize > 65536 || !cinTable[currentHandle].Cin->RoQFrameSize ) {
		return;
	}

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

	if ( !cinTable[currentHandle].Cin->OutputFrame ) {
		return;
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return;
	}
	Com_DPrintf( "finished cinematic\n" );
	cinTable[currentHandle].status = FMV_IDLE;

	if ( cinTable[currentHandle].alterGameState ) {
		cls.state = CA_DISCONNECTED;
		// we can't just do a vstr nextmap, because
		// if we are aborting the intro cinematic with
		// a devmap command, nextmap would be valid by
		// the time it was referenced
		s = Cvar_VariableString( "nextmap" );
		if ( s[0] ) {
			Cbuf_ExecuteText( EXEC_APPEND, va( "%s\n", s ) );
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
e_status CIN_StopCinematic( int handle ) {

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return FMV_EOF;
	}
	currentHandle = handle;

	Com_DPrintf( "trFMV::stop(), closing %s\n", cinTable[currentHandle].Cin->Name );

	if ( !cinTable[currentHandle].Cin->OutputFrame ) {
		return FMV_EOF;
	}

	if ( cinTable[currentHandle].alterGameState ) {
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


e_status CIN_RunCinematic( int handle ) {
	// bk001204 - init
	int start = 0;
	int thisTime = 0;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return FMV_EOF;
	}

	if ( cin.currentHandle != handle ) {
		currentHandle = handle;
		cin.currentHandle = currentHandle;
		cinTable[currentHandle].status = FMV_EOF;
		RoQReset();
	}

	if ( cinTable[handle].playonwalls < -1 ) {
		return cinTable[handle].status;
	}

	currentHandle = handle;

	if ( cinTable[currentHandle].alterGameState ) {
		if ( cls.state != CA_CINEMATIC ) {
			return cinTable[currentHandle].status;
		}
	}

	if ( cinTable[currentHandle].status == FMV_IDLE ) {
		return cinTable[currentHandle].status;
	}

	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	thisTime = CL_ScaledMilliseconds() * com_timescale->value;
	if ( cinTable[currentHandle].shader && ( abs( thisTime - (int)cinTable[currentHandle].lastTime ) ) > 100 ) {
		cinTable[currentHandle].startTime += thisTime - cinTable[currentHandle].lastTime;
	}
	// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
	cinTable[currentHandle].tfps = ( ( ( ( CL_ScaledMilliseconds() * com_timescale->value ) - cinTable[currentHandle].startTime ) * 3 ) / 100 );

	start = cinTable[currentHandle].startTime;
	while (  ( cinTable[currentHandle].tfps != cinTable[currentHandle].Cin->numQuads )
			 && ( cinTable[currentHandle].status == FMV_PLAY ) )
	{
		RoQInterrupt();
		if ( start != cinTable[currentHandle].startTime ) {
			// we need to use CL_ScaledMilliseconds because of the smp mode calls from the renderer
			cinTable[currentHandle].tfps = ( ( ( ( CL_ScaledMilliseconds() * com_timescale->value )
												 - cinTable[currentHandle].startTime ) * 3 ) / 100 );
			start = cinTable[currentHandle].startTime;
		}
	}

	cinTable[currentHandle].lastTime = thisTime;

	if ( cinTable[currentHandle].status == FMV_LOOPED ) {
		cinTable[currentHandle].status = FMV_PLAY;
	}

	if ( cinTable[currentHandle].status == FMV_EOF ) {
		if ( cinTable[currentHandle].looping ) {
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
	char name[MAX_OSPATH];
	int i;

	if ( strstr( arg, "/" ) == NULL && strstr( arg, "\\" ) == NULL ) {
		String::Sprintf( name, sizeof( name ), "video/%s", arg );
	} else {
		String::Sprintf( name, sizeof( name ), "%s", arg );
	}

	if ( !( systemBits & CIN_system ) ) {
		for ( i = 0 ; i < MAX_VIDEO_HANDLES ; i++ ) {
			if ( cinTable[i].Cin && !String::Cmp( cinTable[i].Cin->Name, name ) ) {
				return i;
			}
		}
	}

	Com_DPrintf( "SCR_PlayCinematic( %s )\n", arg );

	Com_Memset( &cin, 0, sizeof( cinematics_t ) );
	currentHandle = CIN_HandleForVideo();

	cin.currentHandle = currentHandle;

	cinTable[currentHandle].Cin = new QCinematicRoq();
	String::Cpy( cinTable[currentHandle].Cin->Name, name );

	cinTable[currentHandle].Cin->ROQSize = 0;
	cinTable[currentHandle].Cin->ROQSize = FS_FOpenFileRead( cinTable[currentHandle].Cin->Name, &cinTable[currentHandle].Cin->iFile, qtrue );

	if ( cinTable[currentHandle].Cin->ROQSize <= 0 ) {
		Com_DPrintf( "play(%s), ROQSize<=0\n", arg );
		delete cinTable[currentHandle].Cin;
		cinTable[currentHandle].Cin = NULL;
		return -1;
	}

	CIN_SetExtents( currentHandle, x, y, w, h );
	CIN_SetLooping( currentHandle, ( systemBits & CIN_loop ) != 0 );

	cinTable[currentHandle].Cin->Height = QCinematicRoq::DEFAULT_CIN_HEIGHT;
	cinTable[currentHandle].Cin->Width =  QCinematicRoq::DEFAULT_CIN_WIDTH;
	cinTable[currentHandle].holdAtEnd = ( systemBits & CIN_hold ) != 0;
	cinTable[currentHandle].alterGameState = ( systemBits & CIN_system ) != 0;
	cinTable[currentHandle].playonwalls = 1;
	cinTable[currentHandle].Cin->Silent = ( systemBits & CIN_silent ) != 0;
	cinTable[currentHandle].shader = ( systemBits & CIN_shader ) != 0;

	if ( cinTable[currentHandle].alterGameState ) {
		// close the menu
		if ( uivm ) {
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NONE );
		}
	} else {
		cinTable[currentHandle].playonwalls = cl_inGameVideo->integer;
	}

	initRoQ_();

	FS_Read( cinTable[currentHandle].Cin->file, 16, cinTable[currentHandle].Cin->iFile );

	RoQID = ( unsigned short )( cinTable[currentHandle].Cin->file[0] ) + ( unsigned short )( cinTable[currentHandle].Cin->file[1] ) * 256;
	if ( RoQID == 0x1084 ) {
		RoQ_init();
//		FS_Read (cin.file, cinTable[currentHandle].RoQFrameSize+8, cinTable[currentHandle].iFile);

		cinTable[currentHandle].status = FMV_PLAY;
		Com_DPrintf( "trFMV::play(), playing %s\n", arg );

		if ( cinTable[currentHandle].alterGameState ) {
			cls.state = CA_CINEMATIC;
		}

		Con_Close();

//		s_rawend = s_soundtime;

		return currentHandle;
	}
	Com_DPrintf( "trFMV::play(), invalid RoQ ID\n" );

	RoQShutdown();
	return -1;
}

void CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return;
	}
	cinTable[handle].xpos = x;
	cinTable[handle].ypos = y;
	cinTable[handle].width = w;
	cinTable[handle].height = h;
	cinTable[handle].Cin->Dirty = qtrue;
}

void CIN_SetLooping( int handle, qboolean loop ) {
	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return;
	}
	cinTable[handle].looping = loop;
}

/*
==================
SCR_DrawCinematic

==================
*/
void CIN_DrawCinematic( int handle ) {
	float x, y, w, h;
	byte    *buf;

	if ( handle < 0 || handle >= MAX_VIDEO_HANDLES || cinTable[handle].status == FMV_EOF ) {
		return;
	}

	if ( !cinTable[handle].Cin->OutputFrame ) {
		return;
	}

	x = cinTable[handle].xpos;
	y = cinTable[handle].ypos;
	w = cinTable[handle].width;
	h = cinTable[handle].height;
	buf = cinTable[handle].Cin->OutputFrame;
	SCR_AdjustFrom640( &x, &y, &w, &h );

	if ( cinTable[handle].Cin->Dirty && ( cinTable[handle].Cin->Width != cinTable[handle].drawX || cinTable[handle].Cin->Height != cinTable[handle].drawY ) ) {
		int ix, iy, *buf2, *buf3, xm, ym, ll;

		xm = cinTable[handle].Cin->Width / 256;
		ym = cinTable[handle].Cin->Height / 256;
		ll = 8;
		if ( cinTable[handle].Cin->Width == 512 ) {
			ll = 9;
		}

		buf3 = (int*)buf;
		buf2 = (int*)Hunk_AllocateTempMemory( 256 * 256 * 4 );
		if ( xm == 2 && ym == 2 ) {
			byte *bc2, *bc3;
			int ic, iiy;

			bc2 = (byte *)buf2;
			bc3 = (byte *)buf3;
			for ( iy = 0; iy < 256; iy++ ) {
				iiy = iy << 12;
				for ( ix = 0; ix < 2048; ix += 8 ) {
					for ( ic = ix; ic < ( ix + 4 ); ic++ ) {
						*bc2 = ( bc3[iiy + ic] + bc3[iiy + 4 + ic] + bc3[iiy + 2048 + ic] + bc3[iiy + 2048 + 4 + ic] ) >> 2;
						bc2++;
					}
				}
			}
		} else if ( xm == 2 && ym == 1 ) {
			byte *bc2, *bc3;
			int ic, iiy;

			bc2 = (byte *)buf2;
			bc3 = (byte *)buf3;
			for ( iy = 0; iy < 256; iy++ ) {
				iiy = iy << 11;
				for ( ix = 0; ix < 2048; ix += 8 ) {
					for ( ic = ix; ic < ( ix + 4 ); ic++ ) {
						*bc2 = ( bc3[iiy + ic] + bc3[iiy + 4 + ic] ) >> 1;
						bc2++;
					}
				}
			}
		} else {
			for ( iy = 0; iy < 256; iy++ ) {
				for ( ix = 0; ix < 256; ix++ ) {
					buf2[( iy << 8 ) + ix] = buf3[( ( iy * ym ) << ll ) + ( ix * xm )];
				}
			}
		}
		re.DrawStretchRaw( x, y, w, h, 256, 256, (byte *)buf2, handle, qtrue );
		cinTable[handle].Cin->Dirty = qfalse;
		Hunk_FreeTempMemory( buf2 );
		return;
	}

	re.DrawStretchRaw( x, y, w, h, cinTable[handle].drawX, cinTable[handle].drawY, buf, handle, cinTable[handle].Cin->Dirty );
	cinTable[handle].Cin->Dirty = qfalse;
}

void CL_PlayCinematic_f( void ) {
	char    *arg, *s;
	qboolean holdatend;
	int bits = CIN_system;

	Com_DPrintf( "CL_PlayCinematic_f\n" );
	if ( cls.state == CA_CINEMATIC ) {
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
		} while ( cinTable[currentHandle].Cin->OutputFrame == NULL && cinTable[currentHandle].status == FMV_PLAY );        // wait for first frame (load codebook and sound)
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
	if ( handle >= 0 && handle < MAX_VIDEO_HANDLES ) {
		if ( !cinTable[handle].Cin->OutputFrame ) {
			return;
		}
		if ( cinTable[handle].playonwalls <= 0 && cinTable[handle].Cin->Dirty ) {
			if ( cinTable[handle].playonwalls == 0 ) {
				cinTable[handle].playonwalls = -1;
			} else {
				if ( cinTable[handle].playonwalls == -1 ) {
					cinTable[handle].playonwalls = -2;
				} else {
					cinTable[handle].Cin->Dirty = qfalse;
				}
			}
		}
		re.UploadCinematic( 256, 256, 256, 256, cinTable[handle].Cin->OutputFrame, handle, cinTable[handle].Cin->Dirty );
		if ( cl_inGameVideo->integer == 0 && cinTable[handle].playonwalls == 1 ) {
			cinTable[handle].playonwalls--;
		}
	}
}
