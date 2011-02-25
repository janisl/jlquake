/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
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
#include "cm_local.h"

/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void CM_FloodArea_r( int areaNum, int floodnum) {
	int		i;
	cArea_t *area;
	int		*con;

	area = &CMap->areas[ areaNum ];

	if ( area->floodvalid == CMap->floodvalid ) {
		if (area->floodnum == floodnum)
			return;
		throw QDropException("FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = CMap->floodvalid;
	con = CMap->areaPortals + areaNum * CMap->numAreas;
	for ( i=0 ; i < CMap->numAreas  ; i++ ) {
		if ( con[i] > 0 ) {
			CM_FloodArea_r( i, floodnum );
		}
	}
}

/*
====================
CM_FloodAreaConnections

====================
*/
void	CM_FloodAreaConnections( void ) {
	int		i;
	cArea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	CMap->floodvalid++;
	floodnum = 0;

	for (i = 0 ; i < CMap->numAreas ; i++) {
		area = &CMap->areas[i];
		if (area->floodvalid == CMap->floodvalid) {
			continue;		// already flooded into
		}
		floodnum++;
		CM_FloodArea_r (i, floodnum);
	}

}

/*
====================
CM_AdjustAreaPortalState

====================
*/
void	CM_AdjustAreaPortalState( int area1, int area2, qboolean open ) {
	if ( area1 < 0 || area2 < 0 ) {
		return;
	}

	if ( area1 >= CMap->numAreas || area2 >= CMap->numAreas ) {
		throw QDropException("CM_ChangeAreaPortalState: bad area number");
	}

	if ( open ) {
		CMap->areaPortals[ area1 * CMap->numAreas + area2 ]++;
		CMap->areaPortals[ area2 * CMap->numAreas + area1 ]++;
	} else {
		CMap->areaPortals[ area1 * CMap->numAreas + area2 ]--;
		CMap->areaPortals[ area2 * CMap->numAreas + area1 ]--;
		if ( CMap->areaPortals[ area2 * CMap->numAreas + area1 ] < 0 ) {
			throw QDropException("CM_AdjustAreaPortalState: negative reference count");
		}
	}

	CM_FloodAreaConnections ();
}

/*
====================
CM_AreasConnected

====================
*/
qboolean	CM_AreasConnected( int area1, int area2 ) {
	if ( cm_noAreas->integer ) {
		return true;
	}

	if ( area1 < 0 || area2 < 0 ) {
		return false;
	}

	if (area1 >= CMap->numAreas || area2 >= CMap->numAreas) {
		throw QDropException("area >= cm.numAreas");
	}

	if (CMap->areas[area1].floodnum == CMap->areas[area2].floodnum) {
		return true;
	}
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (CMap->numAreas+7)>>3;

	if (cm_noAreas->integer || area == -1)
	{	// for debugging, send everything
		Com_Memset (buffer, 255, bytes);
	}
	else
	{
		floodnum = CMap->areas[area].floodnum;
		for (i=0 ; i<CMap->numAreas ; i++)
		{
			if (CMap->areas[i].floodnum == floodnum || area == -1)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}

