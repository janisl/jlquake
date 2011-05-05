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
//**
//**	JPEG LOADING
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "client.h"
#include "render_local.h"
#include "../../libs/jpeg/jpeglib.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

struct my_source_mgr
{
	jpeg_source_mgr	pub;

	unsigned char*	infile;
	int				bytes_left;
	JOCTET*			buffer;
	bool			start_of_file;
};

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  byte* outfile;		/* target stream */
  int	size;
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int		hackSize;

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	my_jpeg_error_exit
//
//==========================================================================

static void my_jpeg_error_exit(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	//	Create the message.
	(*cinfo->err->format_message)(cinfo, buffer);

	//	Let the memory manager delete any temp files before we die.
	jpeg_destroy(cinfo);

	throw QException(buffer);
}

//==========================================================================
//
//	my_jpeg_output_message
//
//==========================================================================

static void my_jpeg_output_message(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	//	Create the message
	(*cinfo->err->format_message)(cinfo, buffer);

	GLog.Write("%s\n", buffer);
}

//==========================================================================
//
//	my_jpeg_init_source
//
//==========================================================================

static void my_jpeg_init_source(j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr)cinfo->src;
	src->start_of_file = TRUE;
}

//==========================================================================
//
//	my_jpeg_fill_input_buffer
//
//==========================================================================

static boolean my_jpeg_fill_input_buffer(j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr)cinfo->src;

	if (!src->bytes_left)
	{
		return FALSE;
	}
	int Cnt = INPUT_BUF_SIZE;
	if (Cnt > src->bytes_left)
	{
		Cnt = src->bytes_left;
	}
	Com_Memcpy(src->buffer, src->infile, Cnt);

	src->infile += Cnt;
	src->bytes_left -= Cnt;

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = Cnt;
	src->start_of_file = FALSE;

	return TRUE;
}

//==========================================================================
//
//	my_jpeg_skip_input_data
//
//==========================================================================

static void my_jpeg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;
	if (num_bytes > 0)
	{
		while (num_bytes > (long)src->pub.bytes_in_buffer)
		{
			num_bytes -= (long)src->pub.bytes_in_buffer;
			if (!my_jpeg_fill_input_buffer(cinfo))
			{
				return;
			}
		}
		src->pub.next_input_byte += (size_t)num_bytes;
		src->pub.bytes_in_buffer -= (size_t)num_bytes;
	}
}

//==========================================================================
//
//	my_jpeg_term_source
//
//==========================================================================

static void my_jpeg_term_source(j_decompress_ptr cinfo)
{
}

//==========================================================================
//
//	my_jpeg_src
//
//==========================================================================

static void my_jpeg_src(j_decompress_ptr cinfo, byte* infile, int size)
{
	my_src_ptr src;

	if (cinfo->src == NULL)
	{
		cinfo->src = (jpeg_source_mgr*)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
			sizeof(my_source_mgr));
		src = (my_src_ptr)cinfo->src;
		src->buffer = (JOCTET *)
			(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT,
			INPUT_BUF_SIZE * sizeof(JOCTET));
	}

	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = my_jpeg_init_source;
	src->pub.fill_input_buffer = my_jpeg_fill_input_buffer;
	src->pub.skip_input_data = my_jpeg_skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = my_jpeg_term_source;
	src->infile = infile;
	src->bytes_left = size;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
}

//==========================================================================
//
//	R_LoadJPG
//
//==========================================================================

void R_LoadJPG(const char* filename, unsigned char** pic, int* width, int* height)
{
	byte* fbuffer;
 	int FileSize = FS_ReadFile((char*)filename, (void**)&fbuffer);
	if (!fbuffer)
	{
		return;
	}

	//	Allocate and initialize JPEG decompression object.
	jpeg_error_mgr jerr;
	jpeg_decompress_struct cinfo;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = my_jpeg_error_exit;
	cinfo.err->output_message = my_jpeg_output_message;

	jpeg_create_decompress(&cinfo);
	my_jpeg_src(&cinfo, fbuffer, FileSize);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	int row_stride = cinfo.output_width * 4;
	byte* out = new byte[cinfo.output_width * cinfo.output_height * 4];

	*pic = out;
	*width = cinfo.output_width;
	*height = cinfo.output_height;

	QArray<JSAMPLE> ScanLine;
	ScanLine.SetNum(cinfo.output_width * cinfo.output_components);
	JSAMPROW RowPtr = ScanLine.Ptr();
	JSAMPARRAY buffer = &RowPtr;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		JSAMPROW pSrc = ScanLine.Ptr();
		byte* pDst = out + (row_stride * cinfo.output_scanline);
		jpeg_read_scanlines(&cinfo, buffer, 1);
		for (int i = 0; i < (int)cinfo.output_width; i++, pSrc += cinfo.output_components, pDst += 4)
		{
			pDst[0] = pSrc[0];
			pDst[1] = pSrc[1];
			pDst[2] = pSrc[2];
			// clear all the alphas to 255
			pDst[3] = 255;
		}
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	FS_FreeFile(fbuffer);
}

//==========================================================================
//
//	init_destination
//
//==========================================================================

static void init_destination(j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr) cinfo->dest;

	dest->pub.next_output_byte = dest->outfile;
	dest->pub.free_in_buffer = dest->size;
}


//==========================================================================
//
//	empty_output_buffer
//
//==========================================================================

boolean empty_output_buffer(j_compress_ptr cinfo)
{
	return TRUE;
}

//==========================================================================
//
//	term_destination
//
//==========================================================================

void term_destination(j_compress_ptr cinfo)
{
	my_dest_ptr dest = (my_dest_ptr)cinfo->dest;
	size_t datacount = dest->size - dest->pub.free_in_buffer;
	hackSize = datacount;
}

//==========================================================================
//
//	jpegDest
//
//==========================================================================

void jpegDest(j_compress_ptr cinfo, byte* outfile, int size)
{
	my_dest_ptr dest;

	if (cinfo->dest == NULL)
	{
		cinfo->dest = (jpeg_destination_mgr*)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
			sizeof(my_destination_mgr));
	}

	dest = (my_dest_ptr)cinfo->dest;
	dest->pub.init_destination = init_destination;
	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;
	dest->outfile = outfile;
	dest->size = size;
}

//==========================================================================
//
//	R_SaveJPG
//
//==========================================================================

void R_SaveJPG(const char* filename, int quality, int image_width, int image_height, byte* image_buffer)
{
	jpeg_error_mgr jerr;
	jpeg_compress_struct cinfo;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	byte* out = new byte[image_width * image_height * 4];
	jpegDest(&cinfo, out, image_width * image_height * 4);

	cinfo.image_width = image_width;
	cinfo.image_height = image_height;
	cinfo.input_components = 4;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	int row_stride = image_width * 4;
	while (cinfo.next_scanline < cinfo.image_height)
	{
		JSAMPROW row_pointer[1];
		row_pointer[0] = &image_buffer[((cinfo.image_height - 1) * row_stride) - cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	FS_WriteFile(filename, out, hackSize);

	delete[] out;
}
