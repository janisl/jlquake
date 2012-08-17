/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// server.h

#include "../../server/server.h"
#include "../../server/quake2/local.h"

#include "../../common/file_formats/bsp38.h"

//define	PARANOID			// speed sapping error checking

#include "../qcommon/qcommon.h"

//=============================================================================

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

extern netadr_t net_from;
extern QMsg net_message;

//===========================================================

//
// sv_main.c
//
void SV_InitOperatorCommands(void);

//
// sv_phys.c
//
void SV_PrepWorldFrame(void);
