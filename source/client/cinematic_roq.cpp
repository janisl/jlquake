//**************************************************************************
//**
//**	Copyright (C) 1996-2005 Id Software, Inc.
//**	Copyright (C) 2010 Jānis Legzdiņš
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "cinematic_local.h"
#include "sound_local.h"

// MACROS ------------------------------------------------------------------

#define MAXSIZE				8
#define MINSIZE				4

#define ROQ_QUAD			0x1000
#define ROQ_QUAD_INFO		0x1001
#define ROQ_CODEBOOK		0x1002
#define ROQ_QUAD_VQ			0x1011
#define ROQ_QUAD_JPEG		0x1012
#define ROQ_QUAD_HANG		0x1013
#define ROQ_PACKET			0x1030
#define ZA_SOUND_MONO		0x1020
#define ZA_SOUND_STEREO		0x1021

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static long				ROQ_YY_tab[256];
static long				ROQ_UB_tab[256];
static long				ROQ_UG_tab[256];
static long				ROQ_VG_tab[256];
static long				ROQ_VR_tab[256];

static short			sqrTable[256];

static unsigned short	vq2[256 * 16 * 4];
static unsigned short	vq4[256 * 64 * 4];
static unsigned short	vq8[256 * 256 * 4];

static int				mcomp[256];

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QCinematicRoq::~QCinematicRoq
//
//==========================================================================

QCinematicRoq::~QCinematicRoq()
{
	if (iFile)
	{
		FS_FCloseFile(iFile);
		iFile = 0;
	}
}

//==========================================================================
//
//	ROQ_GenYUVTables
//
//==========================================================================

static void ROQ_GenYUVTables()
{
	float t_ub = (1.77200f / 2.0f) * (float)(1 << 6) + 0.5f;
	float t_vr = (1.40200f / 2.0f) * (float)(1 << 6) + 0.5f;
	float t_ug = (0.34414f / 2.0f) * (float)(1 << 6) + 0.5f;
	float t_vg = (0.71414f / 2.0f) * (float)(1 << 6) + 0.5f;
	for (int i = 0; i < 256; i++)
	{
		float x = (float)(2 * i - 255);
	
		ROQ_UB_tab[i] = (long)(( t_ub * x) + (1 << 5));
		ROQ_VR_tab[i] = (long)(( t_vr * x) + (1 << 5));
		ROQ_UG_tab[i] = (long)((-t_ug * x));
		ROQ_VG_tab[i] = (long)((-t_vg * x) + (1 << 5));
		ROQ_YY_tab[i] = (long)((i << 6) | (i >> 2));
	}
}

//==========================================================================
//
//	RllSetupTable
//
//	Allocates and initializes the square table.
//
//==========================================================================

static void RllSetupTable()
{
	for (int z = 0; z < 128; z++)
	{
		sqrTable[z] = (short)(z * z);
		sqrTable[z + 128] = (short)(-sqrTable[z]);
	}
}

//==========================================================================
//
//	initRoQ
//
//==========================================================================

static void initRoQ()
{
	ROQ_GenYUVTables();
	RllSetupTable();
}

//==========================================================================
//
//	QCinematicRoq::Open
//
//==========================================================================

bool QCinematicRoq::Open(const char* FileName)
{
	QStr::Cpy(fileName, FileName);
	ROQSize = FS_FOpenFileRead(fileName, &iFile, true);

	if (ROQSize <= 0)
	{
		GLog.DWrite("play(%s), ROQSize<=0\n", FileName);
		return false;
	}

	initRoQ();

	FS_Read(file, 16, iFile);

	unsigned short RoQID = (unsigned short)(file[0]) + (unsigned short)(file[1]) * 256;
	if (RoQID != 0x1084)
	{
		GLog.DWrite("trFMV::play(), invalid RoQ ID\n");
		return false;
	}

	init();
	return true;
}

//==========================================================================
//
//	QCinematicRoq::init
//
//==========================================================================

