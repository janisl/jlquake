/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// protocol.h -- communications protocols

#define PROTOCOL_VERSION    28

#define QW_CHECK_HASH 0x5157

//=========================================

#define PORT_CLIENT 27001
#define PORT_MASTER 27000
#define PORT_SERVER 27500

//=========================================

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)

#define S2C_CHALLENGE       'c'
#define S2C_CONNECTION      'j'
#define A2A_PING            'k'	// respond with an A2A_ACK
#define A2A_ACK             'l'	// general acknowledgement without info
#define A2A_NACK            'm'	// [+ comment] general failure
#define A2A_ECHO            'e'	// for echoing
#define A2C_PRINT           'n'	// print a message on client

#define S2M_HEARTBEAT       'a'	// + serverinfo + userlist + fraglist
#define A2C_CLIENT_COMMAND  'B'	// + command line
#define S2M_SHUTDOWN        'C'
