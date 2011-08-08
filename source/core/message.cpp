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
//**	MESSAGE IO
//**
//**	Handles byte ordering and avoids alignment errors
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "core.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

int				oldsize = 0;
int				overflows;
huffman_t		msgHuff;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QMsg::Init
//
//==========================================================================

void QMsg::Init(byte* NewData, int Length)
{
	Com_Memset(this, 0, sizeof(*this));
	_data = NewData;
	maxsize = Length;
}

//==========================================================================
//
//	QMsg::InitOOB
//
//==========================================================================

void QMsg::InitOOB(byte* NewData, int Length)
{
	Com_Memset(this, 0, sizeof(*this));
	_data = NewData;
	maxsize = Length;
	oob = true;
}

//==========================================================================
//
//	QMsg::Clear
//
//==========================================================================

void QMsg::Clear()
{
	cursize = 0;
	overflowed = false;
	bit = 0;					//<- in bits
}

//==========================================================================
//
//	QMsg::Bitstream
//
//==========================================================================

void QMsg::Bitstream()
{
	oob = false;
}

//==========================================================================
//
//	QMsg::Copy
//
//	TTimo
//	Copy a QMsg in case we need to store it as is for a bit
// (as I needed this to keep an QMsg from a static var for later use)
// sets data buffer as Init does prior to do the copy
//
//==========================================================================

void QMsg::Copy(byte* NewData, int Length, QMsg& Src)
{
	if (Length < Src.cursize)
	{
		throw QDropException("QMsg::Copy: can't copy into a smaller QMsg buffer");
	}
	Com_Memcpy(this, &Src, sizeof(QMsg));
	_data = NewData;
	Com_Memcpy(_data, Src._data, Src.cursize);
}

//==========================================================================
//
//	QMsg::BeginReading
//
//==========================================================================

void QMsg::BeginReading()
{
	readcount = 0;
	badread = false;
	bit = 0;
	oob = false;
}

//==========================================================================
//
//	QMsg::BeginReadingOOB
//
//==========================================================================

void QMsg::BeginReadingOOB()
{
	readcount = 0;
	badread = false;
	bit = 0;
	oob = true;
}

//==========================================================================
//
//	QMsg::WriteBits
//
//	Negative bit values include signs
//
//==========================================================================

void QMsg::WriteBits(int Value, int NumBits)
{
	oldsize += NumBits;

	// this isn't an exact overflow check, but close enough
	if (maxsize - cursize < 4)
	{
		if (!allowoverflow)
		{
			throw QException("SZ_GetSpace: overflow without allowoverflow set");
		}
		//	Games before Quake 3 does this.
		//gLog.writeLine("SZ_GetSpace: overflow");
		//Clear(); 
		overflowed = true;
		return;
	}

	if (NumBits == 0 || NumBits < -31 || NumBits > 32)
	{
		throw QDropException(va("QMsg::WriteBits: bad bits %i", NumBits));
	}

	// check for overflows
	if (NumBits != 32)
	{
		if (NumBits > 0)
		{
			if (Value > ((1 << NumBits) - 1) || Value < 0)
			{
				overflows++;
			}
		}
		else
		{
			int r = 1 << (NumBits - 1);
			if (Value >  r - 1 || Value < -r)
			{
				overflows++;
			}
		}
	}
	if (NumBits < 0)
	{
		NumBits = -NumBits;
	}
	if (oob)
	{
		if (NumBits == 8)
		{
			_data[cursize] = Value;
			cursize += 1;
			bit += 8;
		}
		else if (NumBits == 16)
		{
			quint16* sp = (quint16*)&_data[cursize];
			*sp = LittleShort(Value);
			cursize += 2;
			bit += 16;
		}
		else if (NumBits == 32)
		{
			quint32* ip = (quint32*)&_data[cursize];
			*ip = LittleLong(Value);
			cursize += 4;
			bit += 32;
		}
		else
		{
			throw QDropException(va("can't read %d bits\n", NumBits));
		}
	}
	else
	{
		Value &= (0xffffffff >> (32 - NumBits));
		if (NumBits & 7)
		{
			int nbits = NumBits & 7;
			for (int i = 0; i < nbits; i++)
			{
				Huff_putBit((Value & 1), _data, &bit);
				Value = (Value >> 1);
			}
			NumBits = NumBits - nbits;
		}
		if (NumBits)
		{
			for (int i = 0; i < NumBits; i += 8)
			{
				Huff_offsetTransmit(&msgHuff.compressor, (Value & 0xff), _data, &bit);
				Value = (Value >> 8);
			}
		}
		cursize = (bit >> 3) + 1;
	}
}

