//**************************************************************************
//**
//**	$Id$
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
/*

Issues for collision against curved surfaces:

Surface edges need to be handled differently than surface planes

Plane expansion causes raw surfaces to expand past expanded bounding box

Position test of a volume against a surface is tricky.

Position test of a point against a surface is not well defined, because the surface has no volume.


Tracing leading edge points instead of volumes?
Position test by tracing corner to corner? (8*7 traces -- ouch)

coplanar edges
triangulated patches
degenerate patches

  endcaps
  degenerate

WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/

// HEADER FILES ------------------------------------------------------------

#include "core.h"
#include "cm46_local.h"

// MACROS ------------------------------------------------------------------

#define SUBDIVIDE_DISTANCE	16	//4	// never more than this units away from curve
#define WRAP_POINT_EPSILON	0.1
#define POINT_EPSILON		0.1

#define NORMAL_EPSILON		0.0001
#define DIST_EPSILON		0.02

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

vec3_t		debugBlockPoints[4];

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static int				cm_patch_numFacets;
static facet_t			cm_patch_facets[MAX_PATCH_PLANES]; //maybe MAX_FACETS ??

static int				cm_patch_numPlanes;
static patchPlane_t		cm_patch_planes[MAX_PATCH_PLANES];

static bool				debugBlock;

// CODE --------------------------------------------------------------------

/*
================================================================================

GRID SUBDIVISION

================================================================================
*/

//==========================================================================
//
//	cGrid_t::SetWrapWidth
//
//	If the left and right columns are exactly equal, set grid->wrapWidth true
//
//==========================================================================

void cGrid_t::SetWrapWidth()
{
	int i;
	for (i = 0 ; i < height; i++)
	{
		int j;
		for (j = 0; j < 3; j++)
		{
			float d = points[0][i][j] - points[width - 1][i][j];
			if (d < -WRAP_POINT_EPSILON || d > WRAP_POINT_EPSILON)
			{
				break;
			}
		}
		if (j != 3)
		{
			break;
		}
	}
	if (i == height)
	{
		wrapWidth = true;
	}
	else
	{
		wrapWidth = false;
	}
}

//==========================================================================
//
//	cGrid_t::SubdivideColumns
//
//	Adds columns as necessary to the grid until all the aproximating points\
// are within SUBDIVIDE_DISTANCE from the true curve
//
//==========================================================================

void cGrid_t::SubdivideColumns()
{
	for (int i = 0; i < width - 2; )
	{
		// grid->points[i][x] is an interpolating control point
		// grid->points[i+1][x] is an aproximating control point
		// grid->points[i+2][x] is an interpolating control point

		//
		// first see if we can collapse the aproximating collumn away
		//
		int j;
		for (j = 0; j < height; j++)
		{
			if (NeedsSubdivision(points[i][j], points[i + 1][j], points[i + 2][j]))
			{
				break;
			}
		}
		if (j == height)
		{
			// all of the points were close enough to the linear midpoints
			// that we can collapse the entire column away
			for (j = 0; j < height; j++)
			{
				// remove the column
				for (int k = i + 2 ; k < width; k++)
				{
					VectorCopy(points[k][j], points[k - 1][j]);
				}
			}

			width--;

			// go to the next curve segment
			i++;
			continue;
		}

		//
		// we need to subdivide the curve
		//
		for (j = 0; j < height; j++)
		{
			vec3_t	prev, mid, next;

			// save the control points now
			VectorCopy(points[i][j], prev);
			VectorCopy(points[i + 1][j], mid);
			VectorCopy(points[i + 2][j], next);

			// make room for two additional columns in the grid
			// columns i+1 will be replaced, column i+2 will become i+4
			// i+1, i+2, and i+3 will be generated
			for (int k = width - 1; k > i + 1; k--)
			{
				VectorCopy(points[k][j], points[k + 2][j]);
			}

			// generate the subdivided points
			Subdivide(prev, mid, next, points[i + 1][j], points[i + 2][j], points[i + 3][j]);
		}

		width += 2;

		// the new aproximating point at i+1 may need to be removed
		// or subdivided farther, so don't advance i
	}
}

//==========================================================================
//
//	cGrid_t::NeedsSubdivision
//
//	Returns true if the given quadratic curve is not flat enough for our
// collision detection purposes
//
//==========================================================================

