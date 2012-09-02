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

#include "../../client.h"
#include "local.h"

bool clhw_siege;
int clhw_keyholder = -1;
int clhw_doc = -1;
float clhw_timelimit;
float clhw_server_time_offset;
byte clhw_fraglimit;
unsigned int clhw_defLosses;		// Defenders losses in Siege
unsigned int clhw_attLosses;		// Attackers Losses in Siege
