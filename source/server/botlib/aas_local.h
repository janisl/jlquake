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

//area settings
struct aas_areasettings_t
{
	//could also add all kind of statistic fields
	int contents;					//contents of the convex area
	int areaflags;					//several area flags
	int presencetype;				//how a bot can be present in this convex area
	int cluster;					//cluster the area belongs to, if negative it's a portal
	int clusterareanum;				//number of the area in the cluster
	int numreachableareas;			//number of reachable areas from this one
	int firstreachablearea;			//first reachable area in the reachable area index
	// Ridah, add a ground steepness stat, so we can avoid terrain when we can take a close-by flat route
	float groundsteepness;			// 0 = flat, 1 = steep
};
