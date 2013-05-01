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

#include "../../common/Common.h"
#include "local.h"
#include "../../common/common_defs.h"
#include "../../common/strings.h"
#include "../../common/endian.h"

static int AAS_WriteAASLump_offset;

static void AAS_SwapAASData() {
	// Ridah, no need to do anything if this OS doesn't need byte swapping
	if ( LittleLong( 1 ) == 1 ) {
		return;
	}

	for ( int i = 0; i < aasworld->numbboxes; i++ ) {
		aasworld->bboxes[ i ].presencetype = LittleLong( aasworld->bboxes[ i ].presencetype );
		aasworld->bboxes[ i ].flags = LittleLong( aasworld->bboxes[ i ].flags );
		for ( int j = 0; j < 3; j++ ) {
			aasworld->bboxes[ i ].mins[ j ] = LittleFloat( aasworld->bboxes[ i ].mins[ j ] );
			aasworld->bboxes[ i ].maxs[ j ] = LittleFloat( aasworld->bboxes[ i ].maxs[ j ] );
		}
	}
	for ( int i = 0; i < aasworld->numvertexes; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			aasworld->vertexes[ i ][ j ] = LittleFloat( aasworld->vertexes[ i ][ j ] );
		}
	}
	for ( int i = 0; i < aasworld->numplanes; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			aasworld->planes[ i ].normal[ j ] = LittleFloat( aasworld->planes[ i ].normal[ j ] );
		}
		aasworld->planes[ i ].dist = LittleFloat( aasworld->planes[ i ].dist );
		aasworld->planes[ i ].type = LittleLong( aasworld->planes[ i ].type );
	}
	for ( int i = 0; i < aasworld->numedges; i++ ) {
		aasworld->edges[ i ].v[ 0 ] = LittleLong( aasworld->edges[ i ].v[ 0 ] );
		aasworld->edges[ i ].v[ 1 ] = LittleLong( aasworld->edges[ i ].v[ 1 ] );
	}
	for ( int i = 0; i < aasworld->edgeindexsize; i++ ) {
		aasworld->edgeindex[ i ] = LittleLong( aasworld->edgeindex[ i ] );
	}
	for ( int i = 0; i < aasworld->numfaces; i++ ) {
		aasworld->faces[ i ].planenum = LittleLong( aasworld->faces[ i ].planenum );
		aasworld->faces[ i ].faceflags = LittleLong( aasworld->faces[ i ].faceflags );
		aasworld->faces[ i ].numedges = LittleLong( aasworld->faces[ i ].numedges );
		aasworld->faces[ i ].firstedge = LittleLong( aasworld->faces[ i ].firstedge );
		aasworld->faces[ i ].frontarea = LittleLong( aasworld->faces[ i ].frontarea );
		aasworld->faces[ i ].backarea = LittleLong( aasworld->faces[ i ].backarea );
	}
	for ( int i = 0; i < aasworld->faceindexsize; i++ ) {
		aasworld->faceindex[ i ] = LittleLong( aasworld->faceindex[ i ] );
	}
	for ( int i = 0; i < aasworld->numareas; i++ ) {
		aasworld->areas[ i ].areanum = LittleLong( aasworld->areas[ i ].areanum );
		aasworld->areas[ i ].numfaces = LittleLong( aasworld->areas[ i ].numfaces );
		aasworld->areas[ i ].firstface = LittleLong( aasworld->areas[ i ].firstface );
		for ( int j = 0; j < 3; j++ ) {
			aasworld->areas[ i ].mins[ j ] = LittleFloat( aasworld->areas[ i ].mins[ j ] );
			aasworld->areas[ i ].maxs[ j ] = LittleFloat( aasworld->areas[ i ].maxs[ j ] );
			aasworld->areas[ i ].center[ j ] = LittleFloat( aasworld->areas[ i ].center[ j ] );
		}
	}
	for ( int i = 0; i < aasworld->numareasettings; i++ ) {
		aasworld->areasettings[ i ].contents = LittleLong( aasworld->areasettings[ i ].contents );
		aasworld->areasettings[ i ].areaflags = LittleLong( aasworld->areasettings[ i ].areaflags );
		aasworld->areasettings[ i ].presencetype = LittleLong( aasworld->areasettings[ i ].presencetype );
		aasworld->areasettings[ i ].cluster = LittleLong( aasworld->areasettings[ i ].cluster );
		aasworld->areasettings[ i ].clusterareanum = LittleLong( aasworld->areasettings[ i ].clusterareanum );
		aasworld->areasettings[ i ].numreachableareas = LittleLong( aasworld->areasettings[ i ].numreachableareas );
		aasworld->areasettings[ i ].firstreachablearea = LittleLong( aasworld->areasettings[ i ].firstreachablearea );
		aasworld->areasettings[ i ].groundsteepness = LittleFloat( aasworld->areasettings[ i ].groundsteepness );
	}
	for ( int i = 0; i < aasworld->reachabilitysize; i++ ) {
		aasworld->reachability[ i ].areanum = LittleLong( aasworld->reachability[ i ].areanum );
		aasworld->reachability[ i ].facenum = LittleLong( aasworld->reachability[ i ].facenum );
		aasworld->reachability[ i ].edgenum = LittleLong( aasworld->reachability[ i ].edgenum );
		for ( int j = 0; j < 3; j++ ) {
			aasworld->reachability[ i ].start[ j ] = LittleFloat( aasworld->reachability[ i ].start[ j ] );
			aasworld->reachability[ i ].end[ j ] = LittleFloat( aasworld->reachability[ i ].end[ j ] );
		}
		aasworld->reachability[ i ].traveltype = LittleLong( aasworld->reachability[ i ].traveltype );
		aasworld->reachability[ i ].traveltime = LittleShort( aasworld->reachability[ i ].traveltime );
	}
	for ( int i = 0; i < aasworld->numnodes; i++ ) {
		aasworld->nodes[ i ].planenum = LittleLong( aasworld->nodes[ i ].planenum );
		aasworld->nodes[ i ].children[ 0 ] = LittleLong( aasworld->nodes[ i ].children[ 0 ] );
		aasworld->nodes[ i ].children[ 1 ] = LittleLong( aasworld->nodes[ i ].children[ 1 ] );
	}
	for ( int i = 0; i < aasworld->numportals; i++ ) {
		aasworld->portals[ i ].areanum = LittleLong( aasworld->portals[ i ].areanum );
		aasworld->portals[ i ].frontcluster = LittleLong( aasworld->portals[ i ].frontcluster );
		aasworld->portals[ i ].backcluster = LittleLong( aasworld->portals[ i ].backcluster );
		aasworld->portals[ i ].clusterareanum[ 0 ] = LittleLong( aasworld->portals[ i ].clusterareanum[ 0 ] );
		aasworld->portals[ i ].clusterareanum[ 1 ] = LittleLong( aasworld->portals[ i ].clusterareanum[ 1 ] );
	}
	for ( int i = 0; i < aasworld->portalindexsize; i++ ) {
		aasworld->portalindex[ i ] = LittleLong( aasworld->portalindex[ i ] );
	}
	for ( int i = 0; i < aasworld->numclusters; i++ ) {
		aasworld->clusters[ i ].numareas = LittleLong( aasworld->clusters[ i ].numareas );
		aasworld->clusters[ i ].numreachabilityareas = LittleLong( aasworld->clusters[ i ].numreachabilityareas );
		aasworld->clusters[ i ].numportals = LittleLong( aasworld->clusters[ i ].numportals );
		aasworld->clusters[ i ].firstportal = LittleLong( aasworld->clusters[ i ].firstportal );
	}
}