bool cGrid_t::NeedsSubdivision(vec3_t a, vec3_t b, vec3_t c)
{
	vec3_t		cmid;
	vec3_t		lmid;
	vec3_t		delta;

	// calculate the linear midpoint
	for (int i = 0; i < 3; i++)
	{
		lmid[i] = 0.5 * (a[i] + c[i]);
	}

	// calculate the exact curve midpoint
	for (int i = 0; i < 3; i++)
	{
		cmid[i] = 0.5 * (0.5 * (a[i] + b[i]) + 0.5 * (b[i] + c[i]));
	}

	// see if the curve is far enough away from the linear mid
	VectorSubtract(cmid, lmid, delta);
	float dist = VectorLength(delta);
	
	return dist >= SUBDIVIDE_DISTANCE;
}

//==========================================================================
//
//	cGrid_t::Subdivide
//
//	a, b, and c are control points.
//	the subdivided sequence will be: a, out1, out2, out3, c
//
//==========================================================================

void cGrid_t::Subdivide(vec3_t a, vec3_t b, vec3_t c, vec3_t out1, vec3_t out2, vec3_t out3)
{
	for (int i = 0; i < 3; i++)
	{
		out1[i] = 0.5 * (a[i] + b[i]);
		out3[i] = 0.5 * (b[i] + c[i]);
		out2[i] = 0.5 * (out1[i] + out3[i]);
	}
}

//==========================================================================
//
//	cGrid_t::RemoveDegenerateColumns
//
//	If there are any identical columns, remove them
//
//==========================================================================

void cGrid_t::RemoveDegenerateColumns()
{
	for (int i = 0; i < width - 1; i++)
	{
		int j;
		for (j = 0; j < height; j++)
		{
			if (!ComparePoints(points[i][j], points[i + 1][j]))
			{
				break;
			}
		}

		if (j != height)
		{
			continue;	// not degenerate
		}

		for (j = 0; j < height; j++)
		{
			// remove the column
			for (int k = i + 2; k < width; k++)
			{
				VectorCopy(points[k][j], points[k - 1][j]);
			}
		}
		width--;

		// check against the next column
		i--;
	}
}

//==========================================================================
//
//	cGrid_t::ComparePoints
//
//==========================================================================

bool cGrid_t::ComparePoints(const float* a, const float* b)
{
	float d = a[0] - b[0];
	if (d < -POINT_EPSILON || d > POINT_EPSILON)
	{
		return false;
	}
	d = a[1] - b[1];
	if (d < -POINT_EPSILON || d > POINT_EPSILON)
	{
		return false;
	}
	d = a[2] - b[2];
	if (d < -POINT_EPSILON || d > POINT_EPSILON)
	{
		return false;
	}
	return true;
}

//==========================================================================
//
//	cGrid_t::Transpose
//
//	Swaps the rows and columns in place
//
//==========================================================================

void cGrid_t::Transpose()
{
	if (width > height)
	{
		for (int i = 0; i < height; i++)
		{
			for (int j = i + 1; j < width; j++)
			{
				if (j < height)
				{
					// swap the value
					vec3_t temp;
					VectorCopy(points[i][j], temp);
					VectorCopy(points[j][i], points[i][j]);
					VectorCopy(temp, points[j][i]);
				}
				else
				{
					// just copy
					VectorCopy(points[j][i], points[i][j]);
				}
			}
		}
	}
	else
	{
		for (int i = 0; i < width; i++)
		{
			for (int j = i + 1; j < height; j++)
			{
				if (j < width)
				{
					// swap the value
					vec3_t temp;
					VectorCopy(points[j][i], temp);
					VectorCopy(points[i][j], points[j][i]);
					VectorCopy(temp, points[i][j]);
				}
				else
				{
					// just copy
					VectorCopy(points[i][j], points[j][i]);
				}
			}
		}
	}

	int l = width;
	width = height;
	height = l;

	bool tempWrap = wrapWidth;
	wrapWidth = wrapHeight;
	wrapHeight = tempWrap;
}

/*
================================================================================

PATCH COLLIDE GENERATION

================================================================================
*/

//==========================================================================
//
//	QClipMap46::GeneratePatchCollide
//
//	Creates an internal structure that will be used to perform
// collision detection with a patch mesh.
//
//	Points is packed as concatenated rows.
//
//==========================================================================