void QCinematicRoq::init()
{
	RoQPlayed = 24;

	//	get frame rate
	roqFPS = file[6] + file[7] * 256;
	
	if (!roqFPS)
	{
		roqFPS = 30;
	}

	numQuads = -1;

	roq_id = file[8] + file[9] * 256;
	RoQFrameSize = file[10] + file[11] * 256 + file[12] * 65536;
	roq_flags = file[14] + file[15]*256;
}

//==========================================================================
//
//	QCinematicRoq::Update
//
//==========================================================================

bool QCinematicRoq::Update(int NewTime)
{
	int tfps = (NewTime * 3) / 100;

	while (tfps != numQuads)
	{
		if (!ReadFrame())
		{
			return false;
		}
	}
	return true;
}

//==========================================================================
//
//	QCinematicRoq::readQuadInfo
//
//==========================================================================

void QCinematicRoq::readQuadInfo(byte* qData)
{
	xsize = qData[0] + qData[1] * 256;
	ysize = qData[2] + qData[3] * 256;
	maxsize = qData[4] + qData[5] * 256;
	minsize = qData[6] + qData[7] * 256;
	
	samplesPerLine = xsize * 4;
	screenDelta = ysize * samplesPerLine;

	t[0] = screenDelta;
	t[1] = -screenDelta;
}

//==========================================================================
//
//	QCinematicRoq::setupQuad
//
//==========================================================================

void QCinematicRoq::setupQuad()
{
	long numQuadCels = (xsize * ysize) / 16;
	numQuadCels += numQuadCels / 4;
	numQuadCels += 64;							  // for overflow

	onQuad = 0;

	for (long y = 0; y < (long)ysize; y+=16)
	{
		for (long x = 0; x < (long)xsize; x += 16)
		{
			recurseQuad(x, y, 16);
		}
	}

	for (long i = (numQuadCels - 64); i < numQuadCels; i++)
	{
		qStatus[0][i] = NULL;			  // eoq
		qStatus[1][i] = NULL;			  // eoq
	}
}

//==========================================================================
//
//	QCinematicRoq::recurseQuad
//
//==========================================================================

void QCinematicRoq::recurseQuad(long startX, long startY, long quadSize)
{
	if ((startX >= 0) && (startX + quadSize) <= xsize && (startY + quadSize) <= ysize && (startY >= 0) && quadSize <= MAXSIZE)
	{
		byte* scroff = linbuf + startY * samplesPerLine + startX * 4;

		qStatus[0][onQuad] = scroff;
		qStatus[1][onQuad++] = scroff + screenDelta;
	}

	if (quadSize != MINSIZE)
	{
		quadSize >>= 1;
		recurseQuad(startX, startY, quadSize);
		recurseQuad(startX + quadSize, startY, quadSize);
		recurseQuad(startX, startY + quadSize, quadSize);
		recurseQuad(startX + quadSize, startY + quadSize, quadSize);
	}
}

//==========================================================================
//
//	RllDecodeMonoToStereo
//
//	Decode mono source data into a stereo buffer. Output is 4 times the number
// of bytes in the input.
//
//	Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= 1/4 # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
//	Returns:	Number of samples placed in output buffer
//
//==========================================================================

static long RllDecodeMonoToStereo(unsigned char* from, short* to, unsigned int size, char signedOutput, unsigned short flag)
{
	int prev;
	if (signedOutput)
	{
		prev =  flag - 0x8000;
	}
	else 
	{
		prev = flag;
	}

	for (unsigned int z = 0; z < size; z++)
	{
		prev = (short)(prev + sqrTable[from[z]]);
		to[z * 2 + 0] = to[z * 2 + 1] = (short)prev;
	}
	
	return size;	// * 2 * sizeof(short));
}

//==========================================================================
//
//	RllDecodeStereoToStereo
//
//	Decode stereo source data into a stereo buffer.
//
//	Parameters:	from -> buffer holding encoded data
//				to ->	buffer to hold decoded data
//				size =	number of bytes of input (= 1/2 # of bytes of output)
//				signedOutput = 0 for unsigned output, non-zero for signed output
//				flag = flags from asset header
//
//	Returns:	Number of samples placed in output buffer
//
//==========================================================================

