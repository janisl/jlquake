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
//**	MESSAGE IO
//**
//**	Handles byte ordering and avoids alignment errors
//**
//**************************************************************************

#include "core.h"

#if 0
int oldsize = 0;
int overflows;
huffman_t msgHuff;

void QMsg::Init(byte* NewData, int Length)
{
	Com_Memset(this, 0, sizeof(*this));
	_data = NewData;
	maxsize = Length;
}

void QMsg::InitOOB(byte* NewData, int Length)
{
	Com_Memset(this, 0, sizeof(*this));
	_data = NewData;
	maxsize = Length;
	oob = true;
}

void QMsg::Clear()
{
	cursize = 0;
	overflowed = false;
	bit = 0;					//<- in bits
}

void QMsg::Bitstream()
{
	oob = false;
}

//	TTimo
//	Copy a QMsg in case we need to store it as is for a bit
// (as I needed this to keep an QMsg from a static var for later use)
// sets data buffer as Init does prior to do the copy
void QMsg::Copy(byte* NewData, int Length, QMsg& Src)
{
	if (Length < Src.cursize)
	{
		throw DropException("QMsg::Copy: can't copy into a smaller QMsg buffer");
	}
	Com_Memcpy(this, &Src, sizeof(QMsg));
	_data = NewData;
	Com_Memcpy(_data, Src._data, Src.cursize);
}

void QMsg::BeginReading()
{
	readcount = 0;
	badread = false;
	bit = 0;
	oob = false;
}

void QMsg::BeginReadingOOB()
{
	readcount = 0;
	badread = false;
	bit = 0;
	oob = true;
}

