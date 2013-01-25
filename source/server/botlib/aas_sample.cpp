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

#include "../../common/qcommon.h"
#include "../../common/common_defs.h"
#include "../../common/content_types.h"
#include "local.h"

#define NUM_BBOXAREASCACHE          128
#define BBOXAREASCACHE_MAXAREAS     128

#define TRACEPLANE_EPSILON          0.125

#define ON_EPSILON                  0	//0.0005

struct aas_tracestack_t {
	vec3_t start;		//start point of the piece of line to trace
	vec3_t end;			//end point of the piece of line to trace
	int planenum;		//last plane used as splitter
	int nodenum;		//node found after splitting with planenum
};

struct bboxAreasCache_t {
	float lastUsedTime;
	int numUsed;
	vec3_t absmins, absmaxs;
	int areas[ BBOXAREASCACHE_MAXAREAS ];
	int numAreas;
};

static bboxAreasCache_t bboxAreasCache[ NUM_BBOXAREASCACHE ];

void AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs ) {
	//bounding box size for each presence type
	vec3_t boxmins[ 4 ] = {{-15, -15, -24}, {-15, -15, -24}, {-18, -18, -24}, {-18, -18, -24}};
	vec3_t boxmaxs[ 4 ] = {{ 15,  15,  32}, { 15,  15,   8}, { 18,  18,  48}, { 18,  18,  24}};

	int index;
	if ( presencetype == PRESENCE_NORMAL ) {
		index = 0;
	} else if ( presencetype == PRESENCE_CROUCH )     {
		index = 1;
	} else   {
		BotImport_Print( PRT_FATAL, "AAS_PresenceTypeBoundingBox: unknown presence type\n" );
		index = 1;
	}
	if ( GGameType & GAME_ET ) {
		index += 2;
	}
	VectorCopy( boxmins[ index ], mins );
	VectorCopy( boxmaxs[ index ], maxs );
}

void AAS_InitAASLinkHeap() {
	int max_aaslinks = aasworld->linkheapsize;
	//if there's no link heap present
	if ( !aasworld->linkheap ) {
		max_aaslinks = GGameType & GAME_Quake3 ? ( int )LibVarValue( "max_aaslinks", "6144" ) : ( int )4096;
		if ( max_aaslinks < 0 ) {
			max_aaslinks = 0;
		}
		aasworld->linkheapsize = max_aaslinks;
		aasworld->linkheap = ( aas_link_t* )Mem_ClearedAlloc( max_aaslinks * sizeof ( aas_link_t ) );
	} else   {
		// just clear the memory
		Com_Memset( aasworld->linkheap, 0, aasworld->linkheapsize * sizeof ( aas_link_t ) );
	}
	//link the links on the heap
	aasworld->linkheap[ 0 ].prev_ent = NULL;
	aasworld->linkheap[ 0 ].next_ent = &aasworld->linkheap[ 1 ];
	for ( int i = 1; i < max_aaslinks - 1; i++ ) {
		aasworld->linkheap[ i ].prev_ent = &aasworld->linkheap[ i - 1 ];
		aasworld->linkheap[ i ].next_ent = &aasworld->linkheap[ i + 1 ];
	}
	aasworld->linkheap[ max_aaslinks - 1 ].prev_ent = &aasworld->linkheap[ max_aaslinks - 2 ];
	aasworld->linkheap[ max_aaslinks - 1 ].next_ent = NULL;
	//pointer to the first free link
	aasworld->freelinks = &aasworld->linkheap[ 0 ];
}

void AAS_FreeAASLinkHeap() {
	if ( aasworld->linkheap ) {
		Mem_Free( aasworld->linkheap );
	}
	aasworld->linkheap = NULL;
	aasworld->linkheapsize = 0;
}

static aas_link_t* AAS_AllocAASLink() {
	aas_link_t* link = aasworld->freelinks;
	if ( !link ) {
		if ( bot_developer ) {
			BotImport_Print( PRT_FATAL, "empty aas link heap\n" );
		}
		return NULL;
	}
	if ( aasworld->freelinks ) {
		aasworld->freelinks = aasworld->freelinks->next_ent;
	}
	if ( aasworld->freelinks ) {
		aasworld->freelinks->prev_ent = NULL;
	}
	return link;
}