static long RllDecodeStereoToStereo(unsigned char* from, short* to, unsigned int size, char signedOutput, unsigned short flag)
{
	int prevL, prevR;
	if (signedOutput)
	{
		prevL = (flag & 0xff00) - 0x8000;
		prevR = ((flag & 0x00ff) << 8) - 0x8000;
	}
	else
	{
		prevL = flag & 0xff00;
		prevR = (flag & 0x00ff) << 8;
	}

	unsigned char* zz = from;
	for (unsigned int z = 0; z < size; z += 2)
	{
		prevL = (short)(prevL + sqrTable[*zz++]); 
		prevR = (short)(prevR + sqrTable[*zz++]);
		to[z + 0] = (short)(prevL);
		to[z + 1] = (short)(prevR);
	}
	
	return (size>>1);	//*sizeof(short));
}

//==========================================================================
//
//	QCinematicRoq::RoQPrepMcomp
//
//==========================================================================

void QCinematicRoq::RoQPrepMcomp(long xoff, long yoff)
{
	long i, j, x, y, temp, temp2;

	i=samplesPerLine; j = 4;
	if ( xsize == (ysize*4) ) { j = j+j; i = i+i; }
	
	for(y=0;y<16;y++) {
		temp2 = (y+yoff-8)*i;
		for(x=0;x<16;x++) {
			temp = (x+xoff-8)*j;
			mcomp[(x*16)+y] = normalBuffer0-(temp2+temp);
		}
	}
}

//==========================================================================
//
//	move8_32
//
//==========================================================================

static void move8_32(byte* src, byte* dst, int spl)
{
	double* dsrc = (double*)src;
	double* ddst = (double*)dst;
	int dspl = spl >> 3;

	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
}

//==========================================================================
//
//	move4_32
//
//==========================================================================

static void move4_32(byte* src, byte* dst, int spl)
{
	double* dsrc = (double*)src;
	double* ddst = (double*)dst;
	int dspl = spl >> 3;

	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
	dsrc += dspl; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
}

//==========================================================================
//
//	blit8_32
//
//==========================================================================

static void blit8_32(byte* src, byte* dst, int spl)
{
	double* dsrc = (double*)src;
	double* ddst = (double*)dst;
	int dspl = spl >> 3;

	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
	dsrc += 4; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1]; ddst[2] = dsrc[2]; ddst[3] = dsrc[3];
}

//==========================================================================
//
//	blit4_32
//
//==========================================================================

static void blit4_32(byte* src, byte* dst, int spl)
{
	double* dsrc = (double*)src;
	double* ddst = (double*)dst;
	int dspl = spl >> 3;

	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
	dsrc += 2; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
	dsrc += 2; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
	dsrc += 2; ddst += dspl;
	ddst[0] = dsrc[0]; ddst[1] = dsrc[1];
}

//==========================================================================
//
//	blit2_32
//
//==========================================================================

static void blit2_32(byte* src, byte* dst, int spl)
{
	double* dsrc = (double*)src;
	double* ddst = (double*)dst;
	int dspl = spl >> 3;

	ddst[0] = dsrc[0];
	ddst[dspl] = dsrc[1];
}

//==========================================================================
//
//	QCinematicRoq::blitVQQuad32fs
//
//==========================================================================