// dump the current loaded aas file
void AAS_DumpAASData() {
	aasworld->numbboxes = 0;
	if ( aasworld->bboxes ) {
		Mem_Free( aasworld->bboxes );
	}
	aasworld->bboxes = NULL;
	aasworld->numvertexes = 0;
	if ( aasworld->vertexes ) {
		Mem_Free( aasworld->vertexes );
	}
	aasworld->vertexes = NULL;
	aasworld->numplanes = 0;
	if ( aasworld->planes ) {
		Mem_Free( aasworld->planes );
	}
	aasworld->planes = NULL;
	aasworld->numedges = 0;
	if ( aasworld->edges ) {
		Mem_Free( aasworld->edges );
	}
	aasworld->edges = NULL;
	aasworld->edgeindexsize = 0;
	if ( aasworld->edgeindex ) {
		Mem_Free( aasworld->edgeindex );
	}
	aasworld->edgeindex = NULL;
	aasworld->numfaces = 0;
	if ( aasworld->faces ) {
		Mem_Free( aasworld->faces );
	}
	aasworld->faces = NULL;
	aasworld->faceindexsize = 0;
	if ( aasworld->faceindex ) {
		Mem_Free( aasworld->faceindex );
	}
	aasworld->faceindex = NULL;
	aasworld->numareas = 0;
	if ( aasworld->areas ) {
		Mem_Free( aasworld->areas );
	}
	aasworld->areas = NULL;
	aasworld->numareasettings = 0;
	if ( aasworld->areasettings ) {
		Mem_Free( aasworld->areasettings );
	}
	aasworld->areasettings = NULL;
	aasworld->reachabilitysize = 0;
	if ( aasworld->reachability ) {
		Mem_Free( aasworld->reachability );
	}
	aasworld->reachability = NULL;
	aasworld->numnodes = 0;
	if ( aasworld->nodes ) {
		Mem_Free( aasworld->nodes );
	}
	aasworld->nodes = NULL;
	aasworld->numportals = 0;
	if ( aasworld->portals ) {
		Mem_Free( aasworld->portals );
	}
	aasworld->portals = NULL;
	aasworld->numportals = 0;
	if ( aasworld->portalindex ) {
		Mem_Free( aasworld->portalindex );
	}
	aasworld->portalindex = NULL;
	aasworld->portalindexsize = 0;
	if ( aasworld->clusters ) {
		Mem_Free( aasworld->clusters );
	}
	aasworld->clusters = NULL;
	aasworld->numclusters = 0;
	//
	aasworld->loaded = false;
	aasworld->initialized = false;
	aasworld->savefile = false;
}

