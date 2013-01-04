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

#include "qcommon.h"
#include "common_defs.h"

static bool msgInit = false;
static huffman_t msgHuff;

static int msg_hData[256] =
{
	250315,		// 0
	41193,		// 1
	6292,		// 2
	7106,		// 3
	3730,		// 4
	3750,		// 5
	6110,		// 6
	23283,		// 7
	33317,		// 8
	6950,		// 9
	7838,		// 10
	9714,		// 11
	9257,		// 12
	17259,		// 13
	3949,		// 14
	1778,		// 15
	8288,		// 16
	1604,		// 17
	1590,		// 18
	1663,		// 19
	1100,		// 20
	1213,		// 21
	1238,		// 22
	1134,		// 23
	1749,		// 24
	1059,		// 25
	1246,		// 26
	1149,		// 27
	1273,		// 28
	4486,		// 29
	2805,		// 30
	3472,		// 31
	21819,		// 32
	1159,		// 33
	1670,		// 34
	1066,		// 35
	1043,		// 36
	1012,		// 37
	1053,		// 38
	1070,		// 39
	1726,		// 40
	888,		// 41
	1180,		// 42
	850,		// 43
	960,		// 44
	780,		// 45
	1752,		// 46
	3296,		// 47
	10630,		// 48
	4514,		// 49
	5881,		// 50
	2685,		// 51
	4650,		// 52
	3837,		// 53
	2093,		// 54
	1867,		// 55
	2584,		// 56
	1949,		// 57
	1972,		// 58
	940,		// 59
	1134,		// 60
	1788,		// 61
	1670,		// 62
	1206,		// 63
	5719,		// 64
	6128,		// 65
	7222,		// 66
	6654,		// 67
	3710,		// 68
	3795,		// 69
	1492,		// 70
	1524,		// 71
	2215,		// 72
	1140,		// 73
	1355,		// 74
	971,		// 75
	2180,		// 76
	1248,		// 77
	1328,		// 78
	1195,		// 79
	1770,		// 80
	1078,		// 81
	1264,		// 82
	1266,		// 83
	1168,		// 84
	965,		// 85
	1155,		// 86
	1186,		// 87
	1347,		// 88
	1228,		// 89
	1529,		// 90
	1600,		// 91
	2617,		// 92
	2048,		// 93
	2546,		// 94
	3275,		// 95
	2410,		// 96
	3585,		// 97
	2504,		// 98
	2800,		// 99
	2675,		// 100
	6146,		// 101
	3663,		// 102
	2840,		// 103
	14253,		// 104
	3164,		// 105
	2221,		// 106
	1687,		// 107
	3208,		// 108
	2739,		// 109
	3512,		// 110
	4796,		// 111
	4091,		// 112
	3515,		// 113
	5288,		// 114
	4016,		// 115
	7937,		// 116
	6031,		// 117
	5360,		// 118
	3924,		// 119
	4892,		// 120
	3743,		// 121
	4566,		// 122
	4807,		// 123
	5852,		// 124
	6400,		// 125
	6225,		// 126
	8291,		// 127
	23243,		// 128
	7838,		// 129
	7073,		// 130
	8935,		// 131
	5437,		// 132
	4483,		// 133
	3641,		// 134
	5256,		// 135
	5312,		// 136
	5328,		// 137
	5370,		// 138
	3492,		// 139
	2458,		// 140
	1694,		// 141
	1821,		// 142
	2121,		// 143
	1916,		// 144
	1149,		// 145
	1516,		// 146
	1367,		// 147
	1236,		// 148
	1029,		// 149
	1258,		// 150
	1104,		// 151
	1245,		// 152
	1006,		// 153
	1149,		// 154
	1025,		// 155
	1241,		// 156
	952,		// 157
	1287,		// 158
	997,		// 159
	1713,		// 160
	1009,		// 161
	1187,		// 162
	879,		// 163
	1099,		// 164
	929,		// 165
	1078,		// 166
	951,		// 167
	1656,		// 168
	930,		// 169
	1153,		// 170
	1030,		// 171
	1262,		// 172
	1062,		// 173
	1214,		// 174
	1060,		// 175
	1621,		// 176
	930,		// 177
	1106,		// 178
	912,		// 179
	1034,		// 180
	892,		// 181
	1158,		// 182
	990,		// 183
	1175,		// 184
	850,		// 185
	1121,		// 186
	903,		// 187
	1087,		// 188
	920,		// 189
	1144,		// 190
	1056,		// 191
	3462,		// 192
	2240,		// 193
	4397,		// 194
	12136,		// 195
	7758,		// 196
	1345,		// 197
	1307,		// 198
	3278,		// 199
	1950,		// 200
	886,		// 201
	1023,		// 202
	1112,		// 203
	1077,		// 204
	1042,		// 205
	1061,		// 206
	1071,		// 207
	1484,		// 208
	1001,		// 209
	1096,		// 210
	915,		// 211
	1052,		// 212
	995,		// 213
	1070,		// 214
	876,		// 215
	1111,		// 216
	851,		// 217
	1059,		// 218
	805,		// 219
	1112,		// 220
	923,		// 221
	1103,		// 222
	817,		// 223
	1899,		// 224
	1872,		// 225
	976,		// 226
	841,		// 227
	1127,		// 228
	956,		// 229
	1159,		// 230
	950,		// 231
	7791,		// 232
	954,		// 233
	1289,		// 234
	933,		// 235
	1127,		// 236
	3207,		// 237
	1020,		// 238
	927,		// 239
	1355,		// 240
	768,		// 241
	1040,		// 242
	745,		// 243
	952,		// 244
	805,		// 245
	1073,		// 246
	740,		// 247
	1013,		// 248
	805,		// 249
	1008,		// 250
	796,		// 251
	996,		// 252
	1057,		// 253
	11457,		// 254
	13504,		// 255
};