void QCinematicRoq::blitVQQuad32fs(byte** status, unsigned char* data)
{
	unsigned short newd = 0;
	unsigned short celdata = 0;
	unsigned int index = 0;

	int spl = samplesPerLine;
        
	do
	{
		if (!newd)
		{
			newd = 7;
			celdata = data[0] + data[1] * 256;
			data += 2;
		}
		else
		{
			newd--;
		}

		unsigned short code = (unsigned short)(celdata & 0xc000);
		celdata <<= 2;

		switch (code)
		{
		case 0x8000:													// vq code
			blit8_32((byte*)&vq8[(*data) * 128], status[index], spl);
			data++;
			index += 5;
			break;

		case 0xc000:													// drop
			index++;													// skip 8x8
			for (int i = 0; i < 4; i++)
			{
				if (!newd)
				{
					newd = 7;
					celdata = data[0] + data[1] * 256;
					data += 2;
				}
				else
				{
					newd--;
				}

				code = (unsigned short)(celdata & 0xc000);
				celdata <<= 2; 

				switch (code)
				{											// code in top two bits of code
				case 0x8000:										// 4x4 vq code
					blit4_32((byte*)&vq4[(*data) * 32], status[index], spl);
					data++;
					break;
				case 0xc000:										// 2x2 vq code
					blit2_32((byte*)&vq2[(*data) * 8], status[index], spl);
					data++;
					blit2_32((byte*)&vq2[(*data) * 8], status[index] + 8, spl);
					data++;
					blit2_32((byte*)&vq2[(*data) * 8], status[index] + spl * 2, spl);
					data++;
					blit2_32((byte*)&vq2[(*data) * 8], status[index] + spl * 2 + 8, spl);
					data++;
					break;
				case 0x4000:										// motion compensation
					move4_32(status[index] + mcomp[(*data)], status[index], spl);
					data++;
					break;
				}
				index++;
			}
			break;

		case 0x4000:													// motion compensation
			move8_32(status[index] + mcomp[(*data)], status[index], spl);
			data++;
			index += 5;
			break;

		case 0x0000:
			index += 5;
			break;
		}
	}
	while (status[index] != NULL);
}
 
//==========================================================================
//
//	yuv_to_rgb24
//
//==========================================================================

static void yuv_to_rgb24(long y, long u, long v, byte* out)
{ 
	long YY = (long)ROQ_YY_tab[y];

	long r = (YY + ROQ_VR_tab[v]) >> 6;
	long g = (YY + ROQ_UG_tab[u] + ROQ_VG_tab[v]) >> 6;
	long b = (YY + ROQ_UB_tab[u]) >> 6;
	
	if (r < 0)
	{
		r = 0;
	}
	if (g < 0)
	{
		g = 0;
	}
	if (b < 0)
	{
		b = 0;
	}
	if (r > 255)
	{
		r = 255;
	}
	if (g > 255)
	{
		g = 255;
	}
	if (b > 255)
	{
		b = 255;
	}
	
	out[0] = r;
	out[1] = g;
	out[2] = b;
	out[3] = 255;
}

//==========================================================================
//
//	decodeCodeBook
//
//==========================================================================

static void decodeCodeBook(byte* input, unsigned short roq_flags)
{
	int two, four;
	if (!roq_flags)
	{
		two = four = 256;
	}
	else
	{
		two = roq_flags >> 8;
		if (!two)
		{
			two = 256;
		}
		four = roq_flags & 0xff;
	}

	four *= 2;

	byte* rgb_ptr = (byte*)vq2;
	for (int i = 0; i < two; i++)
	{
		long y0 = (long)*input++;
		long y1 = (long)*input++;
		long y2 = (long)*input++;
		long y3 = (long)*input++;
		long cr = (long)*input++;
		long cb = (long)*input++;
		yuv_to_rgb24(y0, cr, cb, rgb_ptr);
		yuv_to_rgb24(y1, cr, cb, rgb_ptr + 4);
		yuv_to_rgb24(y2, cr, cb, rgb_ptr + 8);
		yuv_to_rgb24(y3, cr, cb, rgb_ptr + 12);
		rgb_ptr += 16;
	}

	unsigned int* icptr = (unsigned int*)vq4;
	unsigned int* idptr = (unsigned int*)vq8;

#define VQ2TO4(a,b,c,d) { \
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

	for (int i = 0; i < four; i++)
	{
		unsigned int* iaptr = (unsigned int*)vq2 + (*input++) * 4;
		unsigned int* ibptr = (unsigned int*)vq2 + (*input++) * 4;
		for (int j = 0; j < 2; j++)
		{
			VQ2TO4(iaptr, ibptr, icptr, idptr);
		}
	}
}

