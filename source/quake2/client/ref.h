/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#ifndef _ref_h
#define _ref_h

#include "../qcommon/qcommon.h"

#define	MAX_PARTICLES	4096
#define	MAX_LIGHTSTYLES_Q2	256

//
// these are the functions exported by the refresh module
//
typedef struct
{
	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	void	(*BeginRegistration) (const char *map);
	void	(*EndRegistration) (void);

	void	(*RenderFrame) (refdef_t *fd);

	/*
	** video mode and refresh state management entry points
	*/
	void	(*BeginFrame)( float camera_separation );
	void	(*EndFrame) (void);

} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct
{
	void	(*Sys_Error) (int err_level, char *str, ...);

	void	(*Con_Printf) (int print_level, char *str, ...);
} refimport_t;


// this is the only function actually exported at the linker level
typedef	refexport_t	(*GetRefAPI_t) (refimport_t);

void R_ClearScreen();

extern float		v_blend[4];

#endif
