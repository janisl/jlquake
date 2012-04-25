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
//**	.MD2 triangle model file format
//**
//**************************************************************************

#ifndef _MD2FILE_H
#define _MD2FILE_H

#define IDMESH2HEADER       (('2' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define MESH2_VERSION       8

#define MAX_MD2_VERTS       2048
#define MAX_MD2_SKINS       32
#define MAX_MD2_SKINNAME    64

struct dmd2_stvert_t
{
	short s;
	short t;
};

struct dmd2_triangle_t
{
	short index_xyz[3];
	short index_st[3];
};

struct dmd2_trivertx_t
{
	byte v[3];				// scaled byte to fit in frame mins/maxs
	byte lightnormalindex;
};

struct dmd2_frame_t
{
	float scale[3];				// multiply byte verts by this
	float translate[3];				// then add this
	char name[16];				// frame name from grabbing
	dmd2_trivertx_t verts[1];	// variable sized
};

// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.

struct dmd2_t
{
	int ident;
	int version;

	int skinwidth;
	int skinheight;
	int framesize;				// byte size of each frame

	int num_skins;
	int num_xyz;
	int num_st;					// greater than num_xyz for seams
	int num_tris;
	int num_glcmds;				// dwords in strip/fan command list
	int num_frames;

	int ofs_skins;				// each skin is a MAX_MD2_SKINNAME string
	int ofs_st;					// byte offset from start for stverts
	int ofs_tris;				// offset for dtriangles
	int ofs_frames;				// offset for first frame
	int ofs_glcmds;
	int ofs_end;				// end of file
};

#endif
