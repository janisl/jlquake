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
//**	BOX TRACING
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "../../Common.h"
#include "local.h"

// MACROS ------------------------------------------------------------------

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON    ( 0.03125 )

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QClipMap38::BoxTraceQ2
//
//==========================================================================

q2trace_t QClipMap38::BoxTraceQ2( const vec3_t Start, const vec3_t End,
	const vec3_t Mins, const vec3_t Maxs, clipHandle_t Model, int BrushMask ) {
	cmodel_t* cmod = ClipHandleToModel( Model );
	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	Com_Memset( &trace_trace, 0, sizeof ( trace_trace ) );
	trace_trace.fraction = 1;
	trace_trace.surface = &( QClipMap38::nullsurface.c );

	if ( !numnodes ) {	// map not loaded
		return trace_trace;
	}

	trace_contents = BrushMask;
	VectorCopy( Start, trace_start );
	VectorCopy( End, trace_end );
	VectorCopy( Mins, trace_mins );
	VectorCopy( Maxs, trace_maxs );

	//
	// check for position test special case
	//
	if ( Start[ 0 ] == End[ 0 ] && Start[ 1 ] == End[ 1 ] && Start[ 2 ] == End[ 2 ] ) {
		int leafs[ 1024 ];

		leafList_t ll;

		VectorAdd( Start, Mins, ll.bounds[ 0 ] );
		VectorAdd( Start, Maxs, ll.bounds[ 1 ] );
		for ( int i = 0; i < 3; i++ ) {
			ll.bounds[ 0 ][ i ] -= 1;
			ll.bounds[ 1 ][ i ] += 1;
		}

		ll.list = leafs;
		ll.count = 0;
		ll.maxcount = 1024;
		ll.topnode = -1;
		ll.lastLeaf = 0;

		BoxLeafnums_r( &ll, cmod->headnode );

		for ( int i = 0; i < ll.count; i++ ) {
			TestInLeaf( leafs[ i ] );
			if ( trace_trace.allsolid ) {
				break;
			}
		}
		VectorCopy( Start, trace_trace.endpos );
		return trace_trace;
	}

	//
	// check for point special case
	//
	if ( Mins[ 0 ] == 0 && Mins[ 1 ] == 0 && Mins[ 2 ] == 0 &&
		 Maxs[ 0 ] == 0 && Maxs[ 1 ] == 0 && Maxs[ 2 ] == 0 ) {
		trace_ispoint = true;
		VectorClear( trace_extents );
	} else   {
		trace_ispoint = false;
		trace_extents[ 0 ] = -Mins[ 0 ] > Maxs[ 0 ] ? -Mins[ 0 ] : Maxs[ 0 ];
		trace_extents[ 1 ] = -Mins[ 1 ] > Maxs[ 1 ] ? -Mins[ 1 ] : Maxs[ 1 ];
		trace_extents[ 2 ] = -Mins[ 2 ] > Maxs[ 2 ] ? -Mins[ 2 ] : Maxs[ 2 ];
	}

	//
	// general sweeping through world
	//
	RecursiveHullCheck( cmod->headnode, 0, 1, Start, End );

	if ( trace_trace.fraction == 1 ) {
		VectorCopy( End, trace_trace.endpos );
	} else   {
		for ( int i = 0; i < 3; i++ ) {
			trace_trace.endpos[ i ] = Start[ i ] + trace_trace.fraction * ( End[ i ] - Start[ i ] );
		}
	}
	return trace_trace;
}

//==========================================================================
//
//	QClipMap38::TransformedBoxTraceQ2
//
//	Handles offseting and rotation of the end points for moving and
// rotating entities
//
//==========================================================================