static void MSG_initHuffman()
{
	int i,j;

	msgInit = true;
	Huff_Init(&msgHuff);
	for (i = 0; i < 256; i++)
	{
		for (j = 0; j < msg_hData[i]; j++)
		{
			Huff_addRef(&msgHuff.compressor,    (byte)i);			// Do update
			Huff_addRef(&msgHuff.decompressor,  (byte)i);			// Do update
		}
	}
}

void QMsg::Init(byte* NewData, int Length)
{
	if (!msgInit)
	{
		MSG_initHuffman();
	}
	Com_Memset(this, 0, sizeof(*this));
	_data = NewData;
	maxsize = Length;
	if (GGameType & GAME_Tech3)
	{
		allowoverflow = true;
	}
}

void QMsg::InitOOB(byte* NewData, int Length)
{
	if (!msgInit)
	{
		MSG_initHuffman();
	}
	Com_Memset(this, 0, sizeof(*this));
	_data = NewData;
	maxsize = Length;
	oob = true;
	if (GGameType & GAME_Tech3)
	{
		allowoverflow = true;
	}
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

void QMsg::Uncompressed()
{
	// align to byte-boundary
	bit = (bit + 7) & ~7;
	oob = true;
}

//	TTimo
//	Copy a QMsg in case we need to store it as is for a bit
// (as I needed this to keep an QMsg from a static var for later use)
// sets data buffer as Init does prior to do the copy
void QMsg::Copy(byte* NewData, int Length, QMsg& Src)
{
	if (Length < Src.cursize)
	{
		common->Error("QMsg::Copy: can't copy into a smaller QMsg buffer");
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
	uncompsize += NumBits;

	if (maxsize - cursize < (abs(NumBits) + 7) / 8)
	{
		if (!allowoverflow)
		{
			common->FatalError("SZ_GetSpace: overflow without allowoverflow set");
		}
		if (!(GGameType & GAME_Tech3))
		{
			common->Printf("SZ_GetSpace: overflow\n");
			Clear();
		}
		overflowed = true;
		return;
	}

	if (NumBits == 0 || NumBits < -31 || NumBits > 32)
	{
		common->Error("QMsg::WriteBits: bad bits %i", NumBits);
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
			common->Error("can't read %d bits\n", NumBits);
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
	bool Sgn;

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
			common->Error("can't read %d bits\n", NumBits);
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
	{
		common->FatalError("MSG_WriteChar: range error");
	}
#endif

	WriteBits(C, 8);
}

void QMsg::WriteByte(int C)
{
#ifdef PARANOID
	if (C < 0 || C > MAX_QUINT8)
	{
		common->FatalError("MSG_WriteByte: range error");
	}
#endif

	WriteBits(C, 8);
}

void QMsg::WriteShort(int C)
{
#ifdef PARANOID
	if (C < MIN_QINT16 || c > MAX_QINT16)
	{
		common->FatalError("QMsg::WriteShort: range error");
	}
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
		char string[MAX_STRING_CHARS];

		int L = String::Length(S);
		if (L >= MAX_STRING_CHARS)
		{
			common->Printf("QMsg::WriteString: MAX_STRING_CHARS");
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

void QMsg::WriteBigString(const char* S)
{
	if (!S)
	{
		WriteData("", 1);
	}
	else
	{
		char string[BIG_INFO_STRING];

		int L = String::Length(S);
		if (L >= BIG_INFO_STRING)
		{
			common->Printf("MSG_WriteString: BIG_INFO_STRING");
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

void QMsg::WritePos(const vec3_t pos)
{
	WriteCoord(pos[0]);
	WriteCoord(pos[1]);
	WriteCoord(pos[2]);
}

void QMsg::WriteDir(const vec3_t dir)
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
	}
	while (L < (int)sizeof(string) - 1);

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
	}
	while (L < (int)sizeof(string) - 1);

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
	}
	while (L < (int)sizeof(string) - 1);

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
	}
	while (L < (int)sizeof(string) - 1);

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
	}
	while (L < (int)sizeof(string) - 1);

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
	for (int i = 0; i < Len; i++)
	{
		((byte*)Buffer)[i] = ReadByte();
	}
}