patchCollide_t* QClipMap46::GeneratePatchCollide(int width, int height, vec3_t* points)
{
	if (width <= 2 || height <= 2 || !points)
	{
		throw QDropException(va("CM_GeneratePatchFacets: bad parameters: (%i, %i, %p)",
			width, height, points));
	}

	if (!(width & 1) || !(height & 1))
	{
		throw QDropException("CM_GeneratePatchFacets: even sizes are invalid for quadratic meshes");
	}

	if (width > MAX_GRID_SIZE || height > MAX_GRID_SIZE)
	{
		throw QDropException("CM_GeneratePatchFacets: source is > MAX_GRID_SIZE");
	}

	// build a grid
	cGrid_t grid;
	grid.width = width;
	grid.height = height;
	grid.wrapWidth = false;
	grid.wrapHeight = false;
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			VectorCopy(points[j * width + i], grid.points[i][j]);
		}
	}

	// subdivide the grid
	grid.SetWrapWidth();
	grid.SubdivideColumns();
	grid.RemoveDegenerateColumns();

	grid.Transpose();

	grid.SetWrapWidth();
	grid.SubdivideColumns();
	grid.RemoveDegenerateColumns();

	// we now have a grid of points exactly on the curve
	// the aproximate surface defined by these points will be
	// collided against
	patchCollide_t* pf = new patchCollide_t;
	Com_Memset(pf, 0, sizeof(*pf));
	ClearBounds(pf->bounds[0], pf->bounds[1]);
	for (int i = 0; i < grid.width; i++)
	{
		for (int j = 0; j < grid.height; j++)
		{
			AddPointToBounds(grid.points[i][j], pf->bounds[0], pf->bounds[1]);
		}
	}

	// generate a bsp tree for the surface
	pf->FromGrid(&grid);

	// expand by one unit for epsilon purposes
	pf->bounds[0][0] -= 1;
	pf->bounds[0][1] -= 1;
	pf->bounds[0][2] -= 1;

	pf->bounds[1][0] += 1;
	pf->bounds[1][1] += 1;
	pf->bounds[1][2] += 1;

	return pf;
}

//==========================================================================
//
//	patchCollide_t::FromGrid
//
//==========================================================================

