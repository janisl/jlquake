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

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "cinematic_local.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QCinematicCin::~QCinematicCin
//
//==========================================================================

QCinematicCin::~QCinematicCin()
{
	if (pic)
	{
		delete[] pic;
		pic = NULL;
	}
	if (pic_pending)
	{
		delete[] pic_pending;
		pic_pending = NULL;
	}
	if (pic32)
	{
		delete[] pic32;
		pic32 = NULL;
	}
	if (cinematic_file)
	{
		FS_FCloseFile(cinematic_file);
		cinematic_file = 0;
	}
	if (hnodes1)
	{
		delete[] hnodes1;
		hnodes1 = NULL;
	}
}

//==========================================================================
//
//	QCinematicCin::Open
//
//==========================================================================

bool QCinematicCin::Open(const char* FileName)
{
	String::Cpy(Name, FileName);
	FS_FOpenFileRead(FileName, &cinematic_file, true);
	if (!cinematic_file)
	{
		return false;
	}

	FS_Read(&Width, 4, cinematic_file);
	FS_Read(&Height, 4, cinematic_file);
	Width = LittleLong(Width);
	Height = LittleLong(Height);

	FS_Read(&s_rate, 4, cinematic_file);
	s_rate = LittleLong(s_rate);
	FS_Read(&s_width, 4, cinematic_file);
	s_width = LittleLong(s_width);
	FS_Read(&s_channels, 4, cinematic_file);
	s_channels = LittleLong(s_channels);

	Huff1TableInit();

	pic32 = new byte[Width * Height * 4];

	//	Initialise palette to game palette
	for (int i = 0; i < 256; i++)
	{
		cinematicpalette[i * 3 + 0] = r_palette[i][0];
		cinematicpalette[i * 3 + 1] = r_palette[i][1];
		cinematicpalette[i * 3 + 2] = r_palette[i][2];
	}

	cinematicframe = 0;
	pic = ReadNextFrame();
	return true;
}

//==========================================================================
//
//	QCinematicCin::Huff1TableInit
//
//	Reads the 64k counts table and initializes the node trees
//
//==========================================================================

void QCinematicCin::Huff1TableInit()
{
	hnodes1 = new int[256 * 256 * 2];
	Com_Memset(hnodes1, 0, 256 * 256 * 2 * 4);

	for (int prev = 0; prev < 256; prev++)
	{
		Com_Memset(h_count, 0, sizeof(h_count));
		Com_Memset(h_used, 0, sizeof(h_used));

		// read a row of counts
		byte counts[256];
		FS_Read(counts, sizeof(counts), cinematic_file);
		for (int j = 0; j < 256; j++)
		{
			h_count[j] = counts[j];
		}

		// build the nodes
		int numhnodes = 256;
		int* nodebase = hnodes1 + prev * 256 * 2;

		while (numhnodes != 511)
		{
			int* node = nodebase + (numhnodes - 256) * 2;

			// pick two lowest counts
			node[0] = SmallestNode1(numhnodes);
			if (node[0] == -1)
			{
				break;	// no more
			}

			node[1] = SmallestNode1(numhnodes);
			if (node[1] == -1)
			{
				break;
			}

			h_count[numhnodes] = h_count[node[0]] + h_count[node[1]];
			numhnodes++;
		}

		numhnodes1[prev] = numhnodes - 1;
	}
}

//==========================================================================
//
//	QCinematicCin::SmallestNode1
//
//==========================================================================

int QCinematicCin::SmallestNode1(int numhnodes)
{
	int best = 99999999;
	int bestnode = -1;
	for (int i = 0; i < numhnodes; i++)
	{
		if (h_used[i])
		{
			continue;
		}
		if (!h_count[i])
		{
			continue;
		}
		if (h_count[i] < best)
		{
			best = h_count[i];
			bestnode = i;
		}
	}

	if (bestnode == -1)
	{
		return -1;
	}

	h_used[bestnode] = true;
	return bestnode;
}

//==========================================================================
//
//	QCinematicCin::Update
//
//==========================================================================