q2trace_t QClipMap38::TransformedBoxTraceQ2( const vec3_t Start, const vec3_t End,
	const vec3_t Mins, const vec3_t Maxs,
	clipHandle_t Model, int BrushMask,
	const vec3_t Origin, const vec3_t Angles ) {
	// subtract origin offset
	vec3_t start_l, end_l;
	VectorSubtract( Start, Origin, start_l );
	VectorSubtract( End, Origin, end_l );

	// rotate start and end into the models frame of reference
	bool rotated = Model != BOX_HANDLE && ( Angles[ 0 ] || Angles[ 1 ] || Angles[ 2 ] );

	if ( rotated ) {
		vec3_t forward, right, up;
		AngleVectors( Angles, forward, right, up );

		vec3_t temp;
		VectorCopy( start_l, temp );
		start_l[ 0 ] = DotProduct( temp, forward );
		start_l[ 1 ] = -DotProduct( temp, right );
		start_l[ 2 ] = DotProduct( temp, up );

		VectorCopy( end_l, temp );
		end_l[ 0 ] = DotProduct( temp, forward );
		end_l[ 1 ] = -DotProduct( temp, right );
		end_l[ 2 ] = DotProduct( temp, up );
	}

	// sweep the box through the model
	q2trace_t trace = BoxTraceQ2( start_l, end_l, Mins, Maxs, Model, BrushMask );

	if ( rotated && trace.fraction != 1.0 ) {
		// FIXME: figure out how to do this with existing angles
		vec3_t a;
		VectorNegate( Angles, a );
		vec3_t forward, right, up;
		AngleVectors( a, forward, right, up );

		vec3_t temp;
		VectorCopy( trace.plane.normal, temp );
		trace.plane.normal[ 0 ] = DotProduct( temp, forward );
		trace.plane.normal[ 1 ] = -DotProduct( temp, right );
		trace.plane.normal[ 2 ] = DotProduct( temp, up );
	}

	trace.endpos[ 0 ] = Start[ 0 ] + trace.fraction * ( End[ 0 ] - Start[ 0 ] );
	trace.endpos[ 1 ] = Start[ 1 ] + trace.fraction * ( End[ 1 ] - Start[ 1 ] );
	trace.endpos[ 2 ] = Start[ 2 ] + trace.fraction * ( End[ 2 ] - Start[ 2 ] );

	return trace;
}

//==========================================================================
//
//	QClipMap38::RecursiveHullCheck
//
//==========================================================================

