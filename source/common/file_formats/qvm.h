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

#define VM_MAGIC    0x12721444

struct vmHeader_t
{
	int vmMagic;

	int instructionCount;

	int codeOffset;
	int codeLength;

	int dataOffset;
	int dataLength;
	int litLength;			// ( dataLength - litLength ) should be byteswapped on load
	int bssLength;			// zero filled memory appended to datalength
};
