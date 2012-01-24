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

#define	MAX_ENT_LEAFS	16

struct qhedict_t
{
	bool free;
	link_t area;			// linked to a division node or leaf
	
	int num_leafs;
	int LeafNums[MAX_ENT_LEAFS];

#ifdef HEXEN2_EDICT
	h2entity_state_t baseline;
#else
	q1entity_state_t baseline;
#endif
	
	float freetime;			// sv.time when the object was freed
#ifdef HEXEN2_EDICT
	float alloctime;		// sv.time when the object was allocated
#endif
	entvars_t v;			// C exported fields from progs
	// other fields from progs come immediately after
};