//==========================================================================
//
//	QMsg::ReadBits
//
//==========================================================================

int QMsg::ReadBits(int NumBits)
{
	bool		Sgn;

	int Value = 0;

	if (NumBits < 0)
	{
		NumBits = -NumBits;
		Sgn = true;
	}
	else
	{
		Sgn = false;
	}

	if (oob)
	{
		if (NumBits == 8)
		{
			Value = _data[readcount];
			readcount += 1;
			bit += 8;
		}
		else if (NumBits == 16)
		{
			quint16* sp = (quint16*)&_data[readcount];
			Value = LittleShort(*sp);
			readcount += 2;
			bit += 16;
		}
		else if (NumBits == 32)
		{
			quint32* ip = (quint32*)&_data[readcount];
			Value = LittleLong(*ip);
			readcount += 4;
			bit += 32;
		}
		else
		{
			throw QDropException(va("can't read %d bits\n", NumBits));
		}
	}
	else
	{
		int nbits = 0;
		if (NumBits & 7)
		{
			nbits = NumBits & 7;
			for (int i = 0; i < nbits; i++)
			{
				Value |= (Huff_getBit(_data, &bit) << i);
			}
			NumBits = NumBits - nbits;
		}
		if (NumBits)
		{
			for (int i = 0; i < NumBits; i += 8)
			{
				int Get;
				Huff_offsetReceive(msgHuff.decompressor.tree, &Get, _data, &bit);
				Value |= (Get << (i + nbits));
			}
		}
		readcount = (bit >> 3) + 1;
	}
	if (Sgn)
	{
		if (Value & (1 << (NumBits - 1)))
		{
			Value |= -1 ^ ((1 << NumBits) - 1);
		}
	}

	return Value;
}

//==========================================================================
//
//	QMsg::WriteChar
//
//==========================================================================

void QMsg::WriteChar(int C)
{
#ifdef PARANOID
	if (C < MIN_QINT8 || C > MAX_QINT8)
		throw QException("MSG_WriteChar: range error");
#endif

	WriteBits(C, 8);
}

//==========================================================================
//
//	QMsg::WriteByte
//
//==========================================================================

void QMsg::WriteByte(int C)
{
#ifdef PARANOID
	if (C < 0 || C > MAX_QUINT8)
		throw QException("MSG_WriteByte: range error");
#endif

	WriteBits(C, 8);
}

//==========================================================================
//
//	QMsg::WriteShort
//
//==========================================================================

void QMsg::WriteShort(int C)
{
#ifdef PARANOID
	if (C < MIN_QINT16 || c > MAX_QINT16)
		throw QException("QMsg::WriteShort: range error");
#endif

	WriteBits(C, 16);
}

//==========================================================================
//
//	QMsg::WriteLong
//
//==========================================================================

void QMsg::WriteLong(int C)
{
	WriteBits(C, 32);
}

//==========================================================================
//
//	QMsg::WriteFloat
//
//==========================================================================

void QMsg::WriteFloat(float F)
{
	union
	{
		float	F;
		int		L;
	} Dat;
	
	Dat.F = F;
	WriteBits(Dat.L, 32);
}

