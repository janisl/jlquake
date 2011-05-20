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
	, pic(NULL)
	, pic_pending(NULL)
	, pic32(NULL)
	, cinematicframe(0)
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

class QCinematicRoq : public QCinematic
{
};
