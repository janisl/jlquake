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

class QMsg
{
public:
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed (with allowoverflow set)
	qboolean	oob;
	qboolean	badread;		// set if a read goes beyond end of message
	byte*		_data;
	int			maxsize;
	int			cursize;
	int			readcount;
	int			bit;			// for bitwise reads and writes

	void Init(byte* NewData, int Length);
	void InitOOB(byte* NewData, int Length);
	void Clear();
	void Bitstream();
	void Copy(byte* NewData, int Length, QMsg& Src);

	void BeginReading();
	void BeginReadingOOB();
	int GetReadCount() const
	{
		return readcount;
	}

	void WriteBits(int Value, int NumBits);
	int ReadBits(int NumBits);

	//	Writing functions
	void WriteChar(int C);
	void WriteByte(int C);
	void WriteShort(int C);
	void WriteLong(int C);
	void WriteFloat(float F);
	void WriteString(const char* S);
	void WriteString2(const char* S);
	void WriteBigString(const char* S);
	void WriteCoord(float F);
	void WriteAngle(float F);
	void WriteAngle16(float F);
	void WriteData(const void* Buffer, int Length);
	void Print(const char* S);

	//	Reading functions
	int ReadChar();
	int ReadByte();
	int ReadShort();
	int ReadLong();
	float ReadFloat();
	const char* ReadString();
	const char* ReadString2();
	const char* ReadBigString();
	const char* ReadStringLine();
	const char* ReadStringLine2();
	float ReadCoord();
	float ReadAngle();
	float ReadAngle16();
	void ReadData(void* Buffer, int Size);
};
