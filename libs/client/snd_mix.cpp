//**************************************************************************
//**
//**	$Id$
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
//**
//**	Portable code to mix sounds for snd_dma.cpp
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "snd_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

portable_samplepair_t	paintbuffer[PAINTBUFFER_SIZE];
extern "C"
{
int*		snd_p;
int			snd_linear_count;
int			snd_vol;
short*		snd_out;
}

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	S_WriteLinearBlastStereo16
//
//==========================================================================

#if id386 && defined _MSC_VER
static __declspec(naked) void S_WriteLinearBlastStereo16()
{
	__asm {

 push edi
 push ebx
 mov ecx,ds:dword ptr[snd_linear_count]
 mov ebx,ds:dword ptr[snd_p]
 mov edi,ds:dword ptr[snd_out]
LWLBLoopTop:
 mov eax,ds:dword ptr[-8+ebx+ecx*4]
 sar eax,8
 cmp eax,07FFFh
 jg LClampHigh
 cmp eax,0FFFF8000h
 jnl LClampDone
 mov eax,0FFFF8000h
 jmp LClampDone
LClampHigh:
 mov eax,07FFFh
LClampDone:
 mov edx,ds:dword ptr[-4+ebx+ecx*4]
 sar edx,8
 cmp edx,07FFFh
 jg LClampHigh2
 cmp edx,0FFFF8000h
 jnl LClampDone2
 mov edx,0FFFF8000h
 jmp LClampDone2
LClampHigh2:
 mov edx,07FFFh
LClampDone2:
 shl edx,16
 and eax,0FFFFh
 or edx,eax
 mov ds:dword ptr[-4+edi+ecx*2],edx
 sub ecx,2
 jnz LWLBLoopTop
 pop ebx
 pop edi
 ret
	}
}
#elif id386 && defined __GNUC__
// forward declare, implementation somewhere else
extern "C" void S_WriteLinearBlastStereo16();
#else
static void S_WriteLinearBlastStereo16()
{
	for (int i = 0; i < snd_linear_count; i+=2)
	{
		int val = snd_p[i] >> 8;
		if (val > 0x7fff)
		{
			snd_out[i] = 0x7fff;
		}
		else if (val < -32768)
		{
			snd_out[i] = -32768;
		}
		else
		{
			snd_out[i] = val;
		}

		val = snd_p[i + 1] >> 8;
		if (val > 0x7fff)
		{
			snd_out[i + 1] = 0x7fff;
		}
		else if (val < -32768)
		{
			snd_out[i + 1] = -32768;
		}
		else
		{
			snd_out[i + 1] = val;
		}
	}
}
#endif

//==========================================================================
//
//	S_TransferStereo16
//
//==========================================================================

static void S_TransferStereo16(int endtime)
{
	snd_p = (int*)paintbuffer;
	int lpaintedtime = s_paintedtime;

	while (lpaintedtime < endtime)
	{
		// handle recirculating buffer issues
		int lpos = lpaintedtime & ((dma.samples >> 1) - 1);

		snd_out = (short*)dma.buffer + (lpos << 1);

		snd_linear_count = (dma.samples >> 1) - lpos;
		if (lpaintedtime + snd_linear_count > endtime)
		{
			snd_linear_count = endtime - lpaintedtime;
		}

		snd_linear_count <<= 1;

		// write a linear blast of samples
		S_WriteLinearBlastStereo16();

		snd_p += snd_linear_count;
		lpaintedtime += (snd_linear_count >> 1);
	}
}

//==========================================================================
//
//	S_TransferPaintBuffer
//
//==========================================================================