void patchCollide_t::FromGrid(cGrid_t* grid)
{
	enum edgeName_t
	{
		EN_TOP,
		EN_RIGHT,
		EN_BOTTOM,
		EN_LEFT
	};

	cm_patch_numPlanes = 0;
	cm_patch_numFacets = 0;

	// find the planes for each triangle of the grid
	int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2];
	for (int i = 0; i < grid->width - 1; i++)
	{
		for (int j = 0; j < grid->height - 1; j++)
		{
			float* p1 = grid->points[i][j];
			float* p2 = grid->points[i + 1][j];
			float* p3 = grid->points[i + 1][j + 1];
			gridPlanes[i][j][0] = FindPlane(p1, p2, p3);

			p1 = grid->points[i + 1][j + 1];
			p2 = grid->points[i][j + 1];
			p3 = grid->points[i][j];
			gridPlanes[i][j][1] = FindPlane(p1, p2, p3);
		}
	}

	// create the borders for each facet
	for (int i = 0; i < grid->width - 1; i++)
	{
		for (int j = 0; j < grid->height - 1; j++)
		{
			int borders[4];
			int noAdjust[4];

			borders[EN_TOP] = -1;
			if (j > 0)
			{
				borders[EN_TOP] = gridPlanes[i][j - 1][1];
			}
			else if (grid->wrapHeight)
			{
				borders[EN_TOP] = gridPlanes[i][grid->height - 2][1];
			} 
			noAdjust[EN_TOP] = (borders[EN_TOP] == gridPlanes[i][j][0]);
			if (borders[EN_TOP] == -1 || noAdjust[EN_TOP])
			{
				borders[EN_TOP] = EdgePlaneNum(grid, gridPlanes, i, j, 0);
			}

			borders[EN_BOTTOM] = -1;
			if (j < grid->height - 2)
			{
				borders[EN_BOTTOM] = gridPlanes[i][j + 1][0];
			}
			else if (grid->wrapHeight)
			{
				borders[EN_BOTTOM] = gridPlanes[i][0][0];
			}
			noAdjust[EN_BOTTOM] = (borders[EN_BOTTOM] == gridPlanes[i][j][1]);
			if (borders[EN_BOTTOM] == -1 || noAdjust[EN_BOTTOM])
			{
				borders[EN_BOTTOM] = EdgePlaneNum(grid, gridPlanes, i, j, 2);
			}

			borders[EN_LEFT] = -1;
			if (i > 0)
			{
				borders[EN_LEFT] = gridPlanes[i - 1][j][0];
			}
			else if (grid->wrapWidth)
			{
				borders[EN_LEFT] = gridPlanes[grid->width - 2][j][0];
			}
			noAdjust[EN_LEFT] = (borders[EN_LEFT] == gridPlanes[i][j][1]);
			if (borders[EN_LEFT] == -1 || noAdjust[EN_LEFT])
			{
				borders[EN_LEFT] = EdgePlaneNum(grid, gridPlanes, i, j, 3);
			}

			borders[EN_RIGHT] = -1;
			if (i < grid->width - 2)
			{
				borders[EN_RIGHT] = gridPlanes[i + 1][j][1];
			}
			else if (grid->wrapWidth)
			{
				borders[EN_RIGHT] = gridPlanes[0][j][1];
			}
			noAdjust[EN_RIGHT] = (borders[EN_RIGHT] == gridPlanes[i][j][0]);
			if (borders[EN_RIGHT] == -1 || noAdjust[EN_RIGHT])
			{
				borders[EN_RIGHT] = EdgePlaneNum(grid, gridPlanes, i, j, 1);
			}

			if (cm_patch_numFacets == MAX_FACETS)
			{
				throw QDropException("MAX_FACETS");
			}
			facet_t* facet = &cm_patch_facets[cm_patch_numFacets];
			Com_Memset(facet, 0, sizeof(*facet));

			if (gridPlanes[i][j][0] == gridPlanes[i][j][1])
			{
				if (gridPlanes[i][j][0] == -1)
				{
					continue;		// degenrate
				}
				facet->surfacePlane = gridPlanes[i][j][0];
				facet->numBorders = 4;
				facet->borderPlanes[0] = borders[EN_TOP];
				facet->borderNoAdjust[0] = noAdjust[EN_TOP];
				facet->borderPlanes[1] = borders[EN_RIGHT];
				facet->borderNoAdjust[1] = noAdjust[EN_RIGHT];
				facet->borderPlanes[2] = borders[EN_BOTTOM];
				facet->borderNoAdjust[2] = noAdjust[EN_BOTTOM];
				facet->borderPlanes[3] = borders[EN_LEFT];
				facet->borderNoAdjust[3] = noAdjust[EN_LEFT];
				SetBorderInward(facet, grid, gridPlanes, i, j, -1);
				if (ValidateFacet(facet))
				{
					AddFacetBevels(facet);
					cm_patch_numFacets++;
				}
			}
			else
			{
				// two seperate triangles
				facet->surfacePlane = gridPlanes[i][j][0];
				facet->numBorders = 3;
				facet->borderPlanes[0] = borders[EN_TOP];
				facet->borderNoAdjust[0] = noAdjust[EN_TOP];
				facet->borderPlanes[1] = borders[EN_RIGHT];
				facet->borderNoAdjust[1] = noAdjust[EN_RIGHT];
				facet->borderPlanes[2] = gridPlanes[i][j][1];
				if (facet->borderPlanes[2] == -1)
				{
					facet->borderPlanes[2] = borders[EN_BOTTOM];
					if (facet->borderPlanes[2] == -1)
					{
						facet->borderPlanes[2] = EdgePlaneNum(grid, gridPlanes, i, j, 4);
					}
				}
 				SetBorderInward(facet, grid, gridPlanes, i, j, 0);
				if (ValidateFacet(facet))
				{
					AddFacetBevels(facet);
					cm_patch_numFacets++;
				}

				if (cm_patch_numFacets == MAX_FACETS)
				{
					throw QDropException("MAX_FACETS");
				}
				facet = &cm_patch_facets[cm_patch_numFacets];
				Com_Memset(facet, 0, sizeof(*facet));

				facet->surfacePlane = gridPlanes[i][j][1];
				facet->numBorders = 3;
				facet->borderPlanes[0] = borders[EN_BOTTOM];
				facet->borderNoAdjust[0] = noAdjust[EN_BOTTOM];
				facet->borderPlanes[1] = borders[EN_LEFT];
				facet->borderNoAdjust[1] = noAdjust[EN_LEFT];
				facet->borderPlanes[2] = gridPlanes[i][j][0];
				if (facet->borderPlanes[2] == -1)
				{
					facet->borderPlanes[2] = borders[EN_TOP];
					if (facet->borderPlanes[2] == -1)
					{
						facet->borderPlanes[2] = EdgePlaneNum(grid, gridPlanes, i, j, 5);
					}
				}
				SetBorderInward(facet, grid, gridPlanes, i, j, 1);
				if (ValidateFacet(facet))
				{
					AddFacetBevels(facet);
					cm_patch_numFacets++;
				}
			}
		}
	}

	// copy the results out
	this->numPlanes = cm_patch_numPlanes;
	this->numFacets = cm_patch_numFacets;
	this->facets = new facet_t[cm_patch_numFacets];
	Com_Memcpy(this->facets, cm_patch_facets, cm_patch_numFacets * sizeof(*this->facets));
	this->planes = new patchPlane_t[cm_patch_numPlanes];
	Com_Memcpy(this->planes, cm_patch_planes, cm_patch_numPlanes * sizeof(*this->planes));
}

