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

#define CIN_STREAM  0	//	Const for the sound stream used for cinematics

//
//	Base class for cinematics.
//
class QCinematic : public Interface
{
public:
	char Name[MAX_OSPATH];
	int Width;
	int Height;
	byte* OutputFrame;
	bool Dirty;
	bool Silent;

	QCinematic()
	: Width(0)
	, Height(0)
	, OutputFrame(NULL)
	, Dirty(false)
	, Silent(false)
	{
		Name[0] = 0;
	}
	virtual bool Open(const char* FileName) = 0;
	virtual bool Update(int NewTime) = 0;
	virtual int GetCinematicTime() const = 0;
	virtual void Reset() = 0;
};

//
//	Quake 2 CIN file.
//
class QCinematicCin : public QCinematic
{
private:
	struct cblock_t
	{
		byte* data;
		int count;
	};

	fileHandle_t cinematic_file;

	int cinematicframe;

	byte cinematicpalette[768];
	byte* pic;
	byte* pic_pending;
	byte* pic32;

	int s_rate;
	int s_width;
	int s_channels;

	int h_used[512];
	int h_count[512];

	// order 1 huffman stuff
	int* hnodes1;		// [256][256][2];
	int numhnodes1[256];

	void Huff1TableInit();
	int SmallestNode1(int numhnodes);
	byte* ReadNextFrame();
	cblock_t Huff1Decompress(cblock_t in);

public:
	QCinematicCin()
	: cinematic_file(0)
	, cinematicframe(0)
	, pic(NULL)
	, pic_pending(NULL)
	, pic32(NULL)
	, s_rate(0)
	, s_width(0)
	, s_channels(0)
	, hnodes1(NULL)
	{}
	~QCinematicCin();
	bool Open(const char* FileName);
	bool Update(int NewTime);
	int GetCinematicTime() const;
	void Reset();
};

//
//	Quake 2 static PCX image.
//
class QCinematicPcx : public QCinematic
{
public:
	QCinematicPcx()
	{}
	~QCinematicPcx();
	bool Open(const char* FileName);
	bool Update(int NewTime);
	int GetCinematicTime() const;
	void Reset();
};

//
//	Quake 3 RoQ file.
//
class QCinematicRoq : public QCinematic
{
private:
	enum
	{
		DEFAULT_CIN_WIDTH = 512,
		DEFAULT_CIN_HEIGHT = 512,
	};

	long samplesPerLine;
	long normalBuffer0;
	long screenDelta;
	long onQuad;
	unsigned int maxsize;
	unsigned int minsize;
	long t[2];
	long ROQSize;
	long RoQPlayed;
	unsigned int RoQFrameSize;
	unsigned int roq_id;
	long roq_flags;
	long roqF0;
	long roqF1;
	int inMemory;
	long roqFPS;
	fileHandle_t iFile;
	long numQuads;
	bool sound;

	byte file[65536];

	byte linbuf[DEFAULT_CIN_WIDTH * DEFAULT_CIN_HEIGHT * 4 * 2];
	byte* qStatus[2][32768];

	unsigned short vq2[256 * 16 * 4];
	unsigned short vq4[256 * 64 * 4];
	unsigned short vq8[256 * 256 * 4];

	int mcomp[256];

	void init();
	void recurseQuad(long startX, long startY, long quadSize);
	void readQuadInfo(byte* qData);
	void setupQuad();
	void RoQPrepMcomp(long xoff, long yoff);
	void blitVQQuad32fs(byte** status, unsigned char* data);
	void decodeCodeBook(byte* input);
	bool ReadFrame();

public:
	QCinematicRoq()
	: samplesPerLine(0)
	, normalBuffer0(0)
	, screenDelta(0)
	, onQuad(0)
	, maxsize(0)
	, minsize(0)
	, ROQSize(0)
	, RoQPlayed(0)
	, RoQFrameSize(0)
	, roq_id(0)
	, roq_flags(0)
	, roqF0(0)
	, roqF1(0)
	, inMemory(0)
	, roqFPS(0)
	, iFile(0)
	, numQuads(0)
	, sound(false)
	{}
	~QCinematicRoq();
	bool Open(const char* FileName);
	bool Update(int NewTime);
	int GetCinematicTime() const;
	void Reset();
};

//
//	Handles playback of cinematic
//
class QCinematicPlayer
{
public:
	QCinematic* Cin;
	int XPos;
	int YPos;
	int Width;
	int Height;
	bool AlterGameState;
	bool Looping;
	bool HoldAtEnd;
	bool Shader;
	bool letterBox;
	e_status Status;
	unsigned int StartTime;
	unsigned int LastTime;
	int PlayOnWalls;

	QCinematicPlayer(QCinematic* ACin, int x, int y, int w, int h, int SystemBits)
	: Cin(ACin)
	, XPos(x)
	, YPos(y)
	, Width(w)
	, Height(h)
	, AlterGameState((SystemBits & CIN_system) != 0)
	, Looping((SystemBits & CIN_loop) != 0)
	, HoldAtEnd((SystemBits & CIN_hold) != 0)
	, Shader((SystemBits & CIN_shader) != 0)
	, letterBox((SystemBits & CIN_letterBox) != 0)
	, Status(FMV_PLAY)
	, StartTime(0)
	, LastTime(0)
	, PlayOnWalls(1)
	{
		Cin->Silent = (SystemBits & CIN_silent) != 0;
		Cin->Dirty = true;
		if (!AlterGameState)
		{
			PlayOnWalls = cl_inGameVideo->integer;
		}
		StartTime = LastTime = CL_ScaledMilliseconds();
	}
	~QCinematicPlayer();
	void SetExtents(int x, int y, int w, int h);
	e_status Run();
	void Reset();
	void Upload(int Handle);
};

void CIN_MakeFullName(const char* Name, char* FullName);
QCinematic* CIN_Open(const char* Name);
int CIN_HandleForVideo();

#define MAX_VIDEO_HANDLES   16

extern QCinematicPlayer* cinTable[MAX_VIDEO_HANDLES];