static void AAS_DeAllocAASLink( aas_link_t* link ) {
	if ( aasworld->freelinks ) {
		aasworld->freelinks->prev_ent = link;
	}
	link->prev_ent = NULL;
	link->next_ent = aasworld->freelinks;
	link->prev_area = NULL;
	link->next_area = NULL;
	aasworld->freelinks = link;
}

void AAS_InitAASLinkedEntities() {
	if ( !aasworld->loaded ) {
		return;
	}
	if ( aasworld->arealinkedentities ) {
		Mem_Free( aasworld->arealinkedentities );
	}
	aasworld->arealinkedentities = ( aas_link_t** )Mem_ClearedAlloc(
		aasworld->numareas * sizeof ( aas_link_t* ) );
}

void AAS_FreeAASLinkedEntities() {
	if ( aasworld->arealinkedentities ) {
		Mem_Free( aasworld->arealinkedentities );
	}
	aasworld->arealinkedentities = NULL;
}

// returns the AAS area the point is in
int AAS_PointAreaNum( const vec3_t point ) {
	if ( !aasworld->loaded ) {
		BotImport_Print( PRT_ERROR, "AAS_PointAreaNum: aas not loaded\n" );
		return 0;
	}

	//start with node 1 because node zero is a dummy used for solid leafs
	int nodenum = 1;
	while ( nodenum > 0 ) {
		aas_node_t* node = &aasworld->nodes[ nodenum ];
		aas_plane_t* plane = &aasworld->planes[ node->planenum ];
		vec_t dist = DotProduct( point, plane->normal ) - plane->dist;
		if ( dist > 0 ) {
			nodenum = node->children[ 0 ];
		} else   {
			nodenum = node->children[ 1 ];
		}
	}
	if ( !nodenum ) {
		return 0;
	}
	return -nodenum;
}

int AAS_PointReachabilityAreaIndex( const vec3_t origin ) {
	if ( !aasworld->initialized ) {
		return 0;
	}

	if ( !origin ) {
		int index = 0;
		for ( int i = 0; i < aasworld->numclusters; i++ ) {
			index += aasworld->clusters[ i ].numreachabilityareas;
		}
		return index;
	}

	int areanum = AAS_PointAreaNum( origin );
	if ( !areanum || !AAS_AreaReachability( areanum ) ) {
		return 0;
	}
	int cluster = aasworld->areasettings[ areanum ].cluster;
	areanum = aasworld->areasettings[ areanum ].clusterareanum;
	if ( cluster < 0 ) {
		cluster = aasworld->portals[ -cluster ].frontcluster;
		areanum = aasworld->portals[ -cluster ].clusterareanum[ 0 ];
	}

	int index = 0;
	for ( int i = 0; i < cluster; i++ ) {
		index += aasworld->clusters[ i ].numreachabilityareas;
	}
	index += areanum;
	return index;
}

// returns the presence types of the given area
int AAS_AreaPresenceType( int areanum ) {
	if ( !aasworld->loaded ) {
		return 0;
	}
	if ( areanum <= 0 || areanum >= aasworld->numareas ) {
		BotImport_Print( PRT_ERROR, "AAS_AreaPresenceType: invalid area number\n" );
		return 0;
	}
	return aasworld->areasettings[ areanum ].presencetype;
}

// returns the presence type at the given point
int AAS_PointPresenceType( const vec3_t point ) {
	if ( !aasworld->loaded ) {
		return 0;
	}

	int areanum = AAS_PointAreaNum( point );
	if ( !areanum ) {
		return PRESENCE_NONE;
	}
	return aasworld->areasettings[ areanum ].presencetype;
}