//==========================================================================
//
//	patchCollide_t::FindPlane
//
//==========================================================================

int patchCollide_t::FindPlane(float* p1, float* p2, float* p3)
{
	float	plane[4];

	if (!PlaneFromPoints(plane, p1, p2, p3))
	{
		return -1;
	}

	// see if the points are close enough to an existing plane
	for (int i = 0; i < cm_patch_numPlanes; i++)
	{
		if (DotProduct(plane, cm_patch_planes[i].plane) < 0)
		{
			continue;	// allow backwards planes?
		}

		float d = DotProduct(p1, cm_patch_planes[i].plane) - cm_patch_planes[i].plane[3];
		if (d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON)
		{
			continue;
		}

		d = DotProduct(p2, cm_patch_planes[i].plane) - cm_patch_planes[i].plane[3];
		if (d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON)
		{
			continue;
		}

		d = DotProduct(p3, cm_patch_planes[i].plane) - cm_patch_planes[i].plane[3];
		if (d < -PLANE_TRI_EPSILON || d > PLANE_TRI_EPSILON)
		{
			continue;
		}

		// found it
		return i;
	}

	// add a new plane
	if (cm_patch_numPlanes == MAX_PATCH_PLANES)
	{
		throw QDropException("MAX_PATCH_PLANES");
	}

	Vector4Copy(plane, cm_patch_planes[cm_patch_numPlanes].plane);
	cm_patch_planes[cm_patch_numPlanes].signbits = SignbitsForNormal(plane);

	cm_patch_numPlanes++;

	return cm_patch_numPlanes - 1;
}

//==========================================================================
//
//	patchCollide_t::SignbitsForNormal
//
//==========================================================================

int patchCollide_t::SignbitsForNormal(vec3_t normal)
{
	int bits = 0;
	for (int j = 0; j < 3; j++)
	{
		if (normal[j] < 0)
		{
			bits |= 1 << j;
		}
	}
	return bits;
}

//==========================================================================
//
//	patchCollide_t::EdgePlaneNum
//
//==========================================================================

int patchCollide_t::EdgePlaneNum(cGrid_t* grid, int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int k)
{
	float	*p1, *p2;
	vec3_t		up;
	int			p;

	switch (k)
	{
	case 0:	// top border
		p1 = grid->points[i][j];
		p2 = grid->points[i + 1][j];
		p = GridPlane(gridPlanes, i, j, 0);
		VectorMA(p1, 4, cm_patch_planes[p].plane, up);
		return FindPlane(p1, p2, up);

	case 2:	// bottom border
		p1 = grid->points[i][j + 1];
		p2 = grid->points[i + 1][j + 1];
		p = GridPlane(gridPlanes, i, j, 1);
		VectorMA(p1, 4, cm_patch_planes[p].plane, up);
		return FindPlane(p2, p1, up);

	case 3: // left border
		p1 = grid->points[i][j];
		p2 = grid->points[i][j + 1];
		p = GridPlane(gridPlanes, i, j, 1);
		VectorMA(p1, 4, cm_patch_planes[p].plane, up);
		return FindPlane(p2, p1, up);

	case 1:	// right border
		p1 = grid->points[i + 1][j];
		p2 = grid->points[i + 1][j + 1];
		p = GridPlane(gridPlanes, i, j, 0);
		VectorMA(p1, 4, cm_patch_planes[p].plane, up);
		return FindPlane(p1, p2, up);

	case 4:	// diagonal out of triangle 0
		p1 = grid->points[i + 1][j + 1];
		p2 = grid->points[i][j];
		p = GridPlane(gridPlanes, i, j, 0);
		VectorMA(p1, 4, cm_patch_planes[p].plane, up);
		return FindPlane(p1, p2, up);

	case 5:	// diagonal out of triangle 1
		p1 = grid->points[i][j];
		p2 = grid->points[i + 1][j + 1];
		p = GridPlane(gridPlanes, i, j, 1);
		VectorMA(p1, 4, cm_patch_planes[p].plane, up);
		return FindPlane(p1, p2, up);

	}

	throw QDropException("CM_EdgePlaneNum: bad k");
	return -1;
}

