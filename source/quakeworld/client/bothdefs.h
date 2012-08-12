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

// defs common to client and server

#define GLQUAKE_VERSION 1.00
#define VERSION     2.40
#define LINUX_VERSION 0.98


#ifdef SERVERONLY		// no asm in dedicated server
#undef id386
#endif

#if id386
#define UNALIGNED_OK    1	// set to 0 if unaligned accesses are not supported
#else
#define UNALIGNED_OK    0
#endif

// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE  32		// used to align key data structures

#define UNUSED(x)   (x = x)	// for pesky compiler / lint warnings

#define SOUND_CHANNELS      8