// recursive subdivision of the line by the BSP tree.
int AAS_TraceAreas( const vec3_t start, const vec3_t end, int* areas, vec3_t* points, int maxareas ) {
	int numareas = 0;
	areas[ 0 ] = 0;
	if ( !aasworld->loaded ) {
		return numareas;
	}

	aas_tracestack_t tracestack[ 127 ];
	aas_tracestack_t* tstack_p = tracestack;
	//we start with the whole line on the stack
	VectorCopy( start, tstack_p->start );
	VectorCopy( end, tstack_p->end );
	tstack_p->planenum = 0;
	//start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1;		//starting at the root of the tree
	tstack_p++;

	while ( 1 ) {
		//pop up the stack
		tstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if ( tstack_p < tracestack ) {
			return numareas;
		}
		//number of the current node to test the line against
		int nodenum = tstack_p->nodenum;
		//if it is an area
		if ( nodenum < 0 ) {
			areas[ numareas ] = -nodenum;
			if ( points ) {
				VectorCopy( tstack_p->start, points[ numareas ] );
			}
			numareas++;
			if ( numareas >= maxareas ) {
				return numareas;
			}
			continue;
		}
		//if it is a solid leaf
		if ( !nodenum ) {
			continue;
		}
		//the node to test against
		aas_node_t* aasnode = &aasworld->nodes[ nodenum ];
		//start point of current line to test against node
		vec3_t cur_start;
		VectorCopy( tstack_p->start, cur_start );
		//end point of the current line to test against node
		vec3_t cur_end;
		VectorCopy( tstack_p->end, cur_end );
		//the current node plane
		aas_plane_t* plane = &aasworld->planes[ aasnode->planenum ];

		float front = DotProduct( cur_start, plane->normal ) - plane->dist;
		float back = DotProduct( cur_end, plane->normal ) - plane->dist;

		//if the whole to be traced line is totally at the front of this node
		//only go down the tree with the front child
		if ( front > 0 && back > 0 ) {
			//keep the current start and end point on the stack
			//and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[ 0 ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
		}
		//if the whole to be traced line is totally at the back of this node
		//only go down the tree with the back child
		else if ( front <= 0 && back <= 0 ) {
			//keep the current start and end point on the stack
			//and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[ 1 ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
		}
		//go down the tree both at the front and back of the node
		else {
			int tmpplanenum = tstack_p->planenum;
			//calculate the hitpoint with the node (split point of the line)
			//put the crosspoint TRACEPLANE_EPSILON pixels on the near side
			float frac;
			if ( front < 0 ) {
				frac = ( front ) / ( front - back );
			} else   {
				frac = ( front ) / ( front - back );
			}
			if ( frac < 0 ) {
				frac = 0;
			} else if ( frac > 1 )     {
				frac = 1;
			}

			vec3_t cur_mid;
			cur_mid[ 0 ] = cur_start[ 0 ] + ( cur_end[ 0 ] - cur_start[ 0 ] ) * frac;
			cur_mid[ 1 ] = cur_start[ 1 ] + ( cur_end[ 1 ] - cur_start[ 1 ] ) * frac;
			cur_mid[ 2 ] = cur_start[ 2 ] + ( cur_end[ 2 ] - cur_start[ 2 ] ) * frac;

			//side the front part of the line is on
			int side = front < 0;
			//first put the end part of the line on the stack (back side)
			VectorCopy( cur_mid, tstack_p->start );
			//not necesary to store because still on stack
			//VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum = aasnode->children[ !side ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
			//now put the part near the start of the line on the stack so we will
			//continue with thats part first. This way we'll find the first
			//hit of the bbox
			VectorCopy( cur_start, tstack_p->start );
			VectorCopy( cur_mid, tstack_p->end );
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum = aasnode->children[ side ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceAreas: stack overflow\n" );
				return numareas;
			}
		}
	}
}

bool AAS_PointInsideFace( int facenum, const vec3_t point, float epsilon ) {
	if ( !aasworld->loaded ) {
		return false;
	}

	aas_face_t* face = &aasworld->faces[ facenum ];
	aas_plane_t* plane = &aasworld->planes[ face->planenum ];

	for ( int i = 0; i < face->numedges; i++ ) {
		int edgenum = aasworld->edgeindex[ face->firstedge + i ];
		aas_edge_t* edge = &aasworld->edges[ abs( edgenum ) ];
		//get the first vertex of the edge
		int firstvertex = edgenum < 0;
		vec_t* v1 = aasworld->vertexes[ edge->v[ firstvertex ] ];
		vec_t* v2 = aasworld->vertexes[ edge->v[ !firstvertex ] ];
		//edge vector
		vec3_t edgevec;
		VectorSubtract( v2, v1, edgevec );
		//vector from first edge point to point possible in face
		vec3_t pointvec;
		VectorSubtract( point, v1, pointvec );

		vec3_t sepnormal;
		CrossProduct( edgevec, plane->normal, sepnormal );

		if ( DotProduct( pointvec, sepnormal ) < -epsilon ) {
			return false;
		}
	}
	return true;
}

