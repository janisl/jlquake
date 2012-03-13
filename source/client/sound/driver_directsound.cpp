//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../client.h"
#include "local.h"
#include "../windows_shared.h"
#include <dsound.h>

// MACROS ------------------------------------------------------------------

#define SECONDARY_BUFFER_SIZE	0x10000

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static bool					dsound_init;
static int					sample16;
static DWORD				gSndBufSize;
static DWORD				locksize;
static LPDIRECTSOUND		pDS;
static LPDIRECTSOUNDBUFFER	pDSBuf;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	DSoundError
//
//==========================================================================

static const char* DSoundError(int error)
{
	switch (error)
	{
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}
	return "unknown";
}

//==========================================================================
//
//	SNDDMA_Init
//
//	Initialize direct sound
//	Returns false if failed
//
//==========================================================================

bool SNDDMA_Init()
{
	HRESULT			hresult;
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	format;
	bool			use8;

	Com_Memset((void *)&dma, 0, sizeof (dma));
	dsound_init = false;

	CoInitialize(NULL);

	Log::write("Initializing DirectSound\n");

	use8 = 1;
    // Create IDirectSound using the primary sound device
    if (FAILED(hresult = CoCreateInstance(CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void**)&pDS)))
	{
		use8 = 0;
	    if (FAILED(hresult = CoCreateInstance(CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound, (void**)&pDS)))
		{
			Log::write("failed\n");
			SNDDMA_Shutdown();
			return false;
		}
	}

	hresult = pDS->Initialize(NULL);

	Log::develWrite("ok\n");

	Log::develWrite("...setting DSSCL_PRIORITY coop level: ");

	if (DS_OK != pDS->SetCooperativeLevel(GMainWindow, DSSCL_PRIORITY))
	{
		Log::write("failed\n");
		SNDDMA_Shutdown();
		return false;
	}
	Log::develWrite("ok\n");

	// create the secondary buffer we'll actually work with
	dma.channels = s_channels_cv->integer;
	dma.samplebits = s_bits->integer;

	if (s_khz->integer == 44)
	{
		dma.speed = 44100;
	}
	else if (s_khz->integer == 22)
	{
		dma.speed = 22050;
	}
	else
	{
		dma.speed = 11025;
	}

	Com_Memset(&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = dma.channels;
    format.wBitsPerSample = dma.samplebits;
    format.nSamplesPerSec = dma.speed;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.cbSize = 0;
    format.nAvgBytesPerSec = format.nSamplesPerSec*format.nBlockAlign; 

	Com_Memset(&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(DSBUFFERDESC);

	// Micah: take advantage of 2D hardware.if available.
	dsbuf.dwFlags = DSBCAPS_LOCHARDWARE;
	if (use8)
	{
		dsbuf.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
	}
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	dsbuf.lpwfxFormat = &format;
	
	Com_Memset(&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	
	Log::develWrite("...creating secondary buffer: ");
	if (DS_OK == pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL))
	{
		Log::write("locked hardware.  ok\n");
	}
	else
	{
		// Couldn't get hardware, fallback to software.
		dsbuf.dwFlags = DSBCAPS_LOCSOFTWARE;
		if (use8)
		{
			dsbuf.dwFlags |= DSBCAPS_GETCURRENTPOSITION2;
		}
		if (DS_OK != pDS->CreateSoundBuffer(&dsbuf, &pDSBuf, NULL))
		{
			Log::write( "failed\n" );
			SNDDMA_Shutdown();
			return false;
		}
		Log::develWrite("forced to software.  ok\n");
	}
		
	// Make sure mixer is active
	if (DS_OK != pDSBuf->Play(0, 0, DSBPLAY_LOOPING))
	{
		Log::write("*** Looped sound play failed ***\n");
		SNDDMA_Shutdown();
		return false;
	}

	// get the returned buffer size
	if (DS_OK != pDSBuf->GetCaps(&dsbcaps))
	{
		Log::write("*** GetCaps failed ***\n");
		SNDDMA_Shutdown();
		return false;
	}

	gSndBufSize = dsbcaps.dwBufferBytes;

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;
	dma.samples = gSndBufSize/(dma.samplebits/8);
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = NULL;			// must be locked first

	sample16 = (dma.samplebits/8) - 1;

	SNDDMA_BeginPainting();
	if (dma.buffer)
	{
		int clear = dma.samplebits == 8 ? 0x80: 0;
		Com_Memset(dma.buffer, clear, dma.samples * dma.samplebits / 8);
	}
	SNDDMA_Submit();

	dsound_init = true;

	Log::develWrite("Completed successfully\n");

    return true;
}

//==========================================================================
//
//	SNDDMA_Shutdown
//
//==========================================================================

void SNDDMA_Shutdown()
{
	Log::develWrite("Shutting down sound system\n");

	if (pDS)
	{
		if (pDSBuf)
		{
			Log::develWrite("...stopping and releasing sound buffer\n");
			pDSBuf->Stop();
			pDSBuf->Release();
		}

		Log::develWrite("...releasing DS object\n");
		pDS->Release();
	}

	pDS = NULL;
	pDSBuf = NULL;
	dsound_init = false;
	Com_Memset((void*)&dma, 0, sizeof(dma));
	CoUninitialize();
}

//==========================================================================
//
//	SNDDMA_GetDMAPos
//
//	Return the current sample position (in mono samples read) inside the
// recirculating dma buffer, so the mixing code will know how many sample
// are required to fill it up.
//
//==========================================================================

int SNDDMA_GetDMAPos()
{
	MMTIME	mmtime;
	int		s;
	DWORD	dwWrite;

	if (!dsound_init)
	{
		return 0;
	}

	mmtime.wType = TIME_SAMPLES;
	pDSBuf->GetCurrentPosition(&mmtime.u.sample, &dwWrite);

	s = mmtime.u.sample;

	s >>= sample16;

	s &= (dma.samples-1);

	return s;
}

//==========================================================================
//
//	SNDDMA_BeginPainting
//
//	Makes sure dma.buffer is valid
//
//==========================================================================

void SNDDMA_BeginPainting()
{
	int		reps;
	DWORD	dwSize2;
	DWORD	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if (!pDSBuf)
	{
		return;
	}

	// if the buffer was lost or stopped, restore it and/or restart it
	if (pDSBuf->GetStatus(&dwStatus) != DS_OK)
	{
		Log::write("Couldn't get sound buffer status\n");
	}
	
	if (dwStatus & DSBSTATUS_BUFFERLOST)
		pDSBuf->Restore();
	
	if (!(dwStatus & DSBSTATUS_PLAYING))
		pDSBuf->Play(0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;

	while ((hresult = pDSBuf->Lock(0, gSndBufSize, (void**)&pbuf, &locksize, 
								   (void**)&pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			Log::write("SNDDMA_BeginPainting: Lock failed with error '%s'\n", DSoundError(hresult));
			SNDDMA_Shutdown();
			return;
		}
		else
		{
			pDSBuf->Restore();
		}

		if (++reps > 2)
			return;
	}
	dma.buffer = (byte*)pbuf;
}

//==========================================================================
//
//	SNDDMA_Submit
//
//	Send sound to device if buffer isn't really the dma buffer
//	Also unlocks the dsound buffer
//
//==========================================================================

void SNDDMA_Submit()
{
    // unlock the dsound buffer
	if (pDSBuf)
	{
		pDSBuf->Unlock(dma.buffer, locksize, NULL, 0);
	}
}

//==========================================================================
//
//	SNDDMA_Activate
//
//	When we change windows we need to do this
//
//==========================================================================

void SNDDMA_Activate()
{
	if (!pDS)
	{
		return;
	}

	if (DS_OK != pDS->SetCooperativeLevel(GMainWindow, DSSCL_PRIORITY))
	{
		Log::write("sound SetCooperativeLevel failed\n");
		SNDDMA_Shutdown();
	}
}