//==========================================================================
//
//	patchCollide_t::GridPlane
//
//==========================================================================

int	patchCollide_t::GridPlane(int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int tri)
{
	int p = gridPlanes[i][j][tri];
	if (p != -1)
	{
		return p;
	}
	p = gridPlanes[i][j][!tri];
	if (p != -1)
	{
		return p;
	}

	// should never happen
	GLog.Write("WARNING: CM_GridPlane unresolvable\n");
	return -1;
}

//==========================================================================
//
//	patchCollide_t::SetBorderInward
//
//==========================================================================

void patchCollide_t::SetBorderInward(facet_t* facet, cGrid_t* grid,
	int gridPlanes[MAX_GRID_SIZE][MAX_GRID_SIZE][2], int i, int j, int which)
{
	float*	points[4];
	int		numPoints;

	switch (which)
	{
	case -1:
		points[0] = grid->points[i][j];
		points[1] = grid->points[i + 1][j];
		points[2] = grid->points[i + 1][j + 1];
		points[3] = grid->points[i][j + 1];
		numPoints = 4;
		break;
	case 0:
		points[0] = grid->points[i][j];
		points[1] = grid->points[i + 1][j];
		points[2] = grid->points[i + 1][j + 1];
		numPoints = 3;
		break;
	case 1:
		points[0] = grid->points[i + 1][j + 1];
		points[1] = grid->points[i][j + 1];
		points[2] = grid->points[i][j];
		numPoints = 3;
		break;
	default:
		throw QException("CM_SetBorderInward: bad parameter");
	}

	for (int k = 0; k < facet->numBorders; k++)
	{
		int front = 0;
		int back = 0;

		for (int l = 0; l < numPoints; l++)
		{
			int side = PointOnPlaneSide(points[l], facet->borderPlanes[k]);
			if (side == SIDE_FRONT)
			{
				front++;
			}
			if (side == SIDE_BACK)
			{
				back++;
			}
		}

		if (front && !back)
		{
			facet->borderInward[k] = true;
		}
		else if (back && !front)
		{
			facet->borderInward[k] = false;
		}
		else if (!front && !back)
		{
			// flat side border
			facet->borderPlanes[k] = -1;
		}
		else
		{
			// bisecting side border
			GLog.DWrite("WARNING: CM_SetBorderInward: mixed plane sides\n" );
			facet->borderInward[k] = false;
			if (!debugBlock)
			{
				debugBlock = true;
				VectorCopy(grid->points[i][j], debugBlockPoints[0]);
				VectorCopy(grid->points[i + 1][j], debugBlockPoints[1]);
				VectorCopy(grid->points[i + 1][j + 1], debugBlockPoints[2]);
				VectorCopy(grid->points[i][j + 1], debugBlockPoints[3]);
			}
		}
	}
}

//==========================================================================
//
//	patchCollide_t::PointOnPlaneSide
//
//==========================================================================

int patchCollide_t::PointOnPlaneSide(const float* p, int planeNum)
{
	if (planeNum == -1)
	{
		return SIDE_ON;
	}
	const float* plane = cm_patch_planes[planeNum].plane;

	float d = DotProduct(p, plane) - plane[3];

	if (d > PLANE_TRI_EPSILON)
	{
		return SIDE_FRONT;
	}

	if (d < -PLANE_TRI_EPSILON)
	{
		return SIDE_BACK;
	}

	return SIDE_ON;
}

//==========================================================================
//
//	patchCollide_t::ValidateFacet
//
//	If the facet isn't bounded by its borders, we screwed up.
//
//==========================================================================

bool patchCollide_t::ValidateFacet(facet_t* facet)
{
	float		plane[4];
	vec3_t		bounds[2];

	if (facet->surfacePlane == -1)
	{
		return false;
	}

	Vector4Copy(cm_patch_planes[facet->surfacePlane].plane, plane);
	winding_t* w = CM46_BaseWindingForPlane(plane, plane[3]);
	for (int j = 0; j < facet->numBorders && w; j++)
	{
		if (facet->borderPlanes[j] == -1)
		{
			return false;
		}
		Vector4Copy(cm_patch_planes[facet->borderPlanes[j]].plane, plane);
		if (!facet->borderInward[j])
		{
			VectorSubtract(vec3_origin, plane, plane);
			plane[3] = -plane[3];
		}
		CM46_ChopWindingInPlace(&w, plane, plane[3], 0.1f);
	}

	if (!w)
	{
		return false;		// winding was completely chopped away
	}

	// see if the facet is unreasonably large
	CM46_WindingBounds(w, bounds[0], bounds[1]);
	CM46_FreeWinding(w);
	
	for (int j = 0; j < 3; j++)
	{
		if (bounds[1][j] - bounds[0][j] > MAX_MAP_BOUNDS)
		{
			return false;		// we must be missing a plane
		}
		if (bounds[0][j] >= MAX_MAP_BOUNDS)
		{
			return false;
		}
		if (bounds[1][j] <= -MAX_MAP_BOUNDS)
		{
			return false;
		}
	}
	return true;		// winding is fine
}