static int AAS_BoxOnPlaneSide2( const vec3_t absmins, const vec3_t absmaxs, const aas_plane_t* p ) {
	vec3_t corners[ 2 ];
	for ( int i = 0; i < 3; i++ ) {
		if ( p->normal[ i ] < 0 ) {
			corners[ 0 ][ i ] = absmins[ i ];
			corners[ 1 ][ i ] = absmaxs[ i ];
		} else   {
			corners[ 1 ][ i ] = absmins[ i ];
			corners[ 0 ][ i ] = absmaxs[ i ];
		}
	}
	float dist1 = DotProduct( p->normal, corners[ 0 ] ) - p->dist;
	float dist2 = DotProduct( p->normal, corners[ 1 ] ) - p->dist;
	int sides = 0;
	if ( dist1 >= 0 ) {
		sides = 1;
	}
	if ( dist2 < 0 ) {
		sides |= 2;
	}

	return sides;
}

// remove the links to this entity from all areas
void AAS_UnlinkFromAreas( aas_link_t* areas ) {
	aas_link_t* nextlink;
	for ( aas_link_t* link = areas; link; link = nextlink ) {
		//next area the entity is linked in
		nextlink = link->next_area;
		//remove the entity from the linked list of this area
		if ( link->prev_ent ) {
			link->prev_ent->next_ent = link->next_ent;
		} else   {
			aasworld->arealinkedentities[ link->areanum ] = link->next_ent;
		}
		if ( link->next_ent ) {
			link->next_ent->prev_ent = link->prev_ent;
		}
		//deallocate the link structure
		AAS_DeAllocAASLink( link );
	}
}

// link the entity to the areas the bounding box is totally or partly
// situated in. This is done with recursion down the tree using the
// bounding box to test for plane sides

aas_link_t* AAS_AASLinkEntity( const vec3_t absmins, const vec3_t absmaxs, int entnum ) {
	struct aas_linkstack_t {
		int nodenum;		//node found after splitting
	};

	if ( !aasworld->loaded ) {
		BotImport_Print( PRT_ERROR, "AAS_LinkEntity: aas not loaded\n" );
		return NULL;
	}

	aas_link_t* areas = NULL;

	aas_linkstack_t linkstack[ 128 ];
	aas_linkstack_t* lstack_p = linkstack;
	//we start with the whole line on the stack
	//start with node 1 because node zero is a dummy used for solid leafs
	lstack_p->nodenum = 1;		//starting at the root of the tree
	lstack_p++;

	while ( 1 ) {
		//pop up the stack
		lstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if ( lstack_p < linkstack ) {
			break;
		}
		//number of the current node to test the line against
		int nodenum = lstack_p->nodenum;
		//if it is an area
		if ( nodenum < 0 ) {
			//NOTE: the entity might have already been linked into this area
			// because several node children can point to the same area
			aas_link_t* link;
			for ( link = aasworld->arealinkedentities[ -nodenum ]; link; link = link->next_ent ) {
				if ( link->entnum == entnum ) {
					break;
				}
			}
			if ( link ) {
				continue;
			}

			link = AAS_AllocAASLink();
			if ( !link ) {
				return areas;
			}
			link->entnum = entnum;
			link->areanum = -nodenum;
			//put the link into the double linked area list of the entity
			link->prev_area = NULL;
			link->next_area = areas;
			if ( areas ) {
				areas->prev_area = link;
			}
			areas = link;
			//put the link into the double linked entity list of the area
			link->prev_ent = NULL;
			link->next_ent = aasworld->arealinkedentities[ -nodenum ];
			if ( aasworld->arealinkedentities[ -nodenum ] ) {
				aasworld->arealinkedentities[ -nodenum ]->prev_ent = link;
			}
			aasworld->arealinkedentities[ -nodenum ] = link;

			continue;
		}
		//if solid leaf
		if ( !nodenum ) {
			continue;
		}
		//the node to test against
		aas_node_t* aasnode = &aasworld->nodes[ nodenum ];
		//the current node plane
		aas_plane_t* plane = &aasworld->planes[ aasnode->planenum ];
		//get the side(s) the box is situated relative to the plane
		int side = AAS_BoxOnPlaneSide2( absmins, absmaxs, plane );
		//if on the front side of the node
		if ( side & 1 ) {
			lstack_p->nodenum = aasnode->children[ 0 ];
			lstack_p++;
		}
		if ( lstack_p >= &linkstack[ 127 ] ) {
			BotImport_Print( PRT_ERROR, "AAS_LinkEntity: stack overflow\n" );
			break;
		}
		//if on the back side of the node
		if ( side & 2 ) {
			lstack_p->nodenum = aasnode->children[ 1 ];
			lstack_p++;
		}
		if ( lstack_p >= &linkstack[ 127 ] ) {
			BotImport_Print( PRT_ERROR, "AAS_LinkEntity: stack overflow\n" );
			break;
		}
	}
	return areas;
}