//	Negative bit values include signs
void QMsg::WriteBits(int Value, int NumBits)
{
	oldsize += NumBits;

	if (maxsize - cursize < (abs(NumBits) + 7) / 8)
	{
		if (!allowoverflow)
		{
			throw Exception("SZ_GetSpace: overflow without allowoverflow set");
		}
		if (!(GGameType & GAME_Quake3))
		{
			Log::writeLine("SZ_GetSpace: overflow");
			Clear(); 
		}
		overflowed = true;
		return;
	}

	if (NumBits == 0 || NumBits < -31 || NumBits > 32)
	{
		throw DropException(va("QMsg::WriteBits: bad bits %i", NumBits));
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
			throw DropException(va("can't read %d bits\n", NumBits));
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
			throw DropException(va("can't read %d bits\n", NumBits));
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

void QMsg::WriteChar(int C)
{
#ifdef PARANOID
	if (C < MIN_QINT8 || C > MAX_QINT8)
		throw Exception("MSG_WriteChar: range error");
#endif

	WriteBits(C, 8);
}

void QMsg::WriteByte(int C)
{
#ifdef PARANOID
	if (C < 0 || C > MAX_QUINT8)
		throw Exception("MSG_WriteByte: range error");
#endif

	WriteBits(C, 8);
}

void QMsg::WriteShort(int C)
{
#ifdef PARANOID
	if (C < MIN_QINT16 || c > MAX_QINT16)
		throw Exception("QMsg::WriteShort: range error");
#endif

	WriteBits(C, 16);
}

void QMsg::WriteLong(int C)
{
	WriteBits(C, 32);
}

void QMsg::WriteFloat(float F)
{
	union
	{
		float F;
		int L;
	} Dat;
	
	Dat.F = F;
	WriteBits(Dat.L, 32);
}

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
			Log::write("QMsg::WriteString: MAX_STRING_CHARS");
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
			Log::write("MSG_WriteString: BIG_INFO_STRING");
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

void QMsg::WriteCoord(float F)
{
	WriteShort((int)(F * 8));
}

void QMsg::WriteDir(vec3_t dir)
{
	WriteByte(DirToByte(dir));
}

void QMsg::WriteAngle(float F)
{
	WriteByte((int)(F * 256 / 360) & 255);
}

void QMsg::WriteAngle16(float F)
{
	WriteShort(ANGLE2SHORT(F));
}

void QMsg::WriteData(const void* Buffer, int Length)
{
	for (int i = 0; i < Length; i++)
	{
		WriteByte(((byte*)Buffer)[i]);
	}
}

//	strcats onto the sizebuf
void QMsg::Print(const char* S)
{
	if (!_data[cursize - 1])
	{
		// write over trailing 0
		cursize--;
	}
	WriteString2(S);
}

//	Returns -1 if no more characters are available
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

float QMsg::ReadFloat()
{
	union
	{
		float F;
		int L;
	} Dat;

	Dat.L = ReadBits(32);
	if (readcount > cursize)
	{
		Dat.F = -1;
		badread = true;
	}
	return Dat.F;
}

const char* QMsg::ReadString()
{
	static char string[MAX_STRING_CHARS];

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

const char* QMsg::ReadString2()
{
	static char string[2048];

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

const char* QMsg::ReadBigString()
{
	static char string[BIG_INFO_STRING];

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

const char* QMsg::ReadStringLine()
{
	static char string[MAX_STRING_CHARS];

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

const char* QMsg::ReadStringLine2()
{
	static char string[2048];

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

float QMsg::ReadCoord()
{
	return ReadShort() * (1.0 / 8.0);
}

void QMsg::ReadPos(vec3_t pos)
{
	pos[0] = ReadCoord();
	pos[1] = ReadCoord();
	pos[2] = ReadCoord();
}

void QMsg::ReadDir(vec3_t dir)
{
	int b = ReadByte();
	ByteToDir(b, dir);
}

float QMsg::ReadAngle()
{
	return ReadChar() * (360.0 / 256.0);
}

float QMsg::ReadAngle16()
{
	return SHORT2ANGLE(ReadShort());
}

void QMsg::ReadData(void* Buffer, int Len)
{
	for (int i = 0; i < Len ; i++)
	{
		((byte*)Buffer)[i] = ReadByte();
	}
}

void MSGQW_WriteDeltaUsercmd(QMsg* buf, qwusercmd_t *from, qwusercmd_t *cmd)
{
	//
	// send the movement message
	//
	int bits = 0;
	if (cmd->angles[0] != from->angles[0])
		bits |= QWCM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])
		bits |= QWCM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])
		bits |= QWCM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)
		bits |= QWCM_FORWARD;
	if (cmd->sidemove != from->sidemove)
		bits |= QWCM_SIDE;
	if (cmd->upmove != from->upmove)
		bits |= QWCM_UP;
	if (cmd->buttons != from->buttons)
		bits |= QWCM_BUTTONS;
	if (cmd->impulse != from->impulse)
		bits |= QWCM_IMPULSE;

    buf->WriteByte(bits);

	if (bits & QWCM_ANGLE1)
		buf->WriteAngle16(cmd->angles[0]);
	if (bits & QWCM_ANGLE2)
		buf->WriteAngle16(cmd->angles[1]);
	if (bits & QWCM_ANGLE3)
		buf->WriteAngle16(cmd->angles[2]);
	
	if (bits & QWCM_FORWARD)
		buf->WriteShort(cmd->forwardmove);
	if (bits & QWCM_SIDE)
	  	buf->WriteShort(cmd->sidemove);
	if (bits & QWCM_UP)
		buf->WriteShort(cmd->upmove);

 	if (bits & QWCM_BUTTONS)
	  	buf->WriteByte(cmd->buttons);
 	if (bits & QWCM_IMPULSE)
	    buf->WriteByte(cmd->impulse);
	buf->WriteByte(cmd->msec);
}

void MSGQW_ReadDeltaUsercmd(QMsg* buf, qwusercmd_t* from, qwusercmd_t* move)
{
	Com_Memcpy(move, from, sizeof(*move));

	int bits = buf->ReadByte();
		
// read current angles
	if (bits & QWCM_ANGLE1)
		move->angles[0] = buf->ReadAngle16();
	if (bits & QWCM_ANGLE2)
		move->angles[1] = buf->ReadAngle16();
	if (bits & QWCM_ANGLE3)
		move->angles[2] = buf->ReadAngle16();
		
// read movement
	if (bits & QWCM_FORWARD)
		move->forwardmove = buf->ReadShort();
	if (bits & QWCM_SIDE)
		move->sidemove = buf->ReadShort();
	if (bits & QWCM_UP)
		move->upmove = buf->ReadShort();
	
// read buttons
	if (bits & QWCM_BUTTONS)
		move->buttons = buf->ReadByte ();

	if (bits & QWCM_IMPULSE)
		move->impulse = buf->ReadByte ();

// read time to run command
	move->msec = buf->ReadByte ();
}
#endif