//==========================================================================
//
//	patchCollide_t::AddFacetBevels
//
//==========================================================================

void patchCollide_t::AddFacetBevels(facet_t* facet)
{
	float plane[4];
	Vector4Copy(cm_patch_planes[facet->surfacePlane].plane, plane);

	winding_t* w = CM46_BaseWindingForPlane(plane,  plane[3]);
	for (int j = 0; j < facet->numBorders && w; j++)
	{
		if (facet->borderPlanes[j] == facet->surfacePlane)
		{
			continue;
		}
		Vector4Copy(cm_patch_planes[facet->borderPlanes[j]].plane, plane);

		if (!facet->borderInward[j])
		{
			VectorSubtract(vec3_origin, plane, plane);
			plane[3] = -plane[3];
		}

		CM46_ChopWindingInPlace(&w, plane, plane[3], 0.1f);
	}
	if (!w)
	{
		return;
	}

	vec3_t mins, maxs;
	CM46_WindingBounds(w, mins, maxs);

	// add the axial planes
	int order = 0;
	for (int axis = 0; axis < 3; axis++)
	{
		for (int dir = -1; dir <= 1; dir += 2, order++)
		{
			VectorClear(plane);
			plane[axis] = dir;
			if (dir == 1)
			{
				plane[3] = maxs[axis];
			}
			else
			{
				plane[3] = -mins[axis];
			}
			//if it's the surface plane
			int flipped;
			if (PlaneEqual(&cm_patch_planes[facet->surfacePlane], plane, &flipped))
			{
				continue;
			}
			// see if the plane is allready present
			int i;
			for (i = 0; i < facet->numBorders; i++)
			{
				if (PlaneEqual(&cm_patch_planes[facet->borderPlanes[i]], plane, &flipped))
				{
					break;
				}
			}

			if (i == facet->numBorders)
			{
				if (facet->numBorders > 4 + 6 + 16)
				{
					GLog.Write("ERROR: too many bevels\n");
				}
				facet->borderPlanes[facet->numBorders] = FindPlane2(plane, &flipped);
				facet->borderNoAdjust[facet->numBorders] = 0;
				facet->borderInward[facet->numBorders] = flipped;
				facet->numBorders++;
			}
		}
	}
	//
	// add the edge bevels
	//
	// test the non-axial plane edges
	for (int j = 0 ; j < w->numpoints ; j++ )
	{
		int k = (j + 1) % w->numpoints;
		vec3_t vec;
		VectorSubtract(w->p[j], w->p[k], vec);
		//if it's a degenerate edge
		if (VectorNormalize(vec) < 0.5)
		{
			continue;
		}
		CM_SnapVector(vec);
		for (k = 0; k < 3 ; k++ )
		{
			if (vec[k] == -1 || vec[k] == 1)
			{
				break;	// axial
			}
		}
		if (k < 3)
		{
			continue;	// only test non-axial edges
		}

		// try the six possible slanted axials from this edge
		for (int axis = 0; axis < 3; axis++)
		{
			for (int dir = -1; dir <= 1; dir += 2)
			{
				// construct a plane
				vec3_t vec2;
				VectorClear(vec2);
				vec2[axis] = dir;
				CrossProduct(vec, vec2, plane);
				if (VectorNormalize(plane) < 0.5)
				{
					continue;
				}
				plane[3] = DotProduct(w->p[j], plane);

				// if all the points of the facet winding are
				// behind this plane, it is a proper edge bevel
				int l;
				for (l = 0; l < w->numpoints; l++)
				{
					float d = DotProduct (w->p[l], plane) - plane[3];
					if (d > 0.1)
					{
						break;	// point in front
					}
				}
				if (l < w->numpoints)
				{
					continue;
				}

				//if it's the surface plane
				int flipped;
				if (PlaneEqual(&cm_patch_planes[facet->surfacePlane], plane, &flipped))
				{
					continue;
				}
				// see if the plane is allready present
				int i;
				for (i = 0; i < facet->numBorders; i++)
				{
					if (PlaneEqual(&cm_patch_planes[facet->borderPlanes[i]], plane, &flipped))
					{
						break;
					}
				}

				if (i == facet->numBorders)
				{
					if (facet->numBorders > 4 + 6 + 16)
					{
						GLog.Write("ERROR: too many bevels\n");
					}
					facet->borderPlanes[facet->numBorders] = FindPlane2(plane, &flipped);

					for (k = 0; k < facet->numBorders; k++)
					{
						if (facet->borderPlanes[facet->numBorders] ==
							facet->borderPlanes[k])
						{
							GLog.Write("WARNING: bevel plane already used\n");
						}
					}

					facet->borderNoAdjust[facet->numBorders] = 0;
					facet->borderInward[facet->numBorders] = flipped;
					//
					winding_t* w2 = CM46_CopyWinding(w);
					float newplane[4];
					Vector4Copy(cm_patch_planes[facet->borderPlanes[facet->numBorders]].plane, newplane);
					if (!facet->borderInward[facet->numBorders])
					{
						VectorNegate(newplane, newplane);
						newplane[3] = -newplane[3];
					}
					CM46_ChopWindingInPlace(&w2, newplane, newplane[3], 0.1f);
					if (!w2)
					{
						GLog.DWrite("WARNING: CM_AddFacetBevels... invalid bevel\n");
						continue;
					}
					else
					{
						CM46_FreeWinding(w2);
					}
					//
					facet->numBorders++;
					//already got a bevel
//					break;
				}
			}
		}
	}
	CM46_FreeWinding(w);

	//add opposite plane
	facet->borderPlanes[facet->numBorders] = facet->surfacePlane;
	facet->borderNoAdjust[facet->numBorders] = 0;
	facet->borderInward[facet->numBorders] = true;
	facet->numBorders++;
}