//==========================================================================
//
//	QMsg::WriteString
//
//==========================================================================

void QMsg::WriteString(const char* S)
{
	if (!S)
	{
		WriteData("", 1);
	}
	else
	{
		char	string[MAX_STRING_CHARS];

		int L = String::Length(S);
		if (L >= MAX_STRING_CHARS)
		{
			gLog.write("QMsg::WriteString: MAX_STRING_CHARS");
			WriteData("", 1);
			return;
		}
		String::NCpyZ(string, S, sizeof(string));

		// get rid of 0xff chars, because old clients don't like them
		for (int i = 0; i < L; i++)
		{
			if (((byte*)string)[i] > 127)
			{
				string[i] = '.';
			}
		}

		WriteData(string, L + 1);
	}
}

//==========================================================================
//
//	QMsg::WriteString2
//
//==========================================================================

void QMsg::WriteString2(const char* S)
{
	if (!S)
	{
		WriteData("", 1);
	}
	else
	{
		WriteData(S, String::Length(S) + 1);
	}
}

//==========================================================================
//
//	QMsg::WriteBigString
//
//==========================================================================

void QMsg::WriteBigString(const char *S)
{
	if (!S)
	{
		WriteData("", 1);
	}
	else
	{
		char	string[BIG_INFO_STRING];

		int L = String::Length(S);
		if (L >= BIG_INFO_STRING)
		{
			gLog.write("MSG_WriteString: BIG_INFO_STRING");
			WriteData("", 1);
			return;
		}
		String::NCpyZ(string, S, sizeof(string));

		// get rid of 0xff chars, because old clients don't like them
		for (int i = 0; i < L; i++)
		{
			if (((byte*)string)[i] > 127)
			{
				string[i] = '.';
			}
		}

		WriteData(string, L + 1);
	}
}

//==========================================================================
//
//	QMsg::WriteCoord
//
//==========================================================================

void QMsg::WriteCoord(float F)
{
	WriteShort((int)(F * 8));
}

//==========================================================================
//
//	QMsg::WriteAngle
//
//==========================================================================

void QMsg::WriteAngle(float F)
{
	WriteByte((int)(F * 256 / 360) & 255);
}

//==========================================================================
//
//	QMsg::WriteAngle16
//
//==========================================================================

void QMsg::WriteAngle16(float F)
{
	WriteShort(ANGLE2SHORT(F));
}

//==========================================================================
//
//	QMsg::WriteData
//
//==========================================================================

void QMsg::WriteData(const void* Buffer, int Length)
{
	for (int i = 0; i < Length; i++)
	{
		WriteByte(((byte*)Buffer)[i]);
	}
}

//==========================================================================
//
//	QMsg::Print
//
//	strcats onto the sizebuf
//
//==========================================================================

void QMsg::Print(const char* S)
{
	if (!_data[cursize - 1])
	{
		// write over trailing 0
		cursize--;
	}
	WriteString2(S);
}

//==========================================================================
//
//	QMsg::ReadByte
//
//	Returns -1 if no more characters are available
//
//==========================================================================

int QMsg::ReadChar()
{
	int C = (qint8)ReadBits(8);
	if (readcount > cursize)
	{
		C = -1;
		badread = true;
	}
	return C;
}

//==========================================================================
//
//	QMsg::ReadByte
//
//==========================================================================

int QMsg::ReadByte()
{
	int C = (quint8)ReadBits(8);
	if (readcount > cursize)
	{
		C = -1;
		badread = true;
	}
	return C;
}

//==========================================================================
//
//	QMsg::ReadShort
//
//==========================================================================

int QMsg::ReadShort()
{
	int C = (qint16)ReadBits(16);
	if (readcount > cursize)
	{
		C = -1;
		badread = true;
	}
	return C;
}

//==========================================================================
//
//	QMsg::ReadLong
//
//==========================================================================

