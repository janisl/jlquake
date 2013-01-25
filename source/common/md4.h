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

#ifndef __MD4_H__
#define __MD4_H__

#include "qcommon.h"

unsigned Com_BlockChecksum( const void* Buffer, int Length );
unsigned Com_BlockChecksumKey( const void* Buffer, int Length, int Key );
void Com_BlockFullChecksum( const void* Buffer, int Length, unsigned char* OutBuf );

#endif