// allocate memory and read a lump of a AAS file
static char* AAS_LoadAASLump( fileHandle_t fp, int offset, int length, int* lastoffset, int size ) {
	if ( !length ) {
		//just alloc a dummy
		return ( char* )Mem_ClearedAlloc( size + 1 );
	}
	//seek to the data
	if ( offset != *lastoffset ) {
		BotImport_Print( PRT_WARNING, "AAS file not sequentially read\n" );
		if ( FS_Seek( fp, offset, FS_SEEK_SET ) ) {
			AAS_Error( "can't seek to aas lump\n" );
			AAS_DumpAASData();
			FS_FCloseFile( fp );
			return 0;
		}
	}
	//allocate memory
	char* buf = ( char* )Mem_ClearedAlloc( length + 1 );
	//read the data
	if ( length ) {
		FS_Read( buf, length, fp );
		*lastoffset += length;
	}
	return buf;
}

static aas_areasettings_t* AAS_LoadAreaSettings5Lump( fileHandle_t fp, int offset, int length, int* lastoffset ) {
	if ( !length ) {
		//just alloc a dummy
		return ( aas_areasettings_t* )Mem_ClearedAlloc( sizeof ( aas_areasettings_t ) );
	}
	//seek to the data
	if ( offset != *lastoffset ) {
		BotImport_Print( PRT_WARNING, "AAS file not sequentially read\n" );
		if ( FS_Seek( fp, offset, FS_SEEK_SET ) ) {
			AAS_Error( "can't seek to aas lump\n" );
			AAS_DumpAASData();
			FS_FCloseFile( fp );
			return 0;
		}
	}
	//allocate memory
	int count = length / sizeof ( aas5_areasettings_t );
	aas_areasettings_t* buf = ( aas_areasettings_t* )Mem_ClearedAlloc( sizeof ( aas_areasettings_t ) * count );
	//read the data
	for ( int i = 0; i < count; i++ ) {
		FS_Read( &buf[ i ], sizeof ( aas5_areasettings_t ), fp );
	}
	*lastoffset += count * sizeof ( aas5_areasettings_t );
	return buf;
}

static void AAS_DData( unsigned char* data, int size ) {
	for ( int i = 0; i < size; i++ ) {
		data[ i ] ^= ( unsigned char )i * 119;
	}
}

