/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*****************************************************************************
 * name:		be_aas_file.c
 *
 * desc:		AAS file loading/writing
 *
 * $Archive: /MissionPack/code/botlib/be_aas_file.c $
 *
 *****************************************************************************/

#include "../common/qcommon.h"
#include "l_utils.h"
#include "botlib.h"
#include "be_interface.h"
#include "be_aas.h"
#include "be_aas_funcs.h"
#include "be_aas_def.h"

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static int AAS_WriteAASLump_offset;

int AAS_WriteAASLump(fileHandle_t fp, aas_header_t* h, int lumpnum, void* data, int length)
{
	aas_lump_t* lump;

	lump = &h->lumps[lumpnum];

	lump->fileofs = LittleLong(AAS_WriteAASLump_offset);	//LittleLong(ftell(fp));
	lump->filelen = LittleLong(length);

	if (length > 0)
	{
		FS_Write(data, length, fp);
	}	//end if

	AAS_WriteAASLump_offset += length;

	return true;
}	//end of the function AAS_WriteAASLump
//===========================================================================
// aas data is useless after writing to file because it is byte swapped
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean AAS_WriteAASFile(char* filename)
{
	aas_header_t header;
	fileHandle_t fp;

	BotImport_Print(PRT_MESSAGE, "writing %s\n", filename);
	//swap the aas data
	AAS_SwapAASData();
	//initialize the file header
	Com_Memset(&header, 0, sizeof(aas_header_t));
	header.ident = LittleLong(AASID);
	header.version = LittleLong(AASVERSION5);
	header.bspchecksum = LittleLong((*aasworld).bspchecksum);
	//open a new file
	FS_FOpenFileByMode(filename, &fp, FS_WRITE);
	if (!fp)
	{
		BotImport_Print(PRT_ERROR, "error opening %s\n", filename);
		return false;
	}	//end if
		//write the header
	FS_Write(&header, sizeof(aas_header_t), fp);
	AAS_WriteAASLump_offset = sizeof(aas_header_t);
	//add the data lumps to the file
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_BBOXES, (*aasworld).bboxes,
			(*aasworld).numbboxes * sizeof(aas_bbox_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_VERTEXES, (*aasworld).vertexes,
			(*aasworld).numvertexes * sizeof(aas_vertex_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_PLANES, (*aasworld).planes,
			(*aasworld).numplanes * sizeof(aas_plane_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_EDGES, (*aasworld).edges,
			(*aasworld).numedges * sizeof(aas_edge_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_EDGEINDEX, (*aasworld).edgeindex,
			(*aasworld).edgeindexsize * sizeof(aas_edgeindex_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_FACES, (*aasworld).faces,
			(*aasworld).numfaces * sizeof(aas_face_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_FACEINDEX, (*aasworld).faceindex,
			(*aasworld).faceindexsize * sizeof(aas_faceindex_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_AREAS, (*aasworld).areas,
			(*aasworld).numareas * sizeof(aas_area_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_AREASETTINGS, (*aasworld).areasettings,
			(*aasworld).numareasettings * sizeof(aas5_areasettings_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_REACHABILITY, (*aasworld).reachability,
			(*aasworld).reachabilitysize * sizeof(aas_reachability_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_NODES, (*aasworld).nodes,
			(*aasworld).numnodes * sizeof(aas_node_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_PORTALS, (*aasworld).portals,
			(*aasworld).numportals * sizeof(aas_portal_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_PORTALINDEX, (*aasworld).portalindex,
			(*aasworld).portalindexsize * sizeof(aas_portalindex_t)))
	{
		return false;
	}
	if (!AAS_WriteAASLump(fp, &header, AASLUMP_CLUSTERS, (*aasworld).clusters,
			(*aasworld).numclusters * sizeof(aas_cluster_t)))
	{
		return false;
	}
	//rewrite the header with the added lumps
	FS_Seek(fp, 0, FS_SEEK_SET);
	AAS_DData((unsigned char*)&header + 8, sizeof(aas_header_t) - 8);
	FS_Write(&header, sizeof(aas_header_t), fp);
	//close the file
	FS_FCloseFile(fp);
	return true;
}	//end of the function AAS_WriteAASFile
