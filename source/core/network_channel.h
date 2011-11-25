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

enum netsrc_t
{
	NS_CLIENT,
	NS_SERVER
};

struct netchan_common_t
{
	netsrc_t sock;

	int dropped;			// between last packet and previous

	netadr_t remoteAddress;
	int qport;				// qport value to write when transmitting

	// sequencing variables
	int incomingSequence;
};
