//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	Sound caching
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "sound_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte*	data_p;
static byte*	iff_end;
static byte*	last_chunk;
static byte*	iff_data;
static int		iff_chunk_len;

// CODE --------------------------------------------------------------------

//**************************************************************************
//	WAV loading
//**************************************************************************

//==========================================================================
//
//	GetLittleShort
//
//==========================================================================

static short GetLittleShort()
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	data_p += 2;
	return val;
}

//==========================================================================
//
//	GetLittleLong
//
//==========================================================================

static int GetLittleLong()
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p + 1) << 8);
	val = val + (*(data_p + 2) << 16);
	val = val + (*(data_p + 3) << 24);
	data_p += 4;
	return val;
}

//==========================================================================
//
//	FindNextChunk
//
//==========================================================================

static void FindNextChunk(const char *name)
{
	while (1)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{
			// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!String::NCmp((char*)data_p, name, 4))
		{
			return;
		}
	}
}

//==========================================================================
//
//	FindChunk
//
//==========================================================================

static void FindChunk(const char *name)
{
	last_chunk = iff_data;
	FindNextChunk(name);
}

//==========================================================================
//
//	GetWavinfo
//
//==========================================================================

static wavinfo_t GetWavinfo(const char* name, byte* wav, int wavlength)
{
	wavinfo_t	info;

	Com_Memset(&info, 0, sizeof(info));

	if (!wav)
	{
		return info;
	}

	iff_data = wav;
	iff_end = wav + wavlength;

	// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !String::NCmp((char*)data_p + 8, "WAVE", 4)))
	{
		GLog.Write("Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;

	FindChunk("fmt ");
	if (!data_p)
	{
		GLog.Write("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		GLog.Write("Microsoft PCM format only\n");
		return info;
	}

	// get cue chunk
	FindChunk("cue ");
	if (!(GGameType & GAME_Quake3) && data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk("LIST");
		if (data_p)
		{
			if (!String::NCmp((char*)data_p + 28, "mark", 4))
			{
				// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				int i = GetLittleLong();	// samples in loop
				info.samples = info.loopstart + i;
			}
		}
	}
	else
	{
		info.loopstart = -1;
	}

	// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		GLog.Write("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	info.samples = GetLittleLong() / info.width;
	info.dataofs = data_p - wav;

	return info;
}

//==========================================================================
//
//	ResampleSfx
//
//	resample / decimate to the current source rate
//
//==========================================================================

static void ResampleSfx(sfx_t* sfx, int inrate, int inwidth, byte* data)
{
	float stepscale = (float)inrate / dma.speed;	// this is usually 0.5, 1, or 2

	int outcount = (int)(sfx->Length / stepscale);
	sfx->Length = outcount;
	if (sfx->LoopStart != -1)
	{
		sfx->LoopStart = (int)(sfx->LoopStart / stepscale);
	}

	int samplefrac = 0;
	int fracstep = stepscale * 256;
	sfx->Data = new short[outcount];

	for (int i = 0; i < outcount; i++)
	{
		int srcsample = samplefrac >> 8;
		samplefrac += fracstep;
		int sample;
		if (inwidth == 2)
		{
			sample = LittleShort(((short*)data)[srcsample]);
		}
		else
		{
			sample = (int)((unsigned char)(data[srcsample]) - 128) << 8;
		}
		sfx->Data[i] = sample;
	}
}

//==========================================================================
//
//	S_LoadSound
//
//	The filename may be different than sfx->name in the case of a forced
// fallback of a player specific sound
//
//==========================================================================

bool S_LoadSound(sfx_t* sfx)
{
	// player specific sounds are never directly loaded
	if (sfx->Name[0] == '*')
	{
		sfx->DefaultSound = true;
		return false;
	}

	// see if still in memory
	if (sfx->Data)
	{
		sfx->InMemory = true;
		return true;
	}

	// load it in
	char* name = sfx->TrueName ? sfx->TrueName : sfx->Name;

	char namebuffer[MAX_QPATH];
	if (GGameType & GAME_Quake3)
	{
		//	Quake 3 uses full names.
		String::Cpy(namebuffer, name);
	}
	else if ((GGameType & GAME_Quake2) && name[0] == '#')
	{
		//	In Quake 2 sounds prefixed with # are followed by full name.
		String::Cpy(namebuffer, &name[1]);
	}
	else
	{
		//	The rest are prefixed with sound/
		String::Sprintf(namebuffer, sizeof(namebuffer), "sound/%s", name);
	}

	Array<byte> data;
	int size = FS_ReadFile(namebuffer, data);
	if (size <= 0)
	{
		GLog.DWrite("Couldn't load %s\n", namebuffer);
		sfx->DefaultSound = true;
		return false;
	}

	wavinfo_t info = GetWavinfo(sfx->Name, data.Ptr(), size);
	if (info.channels != 1)
	{
		GLog.Write("%s is a stereo wav file\n", sfx->Name);
		sfx->DefaultSound = true;
		return false;
	}

	sfx->LastTimeUsed = Com_Milliseconds() + 1;
	sfx->Length = info.samples;
	sfx->LoopStart = info.loopstart;

	ResampleSfx(sfx, info.rate, info.width, &data[info.dataofs]);

	sfx->InMemory = true;
	return true;
}
