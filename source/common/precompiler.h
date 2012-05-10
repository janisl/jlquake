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

//!!!!!!!!!!!!!!! Used by game VMs, do not change !!!!!!!!!!!!!!!!!!!!!
#define MAX_TOKENLENGTH     1024

struct q3pc_token_t
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
};

struct etpc_token_t
{
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
	int line;
	int linescrossed;
};
//!!!!!!!!!!!!!!! End of stuff used by game VMs !!!!!!!!!!!!!!!!!!!!!