bool QCinematicCin::Update(int NewTime)
{
	int frame = NewTime * 14 / 1000;
	if (frame <= cinematicframe)
	{
		return true;
	}
	do
	{
		if (pic)
		{
			delete[] pic;
		}
		pic = pic_pending;
		pic_pending = NULL;
		if (pic)
		{
			for (int i = 0; i < Width * Height; i++)
			{
				pic32[i * 4 + 0] = cinematicpalette[pic[i] * 3 + 0];
				pic32[i * 4 + 1] = cinematicpalette[pic[i] * 3 + 1];
				pic32[i * 4 + 2] = cinematicpalette[pic[i] * 3 + 2];
				pic32[i * 4 + 3] = 255;
			}
			OutputFrame = pic32;
		}
		Dirty = true;
		pic_pending = ReadNextFrame();
	}
	while (pic_pending && frame > cinematicframe);

	return !!pic_pending;
}

//==========================================================================
//
//	QCinematicCin::ReadNextFrame
//
//==========================================================================

byte* QCinematicCin::ReadNextFrame()
{
	// read the next frame
	int command;
	int r = FS_Read(&command, 4, cinematic_file);

	if (r != 4)
	{
		return NULL;
	}
	command = LittleLong(command);
	if (command == 2)
	{
		return NULL;	// last frame marker
	}

	if (command == 1)
	{
		// read palette
		FS_Read(cinematicpalette, sizeof(cinematicpalette), cinematic_file);
	}

	// decompress the next frame
	int size;
	FS_Read(&size, 4, cinematic_file);
	size = LittleLong(size);
	byte compressed[0x20000];
	if (size > (int)sizeof(compressed) || size < 1)
	{
		throw QDropException("Bad compressed frame size");
	}
	FS_Read (compressed, size, cinematic_file);

	// read sound
	int start = cinematicframe * s_rate / 14;
	int end = (cinematicframe + 1) * s_rate / 14;
	int count = end - start;

	byte samples[22050 / 14 * 4];
	FS_Read(samples, count * s_width * s_channels, cinematic_file);

	S_ByteSwapRawSamples(count, s_width, s_channels, samples);

	S_RawSamples(count, s_rate, s_width, s_channels, samples, 1.0);

	cblock_t in;
	in.data = compressed;
	in.count = size;

	cblock_t huf1 = Huff1Decompress(in);

	byte* pic = huf1.data;

	cinematicframe++;

	return pic;
}

//==========================================================================
//
//	QCinematicCin::SmallestNode1
//
//==========================================================================

QCinematicCin::cblock_t QCinematicCin::Huff1Decompress(QCinematicCin::cblock_t in)
{
	cblock_t out;

	// get decompressed count
	int count = in.data[0] + (in.data[1] << 8) + (in.data[2] << 16) + (in.data[3] << 24);
	byte* input = in.data + 4;
	byte* out_p = out.data = new byte[count];

	// read bits

	int* hnodesbase = hnodes1 - 256 * 2;	// nodes 0-255 aren't stored

	int* hnodes = hnodesbase;
	int nodenum = numhnodes1[0];
	while (count)
	{
		int inbyte = *input++;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
		//-----------
		if (nodenum < 256)
		{
			hnodes = hnodesbase + (nodenum << 9);
			*out_p++ = nodenum;
			if (!--count)
			{
				break;
			}
			nodenum = numhnodes1[nodenum];
		}
		nodenum = hnodes[nodenum * 2 + (inbyte & 1)];
		inbyte >>= 1;
	}

	if (input - in.data != in.count && input - in.data != in.count + 1)
	{
		GLog.WriteLine("Decompression overread by %i", (input - in.data) - in.count);
	}
	out.count = out_p - out.data;

	return out;
}

//==========================================================================
//
//	QCinematicCin::GetCinematicTime
//
//==========================================================================

int QCinematicCin::GetCinematicTime() const
{
	return cinematicframe * 1000 / 14;
}

//==========================================================================
//
//	QCinematicCin::Reset
//
//==========================================================================

void QCinematicCin::Reset()
{
	if (pic)
	{
		delete[] pic;
		pic = NULL;
	}
	if (pic_pending)
	{
		delete[] pic_pending;
		pic_pending = NULL;
	}
	if (pic32)
	{
		delete[] pic32;
		pic32 = NULL;
	}
	if (cinematic_file)
	{
		FS_FCloseFile(cinematic_file);
		cinematic_file = 0;
	}
	if (hnodes1)
	{
		delete[] hnodes1;
		hnodes1 = NULL;
	}

	Open(Name);
}