// load an aas file
int AAS_LoadAASFile( const char* filename ) {
	BotImport_Print( PRT_MESSAGE, "trying to load %s\n", filename );
	//dump current loaded aas file
	AAS_DumpAASData();
	//open the file
	fileHandle_t fp;
	FS_FOpenFileByMode( filename, &fp, FS_READ );
	if ( !fp ) {
		AAS_Error( "can't open %s\n", filename );
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTOPENAASFILE : WOLFBLERR_CANNOTOPENAASFILE;
	}
	//read the header
	aas_header_t header;
	FS_Read( &header, sizeof ( aas_header_t ), fp );
	int lastoffset = sizeof ( aas_header_t );
	//check header identification
	header.ident = LittleLong( header.ident );
	if ( header.ident != AASID ) {
		AAS_Error( "%s is not an AAS file\n", filename );
		FS_FCloseFile( fp );
		return GGameType & GAME_Quake3 ? Q3BLERR_WRONGAASFILEID : WOLFBLERR_WRONGAASFILEID;
	}
	//check the version
	header.version = LittleLong( header.version );
	if ( header.version != AASVERSION4 && header.version != AASVERSION5 && header.version != AASVERSION8 ) {
		AAS_Error( "aas file %s is version %i, not %i\n", filename, header.version, AASVERSION5 );
		FS_FCloseFile( fp );
		return GGameType & GAME_Quake3 ? Q3BLERR_WRONGAASFILEVERSION : WOLFBLERR_WRONGAASFILEVERSION;
	}
	//RF, checksum of -1 always passes, hack to fix commercial maps without having to distribute new bsps
	bool nocrc = false;
	if ( GGameType & GAME_ET && LittleLong( header.bspchecksum ) == -1 ) {
		nocrc = true;
	}
	if ( header.version == AASVERSION5 || header.version == AASVERSION8 ) {
		AAS_DData( ( unsigned char* )&header + 8, sizeof ( aas_header_t ) - 8 );
	}
	aasworld->bspchecksum = String::Atoi( LibVarGetString( "sv_mapChecksum" ) );
	if ( !nocrc && LittleLong( header.bspchecksum ) != aasworld->bspchecksum ) {
		AAS_Error( "aas file %s is out of date\n", filename );
		FS_FCloseFile( fp );
		return GGameType & GAME_Quake3 ? Q3BLERR_WRONGAASFILEVERSION : WOLFBLERR_WRONGAASFILEVERSION;
	}

	//load the lumps:
	//bounding boxes
	int offset = LittleLong( header.lumps[ AASLUMP_BBOXES ].fileofs );
	int length = LittleLong( header.lumps[ AASLUMP_BBOXES ].filelen );
	aasworld->bboxes = ( aas_bbox_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_bbox_t ) );
	aasworld->numbboxes = length / sizeof ( aas_bbox_t );
	if ( aasworld->numbboxes && !aasworld->bboxes ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//vertexes
	offset = LittleLong( header.lumps[ AASLUMP_VERTEXES ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_VERTEXES ].filelen );
	aasworld->vertexes = ( aas_vertex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_vertex_t ) );
	aasworld->numvertexes = length / sizeof ( aas_vertex_t );
	if ( aasworld->numvertexes && !aasworld->vertexes ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//planes
	offset = LittleLong( header.lumps[ AASLUMP_PLANES ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_PLANES ].filelen );
	aasworld->planes = ( aas_plane_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_plane_t ) );
	aasworld->numplanes = length / sizeof ( aas_plane_t );
	if ( aasworld->numplanes && !aasworld->planes ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//edges
	offset = LittleLong( header.lumps[ AASLUMP_EDGES ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_EDGES ].filelen );
	aasworld->edges = ( aas_edge_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_edge_t ) );
	aasworld->numedges = length / sizeof ( aas_edge_t );
	if ( aasworld->numedges && !aasworld->edges ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//edgeindex
	offset = LittleLong( header.lumps[ AASLUMP_EDGEINDEX ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_EDGEINDEX ].filelen );
	aasworld->edgeindex = ( aas_edgeindex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_edgeindex_t ) );
	aasworld->edgeindexsize = length / sizeof ( aas_edgeindex_t );
	if ( aasworld->edgeindexsize && !aasworld->edgeindex ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//faces
	offset = LittleLong( header.lumps[ AASLUMP_FACES ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_FACES ].filelen );
	aasworld->faces = ( aas_face_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_face_t ) );
	aasworld->numfaces = length / sizeof ( aas_face_t );
	if ( aasworld->numfaces && !aasworld->faces ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//faceindex
	offset = LittleLong( header.lumps[ AASLUMP_FACEINDEX ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_FACEINDEX ].filelen );
	aasworld->faceindex = ( aas_faceindex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_faceindex_t ) );
	aasworld->faceindexsize = length / sizeof ( aas_faceindex_t );
	if ( aasworld->faceindexsize && !aasworld->faceindex ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//convex areas
	offset = LittleLong( header.lumps[ AASLUMP_AREAS ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_AREAS ].filelen );
	aasworld->areas = ( aas_area_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_area_t ) );
	aasworld->numareas = length / sizeof ( aas_area_t );
	if ( aasworld->numareas && !aasworld->areas ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//area settings
	offset = LittleLong( header.lumps[ AASLUMP_AREASETTINGS ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_AREASETTINGS ].filelen );
	if ( header.version == AASVERSION4 || header.version == AASVERSION5 ) {
		aasworld->areasettings = AAS_LoadAreaSettings5Lump( fp, offset, length, &lastoffset );
	} else {
		aasworld->areasettings = ( aas_areasettings_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_areasettings_t ) );
	}
	aasworld->numareasettings = length / sizeof ( aas5_areasettings_t );
	if ( aasworld->numareasettings && !aasworld->areasettings ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//reachability list
	offset = LittleLong( header.lumps[ AASLUMP_REACHABILITY ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_REACHABILITY ].filelen );
	aasworld->reachability = ( aas_reachability_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_reachability_t ) );
	aasworld->reachabilitysize = length / sizeof ( aas_reachability_t );
	if ( aasworld->reachabilitysize && !aasworld->reachability ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//nodes
	offset = LittleLong( header.lumps[ AASLUMP_NODES ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_NODES ].filelen );
	aasworld->nodes = ( aas_node_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_node_t ) );
	aasworld->numnodes = length / sizeof ( aas_node_t );
	if ( aasworld->numnodes && !aasworld->nodes ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//cluster portals
	offset = LittleLong( header.lumps[ AASLUMP_PORTALS ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_PORTALS ].filelen );
	aasworld->portals = ( aas_portal_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_portal_t ) );
	aasworld->numportals = length / sizeof ( aas_portal_t );
	if ( aasworld->numportals && !aasworld->portals ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//cluster portal index
	offset = LittleLong( header.lumps[ AASLUMP_PORTALINDEX ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_PORTALINDEX ].filelen );
	aasworld->portalindex = ( aas_portalindex_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_portalindex_t ) );
	aasworld->portalindexsize = length / sizeof ( aas_portalindex_t );
	if ( aasworld->portalindexsize && !aasworld->portalindex ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//clusters
	offset = LittleLong( header.lumps[ AASLUMP_CLUSTERS ].fileofs );
	length = LittleLong( header.lumps[ AASLUMP_CLUSTERS ].filelen );
	aasworld->clusters = ( aas_cluster_t* )AAS_LoadAASLump( fp, offset, length, &lastoffset, sizeof ( aas_cluster_t ) );
	aasworld->numclusters = length / sizeof ( aas_cluster_t );
	if ( aasworld->numclusters && !aasworld->clusters ) {
		return GGameType & GAME_Quake3 ? Q3BLERR_CANNOTREADAASLUMP : WOLFBLERR_CANNOTREADAASLUMP;
	}
	//swap everything
	AAS_SwapAASData();
	//aas file is loaded
	aasworld->loaded = true;
	//close the file
	FS_FCloseFile( fp );

	if ( GGameType & GAME_ET ) {
		int j = 0;
		for ( int i = 1; i < aasworld->numareas; i++ ) {
			j += aasworld->areasettings[ i ].numreachableareas;
		}
		if ( j > aasworld->reachabilitysize ) {
			common->Error( "aas reachabilitysize incorrect\n" );
		}
	}

	return BLERR_NOERROR;
}