void QClipMap38::RecursiveHullCheck( int num, float p1f, float p2f, const vec3_t p1, const vec3_t p2 ) {
	cnode_t* node;
	cplane_t* plane;
	float t1, t2, offset;
	float frac, frac2;
	float idist;
	int i;
	vec3_t mid;
	int side;
	float midf;

	if ( trace_trace.fraction <= p1f ) {
		return;					// already hit something nearer
	}

	// if < 0, we are in a leaf node
	if ( num < 0 ) {
		TraceToLeaf( -1 - num );
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = nodes + num;
	plane = node->plane;

	if ( plane->type < 3 ) {
		t1 = p1[ plane->type ] - plane->dist;
		t2 = p2[ plane->type ] - plane->dist;
		offset = trace_extents[ plane->type ];
	} else   {
		t1 = DotProduct( plane->normal, p1 ) - plane->dist;
		t2 = DotProduct( plane->normal, p2 ) - plane->dist;
		if ( trace_ispoint ) {
			offset = 0;
		} else   {
			offset = fabs( trace_extents[ 0 ] * plane->normal[ 0 ] ) +
					 fabs( trace_extents[ 1 ] * plane->normal[ 1 ] ) +
					 fabs( trace_extents[ 2 ] * plane->normal[ 2 ] );
		}
	}

	// see which sides we need to consider
	if ( t1 >= offset && t2 >= offset ) {
		RecursiveHullCheck( node->children[ 0 ], p1f, p2f, p1, p2 );
		return;
	}
	if ( t1 < -offset && t2 < -offset ) {
		RecursiveHullCheck( node->children[ 1 ], p1f, p2f, p1, p2 );
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if ( t1 < t2 ) {
		idist = 1.0 / ( t1 - t2 );
		side = 1;
		frac2 = ( t1 + offset + DIST_EPSILON ) * idist;
		frac = ( t1 - offset + DIST_EPSILON ) * idist;
	} else if ( t1 > t2 )     {
		idist = 1.0 / ( t1 - t2 );
		side = 0;
		frac2 = ( t1 - offset - DIST_EPSILON ) * idist;
		frac = ( t1 + offset + DIST_EPSILON ) * idist;
	} else   {
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if ( frac < 0 ) {
		frac = 0;
	}
	if ( frac > 1 ) {
		frac = 1;
	}

	midf = p1f + ( p2f - p1f ) * frac;
	for ( i = 0; i < 3; i++ ) {
		mid[ i ] = p1[ i ] + frac * ( p2[ i ] - p1[ i ] );
	}

	RecursiveHullCheck( node->children[ side ], p1f, midf, p1, mid );

	// go past the node
	if ( frac2 < 0 ) {
		frac2 = 0;
	}
	if ( frac2 > 1 ) {
		frac2 = 1;
	}

	midf = p1f + ( p2f - p1f ) * frac2;
	for ( i = 0; i < 3; i++ ) {
		mid[ i ] = p1[ i ] + frac2 * ( p2[ i ] - p1[ i ] );
	}

	RecursiveHullCheck( node->children[ side ^ 1 ], midf, p2f, mid, p2 );
}

//==========================================================================
//
//	QClipMap38::TraceToLeaf
//
//==========================================================================

void QClipMap38::TraceToLeaf( int leafnum ) {
	cleaf_t* leaf = &leafs[ leafnum ];
	if ( !( leaf->contents & trace_contents ) ) {
		return;
	}
	// trace line against all brushes in the leaf
	for ( int k = 0; k < leaf->numleafbrushes; k++ ) {
		int brushnum = leafbrushes[ leaf->firstleafbrush + k ];
		cbrush_t* b = &brushes[ brushnum ];
		if ( b->checkcount == checkcount ) {
			continue;			// already checked this brush in another leaf
		}
		b->checkcount = checkcount;

		if ( !( b->contents & trace_contents ) ) {
			continue;
		}
		ClipBoxToBrush( trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b );
		if ( !trace_trace.fraction ) {
			return;
		}
	}

}

//==========================================================================
//
//	QClipMap38::ClipBoxToBrush
//
//==========================================================================

void QClipMap38::ClipBoxToBrush( vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, q2trace_t* trace, cbrush_t* brush ) {
	int i, j;
	cplane_t* plane, * clipplane;
	float dist;
	float enterfrac, leavefrac;
	vec3_t ofs;
	float d1, d2;
	qboolean getout, startout;
	float f;
	cbrushside_t* side, * leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if ( !brush->numsides ) {
		return;
	}

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for ( i = 0; i < brush->numsides; i++ ) {
		side = &brushsides[ brush->firstbrushside + i ];
		plane = side->plane;

		// FIXME: special case for axial

		if ( !trace_ispoint ) {		// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for ( j = 0; j < 3; j++ ) {
				if ( plane->normal[ j ] < 0 ) {
					ofs[ j ] = maxs[ j ];
				} else   {
					ofs[ j ] = mins[ j ];
				}
			}
			dist = DotProduct( ofs, plane->normal );
			dist = plane->dist - dist;
		} else   {					// special point case
			dist = plane->dist;
		}

		d1 = DotProduct( p1, plane->normal ) - dist;
		d2 = DotProduct( p2, plane->normal ) - dist;

		if ( d2 > 0 ) {
			getout = true;		// endpoint is not in solid
		}
		if ( d1 > 0 ) {
			startout = true;
		}

		// if completely in front of face, no intersection
		if ( d1 > 0 && d2 >= d1 ) {
			return;
		}

		if ( d1 <= 0 && d2 <= 0 ) {
			continue;
		}

		// crosses face
		if ( d1 > d2 ) {			// enter
			f = ( d1 - DIST_EPSILON ) / ( d1 - d2 );
			if ( f > enterfrac ) {
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		} else   {					// leave
			f = ( d1 + DIST_EPSILON ) / ( d1 - d2 );
			if ( f < leavefrac ) {
				leavefrac = f;
			}
		}
	}

	if ( !startout ) {				// original point was inside brush
		trace->startsolid = true;
		if ( !getout ) {
			trace->allsolid = true;
		}
		return;
	}
	if ( enterfrac < leavefrac ) {
		if ( enterfrac > -1 && enterfrac < trace->fraction ) {
			if ( enterfrac < 0 ) {
				enterfrac = 0;
			}
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = &( leadside->surface->c );
			trace->contents = brush->contents;
		}
	}
}

//==========================================================================
//
//	QClipMap38::TestInLeaf
//
//==========================================================================

void QClipMap38::TestInLeaf( int leafnum ) {
	cleaf_t* leaf = &leafs[ leafnum ];
	if ( !( leaf->contents & trace_contents ) ) {
		return;
	}
	// trace line against all brushes in the leaf
	for ( int k = 0; k < leaf->numleafbrushes; k++ ) {
		int brushnum = leafbrushes[ leaf->firstleafbrush + k ];
		cbrush_t* b = &brushes[ brushnum ];
		if ( b->checkcount == checkcount ) {
			continue;			// already checked this brush in another leaf
		}
		b->checkcount = checkcount;

		if ( !( b->contents & trace_contents ) ) {
			continue;
		}
		TestBoxInBrush( trace_mins, trace_maxs, trace_start, &trace_trace, b );
		if ( !trace_trace.fraction ) {
			return;
		}
	}
}

//==========================================================================
//
//  QClipMap38::TestBoxInBrush
//
//==========================================================================

void QClipMap38::TestBoxInBrush( vec3_t mins, vec3_t maxs, vec3_t p1, q2trace_t* trace, cbrush_t* brush ) {
	int i, j;
	cplane_t* plane;
	float dist;
	vec3_t ofs;
	float d1;
	cbrushside_t* side;

	if ( !brush->numsides ) {
		return;
	}

	for ( i = 0; i < brush->numsides; i++ ) {
		side = &brushsides[ brush->firstbrushside + i ];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for ( j = 0; j < 3; j++ ) {
			if ( plane->normal[ j ] < 0 ) {
				ofs[ j ] = maxs[ j ];
			} else   {
				ofs[ j ] = mins[ j ];
			}
		}
		dist = DotProduct( ofs, plane->normal );
		dist = plane->dist - dist;

		d1 = DotProduct( p1, plane->normal ) - dist;

		// if completely in front of face, no intersection
		if ( d1 > 0 ) {
			return;
		}
	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}

//==========================================================================
//
//  QClipMap38::HullCheckQ1
//
//==========================================================================

bool QClipMap38::HullCheckQ1( clipHandle_t Handle, vec3_t p1, vec3_t p2, q1trace_t* trace ) {
	common->Error( "Not implemented" );
	return false;
}

//==========================================================================
//
//	QClipMap38::BoxTraceQ3
//
//==========================================================================

void QClipMap38::BoxTraceQ3( q3trace_t* Results, const vec3_t Start, const vec3_t End,
	const vec3_t Mins, const vec3_t Maxs, clipHandle_t Model, int BrushMask, int Capsule ) {
	common->Error( "Not implemented" );
}

//==========================================================================
//
//	QClipMap38::TransformedBoxTraceQ3
//
//==========================================================================

void QClipMap38::TransformedBoxTraceQ3( q3trace_t* Results, const vec3_t Start,
	const vec3_t End, const vec3_t Mins, const vec3_t Maxs, clipHandle_t Model, int BrushMask,
	const vec3_t Origin, const vec3_t Angles, int Capsule ) {
	common->Error( "Not implemented" );
}