//==========================================================================
//
//	patchCollide_t::PlaneEqual
//
//==========================================================================

bool patchCollide_t::PlaneEqual(patchPlane_t* p, float plane[4], int* flipped)
{
	if (fabs(p->plane[0] - plane[0]) < NORMAL_EPSILON &&
		fabs(p->plane[1] - plane[1]) < NORMAL_EPSILON &&
		fabs(p->plane[2] - plane[2]) < NORMAL_EPSILON &&
		fabs(p->plane[3] - plane[3]) < DIST_EPSILON)
	{
		*flipped = false;
		return true;
	}

	float invplane[4];
	VectorNegate(plane, invplane);
	invplane[3] = -plane[3];

	if (fabs(p->plane[0] - invplane[0]) < NORMAL_EPSILON &&
		fabs(p->plane[1] - invplane[1]) < NORMAL_EPSILON &&
		fabs(p->plane[2] - invplane[2]) < NORMAL_EPSILON &&
		fabs(p->plane[3] - invplane[3]) < DIST_EPSILON)
	{
		*flipped = true;
		return true;
	}

	return false;
}

//==========================================================================
//
//	patchCollide_t::FindPlane2
//
//==========================================================================

int patchCollide_t::FindPlane2(float plane[4], int* flipped)
{
	// see if the points are close enough to an existing plane
	for (int i = 0; i < cm_patch_numPlanes; i++)
	{
		if (PlaneEqual(&cm_patch_planes[i], plane, flipped))
		{
			return i;
		}
	}

	// add a new plane
	if (cm_patch_numPlanes == MAX_PATCH_PLANES)
	{
		throw QDropException("MAX_PATCH_PLANES");
	}

	Vector4Copy(plane, cm_patch_planes[cm_patch_numPlanes].plane);
	cm_patch_planes[cm_patch_numPlanes].signbits = SignbitsForNormal(plane);

	cm_patch_numPlanes++;

	*flipped = false;

	return cm_patch_numPlanes - 1;
}

//==========================================================================
//
//	patchCollide_t::CM_SnapVector
//
//==========================================================================

void patchCollide_t::CM_SnapVector(vec3_t normal)
{
	for (int i = 0; i < 3; i++)
	{
		if (fabs(normal[i] - 1) < NORMAL_EPSILON)
		{
			VectorClear(normal);
			normal[i] = 1;
			break;
		}
		if (fabs(normal[i] - -1) < NORMAL_EPSILON)
		{
			VectorClear(normal);
			normal[i] = -1;
			break;
		}
	}
}