static int AAS_WriteAASLump( fileHandle_t fp, aas_header_t* h, int lumpnum, void* data, int length ) {
	aas_lump_t* lump = &h->lumps[ lumpnum ];

	lump->fileofs = LittleLong( AAS_WriteAASLump_offset );		//LittleLong(ftell(fp));
	lump->filelen = LittleLong( length );

	if ( length > 0 ) {
		FS_Write( data, length, fp );
	}

	AAS_WriteAASLump_offset += length;

	return true;
}

static int AAS_WriteAreaSettings5Lump( fileHandle_t fp, aas_header_t* h ) {
	aas_lump_t* lump = &h->lumps[ AASLUMP_AREASETTINGS ];

	lump->fileofs = LittleLong( AAS_WriteAASLump_offset );		//LittleLong(ftell(fp));
	lump->filelen = LittleLong( aasworld->numareasettings * sizeof ( aas5_areasettings_t ) );

	for ( int i = 0; i < aasworld->numareasettings; i++ ) {
		FS_Write( &aasworld->areasettings[ i ], sizeof ( aas5_areasettings_t ), fp );
	}

	AAS_WriteAASLump_offset += aasworld->numareasettings * sizeof ( aas5_areasettings_t );

	return true;
}

// aas data is useless after writing to file because it is byte swapped
bool AAS_WriteAASFile( const char* filename ) {
	aas_header_t header;
	fileHandle_t fp;

	BotImport_Print( PRT_MESSAGE, "writing %s\n", filename );
	//swap the aas data
	AAS_SwapAASData();
	//initialize the file header
	Com_Memset( &header, 0, sizeof ( aas_header_t ) );
	header.ident = LittleLong( AASID );
	header.version = LittleLong( GGameType & GAME_Quake3 ? AASVERSION5 : AASVERSION8 );
	header.bspchecksum = LittleLong( aasworld->bspchecksum );
	//open a new file
	FS_FOpenFileByMode( filename, &fp, FS_WRITE );
	if ( !fp ) {
		BotImport_Print( PRT_ERROR, "error opening %s\n", filename );
		return false;
	}	//end if
		//write the header
	FS_Write( &header, sizeof ( aas_header_t ), fp );
	AAS_WriteAASLump_offset = sizeof ( aas_header_t );
	//add the data lumps to the file
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_BBOXES, aasworld->bboxes,
			 aasworld->numbboxes * sizeof ( aas_bbox_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_VERTEXES, aasworld->vertexes,
			 aasworld->numvertexes * sizeof ( aas_vertex_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_PLANES, aasworld->planes,
			 aasworld->numplanes * sizeof ( aas_plane_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_EDGES, aasworld->edges,
			 aasworld->numedges * sizeof ( aas_edge_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_EDGEINDEX, aasworld->edgeindex,
			 aasworld->edgeindexsize * sizeof ( aas_edgeindex_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_FACES, aasworld->faces,
			 aasworld->numfaces * sizeof ( aas_face_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_FACEINDEX, aasworld->faceindex,
			 aasworld->faceindexsize * sizeof ( aas_faceindex_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_AREAS, aasworld->areas,
			 aasworld->numareas * sizeof ( aas_area_t ) ) ) {
		return false;
	}
	if ( GGameType & GAME_Quake3 ) {
		if ( !AAS_WriteAreaSettings5Lump( fp, &header ) ) {
			return false;
		}
	} else {
		if ( !AAS_WriteAASLump( fp, &header, AASLUMP_AREASETTINGS, aasworld->areasettings,
				 aasworld->numareasettings * sizeof ( aas8_areasettings_t ) ) ) {
			return false;
		}
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_REACHABILITY, aasworld->reachability,
			 aasworld->reachabilitysize * sizeof ( aas_reachability_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_NODES, aasworld->nodes,
			 aasworld->numnodes * sizeof ( aas_node_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_PORTALS, aasworld->portals,
			 aasworld->numportals * sizeof ( aas_portal_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_PORTALINDEX, aasworld->portalindex,
			 aasworld->portalindexsize * sizeof ( aas_portalindex_t ) ) ) {
		return false;
	}
	if ( !AAS_WriteAASLump( fp, &header, AASLUMP_CLUSTERS, aasworld->clusters,
			 aasworld->numclusters * sizeof ( aas_cluster_t ) ) ) {
		return false;
	}
	//rewrite the header with the added lumps
	FS_Seek( fp, 0, FS_SEEK_SET );
	AAS_DData( ( unsigned char* )&header + 8, sizeof ( aas_header_t ) - 8 );
	FS_Write( &header, sizeof ( aas_header_t ), fp );
	//close the file
	FS_FCloseFile( fp );
	return true;
}