static void S_TransferPaintBuffer(int endtime)
{
	if (s_testsound->integer)
	{
		// write a fixed sine wave
		int count = endtime - s_paintedtime;
		for (int i = 0; i < count; i++)
		{
			paintbuffer[i].left = paintbuffer[i].right =
				sin((s_paintedtime + i) * 0.1) * 20000 * 256;
		}
	}

	if (dma.samplebits == 16 && dma.channels == 2)
	{
		// optimized case
		S_TransferStereo16(endtime);
	}
	else
	{
		// general case
		int* p = (int*)paintbuffer;
		int count = (endtime - s_paintedtime) * dma.channels;
		int out_mask = dma.samples - 1; 
		int out_idx = s_paintedtime * dma.channels & out_mask;
		int step = 3 - dma.channels;

		if (dma.samplebits == 16)
		{
			short *out = (short*)dma.buffer;
			while (count--)
			{
				int val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
				{
					val = 0x7fff;
				}
				else if (val < -32768)
				{
					val = -32768;
				}
				out[out_idx] = val;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
		else if (dma.samplebits == 8)
		{
			byte* out = dma.buffer;
			while (count--)
			{
				int val = *p >> 8;
				p+= step;
				if (val > 0x7fff)
				{
					val = 0x7fff;
				}
				else if (val < -32768)
				{
					val = -32768;
				}
				out[out_idx] = (val >> 8) + 128;
				out_idx = (out_idx + 1) & out_mask;
			}
		}
	}
}

//**************************************************************************
//
//	CHANNEL MIXING
//
//**************************************************************************

//==========================================================================
//
//	S_PaintChannelFrom16
//
//==========================================================================

static void S_PaintChannelFrom16(channel_t *ch, const sfx_t* sc, int count,
	int sampleOffset, int bufferOffset)
{
	portable_samplepair_t* samp = &paintbuffer[bufferOffset];

	if (!ch->doppler || ch->dopplerScale == 1.0f)
	{
		int leftvol = ch->leftvol * snd_vol;
		int rightvol = ch->rightvol * snd_vol;
		short* samples = sc->Data;
		for (int i = 0; i < count; i++)
		{
			int data  = samples[sampleOffset++];
			samp[i].left += (data * leftvol) >> 8;
			samp[i].right += (data * rightvol) >> 8;
		}
	}
	else
	{
		sampleOffset = sampleOffset * ch->oldDopplerScale;

		float fleftvol = ch->leftvol * snd_vol;
		float frightvol = ch->rightvol * snd_vol;

		float ooff = sampleOffset;
		short* samples = sc->Data;

		for (int i = 0; i < count; i++)
		{
			int aoff = ooff;
			ooff = ooff + ch->dopplerScale;
			int boff = ooff;
			float fdata = 0;
			for (int j = aoff; j < boff; j++)
			{
				fdata  += samples[j];
			}
			float fdiv = 256 * (boff - aoff);
			samp[i].left += (fdata * fleftvol) / fdiv;
			samp[i].right += (fdata * frightvol) / fdiv;
		}
	}
}

//==========================================================================
//
//	S_PaintChannels
//
//==========================================================================

void S_PaintChannels(int endtime)
{
	int 	i;
	int 	end;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;

	snd_vol = s_volume->value * 255;

//Com_Printf ("%i to %i\n", s_paintedtime, endtime);
	while (s_paintedtime < endtime)
	{
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if (endtime - s_paintedtime > PAINTBUFFER_SIZE)
		{
			end = s_paintedtime + PAINTBUFFER_SIZE;
		}

		// start any playsounds
		while (1)
		{
			playsound_t* ps = s_pendingplays.next;
			if (ps == &s_pendingplays)
			{
				break;	// no more pending sounds
			}
			if (ps->begin <= s_paintedtime)
			{
				S_IssuePlaysound(ps);
				continue;
			}
			if (ps->begin < end)
			{
				end = ps->begin;		// stop here
			}
			break;
		}

		// clear the paint buffer to either music or zeros
		if (s_rawend < s_paintedtime)
		{
			if (s_rawend)
			{
				//Com_DPrintf ("background sound underrun\n");
			}
			Com_Memset(paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
		}
		else
		{
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < s_rawend) ? end : s_rawend;

			for ( i = s_paintedtime ; i < stop ; i++ )
			{
				s = i&(MAX_RAW_SAMPLES-1);
				paintbuffer[i-s_paintedtime] = s_rawsamples[s];
			}
//		if (i != end)
//			Com_Printf ("partial stream\n");
//		else
//			Com_Printf ("full stream\n");
			for ( ; i < end ; i++ )
			{
				paintbuffer[i-s_paintedtime].left =
				paintbuffer[i-s_paintedtime].right = 0;
			}
		}

		// paint in the channels.
		ch = s_channels;
		for (i = 0; i < MAX_CHANNELS; i++, ch++)
		{
			if (!ch->sfx || (!ch->leftvol && !ch->rightvol))
			//if (!ch->sfx || (ch->leftvol < 0.25 && ch->rightvol < 0.25))
			{
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->sfx;

			while (ltime < end)
			{
				sampleOffset = ltime - ch->startSample;
				count = end - ltime;
				if (sampleOffset + count > sc->Length)
				{
					count = sc->Length - sampleOffset;
				}

				if (count > 0)
				{
					S_PaintChannelFrom16(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					ltime += count;
				}

				// if at end of loop, restart
				if (ltime >= ch->startSample + ch->sfx->Length)
				{
					if (ch->sfx->LoopStart >= 0)
					{
						ch->startSample = ltime - ch->sfx->LoopStart;
					}
					else
					{
						// channel just stopped
						ch->sfx = NULL;
						break;
					}
				}
			}
		}

		// paint in the looped channels.
		ch = loop_channels;
		for (i = 0; i < numLoopChannels; i++, ch++)
		{
			if (!ch->sfx || (!ch->leftvol && !ch->rightvol))
			{
				continue;
			}

			ltime = s_paintedtime;
			sc = ch->sfx;

			if (sc->Data == NULL || sc->Length == 0)
			{
				continue;
			}
			// we might have to make two passes if it
			// is a looping sound effect and the end of
			// the sample is hit
			do
			{
				sampleOffset = (ltime % sc->Length);

				count = end - ltime;
				if (sampleOffset + count > sc->Length)
				{
					count = sc->Length - sampleOffset;
				}

				if (count > 0)
				{
					S_PaintChannelFrom16(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					ltime += count;
				}
			}
			while (ltime < end);
		}

		// transfer out according to DMA format
		S_TransferPaintBuffer(end);
		s_paintedtime = end;
	}
}