aas_link_t* AAS_LinkEntityClientBBox( const vec3_t absmins, const vec3_t absmaxs, int entnum, int presencetype ) {
	vec3_t mins, maxs;
	AAS_PresenceTypeBoundingBox( presencetype, mins, maxs );
	vec3_t newabsmins;
	VectorSubtract( absmins, maxs, newabsmins );
	vec3_t newabsmaxs;
	VectorSubtract( absmaxs, mins, newabsmaxs );
	//relink the entity
	return AAS_AASLinkEntity( newabsmins, newabsmaxs, entnum );
}

aas_plane_t* AAS_PlaneFromNum( int planenum ) {
	if ( !aasworld->loaded ) {
		return 0;
	}

	return &aasworld->planes[ planenum ];
}

static int AAS_BBoxAreasCheckCache( const vec3_t absmins, const vec3_t absmaxs, int* areas, int maxareas ) {
	// is this absmins/absmax in the cache?
	bboxAreasCache_t* cache = bboxAreasCache;
	int i;
	for ( i = 0; i < NUM_BBOXAREASCACHE; i++, cache++ ) {
		if ( VectorCompare( absmins, cache->absmins ) && VectorCompare( absmaxs, cache->absmaxs ) ) {
			// found a match
			break;
		}
	}

	if ( i == NUM_BBOXAREASCACHE ) {
		return 0;
	}

	// use this cache
	cache->lastUsedTime = AAS_Time();
	cache->numUsed++;
	if ( cache->numUsed > 99999 ) {
		cache->numUsed = 99999;	// cap it so it doesn't loop back to 0
	}
	if ( cache->numAreas > maxareas ) {
		for ( i = 0; i < maxareas; i++ )
			areas[ i ] = ( int )cache->areas[ i ];
		return maxareas;
	} else   {
		Com_Memcpy( areas, cache->areas, sizeof ( int ) * cache->numAreas );
		return cache->numAreas;
	}
}

static void AAS_BBoxAreasAddToCache( const vec3_t absmins, const vec3_t absmaxs, int* areas, int numareas ) {
	// find a free cache slot
	bboxAreasCache_t* weakestLink = NULL;
	bboxAreasCache_t* cache = bboxAreasCache;
	int i;
	for ( i = 0; i < NUM_BBOXAREASCACHE; i++, cache++ ) {
		if ( !cache->lastUsedTime ) {
			break;
		}
		if ( cache->lastUsedTime < AAS_Time() - 2.0 ) {
			break;	// too old
		}

		if ( !weakestLink ) {
			weakestLink = cache;
		} else   {
			if ( cache->numUsed < weakestLink->numUsed ) {
				weakestLink = cache;
			}
		}
	}

	if ( i == NUM_BBOXAREASCACHE ) {
		// overwrite the weakest link
		cache = weakestLink;
	}

	cache->lastUsedTime = AAS_Time();
	cache->numUsed = 1;
	VectorCopy( absmins, cache->absmins );
	VectorCopy( absmaxs, cache->absmaxs );

	if ( numareas > BBOXAREASCACHE_MAXAREAS ) {
		numareas = BBOXAREASCACHE_MAXAREAS;
	}

	for ( i = 0; i < numareas; i++ ) {
		cache->areas[ i ] = ( unsigned short )areas[ i ];
	}
}

