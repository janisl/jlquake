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

#ifndef __SERVER_LINK_H__
#define __SERVER_LINK_H__

#include "../common/qcommon.h"

//!!!!! Used by Quake 2 game DLLs, do not change.
// link_t is only used for entity area links now
struct link_t {
	link_t* prev;
	link_t* next;
};

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define STRUCT_FROM_LINK( l,t,m ) ( ( t* )( ( byte* )l - ( qintptr ) & ( ( ( t* )0 )->m ) ) )

void ClearLink( link_t* l );
void RemoveLink( link_t* l );
void InsertLinkBefore( link_t* l, link_t* before );

#endif
