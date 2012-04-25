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

#define PROTOCOL_VERSION    15


#define SU_VIEWHEIGHT   (1 << 0)
#define SU_IDEALPITCH   (1 << 1)
#define SU_PUNCH1       (1 << 2)
#define SU_PUNCH2       (1 << 3)
#define SU_PUNCH3       (1 << 4)
#define SU_VELOCITY1    (1 << 5)
#define SU_VELOCITY2    (1 << 6)
#define SU_VELOCITY3    (1 << 7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define SU_ITEMS        (1 << 9)
#define SU_ONGROUND     (1 << 10)		// no data follows, the bit is it
#define SU_INWATER      (1 << 11)		// no data follows, the bit is it
#define SU_WEAPONFRAME  (1 << 12)
#define SU_ARMOR        (1 << 13)
#define SU_WEAPON       (1 << 14)

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

// a sound with no channel is a local only sound
#define SND_VOLUME      (1 << 0)		// a byte
#define SND_ATTENUATION (1 << 1)		// a byte
#define SND_LOOPING     (1 << 2)		// a long


// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT  22


// game types sent by serverinfo
// these determine which intermission screen plays
#define GAME_COOP           0
#define GAME_DEATHMATCH     1