//==========================================================================
//
//	QCinematicRoq::ReadFrame
//
//==========================================================================

bool QCinematicRoq::ReadFrame()
{
	FS_Read(file, RoQFrameSize + 8, iFile);
	if (RoQPlayed >= ROQSize)
	{ 
		return false;
	}

	short sbuf[32768];
        
	byte* framedata = file;
//
// new frame is ready
//
redump:
	switch (roq_id)
	{
	case ROQ_QUAD_VQ:
		if (numQuads & 1)
		{
			normalBuffer0 = t[1];
			RoQPrepMcomp(roqF0, roqF1);
			blitVQQuad32fs(qStatus[1], framedata);
			buf = linbuf + screenDelta;
		}
		else
		{
			normalBuffer0 = t[0];
			RoQPrepMcomp(roqF0, roqF1);
			blitVQQuad32fs(qStatus[0], framedata);
			buf = linbuf;
		}
		if (numQuads == 0)
		{
			// first frame
			Com_Memcpy(linbuf + screenDelta, linbuf, samplesPerLine * ysize);
		}
		numQuads++;
		dirty = true;
		break;

	case ROQ_CODEBOOK:
		decodeCodeBook(framedata, (unsigned short)roq_flags);
		break;

	case ZA_SOUND_MONO:
		if (!silent)
		{
			int ssize = RllDecodeMonoToStereo(framedata, sbuf, RoQFrameSize, 0, (unsigned short)roq_flags);
			S_RawSamples(ssize, 22050, 2, 1, (byte*)sbuf, 1.0f);
		}
		break;

	case ZA_SOUND_STEREO:
		if (!silent)
		{
			if (numQuads == -1)
			{
				S_Update();
				s_rawend = s_soundtime;
			}
			int ssize = RllDecodeStereoToStereo(framedata, sbuf, RoQFrameSize, 0, (unsigned short)roq_flags);
			S_RawSamples(ssize, 22050, 2, 2, (byte*)sbuf, 1.0f);
		}
		break;

	case ROQ_QUAD_INFO:
		if (numQuads == -1)
		{
			readQuadInfo(framedata);
			setupQuad();
		}
		if (numQuads != 1)
		{
			numQuads = 0;
		}
		break;

	case ROQ_PACKET:
		inMemory = roq_flags;
		RoQFrameSize = 0;           // for header
		break;

	case ROQ_QUAD_HANG:
		RoQFrameSize = 0;
		break;

	case ROQ_QUAD_JPEG:
		break;

	default:
		return false;
	}

	//
	// read in next frame data
	//
	if (RoQPlayed >= ROQSize)
	{
		return false;
	}

	framedata += RoQFrameSize;
	roq_id = framedata[0] + framedata[1] * 256;
	RoQFrameSize = framedata[2] + framedata[3] * 256 + framedata[4] * 65536;
	roq_flags = framedata[6] + framedata[7] * 256;
	roqF0 = (char)framedata[7];
	roqF1 = (char)framedata[6];

	if (RoQFrameSize > 65536 || roq_id == 0x1084)
	{
		GLog.DWrite("roq_size>65536||roq_id==0x1084\n");
		return false;
	}

	if (inMemory)
	{
		inMemory--;
		framedata += 8;
		goto redump;
	}

	//
	// one more frame hits the dust
	//
	RoQPlayed += RoQFrameSize + 8;
	return true;
}

//==========================================================================
//
//	QCinematicRoq::Reset
//
//==========================================================================

void QCinematicRoq::Reset()
{
	FS_FCloseFile(iFile);
	FS_FOpenFileRead(fileName, &iFile, true);
	FS_Read(file, 16, iFile);

	init();
}
