/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
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

#define	PROTOCOL_VERSION	28

#define QW_CHECK_HASH 0x5157

//=========================================

#define	PORT_CLIENT	27001
#define	PORT_MASTER	27000
#define	PORT_SERVER	27500

//=========================================

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// byte long (?? not true anymore?)

#define	S2C_CHALLENGE		'c'
#define	S2C_CONNECTION		'j'
#define	A2A_PING			'k'	// respond with an A2A_ACK
#define	A2A_ACK				'l'	// general acknowledgement without info
#define	A2A_NACK			'm'	// [+ comment] general failure
#define A2A_ECHO			'e' // for echoing
#define	A2C_PRINT			'n'	// print a message on client

#define	S2M_HEARTBEAT		'a'	// + serverinfo + userlist + fraglist
#define	A2C_CLIENT_COMMAND	'B'	// + command line
#define	S2M_SHUTDOWN		'C'

//==============================================

// playerinfo flags from server
// playerinfo allways sends: playernum, flags, origin[] and framenumber

#define	PF_MSEC			(1<<0)
#define	PF_COMMAND		(1<<1)
#define	PF_VELOCITY1	(1<<2)
#define	PF_VELOCITY2	(1<<3)
#define	PF_VELOCITY3	(1<<4)
#define	PF_MODEL		(1<<5)
#define	PF_SKINNUM		(1<<6)
#define	PF_EFFECTS		(1<<7)
#define	PF_WEAPONFRAME	(1<<8)		// only sent for view player
#define	PF_DEAD			(1<<9)		// don't block movement any more
#define	PF_GIB			(1<<10)		// offset the view height differently
#define	PF_NOGRAV		(1<<11)		// don't apply gravity for prediction

//==============================================

// if the high bit of the client to server byte is set, the low bits are
// client move cmd bits
// ms and angle2 are allways sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE3 	(1<<1)
#define	CM_FORWARD	(1<<2)
#define	CM_SIDE		(1<<3)
#define	CM_UP		(1<<4)
#define	CM_BUTTONS	(1<<5)
#define	CM_IMPULSE	(1<<6)
#define	CM_ANGLE2 	(1<<7)

//==============================================

// a sound with no channel is a local only sound
// the sound field has bits 0-2: channel, 3-12: entity
#define	SND_VOLUME		(1<<15)		// a byte
#define	SND_ATTENUATION	(1<<14)		// a byte

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// q1svc_print messages have an id, so messages can be filtered
#define	PRINT_LOW			0
#define	PRINT_MEDIUM		1
#define	PRINT_HIGH			2
#define	PRINT_CHAT			3	// also go to chat buffer