int AAS_BBoxAreas( const vec3_t absmins, const vec3_t absmaxs, int* areas, int maxareas ) {
	aas_link_t* linkedareas, * link;
	int num;

	if ( GGameType & GAME_ET && ( num = AAS_BBoxAreasCheckCache( absmins, absmaxs, areas, maxareas ) ) ) {
		return num;
	}

	linkedareas = AAS_AASLinkEntity( absmins, absmaxs, -1 );
	num = 0;
	for ( link = linkedareas; link; link = link->next_area ) {
		areas[ num ] = link->areanum;
		num++;
		if ( num >= maxareas ) {
			break;
		}
	}

	AAS_UnlinkFromAreas( linkedareas );

	if ( GGameType & GAME_ET ) {
		//record this result in the cache
		AAS_BBoxAreasAddToCache( absmins, absmaxs, areas, num );
	}

	return num;
}

int AAS_AreaInfo( int areanum, aas_areainfo_t* info ) {
	if ( !info ) {
		return 0;
	}
	if ( areanum <= 0 || areanum >= aasworld->numareas ) {
		BotImport_Print( PRT_ERROR, "AAS_AreaInfo: areanum %d out of range\n", areanum );
		return 0;
	}
	aas_areasettings_t* settings = &aasworld->areasettings[ areanum ];
	info->cluster = settings->cluster;
	info->contents = settings->contents;
	info->flags = settings->areaflags;
	info->presencetype = settings->presencetype;
	VectorCopy( aasworld->areas[ areanum ].mins, info->mins );
	VectorCopy( aasworld->areas[ areanum ].maxs, info->maxs );
	VectorCopy( aasworld->areas[ areanum ].center, info->center );
	return sizeof ( aas_areainfo_t );
}

void AAS_AreaCenter( int areanum, vec3_t center ) {
	if ( areanum < 0 || areanum >= aasworld->numareas ) {
		BotImport_Print( PRT_ERROR, "AAS_AreaCenter: invalid areanum\n" );
		return;
	}
	VectorCopy( aasworld->areas[ areanum ].center, center );
}

bool AAS_AreaWaypoint( int areanum, vec3_t center ) {
	if ( areanum < 0 || areanum >= aasworld->numareas ) {
		BotImport_Print( PRT_ERROR, "AAS_AreaWaypoint: invalid areanum\n" );
		return false;
	}
	if ( !aasworld->areawaypoints ) {
		return false;
	}
	if ( VectorCompare( aasworld->areawaypoints[ areanum ], vec3_origin ) ) {
		return false;
	}
	VectorCopy( aasworld->areawaypoints[ areanum ], center );
	return true;
}

static bool AAS_AreaEntityCollision( int areanum, const vec3_t start, const vec3_t end,
	int presencetype, int passent, aas_trace_t* trace ) {
	vec3_t boxmins, boxmaxs;
	AAS_PresenceTypeBoundingBox( presencetype, boxmins, boxmaxs );

	bsp_trace_t bsptrace;
	Com_Memset( &bsptrace, 0, sizeof ( bsp_trace_t ) );	//make compiler happy
	//assume no collision
	bsptrace.fraction = 1;
	bool collision = false;
	for ( aas_link_t* link = aasworld->arealinkedentities[ areanum ]; link; link = link->next_ent ) {
		//ignore the pass entity
		if ( link->entnum == passent ) {
			continue;
		}

		if ( AAS_EntityCollision( link->entnum, start, boxmins, boxmaxs, end,
				 BSP46CONTENTS_SOLID | BSP46CONTENTS_PLAYERCLIP, &bsptrace ) ) {
			collision = true;
		}
	}
	if ( collision ) {
		trace->startsolid = bsptrace.startsolid;
		trace->ent = bsptrace.ent;
		VectorCopy( bsptrace.endpos, trace->endpos );
		trace->area = 0;
		trace->planenum = 0;
		return true;
	}
	return false;
}

