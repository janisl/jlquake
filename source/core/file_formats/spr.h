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

//-------------------------------------------------------
// This program generates .spr sprite package files.
// The format of the files is as follows:
//
// dsprite_t file header structure
// <repeat dsprite_t.numframes times>
//   <if spritegroup, repeat dspritegroup_t.numframes times>
//     dspriteframe_t frame header structure
//     sprite bitmap
//   <else (single sprite frame)>
//     dspriteframe_t frame header structure
//     sprite bitmap
// <endrepeat>
//-------------------------------------------------------

#ifndef _SPRFILE_H
#define _SPRFILE_H

//	Little-endian "IDSP"
#define IDSPRITE1HEADER		(('P' << 24) + ('S' << 16) + ('D' << 8) + 'I')

#define SPRITE1_VERSION		1

#define SPR_VP_PARALLEL_UPRIGHT		0
#define SPR_FACING_UPRIGHT			1
#define SPR_VP_PARALLEL				2
#define SPR_ORIENTED				3
#define SPR_VP_PARALLEL_ORIENTED	4

// must match definition in modelgen.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
enum synctype_t
{
	ST_SYNC = 0,
	ST_RAND
};
#endif

enum sprite1frametype_t
{
	SPR_SINGLE = 0,
	SPR_GROUP
};

struct dsprite1_t
{
	qint32		ident;
	qint32		version;
	qint32		type;
	float		boundingradius;
	qint32		width;
	qint32		height;
	qint32		numframes;
	float		beamlength;
	qint32		synctype;
};

struct dsprite1frame_t
{
	qint32		origin[2];
	qint32		width;
	qint32		height;
};

struct dsprite1group_t
{
	qint32		numframes;
};

struct dsprite1interval_t
{
	float		interval;
};

struct dsprite1frametype_t
{
	qint32		type;
};

#endif
