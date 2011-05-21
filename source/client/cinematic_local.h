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

class QCinematic : public QInterface
{
public:
	byte*		buf;

	QCinematic()
	: buf(NULL)
	{}
	~QCinematic();
};

class QCinematicCin : public QCinematic
{
private:
	struct cblock_t
	{
		byte*	data;
		int		count;
	};

	fileHandle_t	cinematic_file;

	int		cinematicframe;

	byte	cinematicpalette[768];
	byte*	pic;
	byte*	pic_pending;
	byte*	pic32;

	int		h_used[512];
	int		h_count[512];

	// order 1 huffman stuff
	int*	hnodes1;	// [256][256][2];
	int		numhnodes1[256];

	void Huff1TableInit();
	int SmallestNode1(int numhnodes);
	cblock_t Huff1Decompress(cblock_t in);

public:
	int		s_rate;
	int		s_width;
	int		s_channels;

	int		width;
	int		height;

	QCinematicCin()
	: cinematic_file(0)
	, cinematicframe(0)
	, pic(NULL)
	, pic_pending(NULL)
	, pic32(NULL)
	, hnodes1(NULL)
	{}
	~QCinematicCin();
	bool Open(const char* FileName);
	bool Update(int NewTime);
	byte* ReadNextFrame();
	int GetCinematicTime() const
	{
		return cinematicframe * 1000 / 14;
	}
};

class QCinematicPcx : public QCinematic
{
public:
	int		width;
	int		height;

	QCinematicPcx()
	{}
	~QCinematicPcx();
	bool Open(const char* FileName);
};

#define DEFAULT_CIN_WIDTH	512
#define DEFAULT_CIN_HEIGHT	512

class QCinematicRoq : public QCinematic
{
private:
	void recurseQuad(long startX, long startY, long quadSize);

public:
	long				samplesPerLine;
	unsigned int		xsize;
	unsigned int		ysize;
	long				normalBuffer0;
	long				screenDelta;
	int					CIN_WIDTH;
	int					CIN_HEIGHT;
	long				onQuad;
	unsigned int		maxsize;
	unsigned int		minsize;
	long				t[2];

	byte				file[65536];
	byte				linbuf[DEFAULT_CIN_WIDTH * DEFAULT_CIN_HEIGHT * 4 * 2];
	byte*				qStatus[2][32768];

	QCinematicRoq()
	: samplesPerLine(0)
	, xsize(0)
	, ysize(0)
	, normalBuffer0(0)
	, screenDelta(0)
	, CIN_WIDTH(0)
	, CIN_HEIGHT(0)
	, onQuad(0)
	, maxsize(0)
	, minsize(0)
	{}
	void readQuadInfo(byte* qData);
	void setupQuad();
	void RoQPrepMcomp(long xoff, long yoff);
	void blitVQQuad32fs(byte** status, unsigned char* data);
};

void initRoQ();
long RllDecodeMonoToStereo(unsigned char* from, short* to, unsigned int size, char signedOutput, unsigned short flag);
long RllDecodeStereoToStereo(unsigned char* from, short* to, unsigned int size, char signedOutput, unsigned short flag);
void decodeCodeBook(byte* input, unsigned short roq_flags);
