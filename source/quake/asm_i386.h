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

#ifndef __ASM_I386__
#define __ASM_I386__

#ifdef ELF
#define C(label) label
#else
#define C(label) _##label
#endif

//
// !!! note that this file must match the corresponding C structures at all
// times !!!
//

// sfxcache_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define sfxc_length		0
#define sfxc_loopstart	4
#define sfxc_speed		8
#define sfxc_width		12
#define sfxc_stereo		16
#define sfxc_data		20

// channel_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define ch_sfx			0
#define ch_leftvol		4
#define ch_rightvol		8
#define ch_end			12
#define ch_pos			16
#define ch_looping		20
#define ch_entnum		24
#define ch_entchannel	28
#define ch_origin		32
#define ch_dist_mult	44
#define ch_master_vol	48
#define ch_size			52

// portable_samplepair_t structure
// !!! if this is changed, it much be changed in sound.h too !!!
#define psp_left		0
#define psp_right		4
#define psp_size		8

#endif