// recursive subdivision of the line by the BSP tree.
aas_trace_t AAS_TraceClientBBox( const vec3_t start, const vec3_t end, int presencetype,
	int passent ) {
	//clear the trace structure
	aas_trace_t trace;
	Com_Memset( &trace, 0, sizeof ( aas_trace_t ) );
	if ( GGameType & GAME_ET ) {
		trace.ent = Q3ENTITYNUM_NONE;
	}

	if ( !aasworld->loaded ) {
		return trace;
	}

	aas_tracestack_t tracestack[ 127 ];
	aas_tracestack_t* tstack_p = tracestack;
	//we start with the whole line on the stack
	VectorCopy( start, tstack_p->start );
	VectorCopy( end, tstack_p->end );
	tstack_p->planenum = 0;
	//start with node 1 because node zero is a dummy for a solid leaf
	tstack_p->nodenum = 1;		//starting at the root of the tree
	tstack_p++;

	while ( 1 ) {
		//pop up the stack
		tstack_p--;
		//if the trace stack is empty (ended up with a piece of the
		//line to be traced in an area)
		if ( tstack_p < tracestack ) {
			tstack_p++;
			//nothing was hit
			trace.startsolid = false;
			trace.fraction = 1.0;
			//endpos is the end of the line
			VectorCopy( end, trace.endpos );
			//nothing hit
			trace.ent = GGameType & GAME_ET ? Q3ENTITYNUM_NONE : 0;
			trace.area = 0;
			trace.planenum = 0;
			return trace;
		}
		//number of the current node to test the line against
		int nodenum = tstack_p->nodenum;
		//if it is an area
		if ( nodenum < 0 ) {
			//if can't enter the area because it hasn't got the right presence type
			if ( !( GGameType & GAME_ET ) && !( aasworld->areasettings[ -nodenum ].presencetype & presencetype ) ) {
				//if the start point is still the initial start point
				//NOTE: no need for epsilons because the points will be
				//exactly the same when they're both the start point
				vec3_t v1, v2;
				if ( tstack_p->start[ 0 ] == start[ 0 ] &&
					 tstack_p->start[ 1 ] == start[ 1 ] &&
					 tstack_p->start[ 2 ] == start[ 2 ] ) {
					trace.startsolid = true;
					trace.fraction = 0.0;
					VectorClear( v1 );
				} else   {
					trace.startsolid = false;
					VectorSubtract( end, start, v1 );
					VectorSubtract( tstack_p->start, start, v2 );
					trace.fraction = VectorLength( v2 ) / VectorNormalize( v1 );
					VectorMA( tstack_p->start, -0.125, v1, tstack_p->start );
				}
				VectorCopy( tstack_p->start, trace.endpos );
				trace.ent = 0;
				trace.area = -nodenum;
				trace.planenum = tstack_p->planenum;
				//always take the plane with normal facing towards the trace start
				aas_plane_t* plane = &aasworld->planes[ trace.planenum ];
				if ( DotProduct( v1, plane->normal ) > 0 ) {
					trace.planenum ^= 1;
				}
				return trace;
			} else   {
				if ( passent >= 0 ) {
					if ( AAS_AreaEntityCollision( -nodenum, tstack_p->start,
							 tstack_p->end, presencetype, passent, &trace ) ) {
						if ( !trace.startsolid ) {
							vec3_t v1, v2;
							VectorSubtract( end, start, v1 );
							VectorSubtract( trace.endpos, start, v2 );
							trace.fraction = VectorLength( v2 ) / VectorLength( v1 );
						}
						return trace;
					}
				}
			}
			trace.lastarea = -nodenum;
			continue;
		}
		//if it is a solid leaf
		if ( !nodenum ) {
			//if the start point is still the initial start point
			//NOTE: no need for epsilons because the points will be
			//exactly the same when they're both the start point
			vec3_t v1;
			if ( tstack_p->start[ 0 ] == start[ 0 ] &&
				 tstack_p->start[ 1 ] == start[ 1 ] &&
				 tstack_p->start[ 2 ] == start[ 2 ] ) {
				trace.startsolid = true;
				trace.fraction = 0.0;
				VectorClear( v1 );
			} else   {
				trace.startsolid = false;
				VectorSubtract( end, start, v1 );
				vec3_t v2;
				VectorSubtract( tstack_p->start, start, v2 );
				trace.fraction = VectorLength( v2 ) / VectorNormalize( v1 );
				VectorMA( tstack_p->start, -0.125, v1, tstack_p->start );
			}
			VectorCopy( tstack_p->start, trace.endpos );
			trace.ent = GGameType & GAME_ET ? Q3ENTITYNUM_NONE : 0;
			trace.area = 0;	//hit solid leaf
			trace.planenum = tstack_p->planenum;
			//always take the plane with normal facing towards the trace start
			aas_plane_t* plane = &aasworld->planes[ trace.planenum ];
			if ( DotProduct( v1, plane->normal ) > 0 ) {
				trace.planenum ^= 1;
			}
			return trace;
		}
		//the node to test against
		aas_node_t* aasnode = &aasworld->nodes[ nodenum ];
		//start point of current line to test against node
		vec3_t cur_start;
		VectorCopy( tstack_p->start, cur_start );
		//end point of the current line to test against node
		vec3_t cur_end;
		VectorCopy( tstack_p->end, cur_end );
		//the current node plane
		aas_plane_t* plane = &aasworld->planes[ aasnode->planenum ];

		float front = DotProduct( cur_start, plane->normal ) - plane->dist;
		float back = DotProduct( cur_end, plane->normal ) - plane->dist;

		//if the whole to be traced line is totally at the front of this node
		//only go down the tree with the front child
		if ( ( front >= -ON_EPSILON && back >= -ON_EPSILON ) ) {
			//keep the current start and end point on the stack
			//and go down the tree with the front child
			tstack_p->nodenum = aasnode->children[ 0 ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
		}
		//if the whole to be traced line is totally at the back of this node
		//only go down the tree with the back child
		else if ( ( front < ON_EPSILON && back < ON_EPSILON ) ) {
			//keep the current start and end point on the stack
			//and go down the tree with the back child
			tstack_p->nodenum = aasnode->children[ 1 ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
		}
		//go down the tree both at the front and back of the node
		else {
			int tmpplanenum = tstack_p->planenum;
			if ( front == back ) {
				front -= 0.001f;
			}
			//calculate the hitpoint with the node (split point of the line)
			//put the crosspoint TRACEPLANE_EPSILON pixels on the near side
			float frac;
			if ( front < 0 ) {
				frac = ( front + TRACEPLANE_EPSILON ) / ( front - back );
			} else   {
				frac = ( front - TRACEPLANE_EPSILON ) / ( front - back );
			}
			if ( frac < 0 ) {
				frac = 0.001f;
			} else if ( frac > 1 )     {
				frac = 0.999f;
			}

			vec3_t cur_mid;
			cur_mid[ 0 ] = cur_start[ 0 ] + ( cur_end[ 0 ] - cur_start[ 0 ] ) * frac;
			cur_mid[ 1 ] = cur_start[ 1 ] + ( cur_end[ 1 ] - cur_start[ 1 ] ) * frac;
			cur_mid[ 2 ] = cur_start[ 2 ] + ( cur_end[ 2 ] - cur_start[ 2 ] ) * frac;

			//side the front part of the line is on
			int side = front < 0;
			//first put the end part of the line on the stack (back side)
			VectorCopy( cur_mid, tstack_p->start );
			//not necesary to store because still on stack
			//VectorCopy(cur_end, tstack_p->end);
			tstack_p->planenum = aasnode->planenum;
			tstack_p->nodenum = aasnode->children[ !side ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
			//now put the part near the start of the line on the stack so we will
			//continue with thats part first. This way we'll find the first
			//hit of the bbox
			VectorCopy( cur_start, tstack_p->start );
			VectorCopy( cur_mid, tstack_p->end );
			tstack_p->planenum = tmpplanenum;
			tstack_p->nodenum = aasnode->children[ side ];
			tstack_p++;
			if ( tstack_p >= &tracestack[ 127 ] ) {
				BotImport_Print( PRT_ERROR, "AAS_TraceBoundingBox: stack overflow\n" );
				return trace;
			}
		}
	}
}
