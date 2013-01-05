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
//**
//**	Sound caching
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "local.h"
#include "../../common/Common.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/system.h"
#include "../../common/event_queue.h"
#include "../../common/endian.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct waveFormat_t
{
	const char* name;
	int format;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static byte* data_p;
static byte* iff_end;
static byte* last_chunk;
static byte* iff_data;
static int iff_chunk_len;

static waveFormat_t waveFormats[] = {
	{ "Windows PCM", 1 },
	{ "Antex ADPCM", 14 },
	{ "Antex ADPCME", 33 },
	{ "Antex ADPCM", 40 },
	{ "Audio Processing Technology", 25 },
	{ "Audiofile, Inc.", 24 },
	{ "Audiofile, Inc.", 26 },
	{ "Control Resources Limited", 34 },
	{ "Control Resources Limited", 37 },
	{ "Creative ADPCM", 200 },
	{ "Dolby Laboratories", 30 },
	{ "DSP Group, Inc", 22 },
	{ "DSP Solutions, Inc.", 15 },
	{ "DSP Solutions, Inc.", 16 },
	{ "DSP Solutions, Inc.", 35 },
	{ "DSP Solutions ADPCM", 36 },
	{ "Echo Speech Corporation", 23 },
	{ "Fujitsu Corp.", 300 },
	{ "IBM Corporation", 5 },
	{ "Ing C. Olivetti & C., S.p.A.", 1000 },
	{ "Ing C. Olivetti & C., S.p.A.", 1001 },
	{ "Ing C. Olivetti & C., S.p.A.", 1002 },
	{ "Ing C. Olivetti & C., S.p.A.", 1003 },
	{ "Ing C. Olivetti & C., S.p.A.", 1004 },
	{ "Intel ADPCM", 11 },
	{ "Intel ADPCM", 11 },
	{ "Unknown", 0 },
	{ "Microsoft ADPCM", 2 },
	{ "Microsoft Corporation", 6 },
	{ "Microsoft Corporation", 7 },
	{ "Microsoft Corporation", 31 },
	{ "Microsoft Corporation", 50 },
	{ "Natural MicroSystems ADPCM", 38 },
	{ "OKI ADPCM", 10 },
	{ "Sierra ADPCM", 13 },
	{ "Speech Compression", 21 },
	{ "Videologic ADPCM", 12 },
	{ "Yamaha ADPCM", 20 },
	{ NULL, 0 }
};

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

static void FindNextChunk(const char* name)
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
		last_chunk = data_p + 8 + ((iff_chunk_len + 1) & ~1);
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

static void FindChunk(const char* name)
{
	last_chunk = iff_data;
	FindNextChunk(name);
}

static const char* GetWaveFormatName(const int format)
{
	for (int i = 0; waveFormats[i].name; i++)
	{
		if (format == waveFormats[i].format)
		{
			return waveFormats[i].name;
		}
	}
	return "Unknown";

}

//==========================================================================
//
//	GetWavinfo
//
//==========================================================================

static wavinfo_t GetWavinfo(const char* name, byte* wav, int wavlength)
{
	wavinfo_t info;

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
		common->Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

	// get "fmt " chunk
	iff_data = data_p + 12;

	FindChunk("fmt ");
	if (!data_p)
	{
		common->Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	info.format = GetLittleShort();
	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4 + 2;
	info.width = GetLittleShort() / 8;

	if (info.format != 1)
	{
		common->Printf("Unsupported format: %s\n", GetWaveFormatName(info.format));
		common->Printf("Microsoft PCM format only\n");
		return info;
	}

	// get cue chunk
	FindChunk("cue ");
	if (!(GGameType & GAME_Tech3) && data_p)
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
		common->Printf("Missing data chunk\n");
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
	if (GGameType & GAME_Tech3)
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
		common->DPrintf("Couldn't load %s\n", namebuffer);
		sfx->DefaultSound = true;
		return false;
	}

	wavinfo_t info = GetWavinfo(sfx->Name, data.Ptr(), size);
	if (info.channels != 1)
	{
		common->Printf("%s is a stereo wav file\n", sfx->Name);
		sfx->DefaultSound = true;
		return false;
	}

	if (GGameType & (GAME_WolfSP | GAME_WolfMP | GAME_ET))
	{
		sfx->LastTimeUsed = Sys_Milliseconds() + 1;
	}
	else
	{
		sfx->LastTimeUsed = Com_Milliseconds() + 1;
	}
	sfx->Length = info.samples;
	sfx->LoopStart = info.loopstart;

	ResampleSfx(sfx, info.rate, info.width, &data[info.dataofs]);

	sfx->InMemory = true;
	return true;
}