int QMsg::ReadLong()
{
	int C = ReadBits(32);
	if (readcount > cursize)
	{
		C = -1;
		badread = true;
	}
	return C;
}

//==========================================================================
//
//	QMsg::ReadFloat
//
//==========================================================================

float QMsg::ReadFloat()
{
	union
	{
		float	F;
		int		L;
	} Dat;

	Dat.L = ReadBits(32);
	if (readcount > cursize)
	{
		Dat.F = -1;
		badread = true;
	}
	return Dat.F;
}

//==========================================================================
//
//	QMsg::ReadString
//
//==========================================================================

const char* QMsg::ReadString()
{
	static char		string[MAX_STRING_CHARS];

	int L = 0;
	do
	{
		int C = ReadByte();		// use ReadByte so -1 is out of bounds
		if (C == -1 || C == 0)
		{
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if (C == '%')
		{
			C = '.';
		}
		// don't allow higher ascii values
		if (C > 127)
		{
			C = '.';
		}

		string[L] = C;
		L++;
	} while (L < (int)sizeof(string) - 1);

	string[L] = 0;

	return string;
}

//==========================================================================
//
//	QMsg::ReadString2
//
//==========================================================================

const char* QMsg::ReadString2()
{
	static char		string[2048];

	int L = 0;
	do
	{
		int C = ReadChar();
		if (C == -1 || C == 0)
		{
			break;
		}
		string[L] = C;
		L++;
	} while (L < (int)sizeof(string) - 1);

	string[L] = 0;

	return string;
}

//==========================================================================
//
//	QMsg::ReadStringLine
//
//==========================================================================

const char* QMsg::ReadBigString()
{
	static char		string[BIG_INFO_STRING];

	int L = 0;
	do
	{
		int C = ReadByte();		// use ReadByte so -1 is out of bounds
		if (C == -1 || C == 0)
		{
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if (C == '%')
		{
			C = '.';
		}

		string[L] = C;
		L++;
	} while (L < (int)sizeof(string) - 1);

	string[L] = 0;

	return string;
}

//==========================================================================
//
//	QMsg::ReadStringLine
//
//==========================================================================

const char* QMsg::ReadStringLine()
{
	static char		string[MAX_STRING_CHARS];

	int L = 0;
	do
	{
		int C = ReadByte();		// use ReadByte so -1 is out of bounds
		if (C == -1 || C == 0 || C == '\n')
		{
			break;
		}
		// translate all fmt spec to avoid crash bugs
		if (C == '%')
		{
			C = '.';
		}
		string[L] = C;
		L++;
	} while (L < (int)sizeof(string) - 1);

	string[L] = 0;

	return string;
}

//==========================================================================
//
//	QMsg::ReadStringLine2
//
//==========================================================================

const char* QMsg::ReadStringLine2()
{
	static char		string[2048];

	int L = 0;
	do
	{
		int C = ReadChar();
		if (C == -1 || C == 0 || C == '\n')
		{
			break;
		}
		string[L] = C;
		L++;
	} while (L < (int)sizeof(string) - 1);

	string[L] = 0;

	return string;
}

//==========================================================================
//
//	QMsg::ReadCoord
//
//==========================================================================

float QMsg::ReadCoord()
{
	return ReadShort() * (1.0 / 8.0);
}

//==========================================================================
//
//	QMsg::ReadAngle
//
//==========================================================================

float QMsg::ReadAngle()
{
	return ReadChar() * (360.0 / 256.0);
}

//==========================================================================
//
//	QMsg::ReadAngle16
//
//==========================================================================

float QMsg::ReadAngle16()
{
	return SHORT2ANGLE(ReadShort());
}

//==========================================================================
//
//	QMsg::ReadData
//
//==========================================================================

void QMsg::ReadData(void* Buffer, int Len)
{
	for (int i = 0; i < Len ; i++)
	{
		((byte*)Buffer)[i] = ReadByte();
	}
}
